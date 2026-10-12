#pragma once
#include <array>
#include <cstddef>
#include <string_view>
namespace halo2 {
constexpr std::size_t sound_class_count = 54;
const std::array<std::string_view, sound_class_count> &sound_class_names();
float sound_linear_to_db(float);
float sound_db_to_linear(float);
// Original 221850/221980/221900: saved class envelopes, advanced by the
// audio wall clock independently of the simulation clock.
class SoundClassFades {
public:
  struct Fade {
    float target{}, current{}, remaining{};
  };
  void set(std::string_view selector, float gain, float seconds,
           bool decibels = false);
  void advance(float seconds);
  const std::array<Fade, sound_class_count> &state() const { return fades_; }
  std::array<float, sound_class_count> gains() const;

private:
  std::array<Fade, sound_class_count> fades_{};
};
} // namespace halo2
