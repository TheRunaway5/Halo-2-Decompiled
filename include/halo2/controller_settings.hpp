#pragma once
#include <array>
#include <cstdint>
typedef struct _SDL_GameController SDL_GameController;
namespace halo2 {
struct ControllerSettings {
  unsigned sticks{}, buttons{}, sensitivity = 3;
  bool invert{}, auto_center{}, vibration = true;
  bool operator==(const ControllerSettings &) const = default;
};
// Source hardware ordinals (45EE72), supplied independently of SDL ordinals.
struct ControllerSample {
  std::array<float, 24> hardware{};
};
struct ControllerInput {
  float forward{}, right{}, look_yaw{}, look_pitch{};
  bool jump{}, crouch{}, fire{}, secondary{}, grenade{}, switch_weapon{},
      reload{}, pickup{}, dual_wield{}, melee{}, zoom{}, flashlight{},
      switch_grenade{}, accept{}, cancel{};
};
struct AutoCenterState {
  unsigned moving_ticks{};
  bool active{};
  float remainder{};
};
// 186E03 admission and1878D0 leveling. Native velocity is units/second.
void auto_center_look(AutoCenterState &, bool enabled, float forward,
                      float pitch_delta, bool crouching, float speed,
                      float seconds, float scale, int minimum_ticks,
                      float &pitch);
ControllerSample sample_controller(SDL_GameController *);
ControllerInput controller_input(const ControllerSample &,
                                 const ControllerSettings &);
void controller_look(const ControllerInput &, const ControllerSettings &,
                     float seconds, float &yaw, float &pitch,bool constrain_pitch=true);
} // namespace halo2
