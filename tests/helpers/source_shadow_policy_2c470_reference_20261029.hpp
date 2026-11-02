#pragma once
// Test-only historical CPU reference; never included by a production source.
// Exact35-site body from source_shadow_frame_tu15_generated.cpp SHA256
// c16e116e8d9d979eda638e363b0e6731de432bfa99834417eb614b1ba294750e.
// Body SHA256 9adafad262bd1c96cbc296579c5b9e059df25a34bd07310030368b6b71b863a3.
#include "physics/runtime/physics_world_runtime_internal.hpp"
namespace halo2::test {
namespace shadow_policy_2c470_reference_detail {
using namespace physics_world_runtime_detail;
inline std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
static void run_2c470(WorldState &s) {
if(s.services.instruction){s.services.instruction(0x2c470u);} s.set(0,s.flags.minus(s.reg(0,4,0),selected(0x2u,s.image),4),4,0);
if(s.services.instruction){s.services.instruction(0x2c473u);} if(s.flags.zf)goto L2c4dc;
if(s.services.instruction){s.services.instruction(0x2c475u);} s.set(0,s.flags.increment(s.reg(0,4,0),4,true),4,0);
if(s.services.instruction){s.services.instruction(0x2c476u);} if(s.flags.zf)goto L2c4dc;
if(s.services.instruction){s.services.instruction(0x2c478u);} s.set(0,s.flags.increment(s.reg(0,4,0),4,true),4,0);
if(s.services.instruction){s.services.instruction(0x2c479u);} if(s.flags.zf)goto L2c4b0;
if(s.services.instruction){s.services.instruction(0x2c47bu);} s.xmm[0]=_mm_set_ss(f(r(s.memory,(selected(0x46a140u,s.image)),4)));
if(s.services.instruction){s.services.instruction(0x2c483u);} w(s.memory,s.observe,0x2c483u,(selected(0x473644u,s.image)),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction){s.services.instruction(0x2c48bu);} s.xmm[0]=_mm_set_ss(f(r(s.memory,(selected(0x46a0d0u,s.image)),4)));
if(s.services.instruction){s.services.instruction(0x2c493u);} s.set(0,selected(0x1u,s.image),1,0);
if(s.services.instruction){s.services.instruction(0x2c495u);} w(s.memory,s.observe,0x2c495u,(selected(0x4c6841u,s.image)),1,selected(0x0u,s.image));
if(s.services.instruction){s.services.instruction(0x2c49cu);} w(s.memory,s.observe,0x2c49cu,(selected(0x4c684cu,s.image)),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction){s.services.instruction(0x2c4a4u);} w(s.memory,s.observe,0x2c4a4u,(selected(0x4c683cu,s.image)),4,selected(0x4u,s.image));
if(s.services.instruction){s.services.instruction(0x2c4aeu);} goto L2c517;
L2c4b0: if(s.services.instruction){s.services.instruction(0x2c4b0u);} s.xmm[0]=_mm_set_ss(f(r(s.memory,(selected(0x46a130u,s.image)),4)));
if(s.services.instruction){s.services.instruction(0x2c4b8u);} w(s.memory,s.observe,0x2c4b8u,(selected(0x473644u,s.image)),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction){s.services.instruction(0x2c4c0u);} w(s.memory,s.observe,0x2c4c0u,(selected(0x4c684cu,s.image)),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction){s.services.instruction(0x2c4c8u);} s.xmm[0]=_mm_set_ss(f(r(s.memory,(selected(0x46a22cu,s.image)),4)));
if(s.services.instruction){s.services.instruction(0x2c4d0u);} w(s.memory,s.observe,0x2c4d0u,(selected(0x4c683cu,s.image)),4,selected(0x2u,s.image));
if(s.services.instruction){s.services.instruction(0x2c4dau);} goto L2c50e;
L2c4dc: if(s.services.instruction){s.services.instruction(0x2c4dcu);} s.xmm[0]=_mm_set_ss(f(r(s.memory,(selected(0x456918u,s.image)),4)));
if(s.services.instruction){s.services.instruction(0x2c4e4u);} w(s.memory,s.observe,0x2c4e4u,(selected(0x473644u,s.image)),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction){s.services.instruction(0x2c4ecu);} s.xmm[0]=_mm_set_ss(f(r(s.memory,(selected(0x46a0ccu,s.image)),4)));
if(s.services.instruction){s.services.instruction(0x2c4f4u);} w(s.memory,s.observe,0x2c4f4u,(selected(0x4c684cu,s.image)),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction){s.services.instruction(0x2c4fcu);} s.xmm[0]=_mm_set_ss(f(r(s.memory,(selected(0x456910u,s.image)),4)));
if(s.services.instruction){s.services.instruction(0x2c504u);} w(s.memory,s.observe,0x2c504u,(selected(0x4c683cu,s.image)),4,selected(0x3u,s.image));
L2c50e: if(s.services.instruction){s.services.instruction(0x2c50eu);} s.set(0,s.flags.logic((s.reg(0,1,0))^(s.reg(0,1,0)),1),1,0);
if(s.services.instruction){s.services.instruction(0x2c510u);} w(s.memory,s.observe,0x2c510u,(selected(0x4c6841u,s.image)),1,selected(0x1u,s.image));
L2c517: if(s.services.instruction){s.services.instruction(0x2c517u);} w(s.memory,s.observe,0x2c517u,(selected(0x4c6840u,s.image)),1,s.reg(0,1,0));
if(s.services.instruction){s.services.instruction(0x2c51cu);} w(s.memory,s.observe,0x2c51cu,(selected(0x4c6842u,s.image)),1,s.reg(0,1,0));
if(s.services.instruction){s.services.instruction(0x2c521u);} w(s.memory,s.observe,0x2c521u,(selected(0x4c6844u,s.image)),1,s.reg(0,1,0));
if(s.services.instruction){s.services.instruction(0x2c526u);} w(s.memory,s.observe,0x2c526u,(selected(0x4c6843u,s.image)),1,s.reg(0,1,0));
if(s.services.instruction){s.services.instruction(0x2c52bu);} w(s.memory,s.observe,0x2c52bu,(selected(0x4c6845u,s.image)),1,s.reg(0,1,0));
if(s.services.instruction){s.services.instruction(0x2c530u);} w(s.memory,s.observe,0x2c530u,(selected(0x4c6848u,s.image)),4,u(_mm_cvtss_f32(s.xmm[0])));
if(s.services.instruction){s.services.instruction(0x2c538u);} s.pop();s.gp[4]+=0u;return;
throw std::runtime_error("Original solver fell outside authenticated CFG");
}
} // namespace shadow_policy_2c470_reference_detail
inline void shadow_policy_2c470_reference(SourcePhysicsContactMemory &memory,
    SourcePhysicsWorldRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe = {}) {
    using namespace physics_world_runtime_detail;
    constexpr auto image=SourcePhysicsContactImage::TitleUpdate15;
    FpScope scope(fp);WorldState state{memory,observe,services,image};state.load(abi);
    try {shadow_policy_2c470_reference_detail::run_2c470(state);}
    catch(...) {abi=state.save();throw;}
    abi=state.save();
}
} // namespace halo2::test
