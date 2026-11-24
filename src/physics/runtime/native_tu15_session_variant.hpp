#pragma once
// Boundary adapter for the session variant query, entered at 15DD10
// (native_tu15_small_queries_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: the result in AL (XOR AL,AL, MOV AL,DL) above the rest of EAX as the original leaves it; ESI saved; ECX the session and EDX as the original leaves them; the flags of the last instruction that
// sets them; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_SMALL_QUERIES=1 restores the translated body.
// HALO2_VERIFY_SMALL_QUERIES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_small_queries_systems.hpp"

namespace halo2::native_session_variant_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_small_queries;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_SMALL_QUERIES");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_SMALL_QUERIES");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_variant, other_word, mode_four, flagged, unflagged, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"session variant query verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct Query : Port<State> {
    using Access=logic::session_variant::Access;
    Query(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case session:return 0x15dd10u;case variant_index:return 0x15dd16u;
        case variant:this->push(0x15dd1cu,this->gp[esi]);return 0x15dd1du;                        // PUSH ESI
        case session_word:return 0x15dd2fu;case game_state:return 0x15dd35u;case mode:return 0x15dd3bu;case session_flag:return 0x15dd41u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case session:gp[ecx]=v;break;
        case variant_index:gp[edx]=v;break;
        // MOV ESI,[...]; XOR AL,AL; TEST ESI,ESI; (MOV EDX,1).
        case variant:gp[esi]=v;set_low_byte(gp[eax],fl.logic(0u,1));this->test32(v);
            if(v)gp[edx]=1u;else this->reached(Branch::no_variant);break;
        case session_word:fl.minus(v&0xffffu,gp[edx]&0xffffu,2);if((v&0xffffu)!=1u)this->reached(Branch::other_word);break;   // CMP word,DX
        case game_state:gp[esi]=v;break;
        case mode:fl.minus(v&0xffu,logic::session_variant::wanted_mode,1);if((v&0xffu)==4u)this->reached(Branch::mode_four);break;
        case session_flag:fl.minus(v,gp[edx],4);this->reached(v==1u?Branch::flagged:Branch::unflagged);break;   // CMP [ECX+C04],EDX
        }
    }
    void run() {
        auto &gp=this->gp;
        Memory<Query> memory{*this};
        if(logic::session_variant::match(memory))set_low_byte(gp[eax],gp[edx]);                     // MOV AL,DL
        gp[esi]=this->pop(0x15dd4bu);this->ret(0x15dd4cu,0);this->publish(0x15dd4cu);
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x15DD10u,0x15dd4cu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Query<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_session_variant_detail
