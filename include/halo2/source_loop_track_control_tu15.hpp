#pragma once
#include <array>
#include <cstdint>

namespace halo2 {
// Original221C3C..221CA4 gain-control slew. Inputs belong to the actual
// track, caller scale, playback+0C rate and audio elapsed-time owner.
float source_loop_track_gain_control_tu15(float current,float target,float rate,float elapsed);
struct SourceLoopPitchLayerTu15 {bool active;float gain_db;};
// Complete220990 attenuation and221EAA..221EE6 layer-domain admission.
// The five signed authored interval words retain their original order.
// This selects no voice, permutation, packet or device admission result.
SourceLoopPitchLayerTu15 source_loop_pitch_layer_tu15(
    const std::array<std::int16_t,5>& interval,float pitch,bool layered);
}
