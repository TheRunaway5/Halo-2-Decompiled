// Four exact TU1.5 CRT callbacks: twelve authenticated source instructions.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/source_environment_crt_pair_tu15.hpp"
namespace halo2 {
namespace {
using namespace physics_world_runtime_detail;
static void run_380d50(WorldState &s) {
if(s.services.instruction){s.services.instruction(0x380d50u);} s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x44923cu),4)));
if(s.services.instruction){s.services.instruction(0x380d58u);} w(s.memory,s.observe,0x380d58u,(0x5564c4u),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction){s.services.instruction(0x380d60u);} s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("Original solver fell outside authenticated CFG");
}
static void run_380d30(WorldState &s) {
if(s.services.instruction){s.services.instruction(0x380d30u);} s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a2ecu),4)));
if(s.services.instruction){s.services.instruction(0x380d38u);} w(s.memory,s.observe,0x380d38u,(0x5564c8u),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction){s.services.instruction(0x380d40u);} s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("Original solver fell outside authenticated CFG");
}
static void run_381010(WorldState &s) {
if(s.services.instruction){s.services.instruction(0x381010u);} s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a2ecu),4)));
if(s.services.instruction){s.services.instruction(0x381018u);} w(s.memory,s.observe,0x381018u,(0x556554u),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction){s.services.instruction(0x381020u);} s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("Original solver fell outside authenticated CFG");
}
static void run_3811b0(WorldState &s) {
if(s.services.instruction){s.services.instruction(0x3811b0u);} s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a0d0u),4)));
if(s.services.instruction){s.services.instruction(0x3811b8u);} w(s.memory,s.observe,0x3811b8u,(0x556e10u),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction){s.services.instruction(0x3811c0u);} s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("Original solver fell outside authenticated CFG");
}
}
void source_run_environment_crt_pair_tu15(M &m,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,FP fp,
    const SourcePhysicsWorldRuntimeServices &services,const O &observe) {
    constexpr auto image=SourcePhysicsContactImage::TitleUpdate15;
    FpScope scope(fp);WorldState s{m,observe,services,image};s.load(abi);
    try {
        switch(entry) {
        case 0x380d50u:run_380d50(s);break;
        case 0x380d30u:run_380d30(s);break;
        case 0x381010u:run_381010(s);break;
        case 0x3811b0u:run_3811b0(s);break;
        default:throw std::runtime_error("Required original environment CRT entry unavailable");
        }
    } catch(...) {abi=s.save();throw;}
    abi=s.save();
}
}
