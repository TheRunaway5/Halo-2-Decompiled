#pragma once
#include "halo2/multiplayer_audio.hpp"
namespace halo2 {
using SourceLoopListenerTransformTu15=std::array<std::uint32_t,13>;
struct SourceLoopCombinedListenerSelectionTu15 {
    std::uint32_t listener=0xffffffffu,distance{};
    std::array<std::uint32_t,3> local_point{};
};
//129F10's listener matrix: scale1, literal forward, up x forward, literal up,
//position. Neither normalization nor a replacement orientation is supplied.
SourceLoopListenerTransformTu15 source_loop_listener_transform_tu15(
    const SourceObjectAudioViewTu15&,const SoundDriverFloatingPointContext&);
// Complete145200 numerical inverse-point operation, including zero/unity
//scale branches. This owns no listener selection or device state.
std::array<std::uint32_t,3> source_loop_listener_local_point_tu15(
    const SourceLoopListenerTransformTu15&,const std::array<std::uint32_t,3>&,
    const SoundDriverFloatingPointContext&);
// Actual12A810 selection, then12D6F0 and145200 for spatial source mode1.
// Present views are the submitted local listeners/players. A selected view
// must carry its real orientation; no orientation is guessed from position.
SourceLoopCombinedListenerSelectionTu15 source_loop_combined_listener_tu15(
    std::span<const std::byte,68>,std::uint32_t maximum_distance_bits,
    const MultiplayerAudioBsp&,const SourceObjectAudioViewsTu15&,
    const SoundDriverFloatingPointContext&);
}
