#include "halo2/sound_classes.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <cmath>
namespace halo2 {
const std::array<std::string_view, sound_class_count> &sound_class_names() {
  static constexpr std::array<std::string_view, sound_class_count> names{
      "projectile_impact",
      "projectile_detonation",
      "projectile_flyby",
      "",
      "weapon_fire",
      "weapon_ready",
      "weapon_reload",
      "weapon_empty",
      "weapon_charge",
      "weapon_overheat",
      "weapon_idle",
      "weapon_melee",
      "weapon_animation",
      "object_impacts",
      "particle_impacts",
      "",
      "",
      "",
      "unit_footsteps",
      "unit_dialog",
      "unit_animation",
      "",
      "vehicle_collision",
      "vehicle_engine",
      "vehicle_animation",
      "",
      "device_door",
      "",
      "device_machinery",
      "device_stationary",
      "",
      "",
      "music",
      "ambient_nature",
      "ambient_machinery",
      "",
      "huge_ass",
      "object_looping",
      "cinematic_music",
      "",
      "",
      "",
      "",
      "",
      "",
      "cortana_mission",
      "cortana_cinematic",
      "mission_dialog",
      "cinematic_dialog",
      "scripted_cinematic_foley",
      "game_event",
      "ui",
      "test",
      "multilingual_test"};
  return names;
}
float sound_linear_to_db(float gain) {
  if (!std::isfinite(gain))
    throw FormatError("nonfinite sound gain");
  return gain > 0
             ? std::clamp(float(20.0 * std::log10(double(std::min(gain, 1.f)))),
                          -64.f, 0.f)
             : -64.f;
}
float sound_db_to_linear(float db) {
  if (!std::isfinite(db))
    throw FormatError("nonfinite sound decibels");
  // Float constants are promoted by the original x87 expression.
  return float(std::exp(double(std::clamp(db, -64.f, 0.f)) * double(.05f) *
                        double(2.3025851f)));
}
void SoundClassFades::set(std::string_view selector, float gain, float seconds,
                          bool db) {
  if (!std::isfinite(gain) || !std::isfinite(seconds))
    throw FormatError("invalid class fade");
  const auto target =
      db ? std::clamp(gain, -64.f, 0.f) : sound_linear_to_db(gain);
  const auto &names = sound_class_names();
  for (std::size_t i = 0; i < names.size(); ++i)
    if (!names[i].empty() &&
        names[i].find(selector) != std::string_view::npos) {
      auto &f = fades_[i];
      f.target = target;
      f.remaining = std::max(seconds, 0.f);
      if (seconds == 0)
        f.current = target;
    }
}
void SoundClassFades::advance(float seconds) {
  if (!std::isfinite(seconds))
    throw FormatError("invalid audio elapsed time");
  if (seconds <= 0)
    return;
  for (auto &f : fades_)
    if (f.remaining > seconds) {
      f.current = (f.target - f.current) * (seconds / f.remaining) + f.current;
      f.remaining -= seconds;
    } else {
      f.current = f.target;
      f.remaining = 0;
    }
}
std::array<float, sound_class_count> SoundClassFades::gains() const {
  std::array<float, sound_class_count> out{};
  for (std::size_t i = 0; i < out.size(); ++i)
    out[i] = fades_[i].current;
  return out;
}
} // namespace halo2
