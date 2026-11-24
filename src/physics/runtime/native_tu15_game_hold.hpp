#pragma once
// Boundary adapter for the game hold query, entered at 13F620
// (native_tu15_small_queries_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: the result in AL (XOR AL,AL, then the hold byte) above the rest of EAX as the original leaves it; ECX the game globals; the flags of the last instruction that
// sets them; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_SMALL_QUERIES=1 restores the translated body.
// HALO2_VERIFY_SMALL_QUERIES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_small_queries_systems.hpp"

namespace halo2::native_game_hold_detail {
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
enum Branch : unsigned {no_game, game_read, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"game hold query verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct Query : Port<State> {
    using Access=logic::game_hold::Access;
    Query(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t){return a==Access::game?0x13f620u:0x13f62cu;}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        // MOV ECX,[51DF50]; XOR AL,AL; TEST ECX,ECX / MOV AL,[ECX+5].
        if(a==Access::game){gp[ecx]=v;set_low_byte(gp[eax],this->flags.logic(0u,1));this->test32(v);this->reached(v?Branch::game_read:Branch::no_game);}
        else set_low_byte(gp[eax],v);
    }
    void run() {
        Memory<Query> memory{*this};
        (void)logic::game_hold::held(memory);
        this->ret(0x13f62fu,0);this->publish(0x13f62fu);
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x13F620u,0x13f62fu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Query<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_game_hold_detail
