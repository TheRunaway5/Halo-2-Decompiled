#pragma once
// Boundary adapter for the packed colour, entered at 18BBC0
// (native_tu15_packed_color_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EAX the colour; EAX the result, ECX the red
// integer; SUB ESP,10 holds the spill and the three integers; XMM0-XMM4 as
// the original leaves them (red, the scale, green, the low end, and red
// read again); the rounding runs on the logical x87 as FLD m32 and FISTP
// m32 do (one free register is required at admission); the flags of the
// final ADD ESP,10; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_PACKED_COLOR=1 restores the translated body.
// HALO2_VERIFY_PACKED_COLOR=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
#include "rendering/effects/native_tu15_packed_color_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_packed_color_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_packed_color;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_PACKED_COLOR");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_PACKED_COLOR");return p && *p;}();
    return value;
}
enum Branch : unsigned {blue_low, blue_high, blue_kept, green_low, green_high, green_kept, red_low, red_high, red_kept,
    branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Packed colour verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr bool backend=logic::sse::available;
template<class State> struct Pack : Port<State> {
    using Access=logic::packed_color::Access;
    using Local=logic::packed_color::Local;
    using Clamp=logic::packed_color::Clamp;
    using Channel=logic::Channel;
    Channel current{};unsigned spills{},reads{};
    Pack(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t local(Local l)const{return this->gp[esp]+4u*unsigned(l);}
    void channel(Channel c){current=c;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        static constexpr std::uint32_t first_pc[]{0x18bbd3u,0x18bbf0u,0x18bc0du},second_pc[]{0x18bbdeu,0x18bbfbu,0x18bc17u},
            third_pc[]{0x18bbebu,0x18bc08u,0u},spill_pc[]{0x18bc2fu,0x18bc4bu,0x18bc5fu},x87_pc[]{0x18bc34u,0x18bc50u,0x18bc64u},
            rounded_pc[]{0x18bc37u,0x18bc53u,0x18bc67u},read_pc[]{0x18bc6bu,0x18bc6fu,0x18bc78u};
        const auto c=unsigned(current);
        using enum Access;
        switch(a) {
        case low:return 0x18bbc0u;
        case high:return 0x18bbc8u;
        case first:return first_pc[c];
        case second:return second_pc[c];
        case third:return third_pc[c];
        case blue_scale:return 0x18bc27u;
        case spill_store:return spill_pc[spills];
        case spill_x87:return x87_pc[spills];
        case rounded_store:return rounded_pc[spills++];
        case other_scale:return 0x18bc3bu;
        case rounded_read:return read_pc[reads++];
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case low:x[3]=lane0_only(v);break;
        // MOVSS XMM0; SUB ESP,10.
        case high:x[0]=lane0_only(v);gp[esp]=this->flags.minus(gp[esp],0x10u,4);break;
        case second:if(current==Channel::red)x[4]=lane0_only(v);break;
        case other_scale:x[1]=lane0_only(v);break;
        // MOV EAX; MOV ECX; SHL EAX,B; OR EAX,ECX; MOV ECX; SHL EAX,B; OR EAX,ECX.
        case rounded_read:
            if(reads==1)gp[eax]=v;
            else if(reads==2){gp[ecx]=v;gp[eax]=this->flags.logic(this->flags.shift(gp[eax],logic::channel_shift,4)|v,4);}
            else{gp[ecx]=v;gp[eax]=this->flags.logic(this->flags.shift(gp[eax],logic::channel_shift,4)|v,4);}
            break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The channel's register: XMM1 (blue), XMM2 (green), XMM0 (red, through XMM4).
    void clamped(Channel c,std::uint32_t value,Clamp kind) {
        auto &x=this->s.xmm;
        this->reached(unsigned(c)*3u+(kind==Clamp::to_low?0u:kind==Clamp::to_high?1u:2u));
        if(c==Channel::red){if(kind!=Clamp::to_high)x[0]=lane0_only(value);return;}
        x[c==Channel::blue?1:2]=lane0_only(value);
    }
    void scaled(Channel c,std::uint32_t value,std::uint32_t) {
        auto &x=this->s.xmm;
        x[c==Channel::blue?1:c==Channel::green?2:0]=lane0_only(value);
    }
    std::uint32_t rounded(std::uint32_t value) {
        auto &fp=this->s.fp;
        fp.push(physics_step_detail::ext(physics_step_detail::f(value)));
        const auto result=physics_contact_runtime_detail::integer_spill(fp.st(0),fp.status);
        fp.pop();
        return result;
    }
    void run() {
        auto &gp=this->gp;
        Memory<Pack> memory{*this};
        (void)logic::packed_color::pack(memory,*this,gp[eax]);
        gp[esp]=this->flags.plus(gp[esp],0x10u,4);this->ret(0x18bc84u,0);this->publish(0x18bc84u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.fp.count>7u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x18bbc0u,0x18bc84u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Pack<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_packed_color_detail
