// Generated complete physical stage and partition source bodies.
#include "physics_world_runtime_internal.hpp"
#include "halo2/physics_physical_storage_runtime.hpp"
namespace halo2 {namespace {using namespace physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){case 0x12cbb3u:return 0x12df53u;case 0x217070u:return 0x21b970u;case 0x2d79d9u:return 0x2db489u;case 0x2d79e6u:return 0x2db496u;case 0x2d81a6u:return 0x2dbc56u;case 0x417ba0u:return 0x41b640u;case 0x417ba8u:return 0x41b648u;case 0x4ed9f0u:return 0x4f2cf0u;case 0x4ed9f4u:return 0x4f2cf4u;case 0x4ed9f8u:return 0x4f2cf8u;case 0x4ed9fcu:return 0x4f2cfcu;case 0x4eda10u:return 0x4f2d10u;case 0x5098b0u:return 0x50ebb0u;case 0x5098b4u:return 0x50ebb4u;case 0x5098b8u:return 0x50ebb8u;default:throw std::runtime_error("Unbound physical stage operand");}}
static void run_12cb70(WorldState &);
static void run_217020(WorldState &);
static void run_2d79aa(WorldState &);
static void run_2d7a1a(WorldState &);
static void run_12cb70(WorldState &s){
if(s.services.instruction)s.services.instruction(0x12cb70u);s.push(0x12cb70u,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x12cb71u);s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x12cb73u);s.push(0x12cb73u,0x4u);
if(s.services.instruction)s.services.instruction(0x12cb75u);s.set(1,0xdu,4,0);
if(s.services.instruction)s.services.instruction(0x12cb7au);s.set(7,selected(0x4ed9f0u,s.image),4,0);
if(s.services.instruction)s.services.instruction(0x12cb7fu);while(s.gp[1]){w(s.memory,s.observe,0x12cb7fu,s.gp[7],4,s.gp[0]);s.gp[7]+=(s.other_flags&1024u)?0xfffffffcu:4u;--s.gp[1];if(s.services.instruction)s.services.instruction(0x12cb7fu);}
if(s.services.instruction)s.services.instruction(0x12cb81u);s.set(1,0x61000u,4,0);
if(s.services.instruction)s.services.instruction(0x12cb86u);s.push(0x12cb86u,0x0u);
if(s.services.instruction)s.services.instruction(0x12cb88u);w(s.memory,s.observe,0x12cb88u,(selected(0x4ed9f0u,s.image)),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x12cb8du);s.set(0,0x3145000u,4,0);
if(s.services.instruction)s.services.instruction(0x12cb92u);s.push(0x12cb92u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x12cb93u);s.push(0x12cb93u,0x30e4000u);
if(s.services.instruction)s.services.instruction(0x12cb98u);w(s.memory,s.observe,0x12cb98u,(selected(0x4ed9f4u,s.image)),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x12cb9eu);w(s.memory,s.observe,0x12cb9eu,(selected(0x4ed9f8u,s.image)),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x12cba3u);w(s.memory,s.observe,0x12cba3u,(selected(0x4ed9fcu,s.image)),4,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x12cba9u);w(s.memory,s.observe,0x12cba9u,(selected(0x4eda10u,s.image)),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x12cbaeu);s.push(0x12cbaeu,selected(0x12cbb3u,s.image));run_2d79aa(s);
if(s.services.instruction)s.services.instruction(0x12cbb3u);s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x12cbb4u);s.pop();s.gp[4]+=0u;return;
}

static void run_217020(WorldState &s){
if(s.services.instruction)s.services.instruction(0x217020u);s.set(2,r(s.memory,(selected(0x4ed9f0u,s.image)),4),4,0);
if(s.services.instruction)s.services.instruction(0x217026u);s.push(0x217026u,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x217027u);s.push(0x217027u,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x217028u);s.set(5,r(s.memory,(selected(0x4ed9fcu,s.image)+s.gp[2]*4u),4),4,0);
if(s.services.instruction)s.services.instruction(0x21702fu);s.set(3,(0x0u+s.gp[1]+s.gp[0]*1u),4,0);
if(s.services.instruction)s.services.instruction(0x217032u);s.set(0,r(s.memory,(selected(0x4eda10u,s.image)+s.gp[2]*4u),4),4,0);
if(s.services.instruction)s.services.instruction(0x217039u);s.push(0x217039u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x21703au);s.set(1,(0xfffu+s.gp[3]),4,0);
if(s.services.instruction)s.services.instruction(0x217040u);s.set(6,(selected(0x4eda10u,s.image)+s.gp[2]*4u),4,0);
if(s.services.instruction)s.services.instruction(0x217047u);s.set(1,s.flags.logic((s.reg(1,4,0))&(0xfffff000u),4),4,0);
if(s.services.instruction)s.services.instruction(0x21704du);s.push(0x21704du,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x21704eu);s.set(0,s.flags.minus(s.reg(0,4,0),s.reg(1,4,0),4),4,0);
if(s.services.instruction)s.services.instruction(0x217050u);s.set(7,s.flags.logic((s.reg(7,4,0))^(s.reg(7,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x217052u);s.flags.minus(s.reg(0,4,0),s.reg(5,4,0),4);
if(s.services.instruction)s.services.instruction(0x217054u);if(s.flags.sf!=s.flags.of)goto L217070;
if(s.services.instruction)s.services.instruction(0x217056u);s.flags.logic(s.reg(0,4,0)&s.reg(0,4,0),4);
if(s.services.instruction)s.services.instruction(0x217058u);w(s.memory,s.observe,0x217058u,(0x0u+s.gp[6]),4,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x21705au);s.set(7,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x21705cu);if(s.flags.zf)goto L217070;
if(s.services.instruction)s.services.instruction(0x21705eu);s.set(0,s.flags.logic((s.reg(0,4,0))|(0x80000000u),4),4,0);
if(s.services.instruction)s.services.instruction(0x217063u);s.set(7,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x217065u);if(s.flags.zf)goto L217070;
if(s.services.instruction)s.services.instruction(0x217067u);s.push(0x217067u,0x4u);
if(s.services.instruction)s.services.instruction(0x217069u);s.push(0x217069u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x21706au);s.push(0x21706au,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x21706bu);s.push(0x21706bu,selected(0x217070u,s.image));run_2d7a1a(s);
L217070: if(s.services.instruction)s.services.instruction(0x217070u);w(s.memory,s.observe,0x217070u,(selected(0x5098b4u,s.image)),4,s.reg(7,4,0));
if(s.services.instruction)s.services.instruction(0x217076u);s.set(0,s.reg(7,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x217078u);s.set(7,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x217079u);s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x21707au);s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x21707bu);w(s.memory,s.observe,0x21707bu,(selected(0x5098b8u,s.image)),4,s.reg(3,4,0));
if(s.services.instruction)s.services.instruction(0x217081u);w(s.memory,s.observe,0x217081u,(selected(0x5098b0u,s.image)),1,0x1u);
if(s.services.instruction)s.services.instruction(0x217088u);s.set(3,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x217089u);s.pop();s.gp[4]+=0u;return;
}

static void run_2d79aa(WorldState &s){
if(s.services.instruction)s.services.instruction(0x2d79aau);s.push(0x2d79aau,s.reg(5,4,0));
if(s.services.instruction)s.services.instruction(0x2d79abu);s.set(5,s.reg(4,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x2d79adu);s.set(0,r(s.memory,(0xcu+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2d79b0u);s.flags.minus(s.reg(0,4,0),0xffffffffu,4);
if(s.services.instruction)s.services.instruction(0x2d79b3u);s.set(1,r(s.memory,(0x8u+s.gp[5]),4),4,0);
if(s.services.instruction)s.services.instruction(0x2d79b6u);if(s.flags.zf)goto L2d79c4;
if(s.services.instruction)s.services.instruction(0x2d79b8u);w(s.memory,s.observe,0x2d79b8u,(0x10u+s.gp[5]),4,s.flags.logic((r(s.memory,(0x10u+s.gp[5]),4))&(0x0u),4));
if(s.services.instruction)s.services.instruction(0x2d79bcu);s.set(2,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x2d79beu);s.set(0,(0xffffffffu+s.gp[1]+s.gp[0]*1u),4,0);
if(s.services.instruction)s.services.instruction(0x2d79c2u);goto L2d79c9;
L2d79c4: if(s.services.instruction)s.services.instruction(0x2d79c4u);s.set(2,s.flags.logic((s.reg(2,4,0))^(s.reg(2,4,0)),4),4,0);
if(s.services.instruction)s.services.instruction(0x2d79c6u);s.set(0,s.flags.logic((s.reg(0,4,0))|(0xffffffffu),4),4,0);
L2d79c9: if(s.services.instruction)s.services.instruction(0x2d79c9u);s.push(0x2d79c9u,s.reg(6,4,0));
if(s.services.instruction)s.services.instruction(0x2d79cau);s.push(0x2d79cau,r(s.memory,(0x14u+s.gp[5]),4));
if(s.services.instruction)s.services.instruction(0x2d79cdu);s.push(0x2d79cdu,r(s.memory,(0x10u+s.gp[5]),4));
if(s.services.instruction)s.services.instruction(0x2d79d0u);s.push(0x2d79d0u,s.reg(0,4,0));
if(s.services.instruction)s.services.instruction(0x2d79d1u);s.push(0x2d79d1u,s.reg(2,4,0));
if(s.services.instruction)s.services.instruction(0x2d79d2u);s.push(0x2d79d2u,s.reg(1,4,0));
if(s.services.instruction)s.services.instruction(0x2d79d3u);s.push(0x2d79d3u,selected(0x2d79d9u,s.image));s.required(0x2d79d3u,r(s.memory,(selected(0x417ba0u,s.image)),4),false);
if(s.services.instruction)s.services.instruction(0x2d79d9u);s.set(6,s.reg(0,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x2d79dbu);s.flags.logic(s.reg(6,4,0)&s.reg(6,4,0),4);
if(s.services.instruction)s.services.instruction(0x2d79ddu);if(!s.flags.zf)goto L2d79e6;
if(s.services.instruction)s.services.instruction(0x2d79dfu);s.push(0x2d79dfu,0x8u);
if(s.services.instruction)s.services.instruction(0x2d79e1u);s.push(0x2d79e1u,selected(0x2d79e6u,s.image));s.required(0x2d79e1u,selected(0x2d81a6u,s.image),false);
L2d79e6: if(s.services.instruction)s.services.instruction(0x2d79e6u);s.set(0,s.reg(6,4,0),4,0);
if(s.services.instruction)s.services.instruction(0x2d79e8u);s.set(6,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x2d79e9u);s.set(5,s.pop(),4,0);
if(s.services.instruction)s.services.instruction(0x2d79eau);s.pop();s.gp[4]+=16u;return;
}

static void run_2d7a1a(WorldState &s){
if(s.services.instruction)s.services.instruction(0x2d7a1au);s.flags.minus(r(s.memory,(0x8u+s.gp[4]),4),0x0u,4);
if(s.services.instruction)s.services.instruction(0x2d7a1fu);if(s.flags.zf)goto L2d7a27;
if(s.services.instruction)s.services.instruction(0x2d7a21u);s.required(0x2d7a21u,r(s.memory,(selected(0x417ba8u,s.image)),4),true);return;
L2d7a27: if(s.services.instruction)s.services.instruction(0x2d7a27u);s.pop();s.gp[4]+=12u;return;
}
}
void source_run_physics_physical_storage_runtime(SourcePhysicsContactMemory &m,
 std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,SourcePhysicsContactImage image,
 SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices &services,
 const SourcePhysicsContactRecordObserver &observe){
 if(image!=SourcePhysicsContactImage::MapPack11 && image!=SourcePhysicsContactImage::TitleUpdate15)
  throw std::runtime_error("Unverified physical stage image");
 FpScope scope(fp);WorldState s{m,observe,services,image};s.load(abi);
 try{switch(entry){
case 0x12cb70u:run_12cb70(s);break;
case 0x217020u:run_217020(s);break;
case 0x2d79aau:run_2d79aa(s);break;
case 0x2d7a1au:run_2d7a1a(s);break;
default:throw std::runtime_error("Unverified physical stage entry");}}
 catch(...){abi=s.save();throw;}abi=s.save();}
} // namespace halo2
