#pragma once
#include "rendering/models/native_tu15_prt_sample.hpp"
#include "rendering/models/native_tu15_prt_rotation_bridge.hpp"
#include "rendering/models/native_tu15_model_view_slots_bridge.hpp"

namespace halo2::prt_update_detail {
using namespace prt_projection_detail;
using prt_sample_detail::subtract;
inline bool equal_ordered(__m128 a, __m128 b) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    unsigned char equal, ordered;
    asm volatile("ucomiss %3,%2; sete %0; setnp %1"
        : "=qm"(equal), "=qm"(ordered) : "x"(a), "x"(b) : "cc");
    return equal && ordered;
#else
    return _mm_ucomieq_ss(a,b) && _mm_movemask_ps(_mm_cmpord_ss(a,b)) & 1;
#endif
}
inline long double sine_cosine(long double value, bool cosine, std::uint16_t& status) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result; std::uint16_t actual;
    if (cosine) asm volatile("fldt %2; fcos; fnstsw %1; fstpt %0"
        : "=m"(result), "=m"(actual) : "m"(value) : "st");
    else asm volatile("fldt %2; fsin; fnstsw %1; fstpt %0"
        : "=m"(result), "=m"(actual) : "m"(value) : "st");
    status = (status & ~0x600u) | (actual & 0x600u);
    return result;
#else
    throw std::runtime_error("Native PRT update requires x87 trigonometry");
#endif
}
// Construct the authored Rodrigues matrix. Scalar SSE grouping, extended
// sine/cosine spills, source rereads and overlapping scratch stores retain the
// original operation order. The rotation child replaces all escaped SIMD lanes.
inline void rotation_matrix(SourcePhysicsContactMemory& memory, std::uint32_t row,
    std::uint32_t frame, std::uint16_t& status,
    const SourcePhysicsContactRecordObserver& observe) {
    const auto get = [&](std::uint32_t at) { return scalar(read(memory, at)); };
    const auto put = [&](std::uint32_t offset, __m128 value, std::uint32_t pc) {
        store(memory, frame + offset, bits(value), 4, pc, observe);
    };
    const auto spill = [&](std::uint32_t offset, long double value, std::uint32_t pc) {
        const auto rounded = prt_basis_detail::round_single(value);
        status = (status & ~0x200u) | (rounded.status & 0x200u);
        store(memory, frame + offset, rounded.bits, 4, pc, observe);
    };
    auto extended = prt_basis_detail::single(memory, row + 0xa0u); status &= ~0x200u;
    auto one = get(0x46a0d0u);
    extended = sine_cosine(extended, false, status);
    auto y_sin = get(row + 0x98u), z_sin = get(row + 0x9cu);
    auto xx = get(row + 0x94u), yy = get(row + 0x98u), zz = get(row + 0x9cu);
    xx = multiply(xx,xx,false); yy = multiply(yy,yy,false); zz = multiply(zz,zz,false);
    spill(0x14u, extended, 0x22952u);
    extended = prt_basis_detail::single(memory, row + 0xa0u); status &= ~0x200u;
    auto sin = get(frame + 0x14u); extended = sine_cosine(extended, true, status);
    z_sin = multiply(z_sin,sin,false); y_sin = multiply(y_sin,sin,false);
    auto diagonal = subtract(one,xx); spill(0x18u, extended, 0x22973u);
    auto cos = get(frame + 0x18u);
    extended = prt_basis_detail::single(memory, frame + 0x14u); status &= ~0x200u;
    extended = physics_step_detail::mul(extended,prt_basis_detail::single(memory,row+0x94u));
    diagonal = add(multiply(diagonal,cos,false),xx,false);
    auto xy = get(row+0x98u); put(0x34u,diagonal,0x22997u);
    spill(0x28u,extended,0x2299du);
    const auto complement = subtract(one,cos);
    xy = multiply(xy,complement,false); xy = multiply(xy,get(row+0x94u),false);
    const auto yx = subtract(xy,z_sin); const auto xy_plus = add(z_sin,xy,false);
    auto zy = get(row+0x9cu); put(0x38u,xy_plus,0x229c7u);
    diagonal = subtract(one,yy); put(0x40u,yx,0x229d4u);
    cos = get(frame+0x18u); diagonal = add(multiply(diagonal,cos,false),yy,false);
    put(0x44u,diagonal,0x229e8u);
    zy = multiply(zy,complement,false);
    auto zx = multiply(zy,get(row+0x94u),false);
    const auto xz = add(zx,y_sin,false); zx = subtract(zx,y_sin);
    put(0x3cu,zx,0x22a04u);
    auto yz = multiply(get(row+0x98u),zy,false);
    const auto x_sin = get(frame+0x28u); put(0x4cu,xz,0x22a20u);
    auto last = subtract(one,zz); last = add(multiply(last,cos,false),zz,false);
    const auto yz_minus = subtract(yz,x_sin); yz = add(yz,x_sin,false);
    put(0x54u,last,0x22a3du); put(0x50u,yz_minus,0x22a43u); put(0x48u,yz,0x22a49u);
}
// Complete22830 controller: reset view slots, rotate each authored probe,
// advance the real source clock, then copy/interpolate the selected SH cache.
// Numerical children are native; no per-instruction arithmetic flag machine is
// used inside traversal. The final flag update is caller-ABI publication.
template<class State> void bridge(State& s) {
    auto& g=s.gp; auto& x=s.xmm;
    const auto get=[&](std::uint32_t at,unsigned width=4){return read(s.memory,at,width);};
    const auto put=[&](std::uint32_t at,std::uint32_t value,std::uint32_t pc,unsigned width=4){
        store(s.memory,at,value,width,pc,s.observe);};
    const auto entry=s.gp[4]; g[4]-=0x68u;
    s.push(0x22833u,g[3]);s.push(0x22834u,g[6]);s.push(0x22835u,g[7]);
    native_tu15_model_view_slots_bridge(s);
    const auto finish=[&]{g[7]=s.pop();g[6]=s.pop();g[3]=s.pop();
        s.flags.plus(g[4],0x68u,4);g[4]+=0x68u;(void)s.pop();};
    const auto copy=[&](std::uint32_t pc){for(g[1]=9;g[1];--g[1]){
        const auto value=get(g[6]);put(g[7],value,pc);g[6]+=4u;g[7]+=4u;}};
    g[7]=get(0x4ecc14u);g[3]=0;put(0x515ec8u,0,0x22878u);
    if(!g[7]||std::int32_t(get(g[7]+0x80u))<=0){finish();return;}
    g[1]=get(0x4ecc18u);if(!g[1]){finish();return;}
    g[0]=get(g[7]+0x84u);if(get(g[0]+0x1cu)==g[6]){finish();return;}
    g[2]=get(g[0]+4u);if(g[2]!=get(g[1]+8u)){finish();return;}
    g[1]=get(g[7]+0x84u);g[2]=get(g[1]+0x38u);
    put(g[4]+0x1cu,g[1],0x228c4u);put(g[4]+0x18u,g[2],0x228c8u);
    if(std::int32_t(g[2])<=0){finish();return;}
    put(g[4]+0x14u,g[2],0x228d2u);s.push(0x228d6u,g[5]);
    const auto frame=g[4];
    do {
        g[5]=get(g[1]+0x3cu);x[0]=scalar(get(g[5]+g[3]+0xa0u));g[5]+=g[3];
        if(!equal_ordered(x[0],scalar(get(0x46a0e8u)))){
            g[7]=g[5]+12u;s.push(0x22946u,3);g[1]=g[7];
            g[0]=g[4]+0x34u;g[6]=g[4]+0x58u;
            auto status=std::uint16_t(s.fp.status);
            rotation_matrix(s.memory,g[5],g[4],status,s.observe);s.fp.status=status;
            s.push(0x22a4fu,0x22a54u);prt_rotation_detail::bridge(s);copy(0x22a59u);
            constexpr std::array offsets{0x30u,0x54u};
            constexpr std::array push_sites{0x22a5eu,0x22a79u};
            constexpr std::array call_sites{0x22a6au,0x22a85u};
            constexpr std::array returns{0x22a6fu,0x22a8au};
            constexpr std::array copy_sites{0x22a74u,0x22a93u};
            for(unsigned channel=0;channel<2;++channel){
                g[7]=g[5]+offsets[channel];s.push(push_sites[channel],3);g[1]=g[7];
                g[0]=g[4]+0x34u;g[6]=g[4]+0x58u;s.push(call_sites[channel],returns[channel]);
                prt_rotation_detail::bridge(s);
                if(channel==1)g[2]=get(g[4]+0x1cu);
                copy(copy_sites[channel]);
            }
            g[7]=get(0x4ecc14u);g[1]=get(g[4]+0x20u);
        }
        g[0]=get(g[4]+0x18u);g[3]+=0xdcu;--g[0];put(g[4]+0x18u,g[0],0x22aaau);
    }while(g[0]);
    x[0]=scalar(get(g[7]+0x34u));x[2]=_mm_setzero_ps();
    const bool advance=prt_sample_detail::above(x[0],x[2]);g[5]=s.pop();
    if(!advance){put(0x515ed4u,0,0x22bfdu,1);finish();return;}
    x[1]=scalar(get(0x515eccu));
    const auto rounded=prt_basis_detail::round_single(prt_basis_detail::double_value(s.memory,0x4920c0u));
    s.fp.status=(s.fp.status&~0x200u)|(rounded.status&0x200u);put(0x515eccu,rounded.bits,0x22ad4u);
    x[0]=scalar(get(0x515eccu));x[0]=subtract(x[0],x[1]);
    x[0]=_mm_div_ss(x[0],scalar(get(g[7]+0x34u)));x[0]=add(x[0],scalar(get(0x515ed0u)),false);
    g[0]=std::uint32_t(_mm_cvttss_si32(x[0]));x[1]=_mm_cvtsi32_ss(x[1],std::bit_cast<std::int32_t>(g[0]));
    x[0]=subtract(x[0],x[1]);const bool clamp=prt_sample_detail::above(x[2],x[0]);
    put(0x515ed4u,1,0x22b02u,1);put(0x515ed0u,bits(x[0]),0x22b09u);
    if(clamp){x[0]=x[2];put(0x515ed0u,bits(x[0]),0x22b16u);}
    if(g[2]==1){
        g[0]=get(g[1]+0x3cu);
        constexpr std::array sources{12u,0x30u,0x54u};
        constexpr std::array targets{0x4c5160u,0x4c5184u,0x4c51a8u};
        constexpr std::array sites{0x22b33u,0x22b42u,0x22b51u};
        for(unsigned channel=0;channel<3;++channel){g[6]=g[0]+sources[channel];g[7]=targets[channel];copy(sites[channel]);}
        finish();return;
    }
    x[2]=scalar(get(0x46a0d0u));g[1]=get(g[1]+0x3cu);--g[2];
    x[3]=_mm_cvtsi32_ss(x[3],std::bit_cast<std::int32_t>(g[2]));x[1]=x[2];
    x[1]=_mm_div_ss(x[1],x[3]);x[2]=_mm_div_ss(x[2],x[1]);x[3]=multiply(x[2],x[0],false);
    g[0]=std::uint32_t(_mm_cvttss_si32(x[3]));x[3]=_mm_cvtsi32_ss(x[3],std::bit_cast<std::int32_t>(g[0]));
    g[0]*=0xdcu;g[6]=g[0]+g[1];x[3]=multiply(x[3],x[1],false);
    s.push(0x22b91u,0x4c5160u);g[2]=g[6]+12u;x[0]=subtract(x[0],x[3]);x[0]=multiply(x[0],x[2],false);
    g[1]=g[6]+0xe8u;s.push(0x22ba7u,g[2]);g[0]=3;put(g[4]+0x1cu,bits(x[0]),0x22badu);
    s.push(0x22bb3u,0x22bb8u);prt_interpolation_detail::bridge(s,0x22730u);
    x[0]=scalar(get(g[4]+0x14u));s.push(0x22bbeu,0x4c5184u);g[0]=g[6]+0x30u;
    s.push(0x22bc6u,g[0]);g[1]=g[6]+0x10cu;g[0]=3;
    s.push(0x22bd2u,0x22bd7u);prt_interpolation_detail::bridge(s,0x22730u);
    x[0]=scalar(get(g[4]+0x14u));g[1]=g[6]+0x130u;s.push(0x22be3u,0x4c51a8u);
    g[6]+=0x54u;s.push(0x22bebu,g[6]);g[0]=3;
    s.push(0x22bf1u,0x22bf6u);prt_interpolation_detail::bridge(s,0x22730u);finish();
    (void)entry;(void)frame;
}
}
