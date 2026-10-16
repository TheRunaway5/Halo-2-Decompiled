#include "halo2/game_pause_menu.hpp"
#include "halo2/multiplayer_kbm.hpp"
#include "halo2/audio.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
unsigned checks{};
void check(bool value,const char *message) {++checks;if(!value)throw FormatError(message);}
void choose(MenuState &state,unsigned row) {state.select(row);state.accept();}
int source_key(unsigned key) {
  if(key>='A' && key<='Z')return SDL_SCANCODE_A+int(key-'A');
  switch(key) {
  case 8:return SDL_SCANCODE_BACKSPACE;
  case 9:return SDL_SCANCODE_TAB;
  case 32:return SDL_SCANCODE_SPACE;
  case 37:return SDL_SCANCODE_LEFT;
  case 38:return SDL_SCANCODE_UP;
  case 39:return SDL_SCANCODE_RIGHT;
  case 40:return SDL_SCANCODE_DOWN;
  case 160:return SDL_SCANCODE_LSHIFT;
  case 161:return SDL_SCANCODE_RSHIFT;
  case 165:return SDL_SCANCODE_RALT;
  case 268:return 0; // International circumflex key has no US physical equivalent.
  default:throw FormatError("Unrecognized original preset key");
  }
}
}
int main(int argc,char **argv) try {
  if(argc!=3 && argc!=4)return 77;
  const std::filesystem::path maps=argv[1],assets=argv[2];
  auto content=Cache(maps/"mainmenu.map").load_frontend();
  nlohmann::json source_tables;
  if(argc==4){std::ifstream tables(argv[3]);tables>>source_tables;}
  content.vista=load_vista_pause(assets/"maps/01a_tutorial.map");content.gameplay_overlay=true;
  check(content.vista->screens.at(18).name=="ui\\screens\\singleplayer\\dialog_pause\\dialog_pause","Pause definition is not the Vista gameplay widget");
  check(!content.vista->screens.contains(5),"Gameplay imported a shell background");
  Mesh mesh;FrontendPainter painter(content,maps/"fonts",mesh);
  MenuState menu(content);std::vector<GamePauseAction> actions;
  menu.set_game_pause_action([&](GamePauseAction a){actions.push_back(a);return std::string{};});
  menu.open_game_pause(false);
  check(menu.stack().size()==1 && menu.frame().entries.size()==3,"Multiplayer pause leaked the title stack or campaign commands");
  check(menu.frame().entries[0].label=="Continue" && menu.frame().entries[1].label=="Settings" && menu.frame().entries[2].label=="Quit","Pause labels differ from actual Vista strings");
  unsigned draws{};
  for(const auto size:{std::array<unsigned,2>{640,480},{1280,720},{1920,1200},{3440,1440}}) {
    const auto draw=painter.draw(menu,2,-1,2000,size);
    check(!draw.vertices.empty() && draw.scenes.empty(),"Pause draws no GUI or introduces a shell scene");
    for(const auto &vertex:draw.vertices)check(vertex.material<mesh.materials.size(),"Pause escaped its source material bank");
    ++draws;
  }
  choose(menu,2);
  check(menu.frame().screen==vista_menu(7) && menu.frame().selected==1 && actions.empty(),"Quit did not require explicit confirmation with Cancel selected");
  check(menu.frame().entries[0].label=="Quit","Quit confirmation falsely promises a durable campaign save");
  painter.draw(menu,2,-1,2000,{1280,720});
  for(int y=0;y<480;y+=2)for(int x=0;x<640;x+=2)
    if(auto hit=painter.pointer_hit(x,y);hit && hit->action==FrontendPointerAction::Entry)
      check(hit->index<2,"Quit dialog admits underlying pause rows");
  const auto stale=FrontendPointerTarget{FrontendPointerAction::Entry,0,menu.revision};
  menu.back();check(!menu.activate_pointer(stale),"Queued confirmation click activated a replacement screen");
  choose(menu,2);choose(menu,1);check(actions.empty() && menu.frame().screen==vista_menu(18),"Cancel exited the match");
  choose(menu,2);choose(menu,0);check(actions.back()==GamePauseAction::Quit,"Confirmed Quit missed its game owner");
  actions.clear();menu.open_game_pause(false);menu.back();check(actions.back()==GamePauseAction::Resume,"Escape at pause root did not Resume");
  for(unsigned row=0;row<4;++row) {
    menu.open_game_pause(false);choose(menu,1);choose(menu,row);
    const unsigned expected[]{200,260,262,261};
    check(menu.frame().screen==vista_menu(expected[row]),"Settings lost the gameplay screen namespace");
    check(!painter.draw(menu,2,-1,2000).vertices.empty(),"Gameplay Settings failed to render");++draws;
    menu.back();check(menu.frame().screen==vista_menu(259),"Settings Back lost pause hub");
  }
  menu.set_pc_display_adapter([](const DisplaySettings &){return std::string{};});
  menu.open_game_pause(false);choose(menu,1);choose(menu,3);choose(menu,8);choose(menu,50);
  check(menu.frame().screen==vista_menu(261) && menu.pc_display_settings().field_of_view==120 &&
      menu.pc_display_settings().vehicle_field_of_view==70,"pause on-foot FOV did not apply independently");
  choose(menu,9);choose(menu,20);
  check(menu.frame().screen==vista_menu(261) && menu.pc_display_settings().vehicle_field_of_view==90,
      "pause vehicle FOV did not return to the video menu");
  check(!painter.draw(menu,2,-1,2000).vertices.empty(),"pause FOV rows did not render");
  choose(menu,10);
  check(menu.frame().screen==vista_menu(261) && menu.pc_display_settings().crt_filter &&
      menu.pc_value_label(menu.frame(),10)=="On" && menu.pc_display_settings().field_of_view==120,
      "pause CRT toggle did not apply in place or changed FOV");
  choose(menu,10);
  check(!menu.pc_display_settings().crt_filter && menu.pc_value_label(menu.frame(),10)=="Off",
      "pause CRT toggle could not disable the filter");
  check(menu.pc_value_label(menu.frame(),11)=="Horizontal","pause two-player default is not horizontal");
  choose(menu,11);
  check(menu.frame().screen==vista_menu(261) && menu.pc_display_settings().two_player_split==TwoPlayerSplit::Vertical &&
      menu.pc_display_settings().field_of_view==120 && !menu.pc_display_settings().crt_filter,
      "pause split toggle did not apply in place or changed other preferences");
  choose(menu,11);check(menu.pc_display_settings().two_player_split==TwoPlayerSplit::Horizontal,
      "pause split toggle could not restore horizontal layout");
  menu.select(10);
  for(const auto size:{std::array<unsigned,2>{640,480},{1280,720},{2560,720}}) {
    painter.draw(menu,2,-1,2000,size);bool hit_crt=false;
    for(int y=0;y<480 && !hit_crt;y+=2)for(int x=0;x<640 && !hit_crt;x+=2)
      if(auto hit=painter.pointer_hit(x,y);hit && hit->action==FrontendPointerAction::Entry && hit->index==10)
        hit_crt=true;
    check(hit_crt,"pause CRT row has no mouse target after scrolling");
    menu.select(11);painter.draw(menu,2,-1,2000,size);bool hit_split=false;
    for(int y=0;y<480 && !hit_split;y+=2)for(int x=0;x<640 && !hit_split;x+=2)
      if(auto hit=painter.pointer_hit(x,y);hit && hit->action==FrontendPointerAction::Entry && hit->index==11)
        hit_split=true;
    check(hit_split,"pause split row has no mouse target after scrolling");menu.select(10);
  }
  for(unsigned row=0;row<6;++row) {
    menu.open_game_pause(false);choose(menu,1);choose(menu,0);choose(menu,row);
    check(is_vista_menu(menu.frame().screen),"Controller submenu lost its Vista artwork");
    check(!painter.draw(menu,2,-1,2000).vertices.empty(),"Controller submenu did not render");
    const auto vibration=menu.controller_settings().vibration;
    menu.accept();check(menu.frame().screen==vista_menu(200),"Controller setting did not return to its settings hub");
    if(row==5)check(menu.controller_settings().vibration==vibration,"Vibration selection did not retain its selected value");
  }
  const auto preferences=std::filesystem::temp_directory_path()/"halo2-pause-settings-test.cfg";
  std::filesystem::remove(preferences);
  {MenuState saved(content,preferences);unsigned display_calls{};
    saved.set_pc_display_adapter([&](const DisplaySettings &settings){++display_calls;check(settings.subtitles==(display_calls==1?0:1),"Display adapter received the wrong subtitles value");return std::string{};});
    saved.open_game_pause(false);choose(saved,1);choose(saved,0);choose(saved,0);choose(saved,3);
    saved.open_game_pause(false);choose(saved,1);choose(saved,0);choose(saved,2);choose(saved,9);
    saved.open_game_pause(false);choose(saved,1);choose(saved,0);choose(saved,5);choose(saved,1);
    check(!saved.controller_settings().vibration && saved.frame().screen==vista_menu(200),"Vibration Off did not reach its preference owner");
    saved.open_game_pause(false);choose(saved,1);choose(saved,2);choose(saved,2);choose(saved,1);
    check(display_calls==2 && saved.pc_display_settings().subtitles==1,"Subtitles On did not reach its display owner");}
  {MenuState saved(content,preferences);unsigned display_calls{};
    saved.set_pc_display_adapter([&](const DisplaySettings &settings){++display_calls;check(settings.subtitles==(display_calls==1?1:display_calls==2?2:0),"Display adapter received the wrong subtitles value");return std::string{};});
    check(saved.controller_settings().sticks==3 && saved.controller_settings().sensitivity==10,
      "Controller settings failed their menu.cfg round trip");
    check(!saved.controller_settings().vibration && saved.pc_display_settings().subtitles==1,"Vibration and subtitles failed their menu.cfg round trip");
    saved.open_game_pause(false);choose(saved,1);choose(saved,0);choose(saved,5);choose(saved,0);
    check(saved.controller_settings().vibration,"Vibration On did not reach its preference owner");
    saved.open_game_pause(false);choose(saved,1);choose(saved,2);choose(saved,2);choose(saved,2);
    check(saved.pc_display_settings().subtitles==2,"Subtitles Off did not reach its display owner");
    saved.open_game_pause(false);choose(saved,1);choose(saved,2);choose(saved,2);choose(saved,0);
    check(display_calls==3 && saved.pc_display_settings().subtitles==0,"Automatic subtitles did not reach its display owner");}
  std::filesystem::remove(preferences);
  for(unsigned preset=0;preset<3;++preset) {
    menu.open_game_pause(false);choose(menu,1);choose(menu,1);choose(menu,0);choose(menu,preset);
    MultiplayerKbm preset_input(17);preset_input.settings(menu.pc_keyboard_settings());SDL_Event key{};
    key.type=SDL_KEYDOWN;key.key.windowID=17;key.key.keysym.scancode=preset==2?SDL_SCANCODE_I:SDL_SCANCODE_W;
    preset_input.handle_event(key);check(preset_input.snapshot().controls.forward==1,"Vista keyboard preset did not affect live movement");
    if(argc==4)for(const auto &source:source_tables)if(source.at("keyboard")==preset) {
      // Original action ordinals, independently compared with the live menu's
      // host bindings. Original mouse motion is outside this digital adapter.
      constexpr unsigned ordinals[]{47,48,45,46,0,13,10,7,4,5,43,14,12,37,39,6,3,23,22};
      static_assert(std::size(ordinals)==pc_action_count);
      const auto settings=menu.pc_keyboard_settings();
      for(unsigned action=0;action<pc_action_count;++action) {
        std::array<int,2> expected{};unsigned slot=0;
        for(const auto &binding:source.at("actions").at(ordinals[action])) {
          const unsigned kind=binding.at(0),value=binding.at(1);
          if(kind==2 || (kind==0 && value>2))continue;
          check(slot<2,"Original preset exceeds native digital binding slots");
          constexpr unsigned mouse[]{SDL_BUTTON_LEFT,SDL_BUTTON_MIDDLE,SDL_BUTTON_RIGHT};
          expected[slot++]=kind==1?source_key(value):pc_mouse_button(mouse[value]);
        }
        if(settings.bindings[action]!=expected[0] || settings.secondary[action]!=expected[1])
          throw FormatError("Pause keyboard preset "+std::to_string(preset)+" action "+std::to_string(action)+
              " differs: native="+std::to_string(settings.bindings[action])+","+std::to_string(settings.secondary[action])+
              " source="+std::to_string(expected[0])+","+std::to_string(expected[1]));
        ++checks;
      }
    }
    MenuState retained(content);
    retained.adopt_pc_settings(menu.pc_keyboard_settings(),{},{});
    retained.open_game_pause(false);choose(retained,1);choose(retained,1);choose(retained,0);
    check(retained.frame().selected==preset,"Returning to a retained menu lost the selected keyboard preset");
  }
  menu.open_game_pause(false);choose(menu,1);choose(menu,1);choose(menu,0);choose(menu,3);
  check(menu.frame().screen==vista_menu(254),"In-game Custom keyboard mapping is unreachable");
  check(!painter.draw(menu,2,-1,2000).vertices.empty(),"In-game Custom mapping did not draw");
  choose(menu,1);check(menu.capturing_binding(),"Custom mapping did not capture input");menu.bind_pc_input(SDL_SCANCODE_I);
  MultiplayerKbm input(17);input.settings(menu.pc_keyboard_settings());SDL_Event key{};
  key.type=SDL_KEYDOWN;key.key.windowID=17;key.key.keysym.scancode=SDL_SCANCODE_I;input.handle_event(key);
  check(input.snapshot().controls.forward==1,"Pause binding did not reach gameplay input");input.release();
  check(input.snapshot().controls.forward==0,"Overlay input release retained movement");
  menu.open_game_pause(true);check(menu.frame().entries.size()==5,"Campaign pause omitted checkpoint/restart actions");
  choose(menu,1);check(actions.back()==GamePauseAction::Revert,"Revert did not reach campaign owner");
  menu.open_game_pause(true);choose(menu,2);check(menu.frame().screen==vista_menu(7),"Restart has no confirmation");
  choose(menu,0);check(actions.back()==GamePauseAction::Restart,"Restart did not reach campaign owner");
  menu.open_game_pause(true);menu.set_game_pause_action([](GamePauseAction){return "No checkpoint";});choose(menu,1);
  check(menu.frame().screen==vista_menu(8) && menu.frame().description=="No checkpoint","Missing checkpoint did not produce a dismissible error");
  choose(menu,0);check(menu.frame().screen==vista_menu(18),"Error OK did not return to pause");
  Mesh input_mesh;GamePauseMenu overlay(maps,assets,input_mesh,false);
  SDL_Event event{};event.type=SDL_KEYDOWN;event.key.keysym.sym=SDLK_ESCAPE;
  check(overlay.event(event) && overlay.visible(),"Escape did not open the gameplay overlay");
  event.key.repeat=1;overlay.event(event);check(overlay.visible(),"Held Escape closed/reopened pause");
  event.key.repeat=0;overlay.event(event);check(!overlay.visible() && !overlay.quit_requested(),"Escape resumed by exiting the match");
  event={};event.type=SDL_CONTROLLERBUTTONDOWN;event.cbutton.button=SDL_CONTROLLER_BUTTON_START;
  overlay.event(event);check(overlay.visible(),"Controller Start did not open pause");overlay.event(event);check(!overlay.visible(),"Controller Start did not resume");
  {
    AudioEngine audio(false);auto sound=std::make_shared<PcmSound>();
    sound->stereo={.1f,.2f,.3f,.4f,.5f,.6f};audio.play(sound);
    const auto first=audio.render(1);check(first[0]==.1f,"Pause audio fixture did not start");
    check(audio.speaker_layout(3).empty() && audio.output_channels()==6,"5.1 output selection did not reach the offline mixer");
    const auto frames=audio.mixed_frames();audio.paused(true);const auto silent=audio.render(2);
    check(std::all_of(silent.begin(),silent.end(),[](float v){return v==0;}) && audio.mixed_frames()==frames,
        "Campaign pause advanced audio or emitted sound");
    audio.paused(false);const auto resumed=audio.render(1);
    check(resumed.size()==6 && resumed[0]==.3f && resumed[1]==.4f && resumed[2]==0 && resumed[5]==0,"Campaign Resume skipped paused audio samples");
  }
  std::cout<<"PASS "<<checks<<" pause source, navigation, confirmation, dialog isolation and input checks; "<<draws<<" layouts\n";
  return 0;
} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
