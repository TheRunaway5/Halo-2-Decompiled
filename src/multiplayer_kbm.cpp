#include "halo2/multiplayer_kbm.hpp"

namespace halo2 {
void MultiplayerKbm::release() {
  keys_.fill(false);
  mouse_.fill(false);
  pressed_.fill(false);
  pickup_hold_={};dual_hold_={};
}
void MultiplayerKbm::press(int binding) {
  for (unsigned i = 0; i < pc_action_count; ++i)
    if (settings_.bindings[i] == binding || settings_.secondary[i] == binding)
      pressed_[i] = true;
}
bool MultiplayerKbm::held(PcAction action) const {
  const auto test = [&](int binding) {
    if (binding > 0 && binding < SDL_NUM_SCANCODES)
      return keys_[binding];
    if (binding > pc_mouse_button(0) && binding <= pc_mouse_button(5))
      return mouse_[binding - 512];
    return false;
  };
  return test(settings_.bindings[unsigned(action)]) ||
         test(settings_.secondary[unsigned(action)]);
}
void MultiplayerKbm::handle_event(const SDL_Event &event) {
  if (event.type == SDL_WINDOWEVENT && own_window(event.window.windowID)) {
    if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
      release();
      focused_ = false;
    } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
      focused_ = true;
    return;
  }
  if (event.type == SDL_APP_WILLENTERBACKGROUND ||
      event.type == SDL_APP_DIDENTERBACKGROUND || event.type == SDL_QUIT) {
    release();
    focused_ = false;
    return;
  }
  if (!focused_)
    return;
  if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
    if (!own_window(event.key.windowID))
      return;
    const auto key = event.key.keysym.scancode != SDL_SCANCODE_UNKNOWN
                         ? event.key.keysym.scancode
                         : SDL_GetScancodeFromKey(event.key.keysym.sym);
    if (key <= SDL_SCANCODE_UNKNOWN || key >= SDL_NUM_SCANCODES)
      return;
    const bool down = event.type == SDL_KEYDOWN;
    if (down && event.key.repeat)
      return;
    if (down && !keys_[key]) {
      press(key);
    }
    keys_[key] = down;
    const auto now=event.key.timestamp?event.key.timestamp:SDL_GetTicks();
    pickup_hold_.set(held(PcAction::Pickup),now);
    dual_hold_.set(held(PcAction::DualPickup),now);
  } else if (event.type == SDL_MOUSEBUTTONDOWN ||
             event.type == SDL_MOUSEBUTTONUP) {
    if (!own_window(event.button.windowID) ||
        event.button.button >= mouse_.size())
      return;
    const bool down = event.type == SDL_MOUSEBUTTONDOWN;
    if (down && !mouse_[event.button.button])
      press(pc_mouse_button(event.button.button));
    mouse_[event.button.button] = down;
    const auto now=event.button.timestamp?event.button.timestamp:SDL_GetTicks();
    pickup_hold_.set(held(PcAction::Pickup),now);dual_hold_.set(held(PcAction::DualPickup),now);
  } else if (event.type == SDL_MOUSEWHEEL && own_window(event.wheel.windowID)) {
    bool moved = event.wheel.x != 0 || event.wheel.y != 0;
#if SDL_VERSION_ATLEAST(2, 0, 18)
    moved |= event.wheel.preciseX != 0 || event.wheel.preciseY != 0;
#endif
    if (moved)
      press(pc_wheel_up);
  }
}
MultiplayerKbmInput MultiplayerKbm::snapshot(float yaw, float pitch,Uint32 milliseconds) const {
  MultiplayerKbmInput r;
  const auto active = [&](PcAction action) {
    return held(action) || pressed_[unsigned(action)];
  };
  auto &c = r.controls;
  c.yaw = yaw;
  c.pitch = pitch;
  c.forward = float(held(PcAction::Forward)) - float(held(PcAction::Back));
  c.right = float(held(PcAction::Right)) - float(held(PcAction::Left));
  c.jump = active(PcAction::Jump);
  c.crouch = held(PcAction::Crouch);
  c.fire = active(PcAction::Fire);
  c.reload = active(PcAction::Reload);
  auto &a = r.actions;
  a.scope_pressed = pressed_[unsigned(PcAction::Zoom)];
  a.scope_held = held(PcAction::Zoom);
  a.switch_weapon = pressed_[unsigned(PcAction::SwitchWeapon)];
  a.melee = pressed_[unsigned(PcAction::Melee)];
  a.grenade = pressed_[unsigned(PcAction::Grenade)];
  const auto pickup=pickup_hold_.sample(milliseconds),dual=dual_hold_.sample(milliseconds);
  c.pickup=pickup.first;c.pickup_milliseconds=pickup.second;
  c.dual_wield=dual.first;c.dual_wield_milliseconds=dual.second;
  c.switch_weapon = a.switch_weapon;
  c.fire_secondary = active(PcAction::DualSecondary);
  a.dual_primary=active(PcAction::DualPrimary);a.dual_secondary=c.fire_secondary;
  c.melee=a.melee;c.flashlight=pressed_[unsigned(PcAction::Flashlight)];
  c.switch_grenade=pressed_[unsigned(PcAction::SwitchGrenade)];
  c.accept=keys_[SDL_SCANCODE_A]||keys_[SDL_SCANCODE_RETURN]||keys_[SDL_SCANCODE_SPACE]||mouse_[SDL_BUTTON_LEFT];
  c.cancel=keys_[SDL_SCANCODE_B];
  c.grenade = a.grenade; // One consumed tick pulse, using the selected binding.
  a.scoreboard = held(PcAction::Scoreboard);
  return r;
}
MultiplayerKbmInput MultiplayerKbm::consume(float yaw, float pitch,Uint32 milliseconds) {
  const auto result = snapshot(yaw, pitch,milliseconds);
  pressed_.fill(false);
  pickup_hold_.consume();dual_hold_.consume();
  return result;
}
} // namespace halo2
