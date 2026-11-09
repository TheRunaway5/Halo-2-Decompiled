#pragma once
#include "physics/runtime/native_tu15_portal_predicate_leaves.hpp"

namespace halo2::native_tu15_leaves {
// 2EA050: decode one broadphase node's packed edge references into a float4
// min/max box. X and Y retain the original extended reciprocal; Z deliberately
// uses the original float-rounded reciprocal spill. Native x87 arithmetic
// follows the caller's precision and rounding controls without a virtual stack.
template<class Io,class Flags> void kernel_2ea050(Io &io,Machine<Flags> &m) {
    using namespace physics_step_detail;
    auto &gp=m.gp;auto &x=m.x;
    const auto number=[&](std::uint32_t pc,std::uint32_t p){return ext(as_float(io.rd(pc,p,4)));};
    const auto spill_word=[&](long double value){return portal_spill_word(value,m.x87_c1);};
    push(io,m,0x2ea050u,gp[5]);gp[5]=gp[4];
    gp[4]=m.flags.logic(gp[4]&~15u,4);gp[4]=m.flags.minus(gp[4],36u,4);
    const auto one_x=number(0x2ea059u,0x46a0d0u);
    gp[0]=io.rd(0x2ea05fu,gp[5]+8u,4);
    const auto reciprocal_x=physics_solver_detail::divide(one_x,number(0x2ea062u,gp[1]+48u));
    gp[2]=io.rd(0x2ea065u,gp[1]+76u,4);
    push(io,m,0x2ea068u,gp[3]);push(io,m,0x2ea069u,gp[6]);
    gp[6]=io.rd(0x2ea06au,gp[0]+8u,2);gp[6]=io.rd(0x2ea06eu,gp[2]+gp[6]*4u,2);
    io.st(0x2ea072u,gp[4]+20u,4,gp[6]);gp[6]=io.rd(0x2ea076u,gp[1]+88u,4);
    push(io,m,0x2ea079u,gp[7]);gp[7]=io.rd(0x2ea07au,gp[0],2);
    gp[7]=io.rd(0x2ea07du,gp[6]+gp[7]*4u,2);gp[3]=io.rd(0x2ea081u,gp[0]+2u,2);
    x[2]=io.vec(0x2ea085u,gp[1]+16u);io.st(0x2ea089u,gp[4]+44u,4,0);
    const auto low_x=static_cast<long double>(si(io.rd(0x2ea091u,gp[4]+24u,4)));
    io.st(0x2ea095u,gp[4]+24u,4,gp[7]);gp[7]=io.rd(0x2ea099u,gp[1]+100u,4);
    gp[3]=io.rd(0x2ea09cu,gp[7]+gp[3]*4u,2);
    const auto decoded_low_x=mul(low_x,reciprocal_x);io.st(0x2ea0a2u,gp[4]+28u,4,gp[3]);
    io.st(0x2ea0a6u,gp[4]+32u,4,spill_word(decoded_low_x));
    const auto one_y=number(0x2ea0aau,0x46a0d0u);
    const auto reciprocal_y=physics_solver_detail::divide(one_y,number(0x2ea0b0u,gp[1]+52u));
    const auto low_y=static_cast<long double>(si(io.rd(0x2ea0b3u,gp[4]+24u,4)));
    io.st(0x2ea0b9u,gp[4]+36u,4,spill_word(mul(low_y,reciprocal_y)));
    const auto one_z=number(0x2ea0bdu,0x46a0d0u);
    const auto reciprocal_z=physics_solver_detail::divide(one_z,number(0x2ea0c3u,gp[1]+56u));
    gp[1]=io.rd(0x2ea0c6u,gp[0]+10u,2);gp[2]=io.rd(0x2ea0cau,gp[2]+gp[1]*4u,2);
    gp[1]=io.rd(0x2ea0ceu,gp[0]+4u,2);gp[0]=io.rd(0x2ea0d2u,gp[0]+6u,2);
    io.st(0x2ea0d6u,gp[4]+24u,4,spill_word(reciprocal_z));
    const auto low_z=static_cast<long double>(si(io.rd(0x2ea0dau,gp[4]+28u,4)));
    io.st(0x2ea0deu,gp[4]+28u,4,gp[2]);gp[2]=io.rd(0x2ea0e2u,gp[6]+gp[1]*4u,2);
    gp[1]=io.rd(0x2ea0e6u,gp[7]+gp[0]*4u,2);
    const auto decoded_low_z=mul(low_z,number(0x2ea0eau,gp[4]+24u));
    gp[0]=io.rd(0x2ea0eeu,gp[5]+12u,4);io.st(0x2ea0f1u,gp[4]+40u,4,spill_word(decoded_low_z));
    x[0]=io.vec(0x2ea0f5u,gp[4]+32u);
    const auto high_x=static_cast<long double>(si(io.rd(0x2ea0fau,gp[4]+28u,4)));
    io.st(0x2ea0feu,gp[4]+28u,4,gp[2]);gp[7]=pop(io,m,0x2ea102u);
    io.st(0x2ea103u,gp[4]+40u,4,0);const auto decoded_high_x=mul(high_x,reciprocal_x);
    gp[6]=pop(io,m,0x2ea10du);x[0]=_mm_sub_ps(x[0],x[2]);
    store_vec(io,0x2ea111u,gp[0],x[0]);
    io.st(0x2ea114u,gp[4]+24u,4,spill_word(decoded_high_x));gp[3]=pop(io,m,0x2ea118u);
    const auto high_y=static_cast<long double>(si(io.rd(0x2ea119u,gp[4]+16u,4)));
    io.st(0x2ea11du,gp[4]+16u,4,gp[1]);
    io.st(0x2ea123u,gp[4]+24u,4,spill_word(mul(high_y,reciprocal_y)));
    const auto high_z=static_cast<long double>(si(io.rd(0x2ea12bu,gp[4]+16u,4)));
    io.st(0x2ea133u,gp[4]+28u,4,spill_word(mul(high_z,number(0x2ea12fu,gp[4]+12u))));
    x[1]=io.vec(0x2ea137u,gp[4]+20u);x[1]=_mm_sub_ps(x[1],x[2]);
    store_vec(io,0x2ea13fu,gp[0]+16u,x[1]);
    gp[4]=gp[5];gp[5]=pop(io,m,0x2ea145u);pop(io,m,0x2ea146u);gp[4]+=8u;
    m.clear_x87_c1=true;
}

// 2EB0C0: resolve the live handle's node, then tail into the complete native
// decoder. The original caller argument slot is a real writable input cell.
template<class Io,class Flags> void kernel_2eb0c0(Io &io,Machine<Flags> &m) {
    auto &gp=m.gp;
    gp[2]=io.rd(0x2eb0c0u,gp[4]+4u,4);gp[0]=io.rd(0x2eb0c4u,gp[2],4);
    gp[2]=io.rd(0x2eb0c6u,gp[1]+64u,4);gp[0]<<=4u;gp[0]+=gp[2];
    io.st(0x2eb0ceu,gp[4]+4u,4,gp[0]);kernel_2ea050(io,m);
}
} // namespace halo2::native_tu15_leaves
