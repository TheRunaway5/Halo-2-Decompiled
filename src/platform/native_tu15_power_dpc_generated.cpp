// Complete authenticated TU1.5 power DPC; no production decoder.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/native_tu15_startup_timer.hpp"
namespace halo2 {
void source_run_tu15_power_dpc(SourcePhysicsContactMemory &memory,SourcePhysicsContactRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsContactRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    using namespace physics_world_runtime_detail;
    FpScope scope(fp);WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try {
        if(services.instruction)services.instruction(0x2e009du);s.flags.minus(r(memory,0x56ee08u,4),0,4);
        if(services.instruction)services.instruction(0x2e00a4u);
        if(!s.flags.zf) {
            if(services.instruction)services.instruction(0x2e00a6u);
            s.push(0x2e00a6u,0x2e00acu);s.required(0x2e00a6u,r(memory,0x41b704u,4),false);
        }
        if(services.instruction)services.instruction(0x2e00acu);s.pop();s.gp[4]+=16u;
    }catch(...) {abi=s.save();throw;}
    abi=s.save();
}
}
