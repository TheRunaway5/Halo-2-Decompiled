#pragma once
#include "halo2/simulation.hpp"
#include "halo2/pc_settings.hpp"
#include "halo2/native_interaction_hold.hpp"
#include <SDL.h>
#include <array>
#include <optional>

namespace halo2 {
// Native host requests. These fields do not establish gameplay support for
// the corresponding original actions.
struct MultiplayerKbmActions {
  bool scope_pressed{}, scope_held{}, switch_weapon{}, scoreboard{}, melee{},
      grenade{},dual_primary{},dual_secondary{};
};
struct MultiplayerKbmInput {
  Controls controls;
  MultiplayerKbmActions actions;
};

class MultiplayerKbm {
public:
  // Zero accepts events from any window; Runtime can bind its shared window.
  explicit MultiplayerKbm(Uint32 window_id = 0,
                          PcKeyboardSettings settings = {})
      : window_id_(window_id), settings_(std::move(settings)) {}
  void handle_event(const SDL_Event &);
  // Render-frame observation leaves presses queued until simulation delivery.
  MultiplayerKbmInput snapshot(float yaw = 0, float pitch = 0, Uint32 milliseconds = SDL_GetTicks()) const;
  MultiplayerKbmInput consume(float yaw = 0, float pitch = 0, Uint32 milliseconds = SDL_GetTicks());
  // Release held inputs and queued presses when an overlay takes or returns
  // input ownership. This does not change window-focus admission.
  void release();
  // During a native handoff, keep focus/physical key state current but admit
  // no gameplay input or queued presses. Held interaction time is frozen;
  // releases of pre-pause interactions retain their completed duration.
  // New taps entirely within the pause are discarded. A newly held button
  // starts at resume, without a synthetic press or elapsed pause duration.
  void suspend(Uint32 milliseconds = SDL_GetTicks());
  void resume(Uint32 milliseconds = SDL_GetTicks());
  bool suspended()const noexcept {return suspended_at_.has_value();}
  void settings(const PcKeyboardSettings &value) {
    if(settings_!=value) {release();settings_=value;}
  }

private:
  Uint32 window_id_{};
  PcKeyboardSettings settings_;
  bool focused_ = true;
  std::optional<Uint32> suspended_at_;
  NativeInteractionHold pickup_hold_,dual_hold_;
  std::array<bool,2> resumed_interactions_{};
  std::array<bool, SDL_NUM_SCANCODES> keys_{};
  std::array<bool, 6> mouse_{};
  std::array<bool, pc_action_count> pressed_{};
  bool held(PcAction) const;
  void press(int binding);
  void update_interactions(Uint32);
  bool own_window(Uint32 id) const { return !window_id_ || id == window_id_; }
};
} // namespace halo2
