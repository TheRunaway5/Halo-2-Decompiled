#pragma once
// Boundary adapter for the animation frame clamp, entered at 1CE810
// (native_tu15_animation_clamp_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EAX the state and the
// frame at [ESP+4], RET 4; ESI saved (the state); 1E3150 gets EDX the
// graph and the animation pushed, and pops it; 1CEC60 gets the registers
// as the original leaves them; every register a child returns comes back;
// XMM0-XMM2 as the original leaves them (upper lanes zero); the flags of
// the last instruction that sets them; each access publishes its original
// instruction.
// HALO2_REFERENCE_ANIMATION_CLAMP=1 restores the translated body.
// HALO2_VERIFY_ANIMATION_CLAMP=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_animation_clamp_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_animation_clamp_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_animation_clamp;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ANIMATION_CLAMP");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ANIMATION_CLAMP");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_animation, animated, below_zero, past_end, within, stopped, held, reaches_end, short_of_end,
    branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Animation clamp verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge clamp_edges[]{
    {0x1ce815u,0x1ce83cu,no_animation},{0x1ce815u,0x1ce81cu,animated},
    {0x1ce867u,0x1ce8b8u,stopped},{0x1ce86eu,0x1ce8b8u,held},
};
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr bool backend=logic::sse::available && logic::scalar::available;
template<class State,class Animation,class Update> struct Clamp : Port<State> {
    using Access=logic::animation_clamp::Access;
    using Local=logic::animation_clamp::Local;
    using Kept=logic::animation_clamp::Kept;
    using Placed=logic::animation_clamp::Placed;
    Animation &animation_child;Update &update_child;
    // The bytes pushed below the return address (ESI's save first).
    std::uint32_t depth{4u};bool reaches{},marked_call{};
    Clamp(State &s,std::uint32_t *c,Trace<State> *t,Animation &a,Update &u)
        :Port<State>(s,c,t,clamp_edges),animation_child(a),update_child(u){}
    std::uint32_t local(Local)const{return this->gp[esp]+depth+4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case index:return 0x1ce815u;
        case graph:return 0x1ce81cu;
        case instances:return 0x1ce81eu;
        // AND EAX,FFFF; SHL EAX,4.
        case graph_address:gp[eax]=this->flags.shift(this->flags.logic(gp[eax]&0xffffu,4),logic::instance_shift,4);return 0x1ce82cu;
        case animation:return 0x1ce830u;
        case frame_argument:this->s.xmm[0]=_mm_setzero_ps();return 0x1ce83cu;                    // XORPS XMM0
        case margin:return 0x1ce845u;
        case count:return 0x1ce84fu;
        case playing:return 0x1ce863u;
        case frame_store:return 0x1ce867u;
        case held:return 0x1ce86eu;
        case game_time:return 0x1ce876u;
        case seconds:return 0x1ce87cu;
        case rate:return 0x1ce881u;
        case next_count:return 0x1ce886u;
        case rate_scale:return 0x1ce88au;
        // OR CL,2 or AND CL,FD.
        case held_store:
            set_low_byte(gp[ecx],reaches?this->flags.logic((gp[ecx]|logic::state::reaches_end)&0xffu,1):
                                         this->flags.logic(gp[ecx]&0xfdu,1));
            return reaches?0x1ce8a6u:0x1ce8b5u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case index:this->flags.minus(v&0xffffu,0xffffu,2);break;
        case graph:case animation:gp[eax]=v;if(a==animation){this->push(0x1ce833u,v);depth+=4u;}break;   // PUSH EAX
        case instances:gp[ecx]=v;break;
        case graph_address:case game_time:gp[edx]=v;break;
        case frame_argument:x[1]=lane0_only(v);this->flags.scalar_compare(x[0],x[1],false);break;   // COMISS XMM0,XMM1
        case margin:x[2]=lane0_only(v);break;
        case count:gp[ecx]=logic::widen16(v);break;
        case playing:this->test8(v&1u);break;
        case held:set_low_byte(gp[ecx],v);this->test8(v&logic::state::held_bits);break;          // TEST CL,9
        case seconds:x[1]=lane0_only(v);break;
        case next_count:gp[eax]=logic::widen16(v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    std::uint32_t animation(Kept &kept) {
        this->call(0x1ce834u,0x1ce839u,0x1e3150u,animation_child);depth-=4u;
        kept={this->gp[esi]};this->reached(Branch::animated);
        return this->gp[eax];
    }
    // CVTSI2SS XMM0,ECX; SUBSS XMM0,XMM2; COMISS XMM1,XMM0; (MOVAPS XMM0,XMM1).
    void placed(std::uint32_t,std::uint32_t last,Placed kind) {
        auto &x=this->s.xmm;
        if(kind==Placed::below_zero){this->reached(Branch::below_zero);return;}
        x[0]=lane0_only(last);this->flags.scalar_compare(x[1],x[0],false);
        if(kind==Placed::within){x[0]=x[1];this->reached(Branch::within);}
        else this->reached(Branch::past_end);
    }
    // ADDSS ... CVTSI2SS XMM0,EAX; SUBSS XMM0,XMM2; COMISS XMM1,XMM0.
    void advanced(std::uint32_t next,std::uint32_t end) {
        auto &x=this->s.xmm;
        x[1]=lane0_only(next);x[0]=lane0_only(end);this->flags.scalar_compare(x[1],x[0],false);
        reaches=!logic::sse::below(next,end);this->reached(reaches?Branch::reaches_end:Branch::short_of_end);
    }
    void update(Kept &kept,bool marked) {
        marked_call=marked;
        if(marked)this->call(0x1ce8a9u,0x1ce8aeu,0x1cec60u,update_child);
        else this->call(0x1ce8b8u,0x1ce8bdu,0x1cec60u,update_child);
        kept={this->gp[esi]};
    }
    void run() {
        auto &gp=this->gp;
        this->push(0x1ce810u,gp[esi]);gp[esi]=gp[eax];gp[eax]=this->flags.logic(0u,4);          // PUSH ESI; MOV ESI,EAX; XOR EAX,EAX
        Memory<Clamp> memory{*this};
        logic::animation_clamp::place(memory,*this,Kept{gp[esi]});
        const auto pc=marked_call?0x1ce8aeu:0x1ce8bdu;
        gp[esi]=this->pop(pc);this->ret(pc+1u,4u);this->publish(pc+1u);
    }
};
template<class State,class Reference,class Animation,class Update>
bool bridge(State &s,Reference reference,Animation animation,Update update) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1ce810u,0x1ce8beu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Clamp<State,Animation,Update>(v,c,t,animation,update);});
    return true;
}
} // namespace halo2::native_animation_clamp_detail
