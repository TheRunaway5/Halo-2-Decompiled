#pragma once
// Boundary adapter for the light admits, entered at C5170
// (native_tu15_light_admits_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EAX the handle (RET); AL the answer over the
// light's address, ECX the second vector's verdict (1 or 0) or the light,
// EDX the light data with DL the flags byte; XMM0 the last z, XMM1 its
// square, XMM2 the last y squared, XMM3 the threshold, XMM4 the last length;
// the flags of the closing AND (or the flags' TEST). Each access publishes
// its original instruction.
// HALO2_REFERENCE_LIGHT_ADMITS=1 restores the translated body.
// HALO2_VERIFY_LIGHT_ADMITS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "pstencil/native_tu15_light_admits_systems.hpp"

namespace halo2::native_light_admits_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_light_admits;
using Access=logic::light_admits::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_LIGHT_ADMITS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_LIGHT_ADMITS");return v;}
enum Branch : unsigned {unflagged, admitted, refused, second_vector, branch_count};
static_assert(unsigned(logic::light_admits::Outcome::refused)==Branch::refused);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Light admits"};return v;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t vector_sites[2][3]{{0xc51bbu,0xc51c3u,0xc51cbu},{0xc51fdu,0xc5205u,0xc520du}};
template<class State> struct Adapter : Port<State> {
    unsigned vector{};
    bool long_enough{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    void measuring(unsigned k){vector=k;if(k)this->reached(Branch::second_vector);}
    void verdict(bool lengthy){long_enough=lengthy;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case lights:return 0xc5170u;
        case data:return 0xc5176u;
        // AND EAX,FFFF; IMUL EAX,EAX,110; ADD EAX,EDX; MOV ECX,EAX.
        case flags:gp[eax]=fl.plus(fl.product(fl.logic(gp[eax]&0xffffu,4),logic::light_size),gp[edx],4);gp[ecx]=gp[eax];
            return 0xc5188u;
        case intensity:return 0xc5196u;
        case epsilon:return 0xc519bu;
        case second_intensity:return 0xc51a8u;
        case x:return vector_sites[vector][0];
        case y:return vector_sites[vector][1];
        case z:return vector_sites[vector][2];
        case threshold:return 0xc51d3u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &xmm=this->s.xmm;
        using enum Access;
        switch(a) {
        case lights:gp[ecx]=v;break;
        case data:gp[edx]=v;break;
        // MOV DL,[ECX+2]; XOR AL,AL; TEST DL,2.
        case flags:set_low_byte(gp[edx],v);set_low_byte(gp[eax],0u);this->test8(v&logic::portal_flag);break;
        case intensity:case second_intensity:xmm[1]=lane0_only(v);break;
        case epsilon:xmm[0]=lane0_only(v);break;
        case x:xmm[2]=lane0_only(v);break;
        case y:xmm[1]=lane0_only(v);break;
        case z:xmm[0]=lane0_only(v);break;
        case threshold:xmm[3]=lane0_only(v);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // MOVAPS/MULSS/ADDSS; COMISS XMM4,XMM3.
    void measured(const logic::light_admits::Length &length) {
        auto &xmm=this->s.xmm;
        xmm[0]=lane0_only(length.z);xmm[1]=lane0_only(length.zz);xmm[2]=lane0_only(length.yy);xmm[4]=lane0_only(length.sum);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        bool answer=false;
        const auto outcome=logic::light_admits::admits(memory,*this,gp[eax],answer);
        this->reached(unsigned(outcome));
        if(outcome!=logic::light_admits::Outcome::unflagged) {
            // (MOV AL,1 / XOR AL,AL); MOV ECX,1 or XOR ECX,ECX; AND AL,CL.
            gp[ecx]=long_enough?1u:this->flags.logic(0u,4);
            set_low_byte(gp[eax],this->flags.logic(answer?1u:0u,1));
        }
        const auto exit=outcome!=logic::light_admits::Outcome::unflagged && !long_enough?0xc523bu:0xc5243u;
        this->ret(exit,0u);this->publish(exit);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0xc5170u,0xc5243u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_light_admits_detail
