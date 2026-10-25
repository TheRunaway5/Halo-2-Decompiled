#pragma once
// Frozen original27-PC C++ control; only delivery uses instruction_literal.
// TU1.5 image SHA21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd.
// Original98-byte prefix SHAecb72c582d2a7bd0e6e0669da3dff5c92d7296fc56a42ce21be762a27810b251.
#include "physics/runtime/physics_contact_runtime_internal.hpp"
namespace halo2::test {
inline void shadow_prefix_2e6a50_reference(physics_contact_runtime_detail::State &s) {
using namespace physics_contact_runtime_detail;
const auto selected=[](std::uint32_t p,SourcePhysicsContactImage){return p;};
if(s.services.instruction){s.instruction_literal(0x2e6a50u);} s.xmm[0]=v(s.memory,(selected(0x10u,s.image)+s.gp[0]));
if(s.services.instruction){s.instruction_literal(0x2e6a54u);} s.xmm[1]=v(s.memory,(selected(0x0u,s.image)+s.gp[0]));
if(s.services.instruction){s.instruction_literal(0x2e6a57u);} s.xmm[7]=v(s.memory,(selected(0xffffffe0u,s.image)+s.gp[0]));
if(s.services.instruction){s.instruction_literal(0x2e6a5bu);} s.xmm[1]=_mm_mul_ps(s.xmm[1],s.xmm[2]);
if(s.services.instruction){s.instruction_literal(0x2e6a5eu);} s.xmm[1]=_mm_add_ps(s.xmm[1],s.xmm[0]);
if(s.services.instruction){s.instruction_literal(0x2e6a61u);} s.xmm[0]=s.xmm[1];
if(s.services.instruction){s.instruction_literal(0x2e6a64u);} s.xmm[1]=v(s.memory,(selected(0xfffffff0u,s.image)+s.gp[0]));
if(s.services.instruction){s.instruction_literal(0x2e6a68u);} s.xmm[7]=_mm_mul_ps(s.xmm[7],s.xmm[4]);
if(s.services.instruction){s.instruction_literal(0x2e6a6bu);} s.xmm[1]=_mm_mul_ps(s.xmm[1],s.xmm[3]);
if(s.services.instruction){s.instruction_literal(0x2e6a6eu);} s.xmm[7]=_mm_add_ps(s.xmm[7],s.xmm[1]);
if(s.services.instruction){s.instruction_literal(0x2e6a71u);} s.xmm[1]=s.xmm[7];
if(s.services.instruction){s.instruction_literal(0x2e6a74u);} s.xmm[7]=v(s.memory,(selected(0xfffffe00u,s.image)+s.gp[0]));
if(s.services.instruction){s.instruction_literal(0x2e6a7bu);} s.xmm[1]=_mm_add_ps(s.xmm[1],s.xmm[0]);
if(s.services.instruction){s.instruction_literal(0x2e6a7eu);} s.xmm[0]=v(s.memory,(selected(0xfffffe10u,s.image)+s.gp[0]));
if(s.services.instruction){s.instruction_literal(0x2e6a85u);} s.xmm[7]=_mm_mul_ps(s.xmm[7],s.xmm[5]);
if(s.services.instruction){s.instruction_literal(0x2e6a88u);} s.xmm[5]=v(s.memory,(selected(0x0u,s.image)+s.gp[2]));
if(s.services.instruction){s.instruction_literal(0x2e6a8bu);} s.xmm[5]=_mm_mul_ps(s.xmm[5],v(s.memory,(selected(0x20u,s.image)+s.gp[4])));
if(s.services.instruction){s.instruction_literal(0x2e6a90u);} s.xmm[7]=_mm_add_ps(s.xmm[7],s.xmm[0]);
if(s.services.instruction){s.instruction_literal(0x2e6a93u);} s.xmm[0]=v(s.memory,(selected(0xfffffdf0u,s.image)+s.gp[0]));
if(s.services.instruction){s.instruction_literal(0x2e6a9au);} s.xmm[0]=_mm_mul_ps(s.xmm[0],s.xmm[6]);
if(s.services.instruction){s.instruction_literal(0x2e6a9du);} s.xmm[5]=_mm_add_ps(s.xmm[5],s.xmm[0]);
if(s.services.instruction){s.instruction_literal(0x2e6aa0u);} s.xmm[0]=s.xmm[5];
if(s.services.instruction){s.instruction_literal(0x2e6aa3u);} s.xmm[0]=_mm_add_ps(s.xmm[0],s.xmm[7]);
if(s.services.instruction){s.instruction_literal(0x2e6aa6u);} s.xmm[5]=s.xmm[0];
if(s.services.instruction){s.instruction_literal(0x2e6aa9u);} s.xmm[5]=_mm_sub_ps(s.xmm[5],s.xmm[1]);
if(s.services.instruction){s.instruction_literal(0x2e6aacu);} s.set(2,std::uint32_t(_mm_movemask_ps(s.xmm[5])),4,0);
if(s.services.instruction){s.instruction_literal(0x2e6aafu);} s.flags.minus(s.reg(2,4,0),selected(0xfu,s.image),4);
}
}
