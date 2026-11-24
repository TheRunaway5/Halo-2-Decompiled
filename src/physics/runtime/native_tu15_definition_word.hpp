#pragma once
// Boundary adapter for the definition word, entered at 1d4140
// (native_tu15_tiny_leaves_systems.hpp). Its callers keep the
// original ABI, which the adapter owns: ECX the record; the word or 0 in
// EAX (XOR EAX,EAX first); ECX (the definition) and EDX (the tag index
// scaled) as the original leaves them; the flags of the TEST; RET.
// HALO2_REFERENCE_TINY_LEAVES=1 restores the translated body.
// HALO2_VERIFY_TINY_LEAVES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_tiny_leaves_systems.hpp"

namespace halo2::native_definition_word_detail {
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
enum Branch : unsigned {undefined, defined, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Tiny leaves (definition word) verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct Leaf : Port<State> {
    Leaf(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    using Access=logic::definition_word::Access;
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a){case tag:return 0x1d4140u;case tags:return 0x1d4143u;case definition:return 0x1d4152u;case word:return 0x1d415cu;}
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case tag:gp[edx]=v;break;
        case tags:gp[ecx]=v;gp[edx]=fl.shift(fl.logic(gp[edx]&0xffffu,4),4u,4);break;              // AND EDX,FFFF; SHL EDX,4
        case definition:gp[ecx]=v;gp[eax]=fl.logic(0u,4);this->test32(v);break;                     // XOR EAX,EAX; TEST ECX,ECX
        case word:break;
        }
    }
    void run() {
        auto &gp=this->gp;
        Memory<Leaf> memory{*this};
        const auto result=logic::definition_word::read(memory,gp[ecx]);
        gp[eax]=result.value;                                                                        // MOVSX EAX,word or the XOR
        this->reached(result.defined?Branch::defined:Branch::undefined);
        this->ret(0x1d4160u,0);this->publish(0x1d4160u);
    }
    template<class A> void stored(A,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1d4140u,0x1d4160u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Leaf<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_definition_word_detail
