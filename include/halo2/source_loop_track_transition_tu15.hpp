#pragma once
#include <array>
#include <cstdint>
namespace halo2 {
struct SourceLoopTrackTransitionTu15 {
    std::uint32_t sound=0xffffffffu,impulse=0xffffffffu;
    std::uint8_t pending{};
    bool traversed{};
};
// Complete221122..22121B/22123B pre-creation transition decision. Sounds are
// the seven authored start/loop/end/alternate-transition references in source
// phase order. The current/pending phases and alternate command are inputs.
// This computes no voice, playback completion, controller or device result.
SourceLoopTrackTransitionTu15 source_loop_track_transition_tu15(
    const std::array<std::uint32_t,7>& sounds,std::uint32_t track_flags,
    std::uint8_t current,std::uint8_t pending,bool alternate);
}
