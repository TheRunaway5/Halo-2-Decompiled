#include "halo2/death_startup_runtime.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "halo2/source_effect_particle_frame_tu15.hpp"
#include "native_tu15_object_property_source_test_harness.hpp"

namespace {
using namespace halo2;
using namespace halo2::object_property_source_test;
constexpr auto entry=0x3eb10u;

J run(const J& row,std::string_view owner,bool observed,std::uint32_t stop) {
    const Seed seed{row};
    Memory memory;memory.banks=seed.banks;
    auto abi=seed.abi;
    J callbacks=J::array(),stores=J::array(),mmx;
    SourcePhysicsContactRecordObserver observe;
    SourcePhysicsContactRuntimeServices services;
    if(observed){
        services.instruction=[&](std::uint32_t pc) {
            callbacks.push_back(pc);memory.pc=pc;
            if(pc==stop)throw Boundary{};
        };
        observe=[&](const SourcePhysicsContactRecordStore& event){
            stores.push_back(J::array({event.instruction,event.address,hex(event.before),hex(event.written)}));
        };
    }
    bool stopped=false;
    auto call=[&](auto&& function){try{function();}catch(const Boundary&){stopped=true;}};
    const auto fp=*abi.floating_point;
    if(owner=="primary"){
        check(source_physics_live_support_tu15_owns_entry(entry),"Primary 3EB10 owner unavailable");
        call([&]{source_run_physics_live_support_tu15(memory,entry,abi,fp,services,observe);});
    } else if(owner=="particle-frame"){
        check(source_effect_particle_frame_tu15_owns_entry(entry),"ParticleFrame 3EB10 owner unavailable");
        call([&]{source_run_effect_particle_frame_tu15(memory,entry,abi,fp,services,observe);});
    } else {
        check(owner=="death-startup"&&source_death_startup_owns_entry_1_1(entry),
            "Map Pack DeathStartup 3EB10 owner unavailable");
        SourceDeathWorldSchedulerServices death;
        death.runtime.instruction=services.instruction;
        SourceUnitAnimationRuntimeAbi animated;
        static_cast<SourcePhysicsContactRuntimeAbi&>(animated)=abi;
        animated.mmx={1,2,3,4,5,6,7,0xfedcba9876543210u};
        call([&]{source_run_death_startup_1_1(memory,entry,animated,fp,death,observe);});
        abi=animated;mmx=animated.mmx;
    }
    J live=J::array(),changes=J::array();
    for(unsigned n=0;n<abi.x87_depth;++n)
        live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&abi.x87[n]),10)));
    for(const auto&[address,before]:seed.banks){
        const auto& after=memory.banks.at(address);
        for(unsigned i=0;i<before.size();++i)
            if(before[i]!=after[i])
                changes.push_back(J::array({address+i,std::to_integer<unsigned>(after[i])}));
    }
    return J{{"owner",owner},{"case",row.at("name")},{"observed",observed},{"stop_pc",stop},{"stopped",stopped},
        {"gp",abi.gp},{"flags",abi.eflags},{"xmm",abi.xmm},{"fs",abi.fs_base},{"mmx",mmx},
        {"fp",J::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr})},
        {"live_x87",live},{"changes",changes},{"callbacks",callbacks},{"observer",stores},
        {"memory",std::move(memory.operations)}};
}
} // namespace

int main(int argc,char** argv)try {
    check(argc==2,"Supply the 3EB10 linked fixture");
    std::ifstream stream(argv[1]);const auto fixture=J::parse(stream);
    J packets=J::array();
    for(std::string_view owner:{"primary","particle-frame","death-startup"}){
        const auto& rows=fixture.at(owner=="death-startup"?"mappack11":"tu15");
        for(unsigned n=0;n<rows.size();++n){
            packets.push_back(run(rows[n],owner,false,0u));
            packets.push_back(run(rows[n],owner,true,0u));
            if(n<9)packets.push_back(run(rows[n],owner,true,0x3eb4fu));
        }
    }
    std::cout<<packets.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
