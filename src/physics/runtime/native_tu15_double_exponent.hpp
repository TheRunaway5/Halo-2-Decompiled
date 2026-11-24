#pragma once
// Boundary adapter for the double exponent, entered at 33210c
// (native_tu15_tiny_leaves_systems.hpp). Its callers keep the
// original ABI, which the adapter owns: the double on the stack (its high
// word at [ESP+8]); the result in EAX; the flags of the CMP; RET.
// HALO2_REFERENCE_TINY_LEAVES=1 restores the translated body.
// HALO2_VERIFY_TINY_LEAVES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_tiny_leaves_systems.hpp"

namespace halo2::native_double_exponent_detail {
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
enum Branch : unsigned {finite, special, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Tiny leaves (double exponent) verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct Leaf : Port<State> {
    Leaf(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    using Access=logic::double_exponent::Access;
    std::uint32_t high_slot()const{return this->gp[esp]+8u;}
    std::uint32_t instruction(Access a,std::uint32_t){return a==Access::high?0x33210cu:0x33211du;}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        // MOV EAX,[ESP+8]; AND EAX,7FF00000; CMP EAX,7FF00000 / MOV EAX,[ESP+8].
        if(a==Access::high){gp[eax]=fl.logic(v&logic::double_exponent::exponent,4);fl.minus(gp[eax],logic::double_exponent::exponent,4);}
        else gp[eax]=v;
    }
    void run() {
        auto &gp=this->gp;
        Memory<Leaf> memory{*this};
        const auto result=logic::double_exponent::bits(memory,*this);
        gp[eax]=result.value;
        this->reached(result.special?Branch::special:Branch::finite);
        const auto exit=result.special?0x332121u:0x33211cu;
        this->ret(exit,0);this->publish(exit);
    }
    template<class A> void stored(A,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x33210cu,0x332121u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Leaf<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_double_exponent_detail
