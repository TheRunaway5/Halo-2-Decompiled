#pragma once
#include "halo2/native_tu15_heap_bootstrap.hpp"
#include <optional>
#include <vector>

namespace halo2 {
// Typed cold CRT/RNG fields and platform function identities. No image or
// saved heap/thread/RNG contents are loaded by the native path.
void mount_native_tu15_rng_startup_data(SourceRuntimeMemory&);
struct NativeTu15RngClock {
    std::function<std::uint64_t()> utc_filetime;
    std::function<std::uint32_t()> monotonic_milliseconds;
};
NativeTu15RngClock native_tu15_rng_clock();
// Native inputs for the complete source148D40 initializer. The original
// time conversion, CRT allocation/rand, CRC and seed XOR remain source code.
// This owner is for the ordinary main executor; interrupt/TLS expansion and
// other thread startup paths require their own retained owners.
class NativeTu15RngStartup final {
public:
    explicit NativeTu15RngStartup(NativeTu15HeapBootstrap&,NativeTu15RngClock = native_tu15_rng_clock());
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsContactRuntimeAbi&);
    void instruction(std::uint32_t);
    std::optional<std::uint64_t> utc_filetime_input()const noexcept{return utc_;}
    std::span<const std::uint64_t> utc_filetime_inputs()const noexcept{return utc_inputs_;}
    std::optional<std::uint32_t> monotonic_millisecond_input()const noexcept{return tick_;}
    std::optional<std::uint32_t> rng_seed_millisecond_input()const noexcept{return seed_tick_;}
    std::span<const std::uint32_t> monotonic_millisecond_inputs()const noexcept{return ticks_;}
    std::uint32_t thread_token()const noexcept{return thread_token_;}
private:
    SourceRuntimeMemory& memory_;
    std::thread::id thread_;
    std::uint32_t thread_token_;
    NativeTu15RngClock clock_;
    std::optional<std::uint64_t> utc_;
    std::vector<std::uint64_t> utc_inputs_;
    std::optional<std::uint32_t> tick_;
    std::optional<std::uint32_t> seed_tick_;
    std::vector<std::uint32_t> ticks_;
};
}
