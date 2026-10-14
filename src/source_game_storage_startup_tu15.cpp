// Complete authenticated TU1.5 13A700 instruction lowering. Original bytes:
// SHA256 1370232f655b7636c1e00e623b3d171d901f36088e978d15e12f398cb56b1405.
// External calls retain the complete source ABI and must execute real owners.
#include "physics_world_runtime_internal.hpp"
#include "halo2/source_game_storage_startup_tu15.hpp"

namespace halo2 {
namespace {
using namespace physics_world_runtime_detail;
void run(WorldState& s) {
    const auto instruction=[&](std::uint32_t pc){if(s.services.instruction)s.services.instruction(pc);};
    instruction(0x13a700);s.push(0x13a700,s.gp[1]);
    instruction(0x13a701);s.push(0x13a701,s.gp[6]);
    instruction(0x13a702);s.push(0x13a702,s.gp[7]);
    instruction(0x13a703);s.push(0x13a703,0x13a708);s.required(0x13a703,0x126640,false);
    instruction(0x13a708);s.gp[0]=r(s.memory,0x4f2954,4);
    instruction(0x13a70d);s.gp[1]=r(s.memory,0x4f2950,4);
    instruction(0x13a713);s.gp[6]=s.gp[0]+s.gp[1];
    instruction(0x13a716);s.gp[0]=s.flags.plus(s.gp[0],0x1200,4);
    instruction(0x13a71b);w(s.memory,s.observe,0x13a71b,0x4f2954,4,s.gp[0]);
    instruction(0x13a720);s.push(0x13a720,0x4f295c);
    instruction(0x13a725);s.gp[7]=4;
    instruction(0x13a72a);s.gp[0]=s.gp[4]+0xc;
    instruction(0x13a72e);w(s.memory,s.observe,0x13a72e,s.gp[4]+0xc,4,0x1200);
    instruction(0x13a736);s.push(0x13a736,0x13a73b);s.required(0x13a736,0x166a80,false);
    instruction(0x13a73b);s.gp[0]=s.flags.logic(s.gp[0]^s.gp[0],4);
    instruction(0x13a73d);s.gp[1]=0x480;
    instruction(0x13a742);s.gp[7]=s.gp[6];
    instruction(0x13a744);s.push(0x13a744,0xfffff);
    instruction(0x13a749);
    while(s.gp[1]) {
        w(s.memory,s.observe,0x13a749,s.gp[7],4,s.gp[0]);
        s.gp[7]+=(s.other_flags&1024u)?0xfffffffcu:4u;--s.gp[1];instruction(0x13a749);
    }
    instruction(0x13a74b);s.push(0x13a74b,0x9001f);
    instruction(0x13a750);w(s.memory,s.observe,0x13a750,0x4f3218,4,s.gp[6]);
    instruction(0x13a756);w(s.memory,s.observe,0x13a756,s.gp[6]+2,2,0xffff);
    instruction(0x13a75c);s.push(0x13a75c,0x13a761);s.required(0x13a75c,0x32b3d6,false);
    instruction(0x13a761);s.gp[4]=s.flags.plus(s.gp[4],8,4);
    instruction(0x13a764);s.gp[6]=s.flags.logic(s.gp[6]^s.gp[6],4);
    for(;;) {
        instruction(0x13a766);const auto target=r(s.memory,0x44aed0+s.gp[6],4);
        s.push(0x13a766,0x13a76c);s.required(0x13a766,target,false);
        instruction(0x13a76c);s.gp[6]=s.flags.plus(s.gp[6],0x24,4);
        instruction(0x13a76f);s.flags.minus(s.gp[6],0x990,4);
        instruction(0x13a775);if(!s.flags.cf)break;
    }
    instruction(0x13a777);s.gp[7]=s.pop();
    instruction(0x13a778);s.gp[6]=s.pop();
    instruction(0x13a779);s.gp[1]=s.pop();
    instruction(0x13a77a);s.pop();
}
}
void source_run_game_storage_startup_tu15(SourcePhysicsContactMemory& memory,
    SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observer) {
    FpScope scope(fp);WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    try{run(state);}catch(...){abi=state.save();throw;}abi=state.save();
}
}
