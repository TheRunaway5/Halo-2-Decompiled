#pragma once
// Frozen original27-PC C++ control; only delivery uses instruction_literal.
// TU1.5 image SHA21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd.
// Original98-byte prefix SHA2a88f4b134df82c047af7903b64d1f23f410ec54e0c532aacdf7a5f072eac29b.
#include "physics/runtime/physics_contact_runtime_internal.hpp"
namespace halo2::test {
inline void shadow_prefix_2e66b2_reference(physics_contact_runtime_detail::State &s) {
using namespace physics_contact_runtime_detail;
const auto selected=[](std::uint32_t p,SourcePhysicsContactImage){return p;};
if(s.services.instruction){s.instruction_literal(0x2e66b2u);} s.xmm[0]=v(s.memory,(selected(0x10u,s.image)+s.gp[1]));
if(s.services.instruction){s.instruction_literal(0x2e66b6u);} s.xmm[1]=v(s.memory,(selected(0x0u,s.image)+s.gp[1]));
if(s.services.instruction){s.instruction_literal(0x2e66b9u);} s.xmm[7]=v(s.memory,(selected(0xffffffe0u,s.image)+s.gp[1]));
if(s.services.instruction){s.instruction_literal(0x2e66bdu);} s.xmm[1]=_mm_mul_ps(s.xmm[1],s.xmm[2]);
if(s.services.instruction){s.instruction_literal(0x2e66c0u);} s.xmm[1]=_mm_add_ps(s.xmm[1],s.xmm[0]);
if(s.services.instruction){s.instruction_literal(0x2e66c3u);} s.xmm[0]=s.xmm[1];
if(s.services.instruction){s.instruction_literal(0x2e66c6u);} s.xmm[1]=v(s.memory,(selected(0xfffffff0u,s.image)+s.gp[1]));
if(s.services.instruction){s.instruction_literal(0x2e66cau);} s.xmm[7]=_mm_mul_ps(s.xmm[7],s.xmm[4]);
if(s.services.instruction){s.instruction_literal(0x2e66cdu);} s.xmm[1]=_mm_mul_ps(s.xmm[1],s.xmm[3]);
if(s.services.instruction){s.instruction_literal(0x2e66d0u);} s.xmm[7]=_mm_add_ps(s.xmm[7],s.xmm[1]);
if(s.services.instruction){s.instruction_literal(0x2e66d3u);} s.xmm[1]=s.xmm[7];
if(s.services.instruction){s.instruction_literal(0x2e66d6u);} s.xmm[7]=v(s.memory,(selected(0xfffffe00u,s.image)+s.gp[1]));
if(s.services.instruction){s.instruction_literal(0x2e66ddu);} s.xmm[1]=_mm_add_ps(s.xmm[1],s.xmm[0]);
if(s.services.instruction){s.instruction_literal(0x2e66e0u);} s.xmm[0]=v(s.memory,(selected(0xfffffe10u,s.image)+s.gp[1]));
if(s.services.instruction){s.instruction_literal(0x2e66e7u);} s.xmm[7]=_mm_mul_ps(s.xmm[7],s.xmm[5]);
if(s.services.instruction){s.instruction_literal(0x2e66eau);} s.xmm[5]=v(s.memory,(selected(0x0u,s.image)+s.gp[2]));
if(s.services.instruction){s.instruction_literal(0x2e66edu);} s.xmm[5]=_mm_mul_ps(s.xmm[5],v(s.memory,(selected(0x40u,s.image)+s.gp[4])));
if(s.services.instruction){s.instruction_literal(0x2e66f2u);} s.xmm[7]=_mm_add_ps(s.xmm[7],s.xmm[0]);
if(s.services.instruction){s.instruction_literal(0x2e66f5u);} s.xmm[0]=v(s.memory,(selected(0xfffffdf0u,s.image)+s.gp[1]));
if(s.services.instruction){s.instruction_literal(0x2e66fcu);} s.xmm[0]=_mm_mul_ps(s.xmm[0],s.xmm[6]);
if(s.services.instruction){s.instruction_literal(0x2e66ffu);} s.xmm[5]=_mm_add_ps(s.xmm[5],s.xmm[0]);
if(s.services.instruction){s.instruction_literal(0x2e6702u);} s.xmm[0]=s.xmm[5];
if(s.services.instruction){s.instruction_literal(0x2e6705u);} s.xmm[0]=_mm_add_ps(s.xmm[0],s.xmm[7]);
if(s.services.instruction){s.instruction_literal(0x2e6708u);} s.xmm[5]=s.xmm[0];
if(s.services.instruction){s.instruction_literal(0x2e670bu);} s.xmm[5]=_mm_sub_ps(s.xmm[5],s.xmm[1]);
if(s.services.instruction){s.instruction_literal(0x2e670eu);} s.set(0,std::uint32_t(_mm_movemask_ps(s.xmm[5])),4,0);
if(s.services.instruction){s.instruction_literal(0x2e6711u);} s.flags.minus(s.reg(0,4,0),selected(0xfu,s.image),4);
}
}
