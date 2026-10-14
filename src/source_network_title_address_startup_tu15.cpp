// Complete authenticated TU1.5 70CE0 body, 74 bytes.
// SHA256 9acaff9cb8b6feb4cf8adb25b99b882b69de7116cb15155db3171ce9a0c9c3e2.
#include "physics_world_runtime_internal.hpp"
#include "halo2/source_network_storage_startup_tu15.hpp"
namespace halo2 {
void source_run_network_title_address_startup_tu15(SourcePhysicsContactMemory& memory,
    SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observer) {
    using namespace physics_world_runtime_detail;
    FpScope scope(fp);WorldState s{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    const auto instruction=[&](std::uint32_t pc){if(s.services.instruction)s.services.instruction(pc);};
    try {
        instruction(0x70ce0);s.gp[4]=s.flags.minus(s.gp[4],0x24,4);
        instruction(0x70ce3);s.gp[0]=s.flags.logic(s.gp[0]^s.gp[0],4);
        for(const auto [pc,offset]:{std::pair{0x70ce5u,0u},{0x70ce8u,4u},{0x70cecu,8u},
            {0x70cf0u,0xcu},{0x70cf4u,0x10u},{0x70cf8u,0x14u},{0x70cfcu,0x18u}}) {
            instruction(pc);w(s.memory,s.observe,pc,s.gp[4]+offset,4,s.gp[0]);
        }
        instruction(0x70d00);s.gp[1]=s.gp[4];
        instruction(0x70d03);w(s.memory,s.observe,0x70d03,s.gp[4]+0x1c,4,s.gp[0]);
        instruction(0x70d07);s.push(0x70d07,s.gp[1]);
        instruction(0x70d08);w(s.memory,s.observe,0x70d08,s.gp[4]+0x24,4,s.gp[0]);
        instruction(0x70d0c);s.push(0x70d0c,0x70d11);s.required(0x70d0c,0x3d725d,false);
        instruction(0x70d11);s.gp[2]=r(s.memory,s.gp[4]+0xa,4);
        instruction(0x70d15);s.set(0,r(s.memory,s.gp[4]+0xe,2),2);
        instruction(0x70d1a);w(s.memory,s.observe,0x70d1a,0x4dc42c,4,s.gp[2]);
        instruction(0x70d20);w(s.memory,s.observe,0x70d20,0x4dc430,2,s.reg(0,2));
        instruction(0x70d26);s.gp[4]=s.flags.plus(s.gp[4],0x24,4);
        instruction(0x70d29);s.pop();
    }catch(...){abi=s.save();throw;}abi=s.save();
}
}
