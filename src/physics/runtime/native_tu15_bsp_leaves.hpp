#pragma once
// Native BSP point classification and oriented-plane extraction. The bridge
// retains the original bank/ABI and observable memory order; the traversal is
// a C++ loop over packed nodes, with no instruction or integer-flag dispatch.
// Installed only for authenticated canonical bodies by specialize_native_operands.py.
// Reference/verification use the shared HALO2_*_MATRIX_LEAVES controls.
#include "physics/runtime/native_tu15_matrix_leaves.hpp"

namespace halo2::native_tu15_leaves {

// EAX: node, ECX: collision BSP, EDX: point. Packed children are signed
// 24-bit references; bit 23 marks a leaf and all ones denotes the outside.
template<class Io, class Flags> void kernel_14cdb0(Io &io, Machine<Flags> &m) {
    auto &gp=m.gp;auto &x=m.x;
    x[0]=_mm_setzero_ps();
    push(io,m,0x14cdb3u,gp[6]);
    gp[6]=io.rd(0x14cdb4u,gp[1]+4u,4);
    push(io,m,0x14cdb7u,gp[7]);
    gp[7]=io.rd(0x14cdb8u,gp[1]+12u,4);
    for (;;) {
        gp[0]&=0x7fffffu;m.defer_logic(gp[0]);
        gp[1]=gp[6]+gp[0]*8u;
        const auto index=io.rd(0x14cdc8u,gp[1],2);
        const auto signed_index=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(index))));
        m.defer_shift(signed_index,4,false);
        gp[0]=signed_index*16u;
        x[1]=_mm_set_ss(as_float(io.rd(0x14cdceu,gp[0]+gp[7]+8u,4)));
        x[1]=_mm_mul_ss(x[1],_mm_set_ss(as_float(io.rd(0x14cdd4u,gp[2]+8u,4))));
        x[2]=_mm_set_ss(as_float(io.rd(0x14cdd9u,gp[0]+gp[7]+4u,4)));
        x[2]=_mm_mul_ss(x[2],_mm_set_ss(as_float(io.rd(0x14cddfu,gp[2]+4u,4))));
        m.defer_plus(gp[0],gp[7]);gp[0]+=gp[7];
        x[1]=_mm_add_ss(x[1],x[2]);
        x[2]=_mm_set_ss(as_float(io.rd(0x14cdeau,gp[0],4)));
        x[2]=_mm_mul_ss(x[2],_mm_set_ss(as_float(io.rd(0x14cdeeu,gp[2],4))));
        x[1]=_mm_add_ss(x[1],x[2]);
        x[1]=_mm_sub_ss(x[1],_mm_set_ss(as_float(io.rd(0x14cdf6u,gp[0]+12u,4))));
        // COMISS is retained for its exact unordered/exception behavior.
        m.flags.scalar_compare(x[1],x[0],false);
        m.clear_fault_flags();
        const auto side=m.flags.cf?0u:1u;
        if(!side)m.defer_logic(0,1);
        gp[0]=side;
        gp[1]+=side*2u;
        const auto packed=io.rd(0x14ce0cu,gp[1]+side+1u,4);
        const auto child=(packed>>8u)|((packed&0x80000000u)?0xff000000u:0u);
        gp[0]=child;
        if (child&0x800000u) break;
    }
    const bool outside=gp[0]==0xffffffffu;
    m.defer_minus(gp[0],0xffffffffu);
    gp[7]=pop(io,m,0x14ce1du);
    gp[6]=pop(io,m,0x14ce1eu);
    gp[0]=m.flags.logic(outside?0xffffffffu:(gp[0]&0x7fffffu),4);
    m.clear_fault_flags();
    pop(io,m,outside?0x14ce2au:0x14ce26u);
}

// EAX: destination, EDX: oriented plane index, [ESP+4]: BSP. Negation
// deliberately uses 0-value rather than XOR: signed zero and NaNs must follow
// the original SSE arithmetic. Ordered scalar stores also preserve aliasing.
template<class Io, class Flags> void kernel_14ce30(Io &io, Machine<Flags> &m) {
    auto &gp=m.gp;auto &x=m.x;
    push(io,m,0x14ce30u,gp[6]);
    gp[6]=io.rd(0x14ce31u,gp[4]+8u,4);
    gp[1]=gp[2]&0x7fffu;m.defer_logic(gp[1]);
    push(io,m,0x14ce3du,gp[7]);
    gp[7]=io.rd(0x14ce3eu,gp[6]+12u,4);
    m.defer_plus(gp[1]*16u,gp[7]);gp[1]=gp[1]*16u+gp[7];
    m.flags.logic((gp[2]>>8u)&255u,1);
    m.clear_fault_flags();
    if (gp[2]&0x8000u) {
        x[0]=_mm_setzero_ps();
        x[1]=x[0];x[1]=_mm_sub_ss(x[1],_mm_set_ss(as_float(io.rd(0x14ce50u,gp[1],4))));
        io.st(0x14ce54u,gp[0],4,low_bits(x[1]));
        x[1]=x[0];x[1]=_mm_sub_ss(x[1],_mm_set_ss(as_float(io.rd(0x14ce5bu,gp[1]+4u,4))));
        io.st(0x14ce60u,gp[0]+4u,4,low_bits(x[1]));
        x[1]=x[0];x[1]=_mm_sub_ss(x[1],_mm_set_ss(as_float(io.rd(0x14ce68u,gp[1]+8u,4))));
        io.st(0x14ce6du,gp[0]+8u,4,low_bits(x[1]));
        x[0]=_mm_sub_ss(x[0],_mm_set_ss(as_float(io.rd(0x14ce72u,gp[1]+12u,4))));
        gp[7]=pop(io,m,0x14ce77u);
        io.st(0x14ce78u,gp[0]+12u,4,low_bits(x[0]));
        gp[6]=pop(io,m,0x14ce7du);
        pop(io,m,0x14ce7eu);
    } else {
        gp[6]=io.rd(0x14ce81u,gp[1],4);
        gp[2]=gp[0];
        io.st(0x14ce85u,gp[2],4,gp[6]);
        gp[6]=io.rd(0x14ce87u,gp[1]+4u,4);
        io.st(0x14ce8au,gp[2]+4u,4,gp[6]);
        gp[6]=io.rd(0x14ce8du,gp[1]+8u,4);
        io.st(0x14ce90u,gp[2]+8u,4,gp[6]);
        gp[1]=io.rd(0x14ce93u,gp[1]+12u,4);
        gp[7]=pop(io,m,0x14ce96u);
        io.st(0x14ce97u,gp[2]+12u,4,gp[1]);
        gp[6]=pop(io,m,0x14ce9au);
        pop(io,m,0x14ce9bu);
    }
    gp[4]+=4u;
}
} // namespace halo2::native_tu15_leaves
