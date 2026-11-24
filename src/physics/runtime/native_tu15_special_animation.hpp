#pragma once
// Boundary adapter for the special animation query, entered at c9590
// (native_tu15_small_calls_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: the registers and stack as each hook
// below states (the original's own instructions); every register the child
// returns comes back; the flags of the last instruction that sets them. Each
// access publishes its original instruction.
// HALO2_REFERENCE_SMALL_CALLS=1 restores the translated body.
// HALO2_VERIFY_SMALL_CALLS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_small_calls_systems.hpp"

namespace halo2::native_special_animation_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_small_calls;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_SMALL_CALLS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_SMALL_CALLS");return p && *p;}();
    return value;
}
enum Branch : unsigned {answered, special, other, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Special animation verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Child> struct Query : Port<State> {
    Child &child;
    Query(State &s,std::uint32_t *c,Trace<State> *t,Child &k):Port<State>(s,c,t,{}),child(k){}
    using Access=logic::special_animation::Access;
    using Kept=logic::special_animation::Kept;
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a){case headers:return 0xc959eu;case header_datums:return 0xc95a4u;case object_address:return 0xc95b0u;
            case block_offset:return 0xc95b4u;case animation:return 0xc95bbu;}
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[edx]=v;break;
        // MOV EDX,[EDX+44]; AND ESI,FFFF; LEA ECX,[ESI+ESI*2].
        case header_datums:gp[edx]=v;gp[esi]=fl.logic(gp[esi]&0xffffu,4);gp[ecx]=gp[esi]*3u;break;
        case object_address:gp[ecx]=v;break;
        case block_offset:gp[edx]=logic::widen16(v);break;                                          // MOVSX EDX
        case animation:gp[ecx]=v;break;
        }
    }
    // MOV ECX,ESI; CALL C9550; TEST AL,AL.
    std::uint32_t ask(Kept &kept) {
        auto &gp=this->gp;
        gp[ecx]=gp[esi];this->call(0xc9595u,0xc959au,0xc9550u,child);this->test8(gp[eax]);
        kept.handle=gp[esi];
        return gp[eax]&0xffu;
    }
    void compared(std::uint32_t value,std::uint32_t code){this->flags.minus(value,code,4);}     // CMP ECX,code
    void run() {
        auto &gp=this->gp;
        using Answer=logic::special_animation::Answer;
        this->push(0xc9590u,gp[esi]);gp[esi]=gp[eax];                                              // PUSH ESI; MOV ESI,EAX
        Memory<Query> memory{*this};
        switch(logic::special_animation::test(memory,*this,gp[eax])) {
        case Answer::child:this->reached(Branch::answered);break;                                  // AL as C9550 left it
        case Answer::special:set_low_byte(gp[eax],1u);this->reached(Branch::special);break;        // MOV AL,1
        case Answer::other:this->reached(Branch::other);break;
        }
        gp[esi]=this->pop(0xc95d9u);this->ret(0xc95dau,0);this->publish(0xc95dau);
    }
    template<class A> void stored(A,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference,class Child> bool bridge(State &s,Reference reference,Child child) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xc9590u,0xc95dau,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Query<State,Child>(v,c,t,child);});
    return true;
}
} // namespace halo2::native_special_animation_detail
