// Exact TU1.5 3817F0 CRT: authenticated MOVSS/MOVSS/RET instructions.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/source_ratio_threshold_crt_tu15.hpp"
namespace halo2 {
namespace {
using namespace physics_world_runtime_detail;
static void run_3817f0(WorldState &s) {
if(s.services.instruction){s.services.instruction(0x3817f0u);} s.xmm[0]=_mm_set_ss(f(r(s.memory,(0x46a4b0u),4)));
if(s.services.instruction){s.services.instruction(0x3817f8u);} w(s.memory,s.observe,0x3817f8u,(0x55d700u),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction){s.services.instruction(0x381800u);} s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("Original solver fell outside authenticated CFG");
}
}
void source_run_ratio_threshold_crt_tu15(M &m,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,FP fp,
    const SourcePhysicsWorldRuntimeServices &services,const O &observe) {
    constexpr auto image=SourcePhysicsContactImage::TitleUpdate15;
    FpScope scope(fp);WorldState s{m,observe,services,image};s.load(abi);
    try {
        if(entry!=0x3817f0u)throw std::runtime_error("Required original ratio CRT entry unavailable");
        run_3817f0(s);
    } catch(...) {abi=s.save();throw;}
    abi=s.save();
}
}
