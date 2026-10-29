#pragma once
#include "halo2/multiplayer.hpp"

namespace halo2 {
// Local GAME OVER presentation after losing the authoritative host. This copy
// is never a source snapshot, authority checkpoint or publishable match result.
std::optional<MultiplayerState> multiplayer_departure_game_over(
    const MultiplayerState&,std::span<const OnlineAdmission>,unsigned self,
    std::uint16_t departed_players,const SourceGameOverConstants& = {});

enum class MultiplayerDepartureReturn { unavailable,completed,window_quit };
MultiplayerDepartureReturn run_multiplayer_departure_game_over(
    const MultiplayerOptions&,const MultiplayerState&,std::uint16_t departed_players);
} // namespace halo2
