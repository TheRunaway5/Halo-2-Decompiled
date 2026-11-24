#pragma once
// Boundary adapter for the smootherstep curve, entered at 0x1e1920
// (native_tu15_smooth_curves_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: XMM3 the input (its upper lanes carried into XMM0 and XMM2 as the original's
// MOVAPS copies do), the result in XMM0, XMM1 zero or one;
// the flags of the last COMISS; RET. Each access publishes its original
// instruction.
// HALO2_REFERENCE_SMOOTH_CURVES=1 restores the translated body.
// HALO2_VERIFY_SMOOTH_CURVES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_smooth_curves_systems.hpp"
#include <xmmintrin.h>

namespace halo2::native_smootherstep_detail {
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
        if(calls)std::fprintf(stderr,"Smootherstep verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0(std::uint32_t bits){return logic::sse::scalar(bits);}
template<class State> struct Curve : Port<State> {
    using Access=logic::smootherstep::Access;
    Curve(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case ten:return 0x1e192eu;
        case fifteen:return 0x1e193du;
        case six:return 0x1e1949u;
        case one:return 0x1e1961u;
        }
        return 0;
    }
    // MOVAPS XMM1,XMM3; ...; MOVAPS XMM2,XMM1; ...; MOVAPS XMM0,XMM2; ...: the lanes above stay XMM3's.
    void shaped(std::uint32_t value,std::uint32_t fifteen_fourth) {
        auto &x=this->s.xmm;
        x[0]=_mm_move_ss(x[3],lane0(value));x[2]=_mm_move_ss(x[3],lane0(fifteen_fourth));
    }
    void cleared(){this->s.xmm[1]=_mm_setzero_ps();}                                     // XORPS XMM1,XMM1
    // XMM0: the value with its lanes above (within), else MOVAPS XMM0,XMM1 (zero or one).
    void finish(const logic::Result &r) {
        auto &x=this->s.xmm;
        x[0]=r.clamp==logic::Clamp::within?_mm_move_ss(x[0],lane0(r.value)):lane0(r.value);
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v){if(a==Access::one)this->s.xmm[1]=lane0(v);}
    void stored(Access,std::uint32_t,std::uint32_t){}
    void compared(std::uint32_t a,std::uint32_t b){this->flags.scalar_compare(lane0(a),lane0(b),false);}
    void run() {
        Memory<Curve> memory{*this};
        const auto result=logic::smootherstep::of(memory,*this,logic::sse::bits(this->s.xmm[3]));
        finish(result);
        this->reached(result.clamp==logic::Clamp::low?Branch::low:result.clamp==logic::Clamp::high?Branch::high:Branch::within);
        this->ret(0x1e1971u,0);this->publish(0x1e1971u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1e1920u,0x1e1971u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Curve<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_smootherstep_detail
