#pragma once
// Boundary adapter for the curve value, 13E670
// (native_tu15_curve_value_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: EAX the function, two arguments on
// the stack (RET 8), the value in XMM0; EDI saved; 13DE70 gets the
// arguments pushed (RET 8) and answers XMM0, every register it returns
// coming back. Each access publishes its original instruction.
// HALO2_REFERENCE_CURVE_VALUE=1 restores the translated body.
// HALO2_VERIFY_CURVE_VALUE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/effects/native_tu15_curve_value_systems.hpp"

namespace halo2::native_curve_value_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_curve_value;
using Access=logic::curve_value::Access;
using Kept=logic::curve_value::Kept;
using Outcome=logic::curve_value::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_CURVE_VALUE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_CURVE_VALUE");return v;}
enum Branch : unsigned {empty, unclamped, below, inside, above, branch_count};
static_assert(unsigned(Outcome::above)==Branch::above);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Curve value"};return v;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t lane0(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
inline constexpr std::uint32_t sites[]{0x13e673u,0x13e67au,0x13e67fu,0x13e683u,0x13e68eu,0x13e691u,0x13e69du,0x13e6a2u,0x13e6a9u};
template<class State,class Fraction> struct Adapter : Port<State> {
    Fraction &fraction_child;
    bool compared_once{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Fraction &f):Port<State>(s,c,t,{}),fraction_child(f){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+8u+4u*k;}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case curve_test:gp[eax]=v;this->test32(v);break;
        case count:this->flags.minus(v,0u,4);break;                                                     // CMP [EDI],0
        case second_argument:gp[eax]=v;break;
        case first_argument:gp[ecx]=v;break;
        case curve_address:gp[edi]=v;break;
        case curve_flags:this->flags.logic(v&logic::curve::unclamped,1);break;                          // TEST byte [EDI+1],F0
        case start:x[2]=lane0_only(v);break;
        case end:x[1]=lane0_only(v);break;
        case one:x[3]=lane0_only(v);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH EAX; PUSH ECX; CALL 13DE70.
    std::uint32_t fraction(std::uint32_t,std::uint32_t,Kept &kept) {
        auto &gp=this->gp;
        this->push(0x13e687u,gp[eax]);this->push(0x13e688u,gp[ecx]);
        this->call(0x13e689u,0x13e68eu,0x13de70u,fraction_child);
        kept.function=gp[edi];
        return lane0(this->s.xmm[0]);
    }
    // XORPS XMM3,XMM3 before the first; COMISS.
    void compared(std::uint32_t a,std::uint32_t b) {
        if(!compared_once){this->s.xmm[3]=_mm_setzero_ps();compared_once=true;}
        this->flags.scalar_compare(lane0_only(a),lane0_only(b),false);
    }
    // MOVAPS XMM0,XMM3 when clamped; SUBSS, MULSS, ADDSS in XMM1; MOVAPS XMM0,XMM1.
    void interpolated(const logic::curve_value::Interpolation &v) {
        auto &x=this->s.xmm;
        x[1]=_mm_move_ss(x[1],lane0_only(v.value));x[0]=x[1];
    }
    void run() {
        auto &gp=this->gp;
        this->push(0x13e670u,gp[edi]);gp[edi]=gp[eax];
        Memory<Adapter> memory{*this};
        const auto outcome=logic::curve_value::of(memory,*this,gp[edi]);
        this->reached(unsigned(outcome));
        std::uint32_t exit=0x13e6cfu;
        if(outcome==Outcome::empty)this->s.xmm[0]=_mm_setzero_ps();                                 // XORPS XMM0,XMM0
        else if(outcome!=Outcome::unclamped)exit=0x13e6c8u;
        gp[edi]=this->pop(exit);this->ret(exit+1u,8u);this->publish(exit+1u);
    }
};
template<class State,class Reference,class Fraction> bool bridge(State &s,Reference reference,Fraction fraction) {
    return run_owner(s,0x13e670u,0x13e6d0u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Fraction>(v,c,t,fraction);});
}
} // namespace halo2::native_curve_value_detail
