#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_kbm.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace halo2;
unsigned checks{};
void check(bool condition, const char *reason) {
  ++checks;
  if (!condition) throw std::runtime_error(reason);
}
bool neutral(const MultiplayerKbmInput &input) {
  const auto &c = input.controls;
  const auto &a = input.actions;
  return !c.forward && !c.right && !c.jump && !c.crouch && !c.fire && !c.reload &&
      !c.pickup && !c.dual_wield && !c.pickup_milliseconds && !c.dual_wield_milliseconds &&
      !a.scope_pressed && !a.scope_held && !a.switch_weapon && !a.scoreboard &&
      !a.melee && !a.grenade;
}
struct EventQueue {
  SDL_Window *window{};
  Uint32 id{};
  Uint32 key_timestamp{};
  EventQueue() {
    SDL_SetMainReady();
    SDL_SetHintWithPriority(SDL_HINT_VIDEODRIVER, "dummy", SDL_HINT_OVERRIDE);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0)
      throw std::runtime_error(std::string("SDL event initialization: ") + SDL_GetError());
    window = SDL_CreateWindow("KBM event fixture", 0, 0, 32, 32, SDL_WINDOW_HIDDEN);
    if (!window) throw std::runtime_error(std::string("SDL fixture window: ") + SDL_GetError());
    id = SDL_GetWindowID(window);
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
  }
  ~EventQueue() { SDL_DestroyWindow(window); SDL_Quit(); }
  void push(SDL_Event event) {
    check(SDL_PushEvent(&event) == 1, "SDL did not enqueue the host input event");
  }
  void drain(MultiplayerKbm &adapter) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if(event.type==SDL_KEYDOWN || event.type==SDL_KEYUP)key_timestamp=event.key.timestamp;
      adapter.handle_event(event);
    }
  }
  void key(SDL_Scancode scan, bool down, bool repeat = false, Uint32 other = 0) {
    SDL_Event event{};
    event.type = down ? SDL_KEYDOWN : SDL_KEYUP;
    event.key.windowID = other ? other : id;
    event.key.state = down ? SDL_PRESSED : SDL_RELEASED;
    event.key.repeat = repeat;
    event.key.keysym.scancode = scan;
    event.key.keysym.sym = SDL_GetKeyFromScancode(scan);
    push(event);
  }
  void button(Uint8 which, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
    event.button.windowID = id;
    event.button.button = which;
    event.button.state = down ? SDL_PRESSED : SDL_RELEASED;
    push(event);
  }
  void wheel(int x, int y) {
    SDL_Event event{};
    event.type = SDL_MOUSEWHEEL;
    event.wheel.windowID = id;
    event.wheel.x = x;
    event.wheel.y = y;
    push(event);
  }
  void focus(bool gained, Uint32 other = 0) {
    SDL_Event event{};
    event.type = SDL_WINDOWEVENT;
    event.window.windowID = other ? other : id;
    event.window.event = gained ? SDL_WINDOWEVENT_FOCUS_GAINED : SDL_WINDOWEVENT_FOCUS_LOST;
    push(event);
  }
  void reset(MultiplayerKbm &adapter) {
    focus(false); focus(true); drain(adapter);
    check(neutral(adapter.snapshot()), "focus reset retained input");
  }
};
} // namespace

int main() {
  try {
    EventQueue queue;
    MultiplayerKbm adapter(queue.id);
    check(neutral(adapter.snapshot()), "new adapter is not neutral");

    // Every input below traverses SDL_PushEvent -> SDL_PollEvent; no direct
    // adapter event call or global keyboard-state assignment admits a press.
    queue.key(SDL_SCANCODE_W, true); queue.key(SDL_SCANCODE_D, true);
    queue.key(SDL_SCANCODE_LSHIFT, true); queue.drain(adapter);
    auto input = adapter.snapshot(1.25f, -.5f);
    check(input.controls.forward == 1 && input.controls.right == 1, "W/D movement bindings");
    check(input.controls.crouch, "left Shift crouch binding");
    check(input.controls.yaw == 1.25f && input.controls.pitch == -.5f, "Runtime camera angles changed");
    queue.key(SDL_SCANCODE_S, true); queue.key(SDL_SCANCODE_A, true);
    queue.key(SDL_SCANCODE_RSHIFT, true); queue.key(SDL_SCANCODE_LSHIFT, false);
    queue.drain(adapter);
    input = adapter.consume();
    check(!input.controls.forward && !input.controls.right, "opposite movement keys do not cancel");
    check(input.controls.crouch, "releasing left Shift released held right Shift");
    queue.key(SDL_SCANCODE_W, false); queue.key(SDL_SCANCODE_D, false);
    queue.key(SDL_SCANCODE_RSHIFT, false); queue.key(SDL_SCANCODE_LCTRL, true);
    queue.drain(adapter); input = adapter.snapshot();
    check(input.controls.forward == -1 && input.controls.right == -1, "S/A movement bindings");
    check(!input.controls.crouch, "crouch still uses the replaced Ctrl binding");
    queue.reset(adapter);

    for (const auto key : {SDL_SCANCODE_SPACE, SDL_SCANCODE_R, SDL_SCANCODE_TAB,
                           SDL_SCANCODE_F, SDL_SCANCODE_Q}) {
      queue.key(key, true); queue.key(key, false);
    }
    queue.button(SDL_BUTTON_LEFT, true); queue.button(SDL_BUTTON_LEFT, false);
    queue.button(SDL_BUTTON_RIGHT, true); queue.button(SDL_BUTTON_RIGHT, false);
    queue.drain(adapter);
    for (unsigned frame = 0; frame < 5; ++frame) {
      input = adapter.snapshot();
      check(input.controls.jump && input.controls.reload && input.controls.fire,
            "short supported taps were dropped before a simulation tick");
      check(input.actions.scope_pressed && !input.actions.scope_held &&
                input.actions.switch_weapon && input.actions.melee && input.actions.grenade,
            "short native requests were dropped during render-frame observation");
    }
    input = adapter.consume();
    check(input.controls.jump && input.controls.reload && input.controls.fire &&
              input.actions.scope_pressed && input.actions.switch_weapon &&
              input.actions.melee && input.actions.grenade, "simulation missed queued requests");
    check(neutral(adapter.consume()), "released taps were delivered twice");

    queue.button(SDL_BUTTON_RIGHT,true);queue.drain(adapter);
    for(unsigned tick=0;tick<3;++tick) {
      const auto held=adapter.consume();
      for(bool inverted:{false,true}) {
        for(bool dual:{false,true}) {
          const auto seated=held.gameplay_controls(dual,true,inverted);
          check(!seated.fire && seated.fire_secondary && seated.crouch,
              "Held vehicle right-click lost original unit or existing physical boost intent");
        }
        const auto on_foot=held.gameplay_controls(false,false,inverted);
        check(!on_foot.fire && !on_foot.fire_secondary && !on_foot.crouch,"On-foot zoom also fired or boosted");
      }
    }
    queue.button(SDL_BUTTON_RIGHT,false);queue.drain(adapter);
    check(!adapter.consume().gameplay_controls(false,true,false).crouch,"Released vehicle boost remained held");
    check(!adapter.consume().gameplay_controls(false,true,false).fire_secondary,
        "Released vehicle secondary trigger remained held");
    queue.reset(adapter);

    // Pin observation time relative to the timestamp delivered by SDL. The
    // authored 233ms boundary is tested at the gameplay admission seam too.
    queue.key(SDL_SCANCODE_E,true);queue.drain(adapter);
    const auto pickup_start=queue.key_timestamp;
    input=adapter.consume(0,0,pickup_start+232);
    check(input.controls.pickup && input.controls.pickup_milliseconds==232,
          "SDL pickup elapsed time changed before the authored hold threshold");
    input=adapter.consume(0,0,pickup_start+233);
    check(input.controls.pickup && input.controls.pickup_milliseconds==233,
          "consuming input reset a held pickup clock");
    queue.key(SDL_SCANCODE_E,true,true);queue.drain(adapter);
    input=adapter.snapshot(0,0,pickup_start+234);
    check(input.controls.pickup_milliseconds==234,"key repeat restarted the pickup hold");
    queue.key(SDL_SCANCODE_X,true);queue.drain(adapter);
    const auto dual_start=queue.key_timestamp;
    input=adapter.snapshot(0,0,dual_start+233);
    check(input.controls.pickup && input.controls.dual_wield &&
              input.controls.dual_wield_milliseconds==233,
          "pickup and dual wield hold clocks do not retain independent requests");
    input=adapter.snapshot(0,0,dual_start+70000);
    check(input.controls.pickup_milliseconds==65535 && input.controls.dual_wield_milliseconds==65535,
          "long interaction hold wrapped its source duration field");
    queue.reset(adapter);
    for(const auto key:{SDL_SCANCODE_E,SDL_SCANCODE_X}) {
      queue.key(key,true);queue.key(key,false);
    }
    queue.drain(adapter);input=adapter.consume();
    check(input.controls.pickup && input.controls.dual_wield,
          "SDL coalesced release lost an interaction request");
    check(neutral(adapter.consume()),"released interaction requests were delivered twice");

    // Custom interaction timing must follow the chosen keyboard/mouse binding.
    PcKeyboardSettings custom;
    custom.bindings[unsigned(PcAction::Pickup)] = SDL_SCANCODE_P;
    custom.secondary[unsigned(PcAction::Pickup)] = pc_mouse_button(SDL_BUTTON_MIDDLE);
    custom.bindings[unsigned(PcAction::DualPickup)] = SDL_SCANCODE_O;
    custom.secondary[unsigned(PcAction::DualPickup)] = 0;
    custom.bindings[unsigned(PcAction::Fire)] = pc_mouse_button(SDL_BUTTON_RIGHT);
    custom.secondary[unsigned(PcAction::Fire)] = 0;
    custom.bindings[unsigned(PcAction::DualPrimary)] = pc_mouse_button(SDL_BUTTON_LEFT);
    custom.secondary[unsigned(PcAction::DualPrimary)] = 0;
    adapter.settings(custom);
    queue.key(SDL_SCANCODE_P, true); queue.drain(adapter);
    const auto remapped_start = queue.key_timestamp;
    input = adapter.snapshot(0, 0, remapped_start + 233);
    check(input.controls.pickup && input.controls.pickup_milliseconds == 233,
          "custom pickup binding did not retain its authored hold duration");
    queue.key(SDL_SCANCODE_P, false); queue.drain(adapter); adapter.consume();
    queue.button(SDL_BUTTON_MIDDLE, true); queue.drain(adapter);
    check(adapter.snapshot().controls.pickup,
          "secondary mouse pickup binding did not activate its hold watcher");
    queue.reset(adapter);
    queue.key(SDL_SCANCODE_E, true); queue.drain(adapter);
    check(!adapter.snapshot().controls.pickup,
          "old hard-coded pickup key survived a custom rebind");
    queue.reset(adapter);
    queue.button(SDL_BUTTON_RIGHT, true); queue.drain(adapter);
    input = adapter.consume();
    check(input.controls.fire && !input.actions.dual_primary,
          "single and dual-wield primary bindings lost their distinct source requests");
    queue.reset(adapter);
    queue.button(SDL_BUTTON_LEFT, true); queue.drain(adapter);
    input = adapter.consume();
    check(!input.controls.fire && input.actions.dual_primary,
          "dual-wield primary request followed the single-weapon binding");
    queue.reset(adapter);
    adapter.settings(PcKeyboardSettings{});

    queue.key(SDL_SCANCODE_SPACE, true); queue.key(SDL_SCANCODE_R, true);
    queue.key(SDL_SCANCODE_F3, true); queue.button(SDL_BUTTON_LEFT, true);
    queue.button(SDL_BUTTON_RIGHT, true); queue.key(SDL_SCANCODE_TAB, true);
    queue.key(SDL_SCANCODE_F, true); queue.key(SDL_SCANCODE_Q, true);
    queue.drain(adapter); input = adapter.consume();
    check(input.actions.scope_pressed && input.actions.switch_weapon && input.actions.melee &&
              input.actions.grenade, "first held request did not deliver its press");
    input = adapter.consume();
    check(input.controls.jump && input.controls.reload && input.controls.fire &&
              input.actions.scope_held && input.actions.scoreboard, "held actions did not persist");
    check(!input.actions.scope_pressed && !input.actions.switch_weapon && !input.actions.melee &&
              !input.actions.grenade, "held one-shot action repeated on another tick");
    for (const auto key : {SDL_SCANCODE_TAB, SDL_SCANCODE_F, SDL_SCANCODE_Q})
      queue.key(key, true, true);
    queue.drain(adapter); input = adapter.consume();
    check(!input.actions.switch_weapon && !input.actions.melee && !input.actions.grenade,
          "key repeat generated native action presses");
    queue.key(SDL_SCANCODE_TAB, false); queue.key(SDL_SCANCODE_TAB, true);
    queue.drain(adapter);
    check(adapter.consume().actions.switch_weapon, "fresh Tab press did not rearm");
    queue.reset(adapter);

    // Unbound F1 has no gameplay action or debug overlay.
    queue.key(SDL_SCANCODE_F1, true); queue.drain(adapter);
    check(neutral(adapter.snapshot()), "F1 still admits gameplay input");
    queue.key(SDL_SCANCODE_F1, false); queue.drain(adapter);
    check(neutral(adapter.consume()), "F1 release queued gameplay input");

    // Overlay ownership changes must clear held controls and queued requests
    // without fabricating a window-focus change or delivering them on return.
    for (const auto key : {SDL_SCANCODE_W, SDL_SCANCODE_D, SDL_SCANCODE_SPACE,
                           SDL_SCANCODE_LSHIFT, SDL_SCANCODE_R, SDL_SCANCODE_TAB,
                           SDL_SCANCODE_F, SDL_SCANCODE_Q, SDL_SCANCODE_F3})
      queue.key(key, true);
    queue.button(SDL_BUTTON_LEFT, true); queue.button(SDL_BUTTON_RIGHT, true);
    queue.wheel(0, 1); queue.drain(adapter);
    input = adapter.snapshot();
    check(input.controls.forward && input.controls.right && input.controls.jump &&
              input.controls.crouch && input.controls.fire && input.controls.reload &&
              input.actions.scope_pressed && input.actions.scope_held &&
              input.actions.switch_weapon && input.actions.scoreboard &&
              input.actions.melee && input.actions.grenade,
          "overlay-release fixture omitted held or pending input");
    adapter.release();
    check(neutral(adapter.snapshot()) && neutral(adapter.consume()),
          "public release retained held controls or queued requests");
    input = adapter.snapshot(1.25f, -.5f);
    check(input.controls.yaw == 1.25f && input.controls.pitch == -.5f,
          "public release changed the runtime camera angles");
    for (const auto key : {SDL_SCANCODE_W, SDL_SCANCODE_D, SDL_SCANCODE_SPACE,
                           SDL_SCANCODE_LSHIFT, SDL_SCANCODE_R, SDL_SCANCODE_TAB,
                           SDL_SCANCODE_F, SDL_SCANCODE_Q, SDL_SCANCODE_F3})
      queue.key(key, false);
    queue.button(SDL_BUTTON_LEFT, false); queue.button(SDL_BUTTON_RIGHT, false);
    queue.drain(adapter);
    check(neutral(adapter.consume()), "post-capture releases restored stale input");
    queue.key(SDL_SCANCODE_W, true); queue.key(SDL_SCANCODE_R, true);
    queue.key(SDL_SCANCODE_TAB, true); queue.button(SDL_BUTTON_LEFT, true);
    queue.drain(adapter); input = adapter.consume();
    check(input.controls.forward == 1 && input.controls.reload && input.controls.fire &&
              input.actions.switch_weapon,
          "public release prevented fresh input without a focus cycle");
    queue.reset(adapter);

    for (const auto key : {SDL_SCANCODE_SPACE, SDL_SCANCODE_R, SDL_SCANCODE_TAB,
                           SDL_SCANCODE_F, SDL_SCANCODE_Q}) queue.key(key, true, true);
    queue.drain(adapter);
    check(neutral(adapter.consume()), "repeat without an initial press admitted input");
    queue.wheel(0, 0); queue.drain(adapter);
    check(!adapter.consume().actions.switch_weapon, "zero mouse wheel admitted a switch");
    for (const auto wheel : {std::array{0, 1}, std::array{0, -1}, std::array{1, 0}}) {
      queue.wheel(wheel[0], wheel[1]); queue.drain(adapter);
      check(adapter.snapshot().actions.switch_weapon, "nonzero wheel switch did not persist");
      check(adapter.consume().actions.switch_weapon, "nonzero wheel did not deliver a switch");
      check(!adapter.consume().actions.switch_weapon, "wheel switch delivered twice");
    }
    queue.button(SDL_BUTTON_MIDDLE, true); queue.button(SDL_BUTTON_MIDDLE, false);
    queue.drain(adapter); check(neutral(adapter.consume()), "unbound mouse button admitted an action");

    queue.key(SDL_SCANCODE_F, true); queue.key(SDL_SCANCODE_F, false); queue.drain(adapter);
    input = adapter.consume();
    check(input.actions.melee && !input.controls.reload, "F melee or R reload binding changed");
    queue.key(SDL_SCANCODE_F3, true); queue.key(SDL_SCANCODE_F3, false); queue.drain(adapter);
    check(!adapter.consume().actions.scoreboard, "released F3 remained held");
    queue.key(SDL_SCANCODE_W, true, false, queue.id + 1); queue.focus(false, queue.id + 1);
    queue.drain(adapter); check(neutral(adapter.consume()), "foreign window changed input");

    queue.key(SDL_SCANCODE_W, true); queue.key(SDL_SCANCODE_SPACE, true);
    queue.key(SDL_SCANCODE_R, true); queue.key(SDL_SCANCODE_F, true);
    queue.key(SDL_SCANCODE_Q, true); queue.key(SDL_SCANCODE_TAB, true);
    queue.key(SDL_SCANCODE_F3, true); queue.key(SDL_SCANCODE_LSHIFT, true);
    queue.button(SDL_BUTTON_LEFT, true); queue.button(SDL_BUTTON_RIGHT, true);
    queue.wheel(0, 1); queue.focus(false); queue.drain(adapter);
    check(neutral(adapter.consume()), "focus loss retained held state or pending dangerous presses");
    adapter.release();
    queue.key(SDL_SCANCODE_W, true); queue.key(SDL_SCANCODE_R, true);
    queue.key(SDL_SCANCODE_F, true); queue.key(SDL_SCANCODE_Q, true);
    queue.button(SDL_BUTTON_LEFT, true); queue.button(SDL_BUTTON_RIGHT, true);
    queue.wheel(0, 1); queue.drain(adapter);
    check(neutral(adapter.consume()), "public release admitted unfocused input");
    queue.focus(true); queue.drain(adapter);
    check(neutral(adapter.consume()), "focus gain restored stale input");
    queue.button(SDL_BUTTON_LEFT, true); queue.button(SDL_BUTTON_LEFT, false);
    queue.drain(adapter); check(adapter.consume().controls.fire, "fresh LMB did not rearm after focus gain");

#if SDL_VERSION_ATLEAST(2, 0, 14)
    const int device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
        SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
    check(device >= 0, "SDL virtual controller did not attach");
    auto *joystick = SDL_JoystickOpen(device);
    check(joystick != nullptr, "attached SDL controller did not open");
    char guid[33]; SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joystick), guid, sizeof(guid));
    const std::string mapping = std::string(guid) +
        ",KBM idle controller,a:b0,b:b1,x:b2,y:b3,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,";
    check(SDL_GameControllerAddMapping(mapping.c_str()) >= 0, "SDL controller mapping failed");
    auto *controller = SDL_GameControllerOpen(device);
    check(controller && SDL_GameControllerGetAttached(controller), "real SDL controller is not connected");
    for (int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; ++axis) {
      const Sint16 raw = axis < SDL_CONTROLLER_AXIS_TRIGGERLEFT ? 0 : SDL_JOYSTICK_AXIS_MIN;
      check(SDL_JoystickSetVirtualAxis(joystick, axis, raw) == 0, "SDL idle axis delivery failed");
    }
    SDL_GameControllerUpdate();
    for (int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; ++axis)
      check(SDL_GameControllerGetAxis(controller, static_cast<SDL_GameControllerAxis>(axis)) == 0,
            "attached SDL controller has a non-idle axis");
    for (int button = 0; button < SDL_CONTROLLER_BUTTON_MAX; ++button)
      check(SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button)) == 0,
            "attached SDL controller has a held button");
    queue.key(SDL_SCANCODE_W, true); queue.key(SDL_SCANCODE_R, true);
    queue.key(SDL_SCANCODE_R, false); queue.button(SDL_BUTTON_LEFT, true);
    queue.button(SDL_BUTTON_LEFT, false); queue.drain(adapter);
    input = adapter.consume();
    check(input.controls.forward == 1 && input.controls.reload && input.controls.fire,
          "idle connected SDL controller changed keyboard/mouse requests");
    SDL_GameControllerClose(controller); SDL_JoystickClose(joystick);
    check(SDL_JoystickDetachVirtual(device) == 0, "SDL virtual controller did not detach");
    queue.drain(adapter);
    check(adapter.snapshot().controls.forward == 1, "controller removal cleared keyboard hold");
#endif
    {
      MultiplayerKbm paused(queue.id);
      queue.key(SDL_SCANCODE_E,true);queue.drain(paused);
      const auto started=queue.key_timestamp;
      check(paused.consume(0,0,started+100).controls.pickup_milliseconds==100,
          "Pre-pause interaction duration was not admitted");
      queue.key(SDL_SCANCODE_Q,true);queue.key(SDL_SCANCODE_Q,false);queue.drain(paused);
      paused.suspend(started+600);paused.suspend(started+900);
      queue.key(SDL_SCANCODE_E,false);
      queue.key(SDL_SCANCODE_Q,true);queue.key(SDL_SCANCODE_Q,false);
      queue.key(SDL_SCANCODE_F,true);queue.key(SDL_SCANCODE_F,false);queue.wheel(0,1);
      queue.key(SDL_SCANCODE_W,true);queue.key(SDL_SCANCODE_X,true);queue.drain(paused);
      for(unsigned tick=0;tick<4;++tick){
        check(neutral(paused.snapshot(1.25f,-.5f,started+1000+tick)),"Suspended snapshot admitted gameplay");
        const auto blocked=paused.consume(1.25f,-.5f,started+1000+tick);
        check(neutral(blocked)&&blocked.controls.yaw==1.25f&&blocked.controls.pitch==-.5f,
            "Suspended tick consumed a completion or changed look");
      }
      paused.resume(started+10000);paused.resume(started+20000);
      auto resumed=paused.consume(0,0,started+10000);
      check(!paused.suspended()&&resumed.controls.forward==1&&!resumed.actions.grenade&&
          !resumed.actions.melee&&!resumed.actions.switch_weapon,
          "Paused taps leaked into resumed gameplay or held movement was lost");
      check(resumed.controls.pickup&&resumed.controls.pickup_milliseconds==600,
          "Old interaction release lost its frozen duration");
      check(resumed.controls.dual_wield&&resumed.controls.dual_wield_milliseconds==0,
          "New held interaction inherited elapsed pause time");
      resumed=paused.consume(0,0,started+10025);
      check(!resumed.controls.pickup&&!resumed.controls.pickup_milliseconds&&
          resumed.controls.dual_wield_milliseconds==25,"Completion repeated or resumed hold clock changed");
      paused.release();

      queue.key(SDL_SCANCODE_E,true);queue.drain(paused);const auto restarted=queue.key_timestamp;
      paused.consume(0,0,restarted+100);paused.suspend(restarted+400);
      queue.key(SDL_SCANCODE_E,false);queue.key(SDL_SCANCODE_E,true);queue.drain(paused);
      paused.resume(restarted+10000);
      // Runtime polls events after resuming but before delivering a native
      // command. An unrelated fresh key must not start the new hold early.
      queue.key(SDL_SCANCODE_D,true);queue.drain(paused);
      check(paused.consume(0,0,restarted+10000).controls.pickup_milliseconds==400,
          "New physical hold overwrote an old suspended completion");
      check(paused.consume(0,0,restarted+10025).controls.pickup_milliseconds==25,
          "New hold did not start after delivery of the old completion");
      paused.release();

      queue.key(SDL_SCANCODE_E,true);queue.drain(paused);const auto held_start=queue.key_timestamp;
      paused.consume(0,0,held_start+100);paused.suspend(held_start+500);
      paused.resume(held_start+20000);
      check(paused.snapshot(0,0,held_start+20025).controls.pickup_milliseconds==525,
          "Unreleased hold included suspended wall time");
      paused.suspend(held_start+21000);queue.focus(false);queue.focus(true);queue.drain(paused);
      paused.resume(held_start+22000);
      check(neutral(paused.consume(0,0,held_start+22001)),"Focus loss resurrected a suspended interaction");
      NativeInteractionHold wrapped;wrapped.set(true,0xfffffff0u);wrapped.exclude_elapsed(32);
      check(wrapped.sample(0x20u).second==16,"Suspended interaction clock failed across SDL rollover");
    }
    std::cout << checks << " keyboard/mouse host checks passed through the actual SDL event queue; "
              << "action consumer gameplay is outside this adapter\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
