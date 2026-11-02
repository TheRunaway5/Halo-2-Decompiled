#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/original_data.hpp"
#include "halo2/effect_light_receiver.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
using namespace halo2;using J=nlohmann::json;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
std::vector<std::byte> decode(const std::string& text) {
    std::vector<std::byte> bytes;for(std::size_t i=0;i<text.size();i+=2)bytes.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return bytes;
}
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto b:bytes){const auto v=std::to_integer<unsigned>(b);out+=digits[v>>4];out+=digits[v&15];}return out;
}
J store(const SourcePhysicsContactRecordStore& s){return {{"instruction",s.instruction},{"address",s.address},{"before",hex(s.before)},{"written",hex(s.written)}};}
void restore(PhysicsLiveWorldTu15& world,const SourceRuntimeMemory::Checkpoint& baseline) {
    const auto current=world.memory().checkpoint();check(current.regions.size()==baseline.regions.size(),"31650 World topology changed");
    for(unsigned i=0;i<baseline.regions.size();++i) {
        const auto& a=baseline.regions[i];const auto& b=current.regions[i];
        check(a.address==b.address&&a.owner==b.owner&&a.access==b.access&&a.page_bytes==b.page_bytes&&a.committed==b.committed&&a.bytes.size()==b.bytes.size(),"31650 replaced a World lease");
        if(a.bytes==b.bytes)continue;check(a.access==SourceRuntimeMemory::Access::ReadWrite,"31650 changed readonly source bytes");
        for(std::size_t n=0;n<a.bytes.size();) {
            if(a.bytes[n]==b.bytes[n]){++n;continue;}const auto first=n;while(n<a.bytes.size()&&a.bytes[n]!=b.bytes[n])++n;
            world.write_source_call_input(a.address+std::uint32_t(first),std::span(a.bytes).subspan(first,n-first));
        }
    }
}
}
int main(int argc,char** argv)try {
    check(argc==4&&std::getenv("HALO2_TEST_SCRATCH"),"Supply XBE MAP OWNED_SAVES inside run_test.py");
    const auto scratch=std::filesystem::weakly_canonical(std::getenv("HALO2_TEST_SCRATCH"));
    const auto saves=std::filesystem::weakly_canonical(argv[3]),relative=saves.lexically_relative(scratch);
    check(!relative.empty()&&!relative.is_absolute()&&*relative.begin()!="..","31650 saves escape owned scratch");
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    PhysicsLiveWorldTu15 world(cache,argv[1],saves);auto& m=world.memory();
    for(const auto [at,size]:std::array<std::pair<unsigned,unsigned>,2>{{{0x1000000,0x10000},{0x62000000,0x20000}}}) {
        check(m.find_unowned_range(at,at+size,size,4096)==at,"31650 declared inputs overlap a World owner");
        m.mount(at,std::vector<std::byte>(size),SourceRuntimeMemory::Access::ReadWrite,"declared original31650 World input");
    }
    const auto baseline=m.checkpoint();
    struct Cleanup {PhysicsLiveWorldTu15& world;const SourceRuntimeMemory::Checkpoint& baseline;
        ~Cleanup(){world.observe_physics_stores_read_only({});restore(world,baseline);world.memory().release(0x1000000);world.memory().release(0x62000000);}} cleanup{world,baseline};
    for(std::string line;std::getline(std::cin,line);)if(!line.empty()) {
        restore(world,baseline);const auto row=J::parse(line);
        for(const auto& bank:row.at("banks")) {
            const auto at=bank.at("address").get<unsigned>();const auto bytes=decode(bank.at("before"));
            if(!std::ranges::equal(m.view(at,bytes.size()),bytes))world.write_source_call_input(at,bytes);
        }
        SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<decltype(a.gp)>();a.xmm=row.at("xmm").get<decltype(a.xmm)>();a.eflags=0x202;
        const auto fp=row.at("before_fp").get<std::array<unsigned,3>>();a.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(fp[0]),fp[2]};a.x87_status=std::uint16_t(fp[1]);
        const auto word=[&](unsigned at){return Bytes(m.view(at,4)).u32(0);};
        J stores=J::array(),registered=J::array();unsigned entries{};bool queues_restored=true;
        world.observe_physics_stores_read_only([&](auto pc,const auto& s){check(pc==s.instruction,"World31650 lost its diagnostic store PC");registered.push_back(store(s));});
        {
            SourcePhysicsEntryObserverBinding observer(m,[&](bool before,auto entry,const auto&){if(before&&entry==0x31650u)++entries;});
            if(row.value("projection",std::string{})=="typed-world") {
                SourceBspAdmission before_bsp;SourceObjectAdmission before_objects;
                const auto rows=[&](unsigned offset,auto& out) {
                    const auto header=word(a.gp[6]+offset);const auto count=Bytes(m.view(header+4u,2)).u16(0);
                    for(unsigned n=0;n<count;++n)out.push_back({word(word(header+12u)+4*n),Bytes(m.view(word(header+8u)+2*n,2)).u16(0),
                        Bytes(m.view(word(header+20u)+2*n,2)).u16(0),word(word(header+16u)+4*n)});
                };
                rows(12,before_bsp.clusters);rows(20,before_objects.objects);rows(24,before_bsp.instances);
                auto bsp=before_bsp;auto objects=before_objects;
                const auto definition=word(word(0x4f0414u)+16u*(a.gp[1]&65535u)+8u),flags=word(definition);
                source_effect_light_bsp_flags_31650(bsp,flags);
                source_effect_light_object_flags_31650(objects,{flags,word(a.gp[4]+4u),word(a.gp[4]+8u)});
                const auto full=source_effect_light_full_f8_31650(flags,std::uint16_t(before_bsp.clusters.size()));
                const auto old=m.view(0x1000000,0x10000);std::vector<std::byte> expected(old.begin(),old.end());
                const auto record=word(word(0x4ecbdcu)+0x44u)+0x110u*(a.gp[0]&65535u);
                if(full)std::memcpy(expected.data()+record+0xf8u-0x1000000u,m.view(0x46a0d0u,4).data(),4);
                const auto* control=std::getenv("HALO2_REFERENCE_LIGHT_FLAGS_31650");const bool reference=control&&*control&&*control!='0';
                if(reference)world.reference_effect_light_flags_tu15({a.gp[0],word(a.gp[4]+4u),word(a.gp[4]+8u),before_bsp,bsp,before_objects,objects,full});
                else if(full)world.set_effect_light_full_f8_tu15(a.gp[0]);
                queues_restored=std::ranges::equal(expected,m.view(0x1000000,0x10000));check(queues_restored,"31650 reference did not retire its temporary queue projection");
                check(entries==unsigned(reference),"31650 typed reference did not execute its genuine World owner exactly once");
                const auto publish=[&](unsigned offset,const auto& rows) {
                    const auto header=word(a.gp[6]+offset);for(unsigned n=0;n<rows.size();++n) {
                        const auto value=rows[n].flags;world.write_source_call_input(word(header+8u)+2*n,std::as_bytes(std::span(&value,1)));
                    }
                };
                publish(12,bsp.clusters);publish(20,objects.objects);publish(24,bsp.instances);
            }else {
                world.run_actual(0x31650u,a,[&](const auto& s){stores.push_back(store(s));});
                check(stores==registered,"World31650 delivered a store more than once");check(entries==1,"World31650 entry observation duplicated");
            }
        }
        world.observe_physics_stores_read_only({});J banks=J::array();
        for(const auto& bank:row.at("banks")) {
            const auto at=bank.at("address").get<unsigned>();banks.push_back(J{{"address",at},{"after",hex(m.view(at,bank.at("before").get_ref<const std::string&>().size()/2))}});
        }
        std::cout<<J{{"banks",banks},{"stores",stores},{"gp",a.gp},{"xmm",a.xmm},
            {"fp",{a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr}},
            {"entries",entries},{"queues_restored",queues_restored},{"registered_once",true}}.dump()<<'\n'<<std::flush;
    }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
