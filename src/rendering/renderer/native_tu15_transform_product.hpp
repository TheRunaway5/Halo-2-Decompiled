#pragma once
// Boundary adapter for transform product, 145560
// (native_tu15_transform_product_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: ECX the left transform, EDX the right transform
// and the destination pushed on the stack (RET 4); EAX receives the
// destination pointer and XMM0..XMM7 carry the exact SSE register states at
// every access and at exit on the live MXCSR. Each access publishes its
// original instruction.
// HALO2_REFERENCE_TRANSFORM_PRODUCT=1 restores the translated body.
// HALO2_VERIFY_TRANSFORM_PRODUCT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_packed_sse.hpp"
#include "rendering/renderer/native_tu15_transform_product_systems.hpp"

namespace halo2::native_transform_product_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_transform_product;
using Access=logic::transform_product::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_TRANSFORM_PRODUCT");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_TRANSFORM_PRODUCT");return v;}
enum Branch : unsigned {composed, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Transform product"};return v;}
inline constexpr std::uint32_t sites[]{
    0x145560u,0x145564u,0x14556cu,0x145570u,0x145575u,0x14557au,
    0x14558fu,0x145594u,0x145598u,0x14559du,0x1455a1u,0x1455a6u,
    0x1455bcu,0x1455c1u,
    0x1455c8u,0x1455cdu,0x1455d2u,0x1455d6u,0x1455dbu,
    0x1455fbu,0x145600u,0x145605u,0x145609u,0x14560eu,
    0x14562eu,0x145633u,0x145638u,0x14563cu,0x145641u,
    0x145661u,0x145666u,0x14566au};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::out_scale)+1u);
template<class State> struct Adapter : Port<State> {
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t argument_slot()const{return this->gp[esp]+logic::out_argument_offset;}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        if(a==Access::out_argument)this->gp[eax]=v;
    }
    void loaded_vector(Access a,std::uint32_t,__m128 v) {
        using enum Access;
        switch(a) {
        case left_fwd_yz:this->s.xmm[4]=v;break;
        case left_left_yz:this->s.xmm[5]=v;break;
        case left_up_yz:this->s.xmm[6]=v;break;
        case left_pos_yz:this->s.xmm[1]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    __m128 scalar(unsigned reg,std::uint32_t bits) {
        return this->s.xmm[reg]=_mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));
    }
    __m128 broadcast(unsigned reg,__m128 v) {
        return this->s.xmm[reg]=_mm_shuffle_ps(v,v,0);
    }
    __m128 splat(unsigned reg,std::uint32_t bits){return broadcast(reg,scalar(reg,bits));}
    __m128 mul_scalar(unsigned reg,__m128 a,__m128 b){return this->s.xmm[reg]=_mm_mul_ss(a,b);}
    __m128 mul_packed(unsigned reg,__m128 a,__m128 b){return this->s.xmm[reg]=halo2::native_packed_sse::multiply(a,b);}
    __m128 add_packed(unsigned reg,__m128 a,__m128 b){return this->s.xmm[reg]=halo2::native_packed_sse::add(a,b);}
    std::uint32_t lane0(__m128 v)const{return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(v));}
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::transform_product::compose(memory,*this,gp[ecx],gp[edx]);
        this->reached(Branch::composed);
        this->ret(0x14566eu,4u);
        this->publish(0x14566eu);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x145560u,0x14566eu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_transform_product_detail
