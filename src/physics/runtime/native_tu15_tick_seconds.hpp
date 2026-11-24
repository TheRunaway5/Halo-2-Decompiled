#pragma once
// Boundary adapter for the tick seconds, entered at e32c0
// (native_tu15_tiny_leaves_systems.hpp). Its callers keep the
// original ABI, which the adapter owns: EAX the object handle, the out
// pointer on the stack (RET 4); the result in AL (XOR AL,AL; MOV AL,1)
// above the rest of EAX (the game time record once read); ECX (the count
// widened) and EDX (the out pointer) as the original leaves them; XMM0 the
// value, XMM1 the count converted over its upper lanes; the flags of the
// TEST; RET 4.
// HALO2_REFERENCE_TINY_LEAVES=1 restores the translated body.
// HALO2_VERIFY_TINY_LEAVES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_tiny_leaves_systems.hpp"

namespace halo2::native_tick_seconds_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_tiny_leaves;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_TINY_LEAVES");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_TINY_LEAVES");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_ticks, stored, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Tiny leaves (tick seconds) verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct Leaf : Port<State> {
    Leaf(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    using Access=logic::tick_seconds::Access;
    std::uint32_t out_slot()const{return this->gp[esp]+4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a){case headers:return 0xe32c0u;case header_datums:return 0xe32c6u;case object_address:return 0xe32d1u;
            case ticks:return 0xe32d5u;case game_time:return 0xe32e1u;case seconds_per_tick:return 0xe32e6u;case scale:return 0xe32ebu;
            case out:return 0xe32f3u;case store:return 0xe3302u;}
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;break;
        case header_datums:gp[edx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;break;                 // AND EAX,FFFF; LEA
        case object_address:gp[ecx]=v;break;
        case ticks:set_low_byte(gp[ecx],v);set_low_byte(gp[eax],fl.logic(0u,1));this->test8(v);break;   // MOV CL; XOR AL,AL; TEST CL,CL
        case game_time:gp[eax]=v;break;
        case out:gp[edx]=v;break;
        default:break;
        }
    }
    void run() {
        auto &gp=this->gp;
        Memory<Leaf> memory{*this};
        const auto result=logic::tick_seconds::store(memory,*this,gp[eax]);
        if(result.stored) {
            // MOVSX ECX,CL; CVTSI2SS XMM1,ECX; MULSS XMM0,XMM1; MOV AL,1.
            auto &x=this->s.xmm;
            gp[ecx]=result.count;x[1]=_mm_cvtsi32_ss(x[1],int(result.count));x[0]=logic::sse::scalar(result.value);
            set_low_byte(gp[eax],1u);this->reached(Branch::stored);
        } else this->reached(Branch::no_ticks);
        this->ret(0xe3308u,4u);this->publish(0xe3308u);
    }
    template<class A> void stored(A,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xe32c0u,0xe3308u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Leaf<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_tick_seconds_detail
