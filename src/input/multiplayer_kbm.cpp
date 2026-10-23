#include "halo2/multiplayer_kbm.hpp"

namespace halo2 {
Controls MultiplayerKbmInput::gameplay_controls(bool dual,bool seated,bool inverted,bool vehicle_secondary) const {
  auto result=controls;
  dual&=!seated;
  result.fire=dual?(inverted?actions.dual_secondary:actions.dual_primary):controls.fire;
  result.fire_secondary=dual && (inverted?actions.dual_primary:actions.dual_secondary);
  if(seated && vehicle_secondary)result.fire_secondary=actions.dual_secondary;
  // TU1.5 F297A consumes vehicle+148 bit0 as boost. The original driver
  // propagation copies the low control word; ordinary unit bit0 is crouch.
  // Keep this context change in the host adapter, before the existing command
  // and network owners, so right-click zoom cannot boost an on-foot unit.
  if(seated)result.crouch|=actions.dual_secondary;
  return result;
}
void MultiplayerKbm::suspend(Uint32 milliseconds) {
  if(suspended_at_)return;
  suspended_at_=milliseconds;pressed_.fill(false);
}
void MultiplayerKbm::resume(Uint32 milliseconds) {
  if(!suspended_at_)return;
  const auto elapsed=milliseconds-*suspended_at_;
  pickup_hold_.exclude_elapsed(elapsed);dual_hold_.exclude_elapsed(elapsed);
  suspended_at_.reset();
  resumed_interactions_={pickup_hold_.pending&&!pickup_hold_.down,dual_hold_.pending&&!dual_hold_.down};
  // A completed old interaction is delivered before a newly held request.
  // consume() starts that new hold only after delivering the saved completion.
  if(!pickup_hold_.down&&!pickup_hold_.pending)pickup_hold_.set(held(PcAction::Pickup),milliseconds);
  if(!dual_hold_.down&&!dual_hold_.pending)dual_hold_.set(held(PcAction::DualPickup),milliseconds);
}
void MultiplayerKbm::update_interactions(Uint32 milliseconds) {
  const auto update=[&](NativeInteractionHold &hold,PcAction action,bool resumed) {
    const bool down=held(action);
    if(!suspended_at_){
      if(!(resumed&&hold.pending&&!hold.down&&down))hold.set(down,milliseconds);
    }
    else if(hold.down&&!down)hold.set(false,*suspended_at_);
  };
  update(pickup_hold_,PcAction::Pickup,resumed_interactions_[0]);
  update(dual_hold_,PcAction::DualPickup,resumed_interactions_[1]);
}
void MultiplayerKbm::release() {
  keys_.fill(false);
  mouse_.fill(false);
  pressed_.fill(false);
  pickup_hold_={};dual_hold_={};
  resumed_interactions_={};
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
    if (down && !keys_[key] && !suspended_at_) {
      press(key);
    }
    keys_[key] = down;
    const auto now=event.key.timestamp?event.key.timestamp:SDL_GetTicks();
    update_interactions(now);
  } else if (event.type == SDL_MOUSEBUTTONDOWN ||
             event.type == SDL_MOUSEBUTTONUP) {
    if (!own_window(event.button.windowID) ||
        event.button.button >= mouse_.size())
      return;
    const bool down = event.type == SDL_MOUSEBUTTONDOWN;
    if (down && !mouse_[event.button.button] && !suspended_at_)
      press(pc_mouse_button(event.button.button));
    mouse_[event.button.button] = down;
    const auto now=event.button.timestamp?event.button.timestamp:SDL_GetTicks();
    update_interactions(now);
  } else if (event.type == SDL_MOUSEWHEEL && own_window(event.wheel.windowID)) {
    bool moved = event.wheel.x != 0 || event.wheel.y != 0;
#if SDL_VERSION_ATLEAST(2, 0, 18)
    moved |= event.wheel.preciseX != 0 || event.wheel.preciseY != 0;
#endif
    if (moved && !suspended_at_)
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
  if(suspended_at_)return r;
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
  if(suspended_at_)return result;
  pressed_.fill(false);
  pickup_hold_.consume();dual_hold_.consume();
  resumed_interactions_={};
  // A physical press made during suspension cannot overwrite an older
  // completed interaction. Start it after that completion's owning delivery.
  if(!pickup_hold_.down)pickup_hold_.set(held(PcAction::Pickup),milliseconds);
  if(!dual_hold_.down)dual_hold_.set(held(PcAction::DualPickup),milliseconds);
  return result;
}
} // namespace halo2
