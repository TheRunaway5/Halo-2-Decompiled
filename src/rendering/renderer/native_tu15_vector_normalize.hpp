#pragma once
// Boundary adapter for vector normalize, 1E1010
// (native_tu15_vector_normalize_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: ECX the 16-byte vector address (RET 0); EBP
// frame pointer with ESP aligned by AND ESP,FFFFFFF0 and SUB ESP,10; XMM0..XMM3
// carrying exact SSE results on the live MXCSR; RSQRTSS routed through
// s.services.reciprocal_sqrt at 0x1E1042. Each access publishes its original
// instruction.
// HALO2_REFERENCE_VECTOR_NORMALIZE=1 restores the translated body.
// HALO2_VERIFY_VECTOR_NORMALIZE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_packed_sse.hpp"
#include "rendering/renderer/native_tu15_vector_normalize_systems.hpp"

namespace halo2::native_vector_normalize_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_vector_normalize;
using Access=logic::vector_normalize::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_VECTOR_NORMALIZE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_VECTOR_NORMALIZE");return v;}
enum Branch : unsigned {normalized, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Vector normalize"};return v;}
inline constexpr std::uint32_t sites[]{0x1e1019u,0x1e101cu,0x1e103eu,0x1e1046u,0x1e104bu,0x1e105bu,0x1e1075u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::vector_store)+1u);
template<class State> struct Adapter : Port<State> {
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t slot_address()const{return this->gp[esp];}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access,std::uint32_t,std::uint32_t){}
    void loaded_vector(Access a,std::uint32_t,__m128 v) {
        if(a==Access::vector_load)this->s.xmm[2]=v;
        else if(a==Access::estimate_reload)this->s.xmm[1]=v;
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void align_frame() {                                                                                // PUSH EBP; MOV EBP,ESP; AND ESP,FFFFFFF0; SUB ESP,10
        auto &gp=this->gp;
        this->push(0x1e1010u,gp[ebp]);
        gp[ebp]=gp[esp];
        gp[esp]=this->flags.logic(gp[esp]&logic::stack_align_mask,4);
        gp[esp]=this->flags.minus(gp[esp],logic::frame_size,4);
    }
    __m128 scalar(unsigned reg,std::uint32_t bits) {
        return this->s.xmm[reg]=_mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));
    }
    __m128 shuffle(unsigned reg,__m128 a,__m128 b,int imm) {
        __m128 out;
        if(imm==78)out=_mm_shuffle_ps(a,b,78);
        else if(imm==177)out=_mm_shuffle_ps(a,b,177);
        else out=_mm_shuffle_ps(a,b,0);
        return this->s.xmm[reg]=out;
    }
    __m128 mul_packed(unsigned reg,__m128 a,__m128 b){return this->s.xmm[reg]=halo2::native_packed_sse::multiply(a,b);}
    __m128 add_packed(unsigned reg,__m128 a,__m128 b){return this->s.xmm[reg]=halo2::native_packed_sse::add(a,b);}
    __m128 mul_scalar(unsigned reg,__m128 a,__m128 b){return this->s.xmm[reg]=_mm_mul_ss(a,b);}
    __m128 sub_scalar(unsigned reg,__m128 a,__m128 b){return this->s.xmm[reg]=_mm_sub_ss(a,b);}
    std::uint32_t lane0(__m128 v)const{return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(v));}
    __m128 estimate_rsqrt(__m128 dst,__m128 src) {                                                      // RSQRTSS XMM1,XMM0 via services.reciprocal_sqrt
        this->at(0x1e1042u);
        if(!this->s.services.reciprocal_sqrt)
            throw std::runtime_error("Required original contact RSQRTSS owner unavailable");
        const auto est=this->s.services.reciprocal_sqrt(lane0(src));
        return this->s.xmm[1]=_mm_move_ss(dst,_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(est))));
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::vector_normalize::normalize(memory,*this,gp[ecx]);
        this->reached(Branch::normalized);
        gp[esp]=gp[ebp];
        gp[ebp]=this->pop(0x1e107au);
        this->ret(0x1e107bu,0u);
        this->publish(0x1e107bu);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x1e1010u,0x1e107bu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_vector_normalize_detail
