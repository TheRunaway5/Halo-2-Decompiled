#pragma once
#include "halo2/content.hpp"

namespace halo2 {
// Original236299/TU1.5 23CC7E sound-table roles. These immutable DVD
// payloads belong only to the scoped source lobby/matchmaking presentation.
// Generic MCC menus and gameplay audio keep their own admitted assets.
bool original_matchmaking_audio_slot(unsigned slot) noexcept;
EncodedSound load_original_matchmaking_audio(unsigned slot);
} // namespace halo2
