#pragma once
// Boundary adapter for the transition fraction, entered at 1E1620
// (native_tu15_transition_fraction_systems.hpp). Its callers and the curves
// keep the original ABI, which the adapter owns: EAX the transition, the
// result in XMM0, RET; ECX the duration (MOVSX), EDX the elapsed ticks
// (MOVSX), EAX the curve - 1; XMM1 the duration (then zero), XMM2 one, XMM3
// the cube ease's (1 - x)^3; 110FC0 gets XMM2 the fraction and 1E1920 XMM3
// (both through their units' routes), and every register they return comes
// back; the flags of the last instruction that sets them. Each access
// publishes its original instruction.
// HALO2_REFERENCE_TRANSITION_FRACTION=1 restores the translated body.
// HALO2_VERIFY_TRANSITION_FRACTION=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_transition_fraction_systems.hpp"
#include <xmmintrin.h>

namespace halo2::native_transition_fraction_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_transition_fraction;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_TRANSITION_FRACTION");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_TRANSITION_FRACTION");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    no_duration, short_duration, linear, smoothstep, smootherstep, ease_out_square, ease_out_cube, low, high, within,
    branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Transition fraction verify: %llu calls; %llu ordered accesses (%llu stores) "
            "identical, coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0(std::uint32_t bits){return logic::sse::scalar(bits);}
inline std::uint32_t bits0(__m128 x){return logic::sse::bits(x);}
template<class State,class Route,class Smoothstep> struct Fraction : Port<State> {
    using Access=logic::fraction::Access;
    Route &route;Smoothstep &smoothstep_child;
    std::uint32_t one_after_pc{};bool timed_in{},low_exit{},high_exit{};
    Fraction(State &s,std::uint32_t *c,Trace<State> *t,Route &r,Smoothstep &m)
        :Port<State>(s,c,t,{}),route(r),smoothstep_child(m){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case duration:return 0x1e1620u;
        case one:return 0x1e162eu;
        case elapsed:return 0x1e1645u;
        case curve:return 0x1e1648u;
        case curve_case:return 0x1e165au;
        case one_after_curve:return one_after_pc;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        // MOV CL; TEST CL,CL; XORPS XMM0,XMM0.
        case duration:set_low_byte(gp[ecx],v);this->test8(v);x[0]=_mm_setzero_ps();
            if(!(v&0xffu))this->reached(Branch::no_duration);break;
        case one:case one_after_curve:x[2]=lane0(v);break;                                  // MOVSS XMM2
        case elapsed:gp[edx]=std::uint32_t(std::int32_t(std::int8_t(v)));break;              // MOVSX EDX
        // MOVSX EAX; DEC EAX; CMP EAX,3 (the flags of the CMP).
        case curve:gp[eax]=std::uint32_t(std::int32_t(std::int8_t(v)))-1u;this->flags.minus(gp[eax],3u,4);break;
        case curve_case:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // MOVSX ECX,CL; CVTSI2SS XMM1,ECX; COMISS XMM1,XMM2 (its flags are replaced); MOVAPS XMM1,XMM2 when clamped.
    void timed(bool clamped) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        gp[ecx]=std::uint32_t(std::int32_t(std::int8_t(gp[ecx]&0xffu)));
        x[1]=clamped?x[2]:_mm_move_ss(x[1],lane0(logic::float_of_byte(gp[ecx])));timed_in=true;
        if(clamped)this->reached(Branch::short_duration);
    }
    // CVTSI2SS XMM0,EDX; DIVSS XMM0,XMM1 (XMM0 was zero).
    void divided(std::uint32_t value) {
        this->s.xmm[0]=lane0(value);
        if(this->gp[eax]>=logic::curve_cases)this->reached(Branch::linear);
    }
    // MOVAPS XMM2,XMM0; CALL 110FC0.
    std::uint32_t smoothstep(std::uint32_t) {
        this->s.xmm[2]=this->s.xmm[0];
        this->call(0x1e1664u,0x1e1669u,0x110fc0u,smoothstep_child);
        one_after_pc=0x1e1669u;this->reached(Branch::smoothstep);
        return bits0(this->s.xmm[0]);
    }
    // MOVAPS XMM3,XMM0; CALL 1E1920.
    std::uint32_t smootherstep(std::uint32_t) {
        this->s.xmm[3]=this->s.xmm[0];
        this->call_routed(route,0x1e1676u,0x1e167bu,0x1e1920u);
        one_after_pc=0x1e167bu;this->reached(Branch::smootherstep);
        return bits0(this->s.xmm[0]);
    }
    // SUBSS XMM0,XMM2; MOVAPS XMM1,XMM0; MULSS XMM1,XMM0; MOVAPS XMM0,XMM2; SUBSS XMM0,XMM1
    // (XMM1 is zeroed below).
    void squared(std::uint32_t value){this->s.xmm[0]=lane0(value);this->reached(Branch::ease_out_square);}
    // MOVAPS XMM1,XMM2; SUBSS XMM1,XMM0; MOVAPS XMM0,XMM1; MOVAPS XMM3,XMM0; MULSS XMM3,XMM0;
    // MULSS XMM3,XMM1; MOVAPS XMM0,XMM2; SUBSS XMM0,XMM3.
    void cubed(std::uint32_t rest_cubed,std::uint32_t value) {
        this->s.xmm[3]=lane0(rest_cubed);this->s.xmm[0]=lane0(value);this->reached(Branch::ease_out_cube);
    }
    [[noreturn]] void unknown_case(){throw std::runtime_error("Unbound TU1.5 contact dispatch target");}
    // XORPS XMM1,XMM1 (before the first); COMISS.
    void compared(std::uint32_t a,std::uint32_t b) {
        this->s.xmm[1]=_mm_setzero_ps();
        this->flags.scalar_compare(lane0(a),lane0(b),false);
    }
    void clamped_low(){this->s.xmm[0]=this->s.xmm[1];low_exit=true;this->reached(Branch::low);}    // MOVAPS XMM0,XMM1
    void clamped_high(std::uint32_t){this->s.xmm[0]=this->s.xmm[2];high_exit=true;this->reached(Branch::high);}   // MOVAPS XMM0,XMM2
    void run() {
        Memory<Fraction> memory{*this};
        (void)logic::fraction::of(memory,*this,this->gp[eax]);
        if(timed_in && !low_exit && !high_exit)this->reached(Branch::within);
        // RET at 1E16C0 after the low clamp, 1E16C9 otherwise.
        const auto exit=low_exit?0x1e16c0u:0x1e16c9u;
        this->ret(exit,0);this->publish(exit);
    }
};
template<class State,class Reference,class Route,class Smoothstep>
bool bridge(State &s,Reference reference,Route route,Smoothstep smoothstep) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1e1620u,0x1e16c9u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Fraction<State,Route,Smoothstep>(v,c,t,route,smoothstep);});
    return true;
}
} // namespace halo2::native_transition_fraction_detail
