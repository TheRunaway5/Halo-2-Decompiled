#pragma once
// Shared boundary helpers for the small queries' adapters (native_tu15_small_queries_systems.hpp).
// Each keeps its callers' original ABI: the registers each original sets
// (ECX NONE after the reference clear; EAX the state, ECX the gate byte for
// the shadow option; EAX the player; ECX the first word; ECX and EDX the
// last values read by the cluster match), the answer in AL or EAX, and the
// flags of each one's last instruction that sets them. Each access
// publishes its original instruction.
// Each owner's header (native_tu15_<name>.hpp) holds its adapter.
// HALO2_REFERENCE_<NAME>=1 restores an owner's translated body and
// HALO2_VERIFY_<NAME>=1 records its native run and replays the unmodified
// translated body against it, as in native_tu15_object_tick.hpp, for NAME
// REFERENCE_CLEAR, SHADOW_OPTION, SEAT_PLAYER, WORD_CHANGE, CLUSTER_MATCH.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/native_tu15_small_queries_systems.hpp"

namespace halo2::native_small_queries_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_small_queries;
inline bool flag(const char *name){const auto *p=std::getenv(name);return p && *p;}
struct VerifySummary : native_object_tick_detail::Tally {
    const char *name;std::atomic<std::uint64_t> calls{},hit{};
    explicit VerifySummary(const char *n):name(n){}
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"%s verify: %llu calls; %llu ordered accesses (%llu stores) identical, coverage %llx\n",
            name,(unsigned long long)calls.load(),(unsigned long long)accesses.load(),(unsigned long long)stores.load(),
            (unsigned long long)hit.load());
    }
};
// Runs one owner's adapter, as every frame-misc bridge does.
template<class Make,class State,class Reference>
bool run_owner(State &s,std::uint32_t entry,std::uint32_t exit,bool reference_mode,bool verify_mode,std::atomic<std::uint64_t> &coverage,
               VerifySummary &summary,Reference reference,Make make) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,entry,exit,cursor,reference_mode))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode,coverage,summary.calls,summary,reference,make);
    summary.hit=coverage.load();
    return true;
}

} // namespace halo2::native_small_queries_detail
