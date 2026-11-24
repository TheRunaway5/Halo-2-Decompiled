#pragma once
// Boundary adapter for the pixel scales, entered at 1BE90
// (native_tu15_pixel_scales_systems.hpp). Its callers keep the original
// ABI, which the adapter owns (RET): EAX the state, ECX the width (its low
// byte the state's byte +5 once read); XMM0 zero, XMM1 the width over its
// upper lanes, XMM2 one (or the rescaled last), XMM3 the first constant,
// XMM4 the second, each per pixel where the original divides them; the
// flags of the last TEST. Each access publishes its original instruction.
// HALO2_REFERENCE_PIXEL_SCALES=1 restores the translated body.
// HALO2_VERIFY_PIXEL_SCALES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "rendering/shadows/native_tu15_pixel_scales_systems.hpp"

namespace halo2::native_pixel_scales_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_pixel_scales;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_PIXEL_SCALES");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_PIXEL_SCALES");return p && *p;}();
    return value;
}
enum Branch : unsigned {plain, rescaled, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Pixel scales verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t scale_stores[]{0x1bee1u,0x1bee9u,0x1bef1u,0x1bef9u,0x1bf01u,0x1bf09u};
template<class State> struct PixelScales : Port<State> {
    using Access=logic::pixel_scales::Access;
    unsigned stores{};
    PixelScales(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case left:return 0x1be90u;
        case right:return 0x1be97u;
        case first_constant:return 0x1be9eu;
        case one:return 0x1bea6u;
        case state:this->gp[ecx]=this->flags.minus(this->gp[ecx],this->gp[eax],4);return 0x1beb3u;   // SUB ECX,EAX
        case first_store:return 0x1bec5u;
        case second_constant:return 0x1becdu;
        case scale_store:return scale_stores[stores++];
        case Access::rescaled:return 0x1bf13u;
        case rescaled_store:return 0x1bf1eu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case left:gp[eax]=logic::widen16(v);break;
        case right:gp[ecx]=logic::widen16(v);break;
        case first_constant:x[3]=lane0_only(v);break;
        case one:x[2]=lane0_only(v);x[0]=_mm_setzero_ps();break;                                    // (XORPS XMM0,XMM0)
        case state:gp[eax]=v;this->test32(v);break;                                                // TEST EAX,EAX
        case Access::rescaled:set_low_byte(gp[ecx],v);this->test8(v);break;                       // MOV CL,[EAX+5]; TEST CL,CL
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v){if(a==Access::rescaled_store)this->s.xmm[2]=lane0_only(v);}
    // CVTSI2SS XMM1,ECX; MOVAPS XMM4,XMM3; DIVSS ×4.
    void divided(std::uint32_t width,std::uint32_t,std::uint32_t zero,std::uint32_t one,std::uint32_t second) {
        auto &x=this->s.xmm;
        x[1]=_mm_move_ss(x[1],lane0_only(width));
        x[0]=lane0_only(zero);x[2]=lane0_only(one);x[4]=lane0_only(second);
    }
    void run() {
        Memory<PixelScales> memory{*this};
        this->reached(logic::pixel_scales::update(memory,*this)?Branch::rescaled:Branch::plain);
        this->ret(0x1bf26u,0u);this->publish(0x1bf26u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1be90u,0x1bf26u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return PixelScales<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_pixel_scales_detail
