#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/desktop_session.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <vector>

using namespace halo2;
namespace {
void check(bool yes, const char *why) {
  if (!yes) throw std::runtime_error(why);
}
struct Sample { Controls controls; MultiplayerState::Player player; };
void push(SDL_Event event) { check(SDL_PushEvent(&event) == 1, "SDL event queue rejected a probe event"); }
void key(Uint32 window, SDL_Scancode scan, bool down) {
  SDL_Event event{};
  event.type = down ? SDL_KEYDOWN : SDL_KEYUP;
  event.key.windowID = window;
  event.key.state = down ? SDL_PRESSED : SDL_RELEASED;
  event.key.keysym.scancode = scan;
  event.key.keysym.sym = SDL_GetKeyFromScancode(scan);
  push(event);
}
void mouse(Uint32 window, bool down) {
  SDL_Event event{};
  event.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
  event.button.windowID = window;
  event.button.button = SDL_BUTTON_LEFT;
  event.button.state = down ? SDL_PRESSED : SDL_RELEASED;
  push(event);
}
void focus(Uint32 window, bool gained) {
  SDL_Event event{};
  event.type = SDL_WINDOWEVENT;
  event.window.windowID = window;
  event.window.event = gained ? SDL_WINDOWEVENT_FOCUS_GAINED : SDL_WINDOWEVENT_FOCUS_LOST;
  push(event);
}
}
int main(int argc, char **argv) {
#if !SDL_VERSION_ATLEAST(2, 0, 14)
  return 77; // SDL versions without virtual devices cannot run this probe.
#else
  try {
    if (argc != 3 && argc != 4) throw std::runtime_error("Requires MAP SHARED_MAPS [UPDATE_XBE]");
    if (!std::filesystem::exists(argv[1]) || !std::getenv("DISPLAY")) return 77;
    SDL_SetMainReady();
    MultiplayerOptions options;
    options.map = argv[1]; options.shared_maps = argv[2];
    if (argc == 4) {
      if(std::string_view(argv[3])=="native-mcc")
        check(Cache(options.map,options.shared_maps).is_mcc(),"native-mcc requires an imported MCC cache");
      else options.gameplay_xbe = argv[3];
    }
    options.width = 640; options.height = 480; options.local_players = 1;
    options.starting_weapon = 5; options.mute = true;
    options.validation = std::getenv("HALO2_KBM_VALIDATION");
    if (const auto *capture = std::getenv("HALO2_KBM_CAPTURE")) {
      options.screenshot = capture;
      options.capture_tick = 190;
    }
    options.desktop = std::make_shared<DesktopSession>();
    auto *window = options.desktop->window(options.width, options.height, false);
    const auto window_id = SDL_GetWindowID(window);

    const int device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
        SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
    check(device >= 0, "SDL virtual controller did not attach");
    auto *joystick = SDL_JoystickOpen(device);
    check(joystick != nullptr, "SDL virtual controller did not open");
    char guid[33]; SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joystick), guid, sizeof(guid));
    const std::string mapping = std::string(guid) +
        ",Runtime idle controller,a:b0,b:b1,x:b2,y:b3,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,";
    check(SDL_GameControllerAddMapping(mapping.c_str()) >= 0 && SDL_IsGameController(device),
          "Runtime controller was not recognized");
    for (int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; ++axis)
      check(SDL_JoystickSetVirtualAxis(joystick, axis,
          axis < SDL_CONTROLLER_AXIS_TRIGGERLEFT ? 0 : SDL_JOYSTICK_AXIS_MIN) == 0,
          "Could not deliver neutral controller axis");
    auto *controller = SDL_GameControllerOpen(device);
    check(controller && SDL_GameControllerGetAttached(controller), "Controller is not attached");
    SDL_GameControllerUpdate();
    for (int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; ++axis)
      check(SDL_GameControllerGetAxis(controller, static_cast<SDL_GameControllerAxis>(axis)) == 0,
          "Controller is not neutral");

    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    focus(window_id, true);
    key(window_id, SDL_SCANCODE_W, true);
    check(SDL_SetRelativeMouseMode(SDL_TRUE) == 0, "Relative mouse capture failed");
    const auto pad_direction=[&](Sint16 x,Sint16 y,bool jump) {
      check(SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_LEFTX,x)==0 &&
          SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_LEFTY,y)==0 &&
          SDL_JoystickSetVirtualButton(joystick,SDL_CONTROLLER_BUTTON_A,jump)==0,
          "Could not deliver virtual Xbox A plus movement");
      SDL_GameControllerUpdate();
    };
    std::vector<Sample> samples;
    options.input_observer = [&](std::uint32_t step, std::span<const Controls> controls,
                                 const MultiplayerState &state) {
      check(step == samples.size() && controls.size() == 1, "Runtime input observer lost a tick/view");
      samples.push_back({controls[0], state.players[0]});
      // The only stimuli are actual SDL events. This observer never replaces
      // controls, units, magazine timers, physics state, or gameplay outcomes.
      switch (step) {
      case 4: key(window_id, SDL_SCANCODE_W, false); key(window_id, SDL_SCANCODE_S, true); break;
      case 9: key(window_id, SDL_SCANCODE_S, false); key(window_id, SDL_SCANCODE_A, true); break;
      case 14: key(window_id, SDL_SCANCODE_A, false); key(window_id, SDL_SCANCODE_D, true); break;
      case 19: key(window_id, SDL_SCANCODE_D, false); break;
      case 21: key(window_id, SDL_SCANCODE_LCTRL, true); break;
      case 28:
        key(window_id, SDL_SCANCODE_LCTRL, false);
        key(window_id, SDL_SCANCODE_SPACE, true); key(window_id, SDL_SCANCODE_SPACE, false);
        break;
      case 39: mouse(window_id, true); break;
      case 55: mouse(window_id, false); key(window_id, SDL_SCANCODE_Q, true); key(window_id, SDL_SCANCODE_Q, false); break;
      case 60: key(window_id, SDL_SCANCODE_R, true); key(window_id, SDL_SCANCODE_R, false); break;
      case 113: key(window_id, SDL_SCANCODE_W, true); mouse(window_id, true); break;
      case 115: focus(window_id, false); break;
      case 118: focus(window_id, true); break;
      case 121: {
        SDL_Event event{}; event.type = SDL_MOUSEMOTION; event.motion.windowID = window_id;
        event.motion.xrel = 20; event.motion.yrel = -10; push(event); break;
      }
      case 125: key(window_id, SDL_SCANCODE_F1, true); key(window_id, SDL_SCANCODE_F1, false); break;
      case 128:
        key(window_id, SDL_SCANCODE_W, true); mouse(window_id, true);
        key(window_id, SDL_SCANCODE_SPACE, true); key(window_id, SDL_SCANCODE_R, true);
        key(window_id, SDL_SCANCODE_SPACE, false); key(window_id, SDL_SCANCODE_R, false);
        break;
      case 135: key(window_id, SDL_SCANCODE_F1, true); key(window_id, SDL_SCANCODE_F1, false); break;
      case 140:
        key(window_id, SDL_SCANCODE_W, true); mouse(window_id, true);
        break;
      case 146: key(window_id, SDL_SCANCODE_W, false); mouse(window_id, false); break;
      case 150: key(window_id, SDL_SCANCODE_F1, true); key(window_id, SDL_SCANCODE_F1, false); break;
      case 153: key(window_id,SDL_SCANCODE_W,true);key(window_id,SDL_SCANCODE_SPACE,true);break;
      case 156: key(window_id,SDL_SCANCODE_W,false);key(window_id,SDL_SCANCODE_S,true);break;
      case 159: key(window_id,SDL_SCANCODE_S,false);key(window_id,SDL_SCANCODE_A,true);break;
      case 162: key(window_id,SDL_SCANCODE_A,false);key(window_id,SDL_SCANCODE_D,true);break;
      case 165: key(window_id,SDL_SCANCODE_D,false);key(window_id,SDL_SCANCODE_SPACE,false);break;
      case 168: pad_direction(0,-32767,true);break;
      case 171: pad_direction(0,32767,true);break;
      case 174: pad_direction(-32767,0,true);break;
      case 177: pad_direction(32767,0,true);break;
      case 180: pad_direction(32767,0,false);break;
      case 183: pad_direction(0,0,false);break;
      case 187:
        if (options.screenshot.empty()) { SDL_Event quit{}; quit.type = SDL_QUIT; push(quit); }
        break;
      }
    };
    const int result = run_multiplayer(options);
    if (const auto *path = std::getenv("HALO2_KBM_TRACE")) {
      std::ofstream trace(path);
      check(bool(trace), "Could not save real input observation");
      trace << "step forward right yaw pitch jump crouch fire reload x y z vx vy vz player_crouch loaded reserve reload_seconds\n";
      trace << std::setprecision(9);
      for (unsigned step = 0; step < samples.size(); ++step) {
        const auto &c = samples[step].controls;
        const auto &p = samples[step].player;
        trace << step << ' ' << c.forward << ' ' << c.right << ' ' << c.yaw << ' ' << c.pitch
              << ' ' << c.jump << ' ' << c.crouch << ' ' << c.fire << ' ' << c.reload
              << ' ' << p.position.x << ' ' << p.position.y << ' ' << p.position.z
              << ' ' << p.velocity.x << ' ' << p.velocity.y << ' ' << p.velocity.z
              << ' ' << p.crouch << ' ' << p.loaded << ' ' << p.reserve << ' ' << p.reload_seconds << '\n';
      }
      check(bool(trace), "Real input observation write failed");
    }
    check(result == 0 && samples.size() >= 188, "Rendered runtime stopped before input verification");
    const auto sample = [&](unsigned tick) -> const Sample & { return samples.at(tick); };
    check(sample(2).controls.forward == 1 && sample(7).controls.forward == -1 &&
          sample(12).controls.right == -1 && sample(17).controls.right == 1,
          "Idle connected controller overwrote WASD in the live loop");
    const auto movement = sample(4).player.position - sample(0).player.position;
    check(movement.x * movement.x + movement.y * movement.y > 0,
          "W input never moved the actual match player");
    check(sample(25).controls.crouch && sample(28).player.crouch > sample(21).player.crouch,
          "MCC Ctrl did not reach live crouch gameplay");
    unsigned jump_taps{};
    for (unsigned tick = 29; tick < 39; ++tick) jump_taps += sample(tick).controls.jump;
    check(jump_taps == 1, "Short Space tap was lost or repeated across simulation ticks");
    bool jumped{};
    for (unsigned tick = 29; tick < 39; ++tick) jumped |= sample(tick).player.velocity.z > 0;
    check(jumped, "Space did not lift the actual player");
    check(sample(50).controls.fire && sample(55).player.loaded < sample(39).player.loaded,
          "LMB did not expend ammunition in the actual match");
    for (unsigned tick = 56; tick < 61; ++tick)
      check(!sample(tick).controls.reload, "MCC Q melee also triggered live reload");
    unsigned reload_taps{};
    bool reloaded{};
    const auto &before_reload=sample(60).player;
    for (unsigned tick = 61; tick <= 113; ++tick) {
      reload_taps += sample(tick).controls.reload;
      const auto &player=sample(tick).player;
      // Native equipment supplies the actual magazine/reserve bank. The
      // separate C++ presentation timer is not a native reload acknowledgement.
      check(player.weapon==before_reload.weapon&&
          player.loaded+player.reserve==before_reload.loaded+before_reload.reserve,
          "Reload changed the weapon or created/lost ammunition");
      if(player.loaded>before_reload.loaded){
        check(before_reload.reserve-player.reserve==player.loaded-before_reload.loaded,
            "Reload did not transfer the exact reserve ammunition");
        reloaded=true;
      }
    }
    const auto reload_diagnostic=std::string("Short R tap did not perform actual reload: presses=")+std::to_string(reload_taps)+
        " magazine_increased="+std::to_string(reloaded);
    check(reload_taps == 1 && reloaded, reload_diagnostic.c_str());
    for (unsigned tick = 119; tick <= 121; ++tick)
      check(!sample(tick).controls.forward && !sample(tick).controls.fire && !sample(tick).controls.reload,
          "Focus loss/gain restored stale movement or shooting");
    check(sample(124).controls.yaw != sample(121).controls.yaw &&
          sample(124).controls.pitch != sample(121).controls.pitch,
          "Relative SDL mouse motion did not reach the live camera");
    for (unsigned tick = 130; tick <= 144; ++tick)
      check(sample(tick).controls.forward == 1 && sample(tick).controls.fire,
            "F1 interrupted held gameplay input after debug menu removal");
    for (unsigned tick = 148; tick < 154; ++tick)
      check(!sample(tick).controls.forward && !sample(tick).controls.fire,
            "Released gameplay input returned after pressing F1");
    for(const unsigned begin:{154u,169u}) {
      const std::array<std::array<float,2>,4> directions{{{1,0},{-1,0},{0,-1},{0,1}}};
      for(unsigned direction=0;direction<directions.size();++direction)
        for(unsigned held=0;held<3;++held) {
          const auto &input=sample(begin+direction*3+held).controls;
          check(input.jump && input.forward==directions[direction][0] && input.right==directions[direction][1],
              "Rendered runtime lost Space/WASD or Xbox A/stick evasion direction");
        }
    }
    for(unsigned tick=166;tick<169;++tick) {
      const auto &input=sample(tick).controls;
      check(!input.jump && !input.forward && !input.right,"Released keyboard evasion chord replayed in runtime");
    }
    for(unsigned tick=181;tick<184;++tick) {
      const auto &input=sample(tick).controls;
      check(!input.jump && input.right==1 && !input.forward,"Xbox A release also released movement or retained jump");
    }
    for(unsigned tick=184;tick<188;++tick) {
      const auto &input=sample(tick).controls;
      check(!input.jump && !input.forward && !input.right,"Released Xbox evasion chord replayed in runtime");
    }
    SDL_GameControllerClose(controller); SDL_JoystickClose(joystick);
    check(SDL_JoystickDetachVirtual(device) == 0, "Virtual controller did not detach");
    std::cout << "PASS: actual rendered multiplayer, 188+ observed simulation ticks, idle attached controller, "
                 "WASD movement, Ctrl crouch, single Space/R taps, LMB ammunition, R reload, mouse look, focus reset, "
                 "inert F1, all four held Space/WASD and virtual Xbox A/stick directions, and normal held-input release\n";
    std::cout << "LIMIT: Banshee maneuver movement and scope/switch/scoreboard/melee/grenade gameplay handlers are not validated by this probe\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
#endif
}
