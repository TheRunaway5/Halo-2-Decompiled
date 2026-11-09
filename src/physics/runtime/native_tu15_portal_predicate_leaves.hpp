#pragma once
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/physics_solver_internal.hpp"
#include <cmath>

namespace halo2::native_tu15_leaves {
inline __m128 ordered_add_ss(__m128 a,__m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm("addss %1,%0":"+x"(a):"x"(b));return a;
#else
    return _mm_add_ss(a,b);
#endif
}
// Source x87 numeric operations execute under the owning caller's live control
// word. These helpers have native operands/results and no logical FP stack.
inline std::uint32_t portal_abs_word(std::uint32_t bits) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint32_t result;
    asm volatile("flds %1\n\tfabs\n\tfstps %0":"=m"(result):"m"(bits):"st");return result;
#else
    return as_bits(std::fabs(as_float(bits)));
#endif
}
inline bool portal_x87_less(long double a,long double b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char carry;
    asm volatile("fldt %2\n\tfldt %1\n\tfcomip %%st(1),%%st\n\tsetb %0\n\tfstp %%st(0)"
        :"=qm"(carry):"m"(a),"m"(b):"cc","st");return carry;
#else
    return !(a>=b);
#endif
}
inline bool portal_x87_greater(long double a,long double b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char above;
    asm volatile("fldt %2\n\tfldt %1\n\tfcomip %%st(1),%%st\n\tseta %0\n\tfstp %%st(0)"
        :"=qm"(above):"m"(a),"m"(b):"cc","st");return above;
#else
    return a>b;
#endif
}
inline std::uint32_t portal_spill_word(long double value,std::uint16_t &status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint32_t result;std::uint16_t actual;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1":"=m"(result),"=m"(actual):"m"(value):"st");
    status=(status&~0x200u)|(actual&0x200u);return result;
#else
    status&=~0x200u;return as_bits(static_cast<float>(value));
#endif
}

// 123210: dominant absolute component; ties prefer z, then y. Stack-resident
// absolute values retain exact aliases and the original's x87 load semantics.
template<class Io,class Flags> void kernel_123210(Io &io,Machine<Flags> &m) {
    auto &gp=m.gp;auto &x=m.x;
    gp[4]=m.flags.minus(gp[4],12u,4);
    const auto a=portal_abs_word(io.rd(0x123213u,gp[0],4));
    io.st(0x123217u,gp[4]+8u,4,a);
    const auto b=io.rd(0x12321bu,gp[0]+4u,4);
    x[0]=_mm_set_ss(as_float(io.rd(0x12321eu,gp[4]+8u,4)));
    io.st(0x123226u,gp[4]+4u,4,portal_abs_word(b));
    const auto c=portal_abs_word(io.rd(0x12322au,gp[0]+8u,4));
    io.st(0x12322fu,gp[4],4,c);
    const auto y=static_cast<long double>(as_float(io.rd(0x123232u,gp[4]+4u,4)));
    const auto z=static_cast<long double>(as_float(io.rd(0x123236u,gp[4],4)));
    bool choose_z=!portal_x87_less(z,y);
    if(choose_z) {
        x[1]=_mm_set_ss(as_float(io.rd(0x12323fu,gp[4],4)));
        m.flags.scalar_compare(x[1],x[0],false);choose_z=!m.flags.cf;
    }
    std::uint32_t result=2,ret=0x123250u;
    if(!choose_z) {
        x[1]=_mm_set_ss(as_float(io.rd(0x123251u,gp[4]+4u,4)));
        m.flags.scalar_compare(x[1],x[0],false);
        result=m.flags.cf?0u:1u;ret=m.flags.cf?0x12326au:0x123263u;
    }
    gp[0]=(gp[0]&0xffff0000u)|result;
    gp[4]=m.flags.plus(gp[4],12u,4);pop(io,m,ret);m.clear_x87_c1=true;m.x87_c1=0;
}

// 243450: ordered circle-versus-oriented-polygon edge tests. The edge loop
// performs scalar native SSE and preserves the complete caller-visible ABI.
template<class Io,class Flags> void kernel_243450(Io &io,Machine<Flags> &m) {
    auto &gp=m.gp;auto &x=m.x;
    const auto set_al=[&](std::uint32_t value){gp[0]=(gp[0]&0xffffff00u)|(value&255u);};
    gp[2]=(gp[2]&0xffff0000u)|io.rd(0x243450u,gp[4]+4u,2);
    push(io,m,0x243455u,gp[5]);gp[5]=0;
    m.flags.logic(gp[2]&65535u,2);x[6]=x[0];set_al(1);
    x[6]=ordered_mul_ss(x[6],x[0]);
    bool reject=false,entered=false;
    if(!m.flags.zf && m.flags.sf==m.flags.of) {
        entered=true;
        x[5]=_mm_setzero_ps();push(io,m,0x24346du,gp[3]);
        gp[3]=std::uint32_t(std::int32_t(std::int16_t(gp[2])));push(io,m,0x243471u,gp[6]);
        do {
            x[0]=_mm_set_ss(as_float(io.rd(0x243472u,gp[7],4)));
            gp[0]=std::uint32_t(std::int32_t(std::int16_t(gp[5])));
            x[0]=_mm_sub_ss(x[0],_mm_set_ss(as_float(io.rd(0x243479u,gp[1]+gp[0]*8u,4))));
            x[3]=_mm_set_ss(as_float(io.rd(0x24347eu,gp[7]+4u,4)));
            x[3]=_mm_sub_ss(x[3],_mm_set_ss(as_float(io.rd(0x243483u,gp[1]+gp[0]*8u+4u,4))));
            gp[6]=gp[0]+1u;gp[2]=std::int32_t(gp[6])>=std::int32_t(gp[3])?0u:gp[6];
            x[1]=_mm_set_ss(as_float(io.rd(0x243496u,gp[1]+gp[2]*8u,4)));
            x[1]=_mm_sub_ss(x[1],_mm_set_ss(as_float(io.rd(0x24349bu,gp[1]+gp[0]*8u,4))));
            x[2]=_mm_set_ss(as_float(io.rd(0x2434a0u,gp[1]+gp[2]*8u+4u,4)));
            x[2]=_mm_sub_ss(x[2],_mm_set_ss(as_float(io.rd(0x2434a6u,gp[1]+gp[0]*8u+4u,4))));
            gp[6]=gp[1]+gp[0]*8u+4u;
            x[4]=ordered_mul_ss(x[1],x[1]);x[7]=ordered_mul_ss(x[2],x[2]);
            x[4]=ordered_add_ss(x[4],x[7]);m.flags.scalar_compare(x[4],x[5],true);
            gp[0]=(gp[0]&0xffff00ffu)|((m.flags.bits()|2u)&255u)<<8u;
            m.flags.logic((gp[0]>>8u)&0x44u,1);
            if(m.flags.pf) {
                x[0]=ordered_mul_ss(x[0],x[2]);x[3]=ordered_mul_ss(x[3],x[1]);
                x[0]=_mm_sub_ss(x[0],x[3]);m.flags.scalar_compare(x[0],x[5],false);
                if(!m.flags.cf && !m.flags.zf) {
                    x[1]=ordered_mul_ss(x[0],x[0]);x[4]=ordered_mul_ss(x[4],x[6]);
                    m.flags.scalar_compare(x[1],x[4],false);
                    if(!m.flags.cf && !m.flags.zf){reject=true;break;}
                }
            }
            ++gp[5];m.flags.minus(gp[5]&65535u,io.rd(0x2434edu,gp[4]+16u,2),2);
        } while(m.flags.sf!=m.flags.of);
        gp[6]=pop(io,m,reject?0x243500u:0x2434f8u);
        if(reject) {
            set_al(m.flags.logic(0,1));gp[3]=pop(io,m,0x243503u);
        }else{gp[3]=pop(io,m,0x2434f9u);set_al(1);}
    }
    const bool accepted_loop=entered&&!reject;
    gp[5]=pop(io,m,accepted_loop?0x2434fcu:0x243504u);
    pop(io,m,accepted_loop?0x2434fdu:0x243505u);gp[4]+=4u;
}

// 14CEA0: test the sphere against the portal plane, authored bounding sphere,
// then projected polygon. The two plane owners remain separate live reads.
// x87 arithmetic uses native extended locals under the live control word;
// float spills, raw vertex copies and scratch aliases retain source order.
template<class Io,class Flags> void kernel_14cea0(Io &io,Machine<Flags> &m) {
    using namespace physics_step_detail;
    auto &gp=m.gp;auto &x=m.x;
    const auto sx16=[](std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));};
    const auto number=[&](std::uint32_t pc,std::uint32_t p){return ext(as_float(io.rd(pc,p,4)));};
    gp[4]=m.flags.minus(gp[4],0x420u,4);gp[0]=sx16(gp[0]);
    push(io,m,0x14cea9u,gp[5]);push(io,m,0x14ceaau,gp[7]);gp[7]=gp[1];
    gp[1]=io.rd(0x14ceadu,gp[2]+0x60u,4);gp[2]=io.rd(0x14ceb0u,gp[2]+0x18u,4);
    gp[0]*=9u;gp[5]=gp[1]+gp[0]*4u;
    gp[1]=io.rd(0x14ceb9u,gp[5]+4u,4);gp[0]=io.rd(0x14cebcu,gp[2]+12u,4);
    gp[1]<<=4u;gp[0]+=gp[1];io.st(0x14cec4u,gp[4]+12u,4,gp[5]);
    const auto plane_y=number(0x14cec8u,gp[0]+4u),center_y=number(0x14cecbu,gp[7]+4u);
    const auto product_y=mul(plane_y,center_y);
    const auto plane_z=number(0x14ceceu,gp[0]+8u),center_z=number(0x14ced1u,gp[7]+8u);
    const auto product_z=mul(plane_z,center_z);const auto yz=add(product_y,product_z);
    const auto center_x=number(0x14ced6u,gp[7]),plane_x=number(0x14ced8u,gp[0]);
    const auto xyz=add(yz,mul(center_x,plane_x));
    const auto distance=sub(xyz,number(0x14cedcu,gp[0]+12u));
    io.st(0x14cedfu,gp[4]+8u,4,as_bits(spill(distance)));
    bool accepted=portal_x87_greater(number(0x14cee5u,gp[4]+0x42cu),std::fabs(distance));
    if(accepted) {
        x[2]=_mm_set_ss(as_float(io.rd(0x14cef6u,gp[5]+16u,4)));
        x[2]=_mm_sub_ss(x[2],_mm_set_ss(as_float(io.rd(0x14cefbu,gp[7]+8u,4))));
        x[0]=_mm_set_ss(as_float(io.rd(0x14cf00u,gp[5]+8u,4)));
        x[0]=_mm_sub_ss(x[0],_mm_set_ss(as_float(io.rd(0x14cf05u,gp[7],4))));
        x[1]=_mm_set_ss(as_float(io.rd(0x14cf09u,gp[5]+12u,4)));
        x[1]=_mm_sub_ss(x[1],_mm_set_ss(as_float(io.rd(0x14cf0eu,gp[7]+4u,4))));
        x[3]=_mm_set_ss(as_float(io.rd(0x14cf13u,gp[5]+20u,4)));
        x[3]=ordered_add_ss(x[3],_mm_set_ss(as_float(io.rd(0x14cf18u,gp[4]+0x42cu,4))));
        x[4]=ordered_mul_ss(x[2],x[2]);x[2]=ordered_mul_ss(x[0],x[0]);
        x[0]=ordered_mul_ss(x[1],x[1]);x[4]=ordered_add_ss(ordered_add_ss(x[4],x[2]),x[0]);
        x[0]=ordered_mul_ss(x[3],x[3]);m.flags.scalar_compare(x[0],x[4],false);
        accepted=!m.flags.cf&&!m.flags.zf;
    }
    if(accepted) {
        gp[0]=io.rd(0x14cf4eu,0x4ecc0cu,4);
        push(io,m,0x14cf53u,gp[3]);push(io,m,0x14cf54u,gp[6]);
        gp[6]=io.rd(0x14cf55u,gp[0]+12u,4);gp[6]+=gp[1];gp[0]=gp[6];
        push(io,m,0x14cf5cu,0x14cf61u);kernel_123210(io,m);
        x[0]=_mm_setzero_ps();gp[0]=sx16(gp[0]);
        x[1]=_mm_set_ss(as_float(io.rd(0x14cf67u,gp[6]+gp[0]*4u,4)));
        m.flags.scalar_compare(x[1],x[0],false);
        gp[1]=!m.flags.cf&&!m.flags.zf?1u:0u;
        x[0]=_mm_sub_ss(x[0],_mm_set_ss(as_float(io.rd(0x14cf77u,gp[4]+16u,4))));
        x[1]=_mm_set_ss(as_float(io.rd(0x14cf7du,gp[6],4)));
        gp[3]=io.rd(0x14cf81u,gp[5]+28u,4);
        x[1]=ordered_mul_ss(x[1],x[0]);
        x[1]=ordered_add_ss(x[1],_mm_set_ss(as_float(io.rd(0x14cf88u,gp[7],4))));
        io.st(0x14cf8fu,gp[4]+28u,4,low_bits(x[1]));
        x[1]=_mm_set_ss(as_float(io.rd(0x14cf95u,gp[6]+4u,4)));gp[0]+=gp[0]+gp[1];
        x[1]=ordered_mul_ss(x[1],x[0]);
        x[1]=ordered_add_ss(x[1],_mm_set_ss(as_float(io.rd(0x14cfa1u,gp[7]+4u,4))));
        gp[0]*=6u;
        gp[1]=sx16(io.rd(0x14cfabu,0x44ac8cu+gp[0],2));
        gp[2]=sx16(io.rd(0x14cfb2u,0x44ac8eu+gp[0],2));
        io.st(0x14cfb9u,gp[4]+32u,4,low_bits(x[1]));
        x[1]=_mm_set_ss(as_float(io.rd(0x14cfbfu,gp[6]+8u,4)));
        x[1]=ordered_mul_ss(x[1],x[0]);
        x[1]=ordered_add_ss(x[1],_mm_set_ss(as_float(io.rd(0x14cfc8u,gp[7]+8u,4))));
        gp[1]*=4u;io.st(0x14cfd0u,gp[4]+36u,4,low_bits(x[1]));
        x[0]=_mm_set_ss(as_float(io.rd(0x14cfd6u,gp[4]+28u+gp[1],4)));
        gp[2]*=4u;io.st(0x14cfdfu,gp[4]+40u,4,low_bits(x[0]));
        x[0]=_mm_set_ss(as_float(io.rd(0x14cfe5u,gp[4]+28u+gp[2],4)));
        gp[7]=0;io.st(0x14cfefu,gp[4]+44u,4,low_bits(x[0]));
        if(std::int32_t(gp[3])>0) {
            gp[6]=io.rd(0x14cff7u,gp[5]+32u,4);gp[0]=0;
            io.st(0x14cffcu,gp[4]+24u,4,gp[6]);
            do {
                gp[5]=io.rd(0x14d000u,gp[4]+24u,4);gp[6]=gp[4]+48u+gp[0]*8u;
                gp[0]=gp[5]+gp[0]*12u;
                gp[5]=io.rd(0x14d00fu,gp[1]+gp[0],4);gp[0]=io.rd(0x14d012u,gp[2]+gp[0],4);
                ++gp[7];io.st(0x14d016u,gp[6]+4u,4,gp[0]);gp[0]=sx16(gp[7]);
                io.st(0x14d01eu,gp[6],4,gp[5]);
            } while(std::int32_t(gp[0])<std::int32_t(gp[3]));
            gp[5]=io.rd(0x14d022u,gp[4]+20u,4);
        }
        const auto radius_a=number(0x14d026u,gp[4]+0x434u);
        gp[1]=0;const auto radius_b=number(0x14d02fu,gp[4]+0x434u);
        const auto radius_squared=mul(radius_a,radius_b);
        gp[1]=io.rd(0x14d036u,gp[5]+28u,2);
        const auto distance_a=number(0x14d03au,gp[4]+16u);gp[7]=gp[4]+40u;
        const auto distance_b=number(0x14d042u,gp[4]+16u);
        const auto disk_squared=sub(radius_squared,mul(distance_a,distance_b));
        push(io,m,0x14d048u,gp[1]);const auto disk=physics_solver_detail::root(disk_squared);
        gp[1]=gp[4]+52u;io.st(0x14d04fu,gp[4]+24u,4,portal_spill_word(disk,m.x87_c1));
        x[0]=_mm_set_ss(as_float(io.rd(0x14d053u,gp[4]+24u,4)));
        push(io,m,0x14d059u,0x14d05eu);kernel_243450(io,m);accepted=(gp[0]&255u)!=0;
        gp[6]=pop(io,m,0x14d060u);gp[3]=pop(io,m,0x14d061u);
    }
    gp[7]=pop(io,m,accepted?0x14d064u:0x14d071u);
    gp[0]=(gp[0]&0xffffff00u)|(accepted?1u:0u);
    gp[5]=pop(io,m,accepted?0x14d067u:0x14d074u);
    gp[4]=m.flags.plus(gp[4],0x420u,4);pop(io,m,accepted?0x14d06eu:0x14d07bu);gp[4]+=4u;
    m.clear_x87_c1=true;
}
} // namespace halo2::native_tu15_leaves
