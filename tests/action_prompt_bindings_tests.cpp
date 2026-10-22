#include "halo2/action_prompt_bindings.hpp"
#include <SDL.h>
#include <iostream>
#include <stdexcept>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
}
int main()try {
  for(unsigned layout=0;layout<4;++layout)for(unsigned sticks=0;sticks<4;++sticks) {
    ControllerSettings pad;pad.buttons=layout;pad.sticks=sticks;
    check(native_action_prompt_controller_buttons(pad)==std::array<std::uint32_t,2>{0xe102,0xe103},
        "Pickup/dual prompt differs from native X/Y adapter");
    const auto boarding=native_boarding_prompt_controller_buttons(pad);
    check(boarding[0]==(layout==1?0xe107:layout==2?0xe101:0xe106) &&
        boarding[1]==(layout==2?0xe106:layout==3?0xe10f:0xe101),
        "Boarding prompt differs from native grenade/melee adapter");
  }
  PcKeyboardSettings keyboard;
  check(native_action_prompt_keyboard_buttons(keyboard)==std::array<std::string,2>{"E","X"},"Wrong default keyboard actions");
  check(native_boarding_prompt_keyboard_buttons(keyboard)==std::array<std::string,2>{"Q","F"},"Wrong default boarding actions");
  keyboard.bindings[unsigned(PcAction::Pickup)]=0;
  keyboard.secondary[unsigned(PcAction::Pickup)]=pc_mouse_button(4);
  keyboard.bindings[unsigned(PcAction::DualPickup)]=SDL_SCANCODE_SPACE;
  check(native_action_prompt_keyboard_buttons(keyboard)==std::array<std::string,2>{"Mouse 4","Space"},"Remapped action labels lost their bindings");
  unsigned events{};
  for(auto type:{SDL_KEYDOWN,SDL_MOUSEBUTTONDOWN,SDL_MOUSEMOTION,SDL_MOUSEWHEEL}) {
    SDL_Event event{};event.type=type;
    if(type==SDL_KEYDOWN)event.key.windowID=7;
    else if(type==SDL_MOUSEBUTTONDOWN)event.button.windowID=7;
    else if(type==SDL_MOUSEMOTION){event.motion.windowID=7;event.motion.xrel=1;}
    else {event.wheel.windowID=7;event.wheel.y=-1;}
    check(native_action_prompt_keyboard_activity(event,7),"Real keyboard/mouse action did not select its prompt labels");
    check(!native_action_prompt_keyboard_activity(event,8),"Foreign window selected prompt labels");++events;
  }
  SDL_Event event{};event.type=SDL_MOUSEMOTION;event.motion.windowID=7;
  check(!native_action_prompt_keyboard_activity(event,7),"Idle mouse motion stole controller prompts");++events;
  event={};event.type=SDL_MOUSEWHEEL;event.wheel.windowID=7;
  check(!native_action_prompt_keyboard_activity(event,7),"Empty mouse wheel event stole controller prompts");++events;
#if SDL_VERSION_ATLEAST(2, 0, 18)
  event.wheel.preciseY=.25f;
  check(native_action_prompt_keyboard_activity(event,7),"Precise wheel input retained controller prompts");++events;
#endif
  event={};event.type=SDL_CONTROLLERBUTTONDOWN;
  check(!native_action_prompt_keyboard_activity(event,7),"Controller event selected keyboard prompts");++events;
  PlayerPromptDevices owners;
  owners.bind(0,41,PromptDevice::Xbox);owners.bind(1,73,PromptDevice::PlayStation);
  owners.bind(2,-1,PromptDevice::KeyboardMouse);owners.bind(3,109,PromptDevice::Xbox);
  event={};event.type=SDL_KEYDOWN;event.key.windowID=7;owners.event(event,7,0);
  check(owners.device(0)==PromptDevice::KeyboardMouse && owners.device(1)==PromptDevice::PlayStation &&
      owners.device(3)==PromptDevice::Xbox,"Keyboard input changed another player's prompt family");
  owners.bind(0,41,PromptDevice::Xbox);owners.bind(1,73,PromptDevice::PlayStation);
  check(owners.device(0)==PromptDevice::KeyboardMouse,"Unrelated hotplug reset the last-used KBM selection");
  event={};event.type=SDL_CONTROLLERBUTTONDOWN;event.cbutton.which=73;owners.event(event,7,0);
  check(owners.device(0)==PromptDevice::KeyboardMouse && owners.device(1)==PromptDevice::PlayStation,
      "PlayStation input changed the Xbox/KBM player's selection");
  event.cbutton.which=41;owners.event(event,7,0);
  check(owners.device(0)==PromptDevice::Xbox && owners.device(1)==PromptDevice::PlayStation,
      "Xbox input did not select only its owner's prompts");
  event.cbutton.which=777;owners.event(event,7,0);
  check(owners.device(2)==PromptDevice::KeyboardMouse,"Unassigned controller claimed the KBM view");
  event={};event.type=SDL_KEYDOWN;event.key.windowID=8;owners.event(event,7,0);
  check(owners.device(0)==PromptDevice::Xbox,"Foreign-window input stole a player's prompt family");
  event.key.windowID=7;owners.event(event,7,0);
  event={};event.type=SDL_CONTROLLERAXISMOTION;event.caxis.which=41;event.caxis.value=4000;owners.event(event,7,0);
  check(owners.device(0)==PromptDevice::KeyboardMouse,"Stick drift stole keyboard prompts");
  event.caxis.value=20000;owners.event(event,7,0);
  check(owners.device(0)==PromptDevice::Xbox,"Real stick input did not restore its owner's Xbox prompts");
  event={};event.type=SDL_CONTROLLERDEVICEREMOVED;event.cdevice.which=73;owners.event(event,7,0);
  check(owners.device(1)==PromptDevice::KeyboardMouse && owners.device(0)==PromptDevice::Xbox &&
      owners.device(3)==PromptDevice::Xbox,"Unplugging one controller changed another player's icons");
  owners.bind(1,211,PromptDevice::PlayStation);
  check(owners.device(1)==PromptDevice::PlayStation && owners.device(0)==PromptDevice::Xbox,
      "Reconnecting a controller changed a different local player");
  check(controller_prompt_device(nullptr)==PromptDevice::KeyboardMouse,"Missing controller selected gamepad icons");
#if SDL_VERSION_ATLEAST(2,24,0)
  check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)==0,"SDL controller fixture initialization failed");
  for(auto vendor:{0x054c,0x045e}) {
    SDL_VirtualJoystickDesc description{};description.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    description.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;description.naxes=SDL_CONTROLLER_AXIS_MAX;
    description.nbuttons=SDL_CONTROLLER_BUTTON_MAX;description.vendor_id=vendor;
    description.product_id=vendor==0x054c?0x0ce6:0x028e;
    description.name=vendor==0x054c?"Virtual DualSense":"Virtual Xbox";
    description.button_mask=(1u<<SDL_CONTROLLER_BUTTON_MAX)-1;
    description.axis_mask=(1u<<SDL_CONTROLLER_AXIS_MAX)-1;
    const int index=SDL_JoystickAttachVirtualEx(&description);
    check(index>=0,"SDL virtual controller attachment failed");
    auto *pad=SDL_GameControllerOpen(index);check(pad,"SDL virtual controller open failed");
    check(controller_prompt_device(pad)==(vendor==0x054c?PromptDevice::PlayStation:PromptDevice::Xbox),
        "Actual SDL controller identity selected the wrong artwork family");
    SDL_GameControllerClose(pad);check(SDL_JoystickDetachVirtual(index)==0,"Virtual controller cleanup failed");
  }
  SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
#endif
  std::cout<<"PASS prompt bindings: controller_layouts=16 keyboard_defaults_and_remaps=3 mixed_local_players=4 ownership_cases=11 input_events="<<events<<'\n';
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
