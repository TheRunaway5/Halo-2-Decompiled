#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace halo2 {
// Complete retail39A80 over exclusively owned banks. The caller supplies the
// already published resource count and auxiliary random stream; pointer words
// remain source tokens. This does not acquire resources or choose RNG history.
bool source_weather_initialize_particles(std::span<std::byte> definition,
    std::int32_t resource_particle_count, std::span<std::byte> render,
    std::span<std::byte> state, std::uint32_t &auxiliary);
// Complete retail36F50 over a 0x3A8 background-plate bank. Initializes all four
// per-view records; caller owns the definition and supplies its auxiliary RNG.
void source_weather_initialize_plate(std::span<std::byte> definition,
    std::uint32_t &auxiliary);
}
