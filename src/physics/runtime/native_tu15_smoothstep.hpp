#pragma once
// Boundary adapter for the smoothstep curve, entered at 0x110fc0
// (native_tu15_smooth_curves_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: XMM2 the input (its upper lanes carried into XMM0 and XMM1 as the original's
// MOVAPS copies do), the result in XMM0, XMM1 the shaped value;
// the flags of the last COMISS; RET. Each access publishes its original
// instruction.
// HALO2_REFERENCE_SMOOTH_CURVES=1 restores the translated body.
// HALO2_VERIFY_SMOOTH_CURVES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_smooth_curves_systems.hpp"
#include <xmmintrin.h>

namespace halo2::native_smoothstep_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_smooth_curves;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_SMOOTH_CURVES");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_SMOOTH_CURVES");return p && *p;}();
    return value;
}
enum Branch : unsigned {low, high, within, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Smoothstep verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0(std::uint32_t bits){return logic::sse::scalar(bits);}
template<class State> struct Curve : Port<State> {
    using Access=logic::smoothstep::Access;
    Curve(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case three:return 0x110fcau;
        case two:return 0x110fd6u;
        case one:return 0x110feau;
        }
        return 0;
    }
    // MOVAPS XMM0,XMM2; MULSS ...; MOVAPS XMM1,XMM0; ...; SUBSS XMM1,XMM0: the lanes above stay XMM2's.
    void shaped(std::uint32_t value,std::uint32_t two_cube) {
        auto &x=this->s.xmm;
        x[1]=_mm_move_ss(x[2],lane0(value));x[0]=_mm_move_ss(x[2],lane0(two_cube));
    }
    // XORPS XMM0,XMM0 and MOVSS XMM0,[one]: the result register, which finish() sets.
    void cleared(){}
    void loaded(Access,std::uint32_t,std::uint32_t){}
    // XMM0: zero (low), one (high, MOVSS) or the value with XMM1's lanes above (within, MOVAPS).
    void finish(const logic::Result &r) {
        auto &x=this->s.xmm;
        x[0]=r.clamp==logic::Clamp::within?_mm_move_ss(x[1],lane0(r.value)):lane0(r.value);
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void compared(std::uint32_t a,std::uint32_t b){this->flags.scalar_compare(lane0(a),lane0(b),false);}
    void run() {
        Memory<Curve> memory{*this};
        const auto result=logic::smoothstep::of(memory,*this,logic::sse::bits(this->s.xmm[2]));
        finish(result);
        this->reached(result.clamp==logic::Clamp::low?Branch::low:result.clamp==logic::Clamp::high?Branch::high:Branch::within);
        this->ret(0x110ffau,0);this->publish(0x110ffau);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x110fc0u,0x110ffau,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Curve<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_smoothstep_detail
