#include "halo2/controller_settings.hpp"
#include <SDL.h>
#include <algorithm>
#include <cmath>
namespace halo2 {
void auto_center_look(AutoCenterState &state, bool enabled, float forward,
                      float pitch_delta, bool crouching, float speed,
                      float seconds, float scale, int minimum_ticks,
                      float &pitch) {
  if (!enabled || std::abs(forward) <= .5f || std::abs(pitch_delta) >= .0001f ||
      crouching) {
    state = {};
    return;
  }
  // 187510 consumes the preceding tick's admission before186E03 updates it.
  if (state.active) {
    const float maximum = std::max(0.f, scale) * std::max(0.f, speed) *
                          seconds * std::abs(pitch) * .6366197466850281f;
    pitch += std::clamp(-pitch, -maximum, maximum);
  }
  state.remainder += seconds * 30.f;
  const unsigned ticks = unsigned(state.remainder);
  state.remainder -= ticks;
  state.moving_ticks = std::min(127u, state.moving_ticks + ticks);
  state.active = int(state.moving_ticks) > minimum_ticks;
}
ControllerSample sample_controller(SDL_GameController *pad) {
  ControllerSample r;
  if (!pad)
    return r;
  const auto axis = [&](SDL_GameControllerAxis a) {
    const float v = float(SDL_GameControllerGetAxis(pad, a)) / 32767.f;
    return std::abs(v) > .15f ? std::clamp(v, -1.f, 1.f) : 0.f;
  };
  r.hardware[0] = axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT);
  r.hardware[1] = axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
  constexpr SDL_GameControllerButton buttons[]{
      SDL_CONTROLLER_BUTTON_DPAD_UP,
      SDL_CONTROLLER_BUTTON_DPAD_DOWN,
      SDL_CONTROLLER_BUTTON_DPAD_LEFT,
      SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
      SDL_CONTROLLER_BUTTON_START,
      SDL_CONTROLLER_BUTTON_BACK,
      SDL_CONTROLLER_BUTTON_LEFTSTICK,
      SDL_CONTROLLER_BUTTON_RIGHTSTICK,
      SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
      SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
      SDL_CONTROLLER_BUTTON_A,
      SDL_CONTROLLER_BUTTON_B,
      SDL_CONTROLLER_BUTTON_X,
      SDL_CONTROLLER_BUTTON_Y};
  for (unsigned i = 0; i < std::size(buttons); ++i)
    r.hardware[2 + i] = SDL_GameControllerGetButton(pad, buttons[i]);
  const float lx = axis(SDL_CONTROLLER_AXIS_LEFTX),
              ly = axis(SDL_CONTROLLER_AXIS_LEFTY),
              rx = axis(SDL_CONTROLLER_AXIS_RIGHTX),
              ry = axis(SDL_CONTROLLER_AXIS_RIGHTY);
  r.hardware[16] = std::max(0.f, -ly);
  r.hardware[17] = std::max(0.f, ly);
  r.hardware[18] = std::max(0.f, -lx);
  r.hardware[19] = std::max(0.f, lx);
  r.hardware[20] = std::max(0.f, -ry);
  r.hardware[21] = std::max(0.f, ry);
  r.hardware[22] = std::max(0.f, -rx);
  r.hardware[23] = std::max(0.f, rx);
  return r;
}
ControllerInput controller_input(const ControllerSample &s,
                                 const ControllerSettings &settings) {
  // Literal outputs of unchanged Vista45EE72, all16 stick/button combinations.
  const auto layout = std::min(settings.buttons, 3u);
  const auto sticks = std::min(settings.sticks, 3u);
  const auto down = [&](unsigned i) { return s.hardware[i] > .2f; };
  ControllerInput r;
  const unsigned move_y = sticks == 0 || sticks == 2 ? 16 : 20,
                 move_x = sticks == 0 || sticks == 3 ? 18 : 22;
  const unsigned look_y = move_y == 16 ? 20 : 16,
                 look_x = move_x == 18 ? 22 : 18;
  r.forward = s.hardware[move_y] - s.hardware[move_y + 1];
  r.right = s.hardware[move_x + 1] - s.hardware[move_x];
  r.look_pitch = s.hardware[look_y] - s.hardware[look_y + 1];
  r.look_yaw = s.hardware[look_x + 1] - s.hardware[look_x];
  r.jump = down(12);
  r.crouch = down(8);
  r.reload = down(14);
  r.pickup = down(14);
  r.dual_wield = down(15);
  r.switch_weapon = down(15);
  r.fire = down(layout == 1 ? 0 : 1);
  r.secondary = down(layout == 1 ? 1 : 0);
  r.grenade = down(layout == 1 ? 1 : layout == 2 ? 13 : 0);
  r.melee = down(layout == 2 ? 0 : layout == 3 ? 9 : 13);
  r.zoom = down(layout == 3 ? 13 : 9);
  r.flashlight = down(10);
  r.switch_grenade = down(11);
  r.accept = down(12);
  r.cancel = down(13);
  return r;
}
void controller_look(const ControllerInput &input,
                     const ControllerSettings &settings, float seconds,
                     float &yaw, float &pitch) {
  // 6080DC ->461D0B: literal sensitivity tables in degrees, then7B9F60 radians.
  const auto n = std::clamp(settings.sensitivity, 1u, 10u) - 1;
  constexpr float radians = .01745329238474369f;
  yaw -= input.look_yaw * float(80 + 20 * n) * radians * seconds;
  pitch = std::clamp(pitch + input.look_pitch * (settings.invert ? -1.f : 1.f) *
                                 float(40 + 10 * n) * radians * seconds,
                     -1.4922565f, 1.4922565f);
}
} // namespace halo2
