#include "gameplay/objects/native_tu15_object_record_ratio_source_owner.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include <fstream>
#include <iostream>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace {
struct Provider final:Memory {
    std::uint32_t position{},stop_pc{};
    mutable J trace=J::array();
    void event(std::string_view kind,std::uint32_t site,J data)const {
        trace.push_back(J::array({kind,site,position,std::move(data)}));
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        event("read",position,J::array({address,out.size()}));
        if(stop_pc&&position==stop_pc)throw Boundary{};
        Memory::read(address,out);
    }
    void write(std::uint32_t address,std::span<const std::byte> data)override {
        event("write",position,J::array({address,hex(data)}));Memory::write(address,data);
    }
};

J abi_packet(const SourcePhysicsContactRuntimeAbi& abi) {
    J stack=J::array();
    for(unsigned n=0;n<abi.x87_depth;++n)
        stack.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&abi.x87[n]),10)));
    const auto& fp=*abi.floating_point;
    return J{{"gp",abi.gp},{"xmm",abi.xmm},{"eflags",abi.eflags},{"fs",abi.fs_base},
        {"fp",J::array({fp.x87_control_word,abi.x87_status,fp.mxcsr})},{"live_x87",stack}};
}

struct Counts {unsigned normal{},stopped{},child_a{},child_b{};std::size_t events{};};
void run(const J& row,std::ostream& stream,Counts& counts) {
    SourcePhysicsContactRuntimeAbi abi{};abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");
    abi.fs_base=0x12345678u;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<abi.x87_depth;++n){const auto data=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],data.data(),10);}
    Provider provider;provider.stop_pc=row.value("provider_stop_pc",0u);
    for(const auto& bank:row.at("banks"))provider.banks.emplace(bank.at("address"),bytes(bank.at("before")));
    const SourceEffectCompiledSymbolsTu15 symbol_owner{provider,false,[](std::uint32_t)->std::string {
        throw std::runtime_error("Unexpected bound symbol in original-key 1046C0 fixture");
    }};
    SourcePhysicsContactRuntimeServices services;
    unsigned child_a{},child_b{},child_entries{};
    services.instruction=[&](std::uint32_t site){
        provider.position=site;provider.event("instruction",site,J{});
        child_a+=site==0x10481au;child_b+=site==0x104b71u;child_entries+=site==0x107140u;
    };
    const SourcePhysicsContactRecordObserver observer=[&](const auto& event){
        provider.event("observer",event.instruction,J::array({event.address,hex(event.before),hex(event.written)}));
    };
    const auto entry=row.at("entry").get<std::uint32_t>();
    check(source_physics_live_support_tu15_owns_entry(entry),"Missing exact Primary ratio owner");
    bool stopped{};const auto fp=*abi.floating_point;
    try{source_run_physics_live_support_tu15(provider,entry,abi,fp,services,observer);}
    catch(const Boundary&){stopped=true;}
    const auto name=row.at("name").get<std::string>();
    check(stopped==(provider.stop_pc!=0),name+": provider stop changed");
    check(child_entries==1&&child_a+child_b==(entry==0x1046c0u),name+": actual child route changed");
    auto state=abi_packet(abi);
    const auto& expected=row.at("expected");
    for(const auto key:{"gp","xmm","eflags","fp","live_x87"})
        check(state.at(key)==expected.at(key),name+": original "+key+" differs: "+state.at(key).dump()+" != "+expected.at(key).dump());
    for(const auto& bank:row.at("banks"))
        check(provider.banks.at(bank.at("address").get<std::uint32_t>())==bytes(bank.at("after")),name+": original bank differs");
    stream<<J{{"state",std::move(state)},{"trace",provider.trace},{"stopped",stopped}}.dump()<<'\n';
    counts.normal+=!stopped;counts.stopped+=stopped;counts.child_a+=child_a;counts.child_b+=child_b;
    counts.events+=provider.trace.size();
}
}

int main(int argc,char** argv)try {
    check(argc==3,"Supply 107140 fixture and output stream");
    std::ifstream fixture(argv[1]);const auto rows=J::parse(fixture);
    std::ofstream out(argv[2]);check(bool(out),"Cannot create 107140 result stream");
    Counts counts;
    for(const auto& row:rows)run(row,out,counts);
    std::cout<<J{{"normal",counts.normal},{"stopped",counts.stopped},{"first_callsite_returns",counts.child_a},
                 {"second_callsite_returns",counts.child_b},{"events",counts.events},
                 {"native_entries",native_object_record_ratio_owner_detail::activity()}}.dump()<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
