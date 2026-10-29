#define SDL_MAIN_HANDLED
#include "halo2/controller_settings.hpp"
#include "halo2/multiplayer_kbm.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace halo2;
namespace {
void check(bool value,const std::string &message){if(!value)throw std::runtime_error(message);}
int binding(unsigned vk) {
  if(vk==1)return pc_mouse_button(SDL_BUTTON_LEFT);
  if(vk==2)return pc_mouse_button(SDL_BUTTON_RIGHT);
  if(vk==162)return SDL_SCANCODE_LCTRL;
  return SDL_GetScancodeFromKey(SDL_Keycode(vk>='A'&&vk<='Z'?vk+('a'-'A'):vk));
}
}
int main(int argc,char **argv)try {
  check(argc==2,"Supply the MCC shipped-default reference");
  SDL_SetMainReady();check(SDL_Init(SDL_INIT_EVENTS)==0,"Initialize SDL event owner");
  nlohmann::json reference;std::ifstream(argv[1])>>reference;
  check(reference.at("schema")=="HALO2_MCC_DEFAULT_CONTROLS_V1","Unknown MCC default reference");
  const PcKeyboardSettings defaults;
  check(reference.at("keyboard").size()==pc_action_count,"MCC reference omitted an available keyboard action");
  for(const auto &row:reference.at("keyboard")) {
    const auto action=row.at("action").get<unsigned>();
    check(action<pc_action_count && defaults.bindings[action]==binding(row.at("virtual_key")),
        "MCC keyboard default mismatch: "+row.at("source_action").get<std::string>());
    const auto alternate=row.value("secondary_virtual_key",0u);
    check(defaults.secondary[action]==(alternate?binding(alternate):0),
        "MCC alternate binding mismatch: "+row.at("source_action").get<std::string>());
  }
  ControllerSettings pad;
  check(pad.sticks==0 && pad.buttons==0,"MCC Halo 2 Default controller preset is not selected");
  for(unsigned hardware=0;hardware<24;++hardware) {
    ControllerSample sample;sample.hardware[hardware]=1;const auto input=controller_input(sample,pad);
    const std::array<bool,14> actions{input.jump,input.crouch,input.fire,input.secondary,input.grenade,
        input.reload,input.pickup,input.dual_wield,input.switch_weapon,input.melee,input.zoom,
        input.flashlight,input.switch_grenade,input.scoreboard};
    for(unsigned i=0;i<actions.size();++i) {
      const auto &row=reference.at("controller").at(i);
      bool expected=hardware==row.at("hardware").get<unsigned>();
      if(row.contains("additional_hardware"))for(const unsigned alternate:row.at("additional_hardware"))
        expected|=hardware==alternate;
      check(actions[i]==expected,"MCC controller default mismatch: "+row.at("source_action").get<std::string>());
    }
  }
  // The default score key must be a held request, independent of weapon swap,
  // repeats, simulation consumption and the duration of a physical hold.
  MultiplayerKbm input(17);
  const auto key=[&](bool down,bool repeat=false) {
    SDL_Event event{};event.type=down?SDL_KEYDOWN:SDL_KEYUP;event.key.windowID=17;
    event.key.keysym.scancode=SDL_SCANCODE_TAB;event.key.repeat=repeat;
    check(SDL_PushEvent(&event)==1,"Queue Tab score input");
    while(SDL_PollEvent(&event))input.handle_event(event);
  };
  key(true);
  for(unsigned tick=0;tick<90;++tick) {
    if(tick==30)key(true,true);
    const auto held=input.consume();
    check(held.actions.scoreboard && !held.actions.switch_weapon,"Held Tab lost scores or switched weapons");
  }
  key(false);check(!input.consume().actions.scoreboard,"Tab release retained scoreboard input");
  key(true);key(false);check(!input.consume().actions.scoreboard,"Released scoreboard tap became a queued toggle");
  SDL_Quit();std::cout<<"PASS MCC shipped KB/M and Halo 2 Default controller mappings; Tab hold/release without weapon swap\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';SDL_Quit();return 1;}
