#pragma once
// Frozen from source_shadow_frame_tu15_generated.cpp SHA256
// cf7341f8fd24518b5eb478e136dcc48ea8517c2a7df4906924c5e2ffca177d42.
// These27 original instructions are an independent, unchanged C++ control.
// Only callback delivery uses the existing literal-PC specialization. All
// selected operands in this prefix are unchanged TU1.5 literal offsets.
#include "physics/runtime/physics_contact_runtime_internal.hpp"
namespace halo2::test {
inline void shadow_prefix_2e6810_reference(physics_contact_runtime_detail::State &s) {
    using namespace physics_contact_runtime_detail;
    const auto selected=[](std::uint32_t p,SourcePhysicsContactImage){return p;};
if(s.services.instruction){s.instruction_literal(0x2e6810u);} s.xmm[0]=v(s.memory,(selected(0x10u,s.image)+s.gp[1]));
if(s.services.instruction){s.instruction_literal(0x2e6814u);} s.xmm[1]=v(s.memory,(selected(0x0u,s.image)+s.gp[1]));
if(s.services.instruction){s.instruction_literal(0x2e6817u);} s.xmm[7]=v(s.memory,(selected(0xffffffe0u,s.image)+s.gp[1]));
if(s.services.instruction){s.instruction_literal(0x2e681bu);} s.xmm[1]=_mm_mul_ps(s.xmm[1],s.xmm[2]);
if(s.services.instruction){s.instruction_literal(0x2e681eu);} s.xmm[1]=_mm_add_ps(s.xmm[1],s.xmm[0]);
if(s.services.instruction){s.instruction_literal(0x2e6821u);} s.xmm[0]=s.xmm[1];
if(s.services.instruction){s.instruction_literal(0x2e6824u);} s.xmm[1]=v(s.memory,(selected(0xfffffff0u,s.image)+s.gp[1]));
if(s.services.instruction){s.instruction_literal(0x2e6828u);} s.xmm[7]=_mm_mul_ps(s.xmm[7],s.xmm[4]);
if(s.services.instruction){s.instruction_literal(0x2e682bu);} s.xmm[1]=_mm_mul_ps(s.xmm[1],s.xmm[3]);
if(s.services.instruction){s.instruction_literal(0x2e682eu);} s.xmm[7]=_mm_add_ps(s.xmm[7],s.xmm[1]);
if(s.services.instruction){s.instruction_literal(0x2e6831u);} s.xmm[1]=s.xmm[7];
if(s.services.instruction){s.instruction_literal(0x2e6834u);} s.xmm[7]=v(s.memory,(selected(0xfffffe00u,s.image)+s.gp[1]));
if(s.services.instruction){s.instruction_literal(0x2e683bu);} s.xmm[1]=_mm_add_ps(s.xmm[1],s.xmm[0]);
if(s.services.instruction){s.instruction_literal(0x2e683eu);} s.xmm[0]=v(s.memory,(selected(0xfffffe10u,s.image)+s.gp[1]));
if(s.services.instruction){s.instruction_literal(0x2e6845u);} s.xmm[7]=_mm_mul_ps(s.xmm[7],s.xmm[5]);
if(s.services.instruction){s.instruction_literal(0x2e6848u);} s.xmm[5]=v(s.memory,(selected(0x0u,s.image)+s.gp[2]));
if(s.services.instruction){s.instruction_literal(0x2e684bu);} s.xmm[5]=_mm_mul_ps(s.xmm[5],v(s.memory,(selected(0x20u,s.image)+s.gp[4])));
if(s.services.instruction){s.instruction_literal(0x2e6850u);} s.xmm[7]=_mm_add_ps(s.xmm[7],s.xmm[0]);
if(s.services.instruction){s.instruction_literal(0x2e6853u);} s.xmm[0]=v(s.memory,(selected(0xfffffdf0u,s.image)+s.gp[1]));
if(s.services.instruction){s.instruction_literal(0x2e685au);} s.xmm[0]=_mm_mul_ps(s.xmm[0],s.xmm[6]);
if(s.services.instruction){s.instruction_literal(0x2e685du);} s.xmm[5]=_mm_add_ps(s.xmm[5],s.xmm[0]);
if(s.services.instruction){s.instruction_literal(0x2e6860u);} s.xmm[0]=s.xmm[5];
if(s.services.instruction){s.instruction_literal(0x2e6863u);} s.xmm[0]=_mm_add_ps(s.xmm[0],s.xmm[7]);
if(s.services.instruction){s.instruction_literal(0x2e6866u);} s.xmm[5]=s.xmm[0];
if(s.services.instruction){s.instruction_literal(0x2e6869u);} s.xmm[5]=_mm_sub_ps(s.xmm[5],s.xmm[1]);
if(s.services.instruction){s.instruction_literal(0x2e686cu);} s.set(3,std::uint32_t(_mm_movemask_ps(s.xmm[5])),4,0);
if(s.services.instruction){s.instruction_literal(0x2e686fu);} s.flags.minus(s.reg(3,4,0),selected(0xfu,s.image),4);

}
} // namespace halo2::test
