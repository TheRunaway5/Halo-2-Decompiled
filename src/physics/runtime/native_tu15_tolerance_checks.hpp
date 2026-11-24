#pragma once
// Boundary adapters for the tolerance checks, entered at AA200 and AA270
// (native_tu15_tolerance_checks_systems.hpp). Their callers keep the
// original ABI, which the adapters own:
//   - AA200: EAX the vector; SUB ESP,8 holds the difference twice; EAX the
//     result; XMM0-XMM3 as the original leaves them (the difference, z, z
//     squared, y squared); RET;
//   - AA270: XMM0 the value (its upper lanes kept), the target at [ESP+4];
//     PUSH ECX makes the slot that ECX is popped from (the difference);
//     the target's slot holds the difference; RET 4;
//   - the x87 comparison runs on the logical x87 as the original's FLD m32,
//     FABS, FLD m32, FCOMIP and FSTP ST0 do (two free registers are
//     required at admission); the flags of the last instruction that sets
//     them; each access publishes its original instruction.
// HALO2_REFERENCE_TOLERANCE_CHECKS=1 restores the translated bodies.
// HALO2_VERIFY_TOLERANCE_CHECKS=1 records the native runs and replays the
// unmodified translated bodies against them, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_tolerance_checks_systems.hpp"
#include <cmath>
#include <emmintrin.h>

namespace halo2::native_tolerance_checks_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_tolerance_checks;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_TOLERANCE_CHECKS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_TOLERANCE_CHECKS");return p && *p;}();
    return value;
}
enum Branch : unsigned {not_finite, finite, within, outside, branch_count};
struct Summaries {
    struct One : native_object_tick_detail::Tally {
        const char *name;std::atomic<std::uint64_t> calls{},hit{};
        explicit One(const char *n):name(n){}
        ~One() {
            if(calls)std::fprintf(stderr,"%s verify: %llu calls; %llu ordered accesses (%llu stores) identical, coverage %llx\n",
                name,(unsigned long long)calls.load(),(unsigned long long)accesses.load(),(unsigned long long)stores.load(),
                (unsigned long long)hit.load());
        }
    };
};
inline Summaries::One &length_summary(){static Summaries::One value{"Unit length tolerance"};return value;}
inline Summaries::One &near_summary(){static Summaries::One value{"Near tolerance"};return value;}
inline std::atomic<std::uint64_t> &length_coverage(){return length_summary().hit;}
inline std::atomic<std::uint64_t> &near_coverage(){return near_summary().hit;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline __m128 with_lane0(__m128 x,std::uint32_t bits){return _mm_move_ss(x,lane0_only(bits));}
// FLD m32, as the translated route converts it.
inline long double extended(std::uint32_t bits){return physics_step_detail::ext(physics_step_detail::f(bits));}
inline constexpr bool backend=logic::sse::available;

// The shared x87 test: FLD the difference; FABS; FLD the tolerance; FCOMIP
// ST,ST1; FSTP ST0; JBE.
template<class State> struct ToleranceX87 : Port<State> {
    using Port<State>::Port;
    void x87_load(std::uint32_t v,bool absolute) {
        auto &fp=this->s.fp;
        fp.push(extended(v));if(absolute)fp.st(0)=std::fabs(fp.st(0));
    }
    bool within() {
        auto &fp=this->s.fp;
        this->s.flags=this->flags;this->s.compare_and_pop(fp.st(1));this->flags=this->s.flags;
        fp.st(0)=fp.st(0);fp.pop();
        const bool inside=!this->flags.cf && !this->flags.zf;
        this->reached(inside?Branch::within:Branch::outside);
        return inside;
    }
    // AND EAX,7F800000; CMP EAX,7F800000.
    void exponent(std::uint32_t v) {
        this->gp[eax]=this->flags.logic(v&logic::exponent_bits,4);this->compare32(this->gp[eax],logic::exponent_bits);
        this->reached((v&logic::exponent_bits)==logic::exponent_bits?Branch::not_finite:Branch::finite);
    }
};

template<class State> struct UnitLength : ToleranceX87<State> {
    using Access=logic::unit_length::Access;
    using Local=logic::unit_length::Local;
    UnitLength(State &s,std::uint32_t *c,Trace<State> *t):ToleranceX87<State>(s,c,t,{}){}
    std::uint32_t local(Local l)const{return this->gp[esp]+(l==Local::difference?0u:4u);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case y:return 0xaa203u;
        case z:return 0xaa208u;
        case x:return 0xaa20du;
        case one:return 0xaa22bu;
        case difference_store:return 0xaa233u;
        case difference_bits:return 0xaa238u;
        case difference_copy_store:return 0xaa245u;
        case difference_x87:return 0xaa24du;
        case tolerance:return 0xaa253u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        using enum Access;
        if(a==difference_bits)this->exponent(v);
        else if(a==difference_x87)this->x87_load(v,true);
        else if(a==tolerance)this->x87_load(v,false);
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void squares(const logic::unit_length::Squares &q) {
        auto &x=this->s.xmm;
        x[0]=lane0_only(q.difference);x[1]=lane0_only(q.z);x[2]=lane0_only(q.z_squared);x[3]=lane0_only(q.y_squared);
    }
    void run() {
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],8u,4);                                                  // SUB ESP,8
        Memory<UnitLength> memory{*this};
        const auto result=logic::unit_length::check(memory,*this,gp[eax]);
        gp[eax]=result?1u:this->flags.logic(0u,4);                                                // MOV EAX,1 or XOR
        gp[esp]=this->flags.plus(gp[esp],8u,4);
        const auto pc=result?0xaa267u:0xaa26du;
        this->ret(pc,0);this->publish(pc);
    }
};

template<class State> struct Near : ToleranceX87<State> {
    using Access=logic::near::Access;
    using Local=logic::near::Local;
    Near(State &s,std::uint32_t *c,Trace<State> *t):ToleranceX87<State>(s,c,t,{}){}
    std::uint32_t local(Local l)const{return this->gp[esp]+(l==Local::target?8u:0u);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case target:return 0xaa271u;
        case difference_store:return 0xaa277u;
        case difference_bits:return 0xaa27du;
        case slot_store:return 0xaa28bu;
        case difference_x87:return 0xaa292u;
        case tolerance:return 0xaa297u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        using enum Access;
        if(a==difference_bits)this->exponent(v);
        else if(a==difference_x87)this->x87_load(v,true);
        else if(a==tolerance)this->x87_load(v,false);
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void difference(std::uint32_t d){this->s.xmm[0]=with_lane0(this->s.xmm[0],d);}                // SUBSS XMM0,m32
    void run() {
        auto &gp=this->gp;
        this->push(0xaa270u,gp[ecx]);                                                            // PUSH ECX
        Memory<Near> memory{*this};
        const auto value=logic::sse::bits(this->s.xmm[0]);
        const auto result=logic::near::check(memory,*this,value);
        gp[eax]=result?1u:this->flags.logic(0u,4);                                                // MOV EAX,1 or XOR
        const auto pc=result?0xaa2a8u:0xaa2aeu;
        gp[ecx]=this->pop(pc);this->ret(pc+1u,4u);this->publish(pc+1u);
    }
};

template<class State> bool admissible(State &s) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    return s.fp.count<=6u;
}
template<class State,class Reference> bool bridge_unit_length(State &s,Reference reference) {
    if(!admissible(s))return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xaa200u,0xaa26du,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),length_coverage(),length_summary().calls,length_summary(),
        reference,[&](State &v,std::uint32_t *c,Trace<State> *t){return UnitLength<State>(v,c,t);});
    return true;
}
template<class State,class Reference> bool bridge_near(State &s,Reference reference) {
    if(!admissible(s))return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xaa270u,0xaa2afu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),near_coverage(),near_summary().calls,near_summary(),
        reference,[&](State &v,std::uint32_t *c,Trace<State> *t){return Near<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_tolerance_checks_detail
