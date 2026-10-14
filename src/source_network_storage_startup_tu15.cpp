// Authenticated TU1.5 source bodies. Per-body byte SHA256 values are verified
// by tools/oracle_native_tu15_network_storage.py; no executable is read here.
#include "physics_world_runtime_internal.hpp"
#include "halo2/source_network_storage_startup_tu15.hpp"

namespace halo2 {
namespace {
using namespace physics_world_runtime_detail;
void instruction(WorldState& s,std::uint32_t pc){if(s.services.instruction)s.services.instruction(pc);}
void clear_words(WorldState& s,std::uint32_t pc) {
    instruction(s,pc);
    while(s.gp[1]) {w(s.memory,s.observe,pc,s.gp[7],4,s.gp[0]);s.gp[7]+=(s.other_flags&1024u)?0xfffffffcu:4u;--s.gp[1];instruction(s,pc);}
}
void run_70660(WorldState& s) {
    instruction(s,0x70660);s.gp[0]=s.flags.logic(s.gp[0]^s.gp[0],4);
    instruction(s,0x70662);s.push(0x70662,s.gp[7]);
    instruction(s,0x70663);s.gp[1]=0x51;
    instruction(s,0x70668);s.gp[7]=0x4dc3f0;
    clear_words(s,0x7066d);
    instruction(s,0x7066f);s.gp[0]=r(s.memory,0x4dc064,4);
    instruction(s,0x70674);s.gp[1]=s.flags.logic(s.gp[1]^s.gp[1],4);
    instruction(s,0x70676);w(s.memory,s.observe,0x70676,0x4dc068+s.gp[0]*4,4,s.gp[1]);
    instruction(s,0x7067d);s.gp[2]=r(s.memory,0x4dc064,4);
    instruction(s,0x70683);s.gp[0]=0x707c0;
    instruction(s,0x70688);w(s.memory,s.observe,0x70688,0x4dc088+s.gp[2]*4,4,s.gp[0]);
    instruction(s,0x7068f);s.gp[2]=r(s.memory,0x4dc064,4);
    instruction(s,0x70695);w(s.memory,s.observe,0x70695,0x4dc0a8+s.gp[2]*4,4,s.gp[0]);
    instruction(s,0x7069c);s.gp[0]=r(s.memory,0x4dc064,4);
    instruction(s,0x706a1);w(s.memory,s.observe,0x706a1,0x4dc0c8+s.gp[0]*4,4,s.gp[1]);
    instruction(s,0x706a8);w(s.memory,s.observe,0x706a8,0x4dc064,4,s.flags.increment(r(s.memory,0x4dc064,4),4,false));
    instruction(s,0x706ae);w(s.memory,s.observe,0x706ae,0x4dc3f0,1,1);
    instruction(s,0x706b5);s.gp[7]=s.pop();
    instruction(s,0x706b6);s.pop();
}
void run_7ec90(WorldState& s) {
    instruction(s,0x7ec90);s.gp[1]=r(s.memory,0x474cb8,4);
    instruction(s,0x7ec96);s.push(0x7ec96,s.gp[6]);
    instruction(s,0x7ec97);s.gp[0]=s.flags.logic(s.gp[0]^s.gp[0],4);
    instruction(s,0x7ec99);w(s.memory,s.observe,0x7ec99,0x4dc534,4,s.gp[0]);
    instruction(s,0x7ec9e);s.push(0x7ec9e,s.gp[7]);
    instruction(s,0x7ec9f);w(s.memory,s.observe,0x7ec9f,0x4dc538,4,s.gp[0]);
    instruction(s,0x7eca4);s.gp[2]=r(s.memory,s.gp[1],4);
    instruction(s,0x7eca6);s.push(0x7eca6,0x150);
    instruction(s,0x7ecab);s.gp[7]=s.gp[1];
    instruction(s,0x7ecad);const auto allocator=r(s.memory,s.gp[2],4);s.push(0x7ecad,0x7ecaf);s.required(0x7ecad,allocator,false);
    instruction(s,0x7ecaf);s.gp[6]=s.gp[0];
    instruction(s,0x7ecb1);s.flags.logic(s.gp[6]&s.gp[6],4);
    instruction(s,0x7ecb3);
    if(!s.flags.zf) {
        instruction(s,0x7ecb5);s.push(0x7ecb5,s.gp[3]);
        instruction(s,0x7ecb6);s.push(0x7ecb6,s.gp[7]);
        instruction(s,0x7ecb7);s.push(0x7ecb7,0x45c9b4);
        instruction(s,0x7ecbc);s.push(0x7ecbc,s.gp[6]);
        instruction(s,0x7ecbd);s.gp[1]=s.flags.logic(s.gp[1]^s.gp[1],4);
        instruction(s,0x7ecbf);s.gp[2]=8;
        instruction(s,0x7ecc4);s.gp[3]=0x20;
        instruction(s,0x7ecc9);s.push(0x7ecc9,0x7ecce);s.required(0x7ecc9,0x16e450,false);
        instruction(s,0x7ecce);w(s.memory,s.observe,0x7ecce,s.gp[6]+0x2a,1,s.flags.logic(r(s.memory,s.gp[6]+0x2a,1)|4,1));
        instruction(s,0x7ecd2);s.gp[3]=s.pop();
    }
    instruction(s,0x7ecd3);s.gp[7]=s.pop();
    instruction(s,0x7ecd4);w(s.memory,s.observe,0x7ecd4,0x4dc538,4,s.gp[6]);
    instruction(s,0x7ecda);s.gp[6]=s.pop();
    instruction(s,0x7ecdb);s.pop();
}
void run_6de60(WorldState& s) {
    instruction(s,0x6de60);s.push(0x6de60,s.gp[7]);
    instruction(s,0x6de61);s.gp[0]=s.flags.logic(s.gp[0]^s.gp[0],4);
    instruction(s,0x6de63);s.gp[1]=0x22;
    instruction(s,0x6de68);s.gp[7]=0x4dc060;
    clear_words(s,0x6de6d);
    instruction(s,0x6de6f);s.push(0x6de6f,0x6de74);run_70660(s);
    instruction(s,0x6de74);s.push(0x6de74,0x6de79);run_7ec90(s);
    instruction(s,0x6de79);w(s.memory,s.observe,0x6de79,0x4dc060,1,1);
    instruction(s,0x6de80);s.push(0x6de80,0x6de85);s.required(0x6de80,0x37654d,false);
    instruction(s,0x6de85);s.flags.minus(r(s.memory,0x56e1cc,4),s.gp[0],4);
    instruction(s,0x6de8b);s.gp[7]=s.pop();
    instruction(s,0x6de8c);if(!s.flags.zf){instruction(s,0x6de8e);w(s.memory,s.observe,0x6de8e,0x56e1cc,4,s.gp[0]);}
    instruction(s,0x6de93);s.flags.logic(s.reg(0,1,0)&1,1);
    instruction(s,0x6de95);
    if(!s.flags.zf){instruction(s,0x6de97);s.required(0x6de97,0x6e010,true);return;}
    instruction(s,0x6de9c);s.pop();
}
}
void source_run_network_storage_startup_tu15(SourcePhysicsContactMemory& memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observer) {
    FpScope scope(fp);WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    try {
        switch(entry){case 0x6de60:run_6de60(state);break;case 0x70660:run_70660(state);break;case 0x7ec90:run_7ec90(state);break;
        default:throw std::invalid_argument("Unowned native network-storage startup entry");}
    }catch(...){abi=state.save();throw;}abi=state.save();
}
}
