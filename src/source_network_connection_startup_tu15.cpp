// Complete authenticated TU1.5 6E010 instruction lowering (295 bytes).
// SHA256 250f77c6b998f2fcb3742c0f5304a2c4d3900eaa32b27dc6572ca26ac477b674.
// Each external API/subsystem keeps its pending frame until its owner runs.
#include "physics_world_runtime_internal.hpp"
#include "halo2/source_network_storage_startup_tu15.hpp"

namespace halo2 {
namespace {
using namespace physics_world_runtime_detail;
void run(WorldState& s) {
    const auto instruction=[&](std::uint32_t pc){if(s.services.instruction)s.services.instruction(pc);};
    const auto call=[&](std::uint32_t pc,std::uint32_t target,std::uint32_t next){
        instruction(pc);s.push(pc,next);s.required(pc,target,false);
    };
    const auto epilogue=[&](std::uint32_t pc){
        instruction(pc);s.gp[7]=s.pop();
        instruction(pc+1);s.gp[6]=s.pop();
        instruction(pc+2);s.gp[4]=s.gp[5];
        instruction(pc+4);s.gp[5]=s.pop();
        instruction(pc+5);s.pop();
    };
    instruction(0x6e010);s.push(0x6e010,s.gp[5]);
    instruction(0x6e011);s.gp[5]=s.gp[4];
    instruction(0x6e013);s.gp[4]=s.flags.logic(s.gp[4]&0xfffffff8u,4);
    instruction(0x6e016);s.gp[4]=s.flags.minus(s.gp[4],0x1a0,4);
    instruction(0x6e01c);s.set(0,r(s.memory,0x4dc061,1),1);
    instruction(0x6e021);s.flags.logic(s.reg(0,1)&s.reg(0,1),1);
    instruction(0x6e023);s.push(0x6e023,s.gp[6]);
    instruction(0x6e024);s.push(0x6e024,s.gp[7]);
    instruction(0x6e025);if(!s.flags.zf){epilogue(0x6e131);return;}
    instruction(0x6e02b);s.gp[0]=s.flags.logic(s.gp[0]^s.gp[0],4);
    instruction(0x6e02d);w(s.memory,s.observe,0x6e02d,s.gp[4]+0x18,2,0);
    instruction(0x6e034);s.gp[1]=0x63;
    instruction(0x6e039);s.gp[7]=s.gp[4]+0x1a;
    instruction(0x6e03d);
    while(s.gp[1]) {
        w(s.memory,s.observe,0x6e03d,s.gp[7],4,s.gp[0]);
        s.gp[7]+=(s.other_flags&1024u)?0xfffffffcu:4u;--s.gp[1];instruction(0x6e03d);
    }
    instruction(0x6e03f);w(s.memory,s.observe,0x6e03f,s.gp[7],2,s.reg(0,2));s.gp[7]+=(s.other_flags&1024u)?0xfffffffeu:2u;
    instruction(0x6e041);s.gp[0]=s.flags.logic(s.gp[0]^s.gp[0],4);
    instruction(0x6e043);w(s.memory,s.observe,0x6e043,s.gp[4]+9,4,s.gp[0]);
    instruction(0x6e047);w(s.memory,s.observe,0x6e047,s.gp[4]+0xd,4,s.gp[0]);
    instruction(0x6e04b);w(s.memory,s.observe,0x6e04b,s.gp[4]+0x11,2,s.reg(0,2));
    instruction(0x6e050);w(s.memory,s.observe,0x6e050,s.gp[4]+0x13,1,s.reg(0,1));
    instruction(0x6e054);s.set(0,0xc,1);
    instruction(0x6e056);w(s.memory,s.observe,0x6e056,s.gp[4]+8,1,s.reg(0,1));
    instruction(0x6e05a);w(s.memory,s.observe,0x6e05a,s.gp[4]+0xa,1,s.reg(0,1));
    instruction(0x6e05e);s.set(0,8,1);
    instruction(0x6e060);w(s.memory,s.observe,0x6e060,s.gp[4]+9,1,0);
    instruction(0x6e065);w(s.memory,s.observe,0x6e065,s.gp[4]+0xb,1,s.reg(0,1));
    instruction(0x6e069);w(s.memory,s.observe,0x6e069,s.gp[4]+0xc,1,4);
    instruction(0x6e06e);w(s.memory,s.observe,0x6e06e,s.gp[4]+0xd,1,s.reg(0,1));
    instruction(0x6e072);w(s.memory,s.observe,0x6e072,s.gp[4]+0xe,1,0x18);
    instruction(0x6e077);w(s.memory,s.observe,0x6e077,s.gp[4]+0xf,1,s.reg(0,1));
    instruction(0x6e07b);w(s.memory,s.observe,0x6e07b,s.gp[4]+0x10,1,s.reg(0,1));
    instruction(0x6e07f);w(s.memory,s.observe,0x6e07f,s.gp[4]+0x11,1,s.reg(0,1));
    instruction(0x6e083);w(s.memory,s.observe,0x6e083,s.gp[4]+0x12,1,0x28);
    instruction(0x6e088);w(s.memory,s.observe,0x6e088,s.gp[4]+0x13,1,0xff);
    call(0x6e08d,0x37654d,0x6e092);
    instruction(0x6e092);s.gp[1]=s.gp[4]+8;
    instruction(0x6e096);s.push(0x6e096,s.gp[1]);
    call(0x6e097,0x3d7656,0x6e09c);
    instruction(0x6e09c);s.flags.logic(s.gp[0]&s.gp[0],4);
    instruction(0x6e09e);if(!s.flags.zf){epilogue(0x6e131);return;}
    instruction(0x6e0a4);s.gp[2]=s.gp[4]+0x18;
    instruction(0x6e0a8);s.push(0x6e0a8,s.gp[2]);
    instruction(0x6e0a9);s.push(0x6e0a9,2);
    call(0x6e0ab,0x3d766d,0x6e0b0);
    instruction(0x6e0b0);s.flags.logic(s.gp[0]&s.gp[0],4);
    instruction(0x6e0b2);if(!s.flags.zf){call(0x6e12c,0x3d775b,0x6e131);epilogue(0x6e131);return;}
    instruction(0x6e0b4);w(s.memory,s.observe,0x6e0b4,s.gp[4]+0x14,4,s.gp[0]);
    instruction(0x6e0b8);s.gp[0]=s.gp[4]+0x14;
    instruction(0x6e0bc);s.push(0x6e0bc,s.gp[0]);
    call(0x6e0bd,0x3aad19,0x6e0c2);
    instruction(0x6e0c2);s.flags.logic(s.gp[0]&s.gp[0],4);
    instruction(0x6e0c4);if(!s.flags.zf){
        call(0x6e127,0x3d7768,0x6e12c);call(0x6e12c,0x3d775b,0x6e131);epilogue(0x6e131);return;
    }
    instruction(0x6e0c6);w(s.memory,s.observe,0x6e0c6,0x4dc061,1,1);
    call(0x6e0cd,0x70ce0,0x6e0d2);
    call(0x6e0d2,0x707d0,0x6e0d7);
    instruction(0x6e0d7);s.set(0,r(s.memory,0x4dc534,1),1);
    instruction(0x6e0dc);s.flags.logic(s.reg(0,1)&s.reg(0,1),1);
    instruction(0x6e0de);if(s.flags.zf){
        instruction(0x6e0e0);s.gp[0]=r(s.memory,0x4dc538,4);
        instruction(0x6e0e5);s.gp[6]=s.gp[0];
        instruction(0x6e0e7);w(s.memory,s.observe,0x6e0e7,s.gp[0]+0x29,1,1);
        call(0x6e0eb,0x16e600,0x6e0f0);
        instruction(0x6e0f0);w(s.memory,s.observe,0x6e0f0,0x4dc534,1,1);
    }
    instruction(0x6e0f7);s.gp[0]=r(s.memory,0x4dc064,4);
    instruction(0x6e0fc);s.gp[6]=s.flags.logic(s.gp[6]^s.gp[6],4);
    instruction(0x6e0fe);s.flags.logic(s.gp[0]&s.gp[0],4);
    instruction(0x6e100);if(s.flags.zf || s.flags.sf!=s.flags.of){epilogue(0x6e131);return;}
    for(;;) {
        instruction(0x6e102);s.gp[0]=r(s.memory,0x4dc068+s.gp[6]*4,4);
        instruction(0x6e109);s.flags.logic(s.gp[0]&s.gp[0],4);
        instruction(0x6e10b);if(!s.flags.zf){
            instruction(0x6e10d);s.gp[1]=r(s.memory,0x4dc0c8+s.gp[6]*4,4);
            instruction(0x6e114);s.push(0x6e114,s.gp[1]);call(0x6e115,s.gp[0],0x6e117);
        }
        instruction(0x6e117);s.gp[0]=r(s.memory,0x4dc064,4);
        instruction(0x6e11c);s.gp[6]=s.flags.increment(s.gp[6],4,false);
        instruction(0x6e11d);s.flags.minus(s.gp[6],s.gp[0],4);
        instruction(0x6e11f);if(s.flags.sf==s.flags.of)break;
    }
    epilogue(0x6e121);
}
}
void source_run_network_connection_startup_tu15(SourcePhysicsContactMemory& memory,
    SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observer) {
    FpScope scope(fp);WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    try{run(state);}catch(...){abi=state.save();throw;}abi=state.save();
}
}
