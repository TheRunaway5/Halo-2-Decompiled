#pragma once
#include <cstdint>

namespace halo2 {
// DirectSound's DSI3DL2LISTENER, as Halo 2's 21ECE0 hands an environment's
// reverb to XAudioSetEffectData (type 0xC): levels in millibels, times in
// seconds, diffusion and density in percent. The defaults are DirectSound's
// own listener (its Generic preset at 3857B8), which the game never
// replaces through SetI3DL2Listener.
struct SoundI3dl2Parameters {
  std::int32_t room = -1000, room_hf = -100;
  float room_rolloff = 0, decay_time = 1.49f, decay_hf_ratio = 0.83f;
  std::int32_t reflections = -2602;
  float reflections_delay = 0.007f;
  std::int32_t reverb = 200;
  float reverb_delay = 0.011f, diffusion = 100, density = 100, hf_reference = 5000;
  friend bool operator==(const SoundI3dl2Parameters &, const SoundI3dl2Parameters &) = default;
};
} // namespace halo2
