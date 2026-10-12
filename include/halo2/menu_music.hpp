#pragma once
#include "halo2/audio.hpp"
#include "halo2/sound_variation.hpp"

namespace halo2 {
struct LoopingSoundSelection {
  std::uint32_t used{};
  std::int16_t previous = -1;
};
// Retail 219110 and its caller's 21CBE0 previous-permutation store. The caller
// supplies the secondary RNG; this does not reconstruct unrelated consumers.
std::int16_t source_looping_sound_permutation(
    std::span<const std::uint16_t> skip_chances, LoopingSoundSelection &,
    std::uint32_t &secondary_seed, bool mirror = false, bool *all_used = nullptr,
    std::vector<SoundRngStore> *stores = nullptr);
// Retail 146240/1462B0 and the executable's CRT rand (321F75), respectively.
std::uint32_t source_secondary_random_seed(std::uint32_t unix_seconds,
                                         std::uint32_t ticks,
                                         std::uint16_t crt_draw);
std::uint16_t source_crt_random(std::uint32_t &state);

class MenuMusic {
public:
  MenuMusic(const Cache &, std::uint32_t datum, std::uint32_t secondary_seed);
  std::vector<AudioEngine::VoiceId> play(AudioEngine &);
  void stop(AudioEngine &, std::span<const AudioEngine::VoiceId>);
  // Complete predecoded bank; the callback performs no decoding or file I/O.
  const std::vector<std::shared_ptr<const PcmSound>> &loops(unsigned track) const;
  std::uint32_t seed() const;
private:
  struct State;
  std::shared_ptr<State> state_;
};
} // namespace halo2
