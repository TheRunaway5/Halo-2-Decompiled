#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/source_first_person_context_tu15.hpp"
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <type_traits>

using AdoptSignature=void (halo2::SourceFirstPersonContextTu15::*)(
    const halo2::SourcePhysicsContactMemory&,const halo2::SourcePhysicsWorldRuntimeAbi&,
    unsigned,const halo2::SourceFirstPersonContextIdentityTu15&,std::uint64_t,std::uint64_t);
static_assert(std::is_same_v<decltype(&halo2::SourceFirstPersonContextTu15::adopt_local),AdoptSignature>);

namespace {
std::atomic<bool> profile_adoption{};
std::atomic<std::uint64_t> adoption_calls{},timed_calls{},adoption_ns{};
std::uint64_t thread_cpu_ns() {
    const auto saved=errno;timespec stamp{};
    const auto result=clock_gettime(CLOCK_THREAD_CPUTIME_ID,&stamp);errno=saved;
    return result?0:std::uint64_t(stamp.tv_sec)*1000000000u+std::uint64_t(stamp.tv_nsec);
}
}

// Integer-only timestamps preserve the native call's errno and FP state.
// This hook exists only in the owned diagnostic executable, never the engine.
extern "C" void native_fp_real(halo2::SourceFirstPersonContextTu15*,
    const halo2::SourcePhysicsContactMemory&,const halo2::SourcePhysicsWorldRuntimeAbi&,
    unsigned,const halo2::SourceFirstPersonContextIdentityTu15&,std::uint64_t,std::uint64_t)
    asm(HALO2_NATIVE_FP_REAL_SYMBOL);
extern "C" void native_fp_probe(halo2::SourceFirstPersonContextTu15*,
    const halo2::SourcePhysicsContactMemory&,const halo2::SourcePhysicsWorldRuntimeAbi&,
    unsigned,const halo2::SourceFirstPersonContextIdentityTu15&,std::uint64_t,std::uint64_t)
    asm(HALO2_NATIVE_FP_WRAP_SYMBOL);
void native_fp_probe(halo2::SourceFirstPersonContextTu15 *context,
    const halo2::SourcePhysicsContactMemory &source,const halo2::SourcePhysicsWorldRuntimeAbi &abi,
    unsigned view,const halo2::SourceFirstPersonContextIdentityTu15 &identity,
    std::uint64_t generation,std::uint64_t action) {
    if(!profile_adoption.load(std::memory_order_relaxed)||
       adoption_calls.fetch_add(1,std::memory_order_relaxed)<120) {
        native_fp_real(context,source,abi,view,identity,generation,action);return;
    }
    const auto begin=thread_cpu_ns();
    native_fp_real(context,source,abi,view,identity,generation,action);
    const auto end=thread_cpu_ns();
    if(begin&&end>=begin) {
        adoption_ns.fetch_add(end-begin,std::memory_order_relaxed);
        timed_calls.fetch_add(1,std::memory_order_relaxed);
    }
}

// Linked only into the disposable probe by build_native_cache_probe.py. The
// production executable/CLI is unchanged. Both roles run the complete native
// application with exactly one simulation tick per rendered frame, through
// its existing finite-observer diagnostic contract.
extern "C" int native_cache_real(const halo2::MultiplayerOptions &,
    halo2::MultiplayerRuntimeOutcome &) asm(HALO2_NATIVE_CACHE_REAL_SYMBOL);
extern "C" int native_cache_probe(const halo2::MultiplayerOptions &,
    halo2::MultiplayerRuntimeOutcome &) asm(HALO2_NATIVE_CACHE_WRAP_SYMBOL);
int native_cache_probe(const halo2::MultiplayerOptions &options,halo2::MultiplayerRuntimeOutcome &outcome) {
    if(!options.frames||options.ticks||options.host_port||!options.join.empty()||
       options.dedicated||options.replay||options.online_match)
        throw std::runtime_error("Native cache timing probe requires finite offline rendered frames");
    auto controlled=options;
    controlled.reference_tick_cadence=true;
    controlled.reference_render_fraction=0.5f;
    controlled.session_random_seed=1u;
    controlled.input_observer=[](std::uint32_t,std::span<const halo2::Controls>,const halo2::MultiplayerState&){};
    const auto *profile=std::getenv("HALO2_PROFILE_NATIVE_FP_ADOPT");
    profile_adoption.store(profile&&*profile,std::memory_order_relaxed);
    const auto status=native_cache_real(controlled,outcome);
    if(profile_adoption.load(std::memory_order_relaxed))
        std::fprintf(stderr,"Native FP adopt CPU: calls=%llu timed=%llu ns=%llu\n",
            static_cast<unsigned long long>(adoption_calls.load()),
            static_cast<unsigned long long>(timed_calls.load()),
            static_cast<unsigned long long>(adoption_ns.load()));
    return status;
}
