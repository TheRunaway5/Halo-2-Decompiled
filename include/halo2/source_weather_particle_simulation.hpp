#pragma once
#include "halo2/source_weather_exclusion.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Caller supplies a source ISA answer for the actual squared-speed word. No
// native intrinsic, square root or recorded answer is chosen by this helper.
// source_weather_rsqrt_xemu_0_8_136 is a separate, explicitly selected
// reference-emulator provider; it makes no physical Xbox precision claim.
using SourceWeatherParticleRsqrt = std::function<std::uint32_t(std::size_t particle,
    std::uint32_t squared_speed)>;
struct SourceWeatherParticleSimulationInput {
    std::span<const std::byte> view; // Complete485600..4856F3 bank.
    std::span<const std::byte> weather; // Actual owningweat root through+AC.
    std::span<const SourceWeatherExclusionVolume> exclusions;
    std::int16_t view_count{}; // Actual4E8C20+8 before source1..4 clamp.
    float elapsed{}, intensity{}, portal_distance{}; //509418,4670FC,XMM0.
    std::uint16_t x87_control{}; // This bounded implementation admits023F.
    std::uint32_t mxcsr{}; // Round-to-nearest, noDAZ/FTZ, masked exceptions.
    SourceWeatherParticleRsqrt rsqrt;
};
struct SourceWeatherParticleStepResult {
    bool source_AL{};
    std::size_t visited{}, rsqrt_calls{}, auxiliary_draws{};
};
// Literal384E5..384FD pre-gate elapsed store. Caller invokes this once before
// actual12DE70, including not-ready calls; no readiness result is manufactured.
void source_weather_advance_particle_elapsed(std::span<std::byte> system,float elapsed);
// Admitted-ready branch of38450 over supplied140-byte system and
//20-byte render/32-byte state rows. The caller has already executed the actual
//12DE70 service; resource readiness, row-age refresh and publication are not
// inferred here. The system already carries the explicit pre-gate elapsed
// store and any actual descriptor changes from that service. Pending/not-ready
// admission and exceptional floating-point state remain outside this API.
// SourceWeatherParticleResource::request now provides the separate native
// synchronous publication/age owner over genuinely loaded cache allocations.
// Its ready result does not certify original asynchronous cache chronology.
// Mutable row banks must not overlap one another or the immutable input banks.
// Rejected input/ISA answers leave row banks and auxiliary unchanged;
// an earlier successful elapsed store remains a separate completed phase.
// external callback side effects cannot be rolled back by this module.
// Native execution is admitted only with observed x86 SSE nearest, masked
// exceptions and noDAZ/noFTZ. Controls are rechecked after every ISA callback;
// this helper never sets the caller's floating-point environment.
// Build with contraction disabled and nofastmath; native float/double
// evaluation must have no excess precision (FLT_EVAL_METHOD0).
SourceWeatherParticleStepResult source_weather_step_ready_particles(
    std::span<const std::byte> system, std::span<std::byte> render,
    std::span<std::byte> state, const SourceWeatherParticleSimulationInput &,
    std::uint32_t &auxiliary);
} // namespace halo2
