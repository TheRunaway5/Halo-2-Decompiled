// Generated from unchanged English TU1.5 only; no production decoder.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/source_tu15_settings.hpp"
namespace halo2 { namespace {
using namespace physics_world_runtime_detail;
static void run_2e101c(WorldState &);
static void run_2e0fe2(WorldState &s) {
if(s.services.instruction)s.services.instruction(0x2e0fe2u);s.push(0x2e0fe2u,s.gp[5]);
if(s.services.instruction)s.services.instruction(0x2e0fe3u);s.gp[5]=s.gp[4];
if(s.services.instruction)s.services.instruction(0x2e0fe5u);s.push(0x2e0fe5u,r(s.memory,(s.gp[5]+0x18u),4));
if(s.services.instruction)s.services.instruction(0x2e0fe8u);s.push(0x2e0fe8u,r(s.memory,(s.gp[5]+0x14u),4));
if(s.services.instruction)s.services.instruction(0x2e0febu);s.push(0x2e0febu,r(s.memory,(s.gp[5]+0x10u),4));
if(s.services.instruction)s.services.instruction(0x2e0feeu);s.push(0x2e0feeu,r(s.memory,(s.gp[5]+0xcu),4));
if(s.services.instruction)s.services.instruction(0x2e0ff1u);s.push(0x2e0ff1u,r(s.memory,(s.gp[5]+0x8u),4));
if(s.services.instruction)s.services.instruction(0x2e0ff4u);s.push(0x2e0ff4u,0x2e0ff9u);run_2e101c(s);
if(s.services.instruction)s.services.instruction(0x2e0ff9u);s.push(0x2e0ff9u,s.gp[0]);
if(s.services.instruction)s.services.instruction(0x2e0ffau);s.push(0x2e0ffau,0x2e1000u);s.required(0x2e0ffau,r(s.memory,(0x41b610u),4),false);
if(s.services.instruction)s.services.instruction(0x2e1000u);s.gp[5]=s.pop();
if(s.services.instruction)s.services.instruction(0x2e1001u);s.pop();s.gp[4]+=20u;return;
throw std::runtime_error("Original TU1.5 settings fell outside complete CFG");
}
static void run_2e101c(WorldState &s) {
if(s.services.instruction)s.services.instruction(0x2e101cu);s.required(0x2e101cu,r(s.memory,(0x41b6f0u),4),true);return;
throw std::runtime_error("Original TU1.5 settings fell outside complete CFG");
}
} // namespace
void source_run_tu15_settings_runtime(SourcePhysicsContactMemory &m,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsContactRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    FpScope scope(fp);WorldState s{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try {run_2e0fe2(s);} catch(...) {abi=s.save();throw;}
    abi=s.save();
}
} // namespace halo2
