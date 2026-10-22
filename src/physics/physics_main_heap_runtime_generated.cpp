// Generated from complete authenticated source; no production decoder.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/physics_main_heap_runtime.hpp"
namespace halo2 { namespace {
using namespace physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){case 0x1482b5u:return 0x149645u;case 0x1482beu:return 0x14964eu;case 0x2d817eu:return 0x2dbc2eu;case 0x33785cu:return 0x33b3acu;case 0x480d90u:return 0x48605cu;default:throw std::runtime_error("Unbound original main heap operand");}}
void producer(WorldState &s){
if(s.services.instruction)s.services.instruction(0x1482a0u);s.push(0x1482a0u,s.gp[6]);
if(s.services.instruction)s.services.instruction(0x1482a1u);s.push(0x1482a1u,0x4u);
if(s.services.instruction)s.services.instruction(0x1482a3u);s.push(0x1482a3u,0x101000u);
if(s.services.instruction)s.services.instruction(0x1482a8u);s.push(0x1482a8u,0x106470u);
if(s.services.instruction)s.services.instruction(0x1482adu);s.gp[6]=s.flags.logic(s.gp[6]^s.gp[6],4);
if(s.services.instruction)s.services.instruction(0x1482afu);s.push(0x1482afu,s.gp[6]);
if(s.services.instruction)s.services.instruction(0x1482b0u);s.push(0x1482b0u,selected(0x1482b5u,s.image));s.required(0x1482b0u,selected(0x33785cu,s.image),false);
if(s.services.instruction)s.services.instruction(0x1482b5u);s.flags.logic(s.gp[0]&s.gp[0],4);
if(s.services.instruction)s.services.instruction(0x1482b7u);if(!s.flags.zf)goto L1482c6;
if(s.services.instruction)s.services.instruction(0x1482b9u);s.push(0x1482b9u,selected(0x1482beu,s.image));s.required(0x1482b9u,selected(0x2d817eu,s.image),false);
if(s.services.instruction)s.services.instruction(0x1482beu);w(s.memory,s.observe,0x1482beu,(selected(0x480d90u,s.image)),4,s.gp[6]);
if(s.services.instruction)s.services.instruction(0x1482c4u);s.gp[6]=s.pop();
if(s.services.instruction)s.services.instruction(0x1482c5u);s.pop();return;
L1482c6: if(s.services.instruction)s.services.instruction(0x1482c6u);s.gp[1]=(0x1000u+s.gp[0]);
if(s.services.instruction)s.services.instruction(0x1482ccu);s.gp[1]=s.flags.logic(s.gp[1]&0xfffff000u,4);
if(s.services.instruction)s.services.instruction(0x1482d2u);s.gp[2]=s.gp[1];
if(s.services.instruction)s.services.instruction(0x1482d4u);s.gp[2]=s.flags.minus(s.gp[2],s.gp[0],4);
if(s.services.instruction)s.services.instruction(0x1482d6u);w(s.memory,s.observe,0x1482d6u,(0xfffffff8u+s.gp[1]),4,0x105470u);
if(s.services.instruction)s.services.instruction(0x1482ddu);w(s.memory,s.observe,0x1482ddu,(0xfffffffcu+s.gp[1]),4,s.gp[2]);
if(s.services.instruction)s.services.instruction(0x1482e0u);w(s.memory,s.observe,0x1482e0u,(selected(0x480d90u,s.image)),4,s.gp[1]);
if(s.services.instruction)s.services.instruction(0x1482e6u);s.gp[6]=s.pop();
if(s.services.instruction)s.services.instruction(0x1482e7u);s.pop();return;
}
} // namespace
void source_run_physics_main_heap_runtime(SourcePhysicsContactMemory &m,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,SourcePhysicsContactImage image,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  if(entry!=0x1482a0u || (image!=SourcePhysicsContactImage::MapPack11 && image!=SourcePhysicsContactImage::TitleUpdate15))
    throw std::runtime_error("Unverified original main heap producer entry/image");
  FpScope scope(fp);WorldState s{m,observe,services,image};s.load(abi);
  try {producer(s);} catch(...) {abi=s.save();throw;}
  abi=s.save();
}
} // namespace halo2
