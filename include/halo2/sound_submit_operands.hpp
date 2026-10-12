#pragma once
#include "halo2/sound_submit.hpp"
#include <array>

namespace halo2 {
// Original immutable table identity and complete bytes. The original callback
// bodies and actual effect/controller/voice ownership remain separate.
struct SoundSubmitImmutableOperands {
  SoundSubmitConstants constants;
  std::array<std::byte,32> callback_table;
};
} // namespace halo2
