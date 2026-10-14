// Complete authenticated TU1.5 707D0 body, 234 bytes.
// SHA256 8807f11d2895268a08793c6c8d3061063f302c00677ef968caa03b854c820775.
#include "physics_world_runtime_internal.hpp"
#include "halo2/source_network_storage_startup_tu15.hpp"
namespace halo2 {
namespace {
using namespace physics_world_runtime_detail;
void run(WorldState& s) {
    const auto instruction=[&](std::uint32_t pc){if(s.services.instruction)s.services.instruction(pc);};
    const auto finish=[&](std::uint32_t pc){
        if(pc==0x708b1){instruction(pc);s.set(0,r(s.memory,0x4dc3f1,1),1);}
        instruction(0x708b6);s.gp[4]=s.flags.plus(s.gp[4],0x24,4);
        instruction(0x708b9);s.pop();
    };
    instruction(0x707d0);s.set(0,r(s.memory,0x4dc3f1,1),1);
    instruction(0x707d5);s.gp[4]=s.flags.minus(s.gp[4],0x24,4);
    instruction(0x707d8);s.flags.logic(s.reg(0,1)&s.reg(0,1),1);
    instruction(0x707da);if(!s.flags.zf){finish(0x708b6);return;}
    instruction(0x707e0);s.gp[0]=s.flags.logic(s.gp[0]^s.gp[0],4);
    for(const auto [pc,p]:{std::pair{0x707e2u,0x4dc3f3u},{0x707e7u,0x4dc3f7u},
        {0x707ecu,0x4dc3fbu},{0x707f1u,0x4dc3ffu}}){instruction(pc);w(s.memory,s.observe,pc,p,4,s.gp[0]);}
    instruction(0x707f6);s.gp[1]=s.flags.logic(s.gp[1]^s.gp[1],4);
    for(const auto [pc,p,reg]:{std::array{0x707f8u,0x4dc403u,0u},{0x707fdu,0x4dc418u,1u},
        {0x70803u,0x4dc407u,0u},{0x70808u,0x4dc41cu,1u},{0x7080eu,0x4dc40bu,0u},{0x70813u,0x4dc420u,1u}}) {
        instruction(pc);w(s.memory,s.observe,pc,p,4,s.gp[reg]);
    }
    instruction(0x70819);s.gp[2]=s.gp[4];
    instruction(0x7081c);w(s.memory,s.observe,0x7081c,0x4dc40f,4,s.gp[0]);
    instruction(0x70821);w(s.memory,s.observe,0x70821,0x4dc424,4,s.gp[1]);
    instruction(0x70827);s.push(0x70827,s.gp[2]);
    instruction(0x70828);w(s.memory,s.observe,0x70828,0x4dc413,4,s.gp[0]);
    instruction(0x7082d);w(s.memory,s.observe,0x7082d,0x4dc428,4,s.gp[1]);
    instruction(0x70833);s.push(0x70833,0x70838);s.required(0x70833,0x3d725d,false);
    instruction(0x70838);s.flags.logic(s.gp[0]&s.gp[0],4);
    instruction(0x7083a);if(s.flags.zf){finish(0x708b1);return;}
    instruction(0x7083c);s.flags.minus(s.gp[0],1,4);
    instruction(0x7083f);if(s.flags.zf){
        instruction(0x708aa);w(s.memory,s.observe,0x708aa,0x4dc3f2,1,0);finish(0x708b1);return;
    }
    instruction(0x70841);s.flags.logic(s.reg(0,1)&0x1c,1);
    instruction(0x70843);s.push(0x70843,s.gp[6]);
    instruction(0x70844);s.push(0x70844,s.gp[7]);
    instruction(0x70845);s.gp[1]=9;
    instruction(0x7084a);s.gp[6]=s.gp[4]+8;
    instruction(0x7084e);s.gp[7]=0x4dc3f3;
    instruction(0x70853);w(s.memory,s.observe,0x70853,0x4dc3f2,1,1);
    instruction(0x7085a);
    while(s.gp[1]) {
        w(s.memory,s.observe,0x7085a,s.gp[7],4,r(s.memory,s.gp[6],4));
        const auto delta=(s.other_flags&1024u)?0xfffffffcu:4u;s.gp[6]+=delta;s.gp[7]+=delta;--s.gp[1];instruction(0x7085a);
    }
    instruction(0x7085c);s.gp[7]=s.pop();
    instruction(0x7085d);s.gp[6]=s.pop();
    instruction(0x7085e);if(!s.flags.zf){
        instruction(0x70860);s.gp[0]=r(s.memory,s.gp[4],4);
        instruction(0x70863);s.gp[1]=s.gp[0];
        instruction(0x70865);s.gp[2]=s.gp[0];
        instruction(0x70867);s.gp[2]=s.flags.right(s.gp[2],16,4,false);
        instruction(0x7086a);s.gp[1]=s.flags.logic(s.gp[1]&0xff0000,4);
        instruction(0x70870);s.gp[1]=s.flags.logic(s.gp[1]|s.gp[2],4);
        instruction(0x70872);s.gp[2]=s.gp[0];
        instruction(0x70874);s.gp[2]=s.flags.logic(s.gp[2]&0xff00,4);
        instruction(0x7087a);s.gp[0]=s.flags.shift(s.gp[0],16,4);
        instruction(0x7087d);s.gp[2]=s.flags.logic(s.gp[2]|s.gp[0],4);
        instruction(0x7087f);s.gp[1]=s.flags.right(s.gp[1],8,4,false);
        instruction(0x70882);s.gp[2]=s.flags.shift(s.gp[2],8,4);
        instruction(0x70885);s.gp[1]=s.flags.logic(s.gp[1]|s.gp[2],4);
        instruction(0x70887);w(s.memory,s.observe,0x70887,0x4dc42a,2,4);
        instruction(0x70890);w(s.memory,s.observe,0x70890,0x4dc418,4,s.gp[1]);
        instruction(0x70896);w(s.memory,s.observe,0x70896,0x4dc428,2,0);
    }
    instruction(0x7089f);s.set(0,1,1);
    instruction(0x708a1);w(s.memory,s.observe,0x708a1,0x4dc3f1,1,s.reg(0,1));
    instruction(0x708a6);s.gp[4]=s.flags.plus(s.gp[4],0x24,4);
    instruction(0x708a9);s.pop();
}
}
void source_run_network_availability_startup_tu15(SourcePhysicsContactMemory& memory,
    SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observer) {
    FpScope scope(fp);WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    try{run(state);}catch(...){abi=state.save();throw;}abi=state.save();
}
void source_run_startup_noop_tu15(SourcePhysicsContactMemory& memory,
    SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices& services) {
    FpScope scope(fp);const SourcePhysicsContactRecordObserver observer{};
    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    try{if(services.instruction)services.instruction(0x31d400);state.pop();}catch(...){abi=state.save();throw;}abi=state.save();
}
}
