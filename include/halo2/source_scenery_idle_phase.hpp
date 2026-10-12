#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
// Literal initialized type6 idle record. The bounded phase port requires zero
// loop frame, no sibling alternatives and no frame/effect/sound events. Those
// paths must not silently lose their callbacks or random-state mutations.
struct SourceSceneryIdlePhaseDefinition {
    std::uint32_t graph=0xffffffffu, animation=0xffffffffu;
    std::int16_t frames{}, loop_frame{};
    std::int16_t previous_sibling=-1, next_sibling=-1;
    std::array<std::uint32_t,3> event_counts{};
};
struct SourceSceneryIdlePhaseState {
    float frame{}, playback_rate=1;
    std::uint8_t delay{}, status{};
    std::uint16_t playback_flags=0x3fu, crossed_events{}, next_events{};
};
// The actual constructor defaults of all 42 supplied multi-frame SCEN idles.
// clock_step is the caller's real simulation-clock bank +4, not elapsed time.
SourceSceneryIdlePhaseState source_scenery_idle_phase_initial(
    const SourceSceneryIdlePhaseDefinition &,float clock_step);
// One explicitly admitted 10A2F0 ->1CB710 ->1C68C0 update. No tick scheduling,
// PVS activation, clock producer or object lifetime is inferred by this helper.
void source_scenery_idle_phase_step(SourceSceneryIdlePhaseState &,
    const SourceSceneryIdlePhaseDefinition &,float clock_step);
} // namespace halo2
