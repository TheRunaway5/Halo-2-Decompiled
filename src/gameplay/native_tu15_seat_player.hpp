#pragma once
// Boundary adapter for the seat player, entered at 150990
// (native_tu15_small_queries_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EAX the player, the flags of CMP ECX,-1. Each access publishes
// its original instruction.
// HALO2_REFERENCE_SEAT_PLAYER=1 restores the translated body.
// HALO2_VERIFY_SEAT_PLAYER=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_small_queries_detail {
namespace seat_player {
using Access=logic::seat_player::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_SEAT_PLAYER");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_SEAT_PLAYER");return v;}
enum Branch : unsigned {no_seat, seated, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Seat player"};return v;}
template<class State> struct Adapter : Port<State> {
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t){return a==Access::seats?0x150998u:0x15099du;}
    void loaded(Access,std::uint32_t,std::uint32_t v){this->gp[eax]=v;}
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        gp[eax]=this->flags.logic(0xffffffffu,4);this->compare32(gp[ecx],logic::none);            // OR EAX,-1; CMP ECX,-1
        Memory<Adapter> memory{*this};
        gp[eax]=logic::seat_player::of(memory,*this,gp[ecx]);
        this->reached(gp[ecx]==logic::none?Branch::no_seat:Branch::seated);
        this->ret(0x1509a1u,0u);this->publish(0x1509a1u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x150990u,0x1509a1u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace seat_player
} // namespace halo2::native_small_queries_detail
