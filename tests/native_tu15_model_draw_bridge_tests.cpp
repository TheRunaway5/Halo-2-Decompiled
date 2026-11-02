#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <iostream>
using namespace halo2;
using J=nlohmann::json;
namespace {
void check(bool ok,const char* message){if(!ok)throw FormatError(message);}
std::vector<std::byte> decode(const std::string& text) {
    check(text.size()%2==0,"Model fixture has truncated bytes");std::vector<std::byte> bytes;
    for(std::size_t i=0;i<text.size();i+=2)bytes.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return bytes;
}
std::string hex(std::span<const std::byte> bytes) {
    std::string text;const char* d="0123456789abcdef";text.reserve(bytes.size()*2);
    for(auto b:bytes){const auto v=std::to_integer<unsigned>(b);text+=d[v>>4];text+=d[v&15];}return text;
}
J store(const SourcePhysicsContactRecordStore& event) {
    return {{"instruction",event.instruction},{"address",event.address},{"before",hex(event.before)},{"written",hex(event.written)}};
}
void restore(PhysicsLiveWorldTu15& world,const SourceRuntimeMemory::Checkpoint& saved) {
    const auto current=world.memory().checkpoint();check(current.regions.size()==saved.regions.size(),"Model fixture changed World topology");
    for(std::size_t i=0;i<saved.regions.size();++i) {
        const auto& a=saved.regions[i];const auto& b=current.regions[i];
        check(a.address==b.address&&a.owner==b.owner&&a.access==b.access&&a.bytes.size()==b.bytes.size()&&a.committed==b.committed,
            "Model fixture replaced an existing World owner");
        if(a.bytes==b.bytes)continue;
        check(a.access==SourceRuntimeMemory::Access::ReadWrite,"Model fixture changed readonly source bytes");
        for(std::size_t n=0;n<a.bytes.size();) {
            if(a.bytes[n]==b.bytes[n]){++n;continue;}
            const auto first=n;while(n<a.bytes.size()&&a.bytes[n]!=b.bytes[n])++n;
            world.write_source_call_input(a.address+std::uint32_t(first),std::span(a.bytes).subspan(first,n-first));
        }
    }
}
}
int main(int argc,char** argv)try {
    check(argc==4,"Supply TU15_XBE OPENING_MAP OWNED_SAVES");
    check(std::getenv("HALO2_TEST_SCRATCH"),"Model bridge fixture needs owned scratch");
    const auto scratch=std::filesystem::weakly_canonical(std::getenv("HALO2_TEST_SCRATCH"));
    const auto saves=std::filesystem::weakly_canonical(argv[3]);const auto relative=saves.lexically_relative(scratch);
    check(!relative.empty()&&!relative.is_absolute()&&*relative.begin()!="..","Model fixture saves escape owned scratch");
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    PhysicsLiveWorldTu15 world(cache,argv[1],saves);auto& memory=world.memory();
    constexpr auto data=0x1000000u,size=0x10000u;
    check(memory.find_unowned_range(data,data+size,size,4096)==data,"Model fixture overlaps another World owner");
    memory.mount(data,std::vector<std::byte>(size),SourceRuntimeMemory::Access::ReadWrite,"declared native model bridge inputs");
    const auto baseline=memory.checkpoint();
    struct Cleanup {PhysicsLiveWorldTu15& world;const SourceRuntimeMemory::Checkpoint& baseline;
        ~Cleanup(){world.observe_physics_stores_read_only({});restore(world,baseline);world.memory().release(data);}} cleanup{world,baseline};
    for(std::string line;std::getline(std::cin,line);)if(!line.empty()) {
        const auto row=J::parse(line);restore(world,baseline);
        for(const auto& bank:row.at("banks")) {
            const auto at=bank.at("address").get<std::uint32_t>();const auto bytes=decode(bank.at("before"));
            if(!std::ranges::equal(memory.view(at,bytes.size()),bytes))world.write_source_call_input(at,bytes);
        }
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<decltype(abi.gp)>();abi.eflags=row.value("eflags",0x202u);
        const auto fp=row.at("before_fp").get<std::array<std::uint32_t,3>>();abi.x87_status=std::uint16_t(fp[1]);
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(fp[0]),fp[2]};
        if(row.contains("xmm"))abi.xmm=row.at("xmm").get<decltype(abi.xmm)>();
        for(unsigned index=0;index<3;++index) {
            const auto value=index==0?row.value("return_pc",0x63000000u):row.at(index==1?"matrix":"source").get<std::uint32_t>();
            std::array<std::byte,4> bytes;std::memcpy(bytes.data(),&value,4);world.write_source_call_input(abi.gp[4]+4u*index,bytes);
        }
        J stores=J::array(),registered=J::array(),calls=J::array();
        const auto observe=[&](const auto& event){stores.push_back(store(event));};
        world.observe_physics_stores_read_only([&](auto pc,const auto& event) {
            check(pc==event.instruction,"Native model store lost its original diagnostic PC");registered.push_back(store(event));
        });
        std::vector<std::uint32_t> pending;
        {
            SourcePhysicsEntryObserverBinding entries(memory,[&](bool before,auto entry,const auto& state) {
                if(entry!=0x2cbd0u)return;
                if(before)pending.push_back(state.gp[4]);
                else check(!pending.empty(),"Model return has no native entry");
                const auto sp=pending.back();check(sp>=0x600e0000u&&sp<0x60100000u,"Model call escaped its declared stack bank");
                calls.push_back(J{{"before",before},{"gp",state.gp},{"xmm",state.xmm},{"fp",J::array({state.floating_point->x87_control_word,state.x87_status,state.floating_point->mxcsr})},
                    {"stack_address",sp},{"stack_above",hex(memory.view(sp,0x60100000u-sp))}});
                if(!before)pending.pop_back();
            });
            world.run_actual(row.at("entry"),abi,observe);
        }
        world.observe_physics_stores_read_only({});check(pending.empty(),"Model bridge left an active native call");
        check(stores==registered,"Native model delivered a registered store more than once");
        J banks=J::array();
        for(const auto& bank:row.at("banks")) {
            const auto at=bank.at("address").get<std::uint32_t>();
            const auto bytes=bank.at("before").get_ref<const std::string&>().size()/2;
            banks.push_back(J{{"address",at},{"after",hex(memory.view(at,bytes))}});
        }
        std::cout<<J{{"gp",abi.gp},{"fp",J::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr})},
            {"banks",banks},{"stores",stores},{"calls",calls},{"registered_once",true}}.dump()<<'\n'<<std::flush;
    }
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
