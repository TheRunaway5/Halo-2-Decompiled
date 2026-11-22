#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {

// Rates one retained 40-byte vehicle movement candidate. The original RET
// 0xC returns through one of the solver's two candidate score sites.
std::uint32_t native_campaign_ai_vehicle_movement_score_tu15(
    SourcePhysicsContactMemory &, SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver &);

namespace vehicle_candidates_tu15 {

// The source arena reserves eight 40-byte rows and their 16-bit count.
inline constexpr std::uint32_t rows = 0x5112d8u;
inline constexpr std::uint32_t stride = 0x28u;
inline constexpr std::uint32_t count = 0x511418u;
inline constexpr std::uint32_t bytes = count - rows + 2u;

// Narrow ordinary-memory admission of the active geometric branch reached
// from 2A4327. Ineligible calls retain the preceding World dispatch policy.
bool admits(const SourcePhysicsContactMemory &, const SourcePhysicsContactRuntimeCall &,
            const SourcePhysicsWorldRuntimeAbi &);

// Runs the admitted 2A2AA0 source frame and publishes its actual RET 0x1C.
std::uint32_t native_campaign_ai_vehicle_movement_candidates_tu15(
    SourcePhysicsContactMemory &, SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsWorldRuntimeServices::RequiredOwner &, const SourcePhysicsContactRecordObserver &);

} // namespace vehicle_candidates_tu15

namespace vehicle_steering_tu15 {

// The active forward steering branch reached at 2A440B. Verify its real
// candidate slot, source frame and single-sample range before any writes.
bool admits(const SourcePhysicsContactMemory &, const SourcePhysicsContactRuntimeCall &,
            const SourcePhysicsWorldRuntimeAbi &);

// Return the source continuation of the admitted 2A3170 RET 0x1C.
std::uint32_t run(SourcePhysicsContactMemory &, SourcePhysicsWorldRuntimeAbi &,
                  const SourcePhysicsWorldRuntimeServices::RequiredOwner &,
                  const SourcePhysicsContactRecordObserver &);

} // namespace vehicle_steering_tu15

namespace vehicle_movement_tu15 {

// The actual 1FBD03 mode-5 path supplies one actor row and its true aliased
// control fields. Check the caller before any source publication.
bool admits(const SourcePhysicsContactMemory &, const SourcePhysicsContactRuntimeCall &,
            const SourcePhysicsWorldRuntimeAbi &);
std::uint32_t run(SourcePhysicsContactMemory &, SourcePhysicsWorldRuntimeAbi &,
                  const SourcePhysicsWorldRuntimeServices::RequiredOwner &,
                  const SourcePhysicsContactRecordObserver &);

} // namespace vehicle_movement_tu15
} // namespace halo2
