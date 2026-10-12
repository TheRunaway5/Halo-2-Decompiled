#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/desktop_session.hpp"
#include <bit>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace halo2;
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void push(SDL_Event event){check(SDL_PushEvent(&event)==1,"SDL rejected live scope stimulus");}
void click(Uint32 window,Uint8 button) {
  SDL_Event e{};e.type=SDL_MOUSEBUTTONDOWN;e.button.windowID=window;
  e.button.button=button;e.button.state=SDL_PRESSED;push(e);
  e.type=SDL_MOUSEBUTTONUP;e.button.state=SDL_RELEASED;push(e);
}
void reload(Uint32 window) {
  SDL_Event e{};e.type=SDL_KEYDOWN;e.key.windowID=window;
  e.key.keysym.scancode=SDL_SCANCODE_R;e.key.keysym.sym=SDLK_r;
  e.key.state=SDL_PRESSED;push(e);e.type=SDL_KEYUP;e.key.state=SDL_RELEASED;push(e);
}
struct Frame{unsigned tick{};float fov{};std::int16_t selector{};};
}
int main(int argc,char **argv) {
  try {
    if(argc!=4 && argc!=5)throw std::runtime_error("Supply actual MAP SHARED_MAPS UPDATE_XBE [--split4]");
    if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
    const Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
    const auto authored=cache.load_content(0,true);
    check(authored.binocular_zoom && authored.original_data->unit_scope_camera_constants,
        "Verified original camera/binocular data required");
    const float wide=authored.player.camera_field_of_view;
    const float magnified=wide/authored.binocular_zoom->minimum;
    SDL_SetMainReady();MultiplayerOptions options;
    options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
    options.width=640;options.height=480;options.local_players=1;options.starting_weapon=5;
    if(argc==5) {
      if(std::string(argv[4])!="--split4")throw std::runtime_error("Unknown scope probe mode");
      options.local_players=4;
    }
    options.mute=true;options.validation=std::getenv("HALO2_SCOPE_VALIDATION");
    options.desktop=std::make_shared<DesktopSession>();
    auto *window=options.desktop->window(640,480,false);const auto window_id=SDL_GetWindowID(window);
    SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
    SDL_Event focus{};focus.type=SDL_WINDOWEVENT;focus.window.windowID=window_id;
    focus.window.event=SDL_WINDOWEVENT_FOCUS_GAINED;push(focus);click(window_id,SDL_BUTTON_RIGHT);
    std::vector<MultiplayerState::Player> samples;std::vector<Controls> delivered;std::vector<Frame> frames;
    std::optional<unsigned> reload_step;
    unsigned current_tick{};
    options.input_observer=[&](std::uint32_t step,std::span<const Controls> controls,const MultiplayerState &state) {
      check(step==samples.size() && controls.size()==options.local_players,"Actual scope command/view lost");
      for(unsigned id=1;id<options.local_players;++id)
        check(state.players[id].source_zoom.current==-1 && state.players[id].source_hud_zoom_mask==0x40,
            "Keyboard scope leaked into another player unit/HUD");
      samples.push_back(state.players[0]);delivered.push_back(controls[0]);current_tick=step;
      if(controls[0].reload) {
        check(!reload_step,"One SDL R tap was delivered more than once");reload_step=step;
        // The observer runs after the real match step. Only queue the denied
        // RMB after the actual reload owns the source action/magazine state.
        click(window_id,SDL_BUTTON_RIGHT);
      }
      switch(step) {
      case 11: case 23: case 40: case 105:click(window_id,SDL_BUTTON_RIGHT);break;
      case 33:click(window_id,SDL_BUTTON_LEFT);break;
      case 50:reload(window_id);break;
      case 118:if(options.screenshot.empty()){SDL_Event quit{};quit.type=SDL_QUIT;push(quit);}break;
      }
    };
    options.view_observer=[&](unsigned view,const Camera &,float fov,std::int16_t selector) {
      check(view<options.local_players,"Scope created another viewport");
      if(view==0)frames.push_back({current_tick,fov,selector});
      else check(selector==-1 && std::bit_cast<std::uint32_t>(fov)==std::bit_cast<std::uint32_t>(wide),
          "One player's scope changed another view's camera/selector");
    };
    if(const auto *capture=std::getenv("HALO2_SCOPE_CAPTURE")) {
      options.screenshot=capture;options.capture_tick=120;
    }
    check(run_multiplayer(options)==0 && samples.size()>=119,"Actual rendered scope stopped prematurely");
    if(const auto *path=std::getenv("HALO2_SCOPE_TRACE")) {
      std::ofstream output(std::string(path)+".units.tsv");
      output<<"tick fire reload delivered_zoom current requested hud_mask action magazine loaded reset_serial\n";
      for(unsigned tick=0;tick<samples.size();++tick) {
        const auto &s=samples[tick];const auto &c=delivered[tick];
        output<<tick<<' '<<c.fire<<' '<<c.reload<<' '<<c.zoom<<' '<<int(s.source_zoom.current)<<' '
          <<int(s.source_zoom.requested)<<' '<<s.source_hud_zoom_mask<<' '<<s.source_action_word<<' '
          <<s.source_zoom_magazine_mode<<' '<<s.loaded<<' '<<s.zoom_reset_serial<<'\n';
      }
      check(bool(output),"Write actual unit and delivered scope observations");
    }
    check(samples.at(10).source_zoom.current==0 && samples.at(22).source_zoom.current==-1 &&
        samples.at(32).source_zoom.current==0,"RMB did not cycle actual retained source zoom state");
    check(samples.at(10).source_hud_zoom_mask==0x480 && samples.at(22).source_hud_zoom_mask==0x40 &&
        samples.at(32).source_hud_zoom_mask==0x480,"Original binocular HUD conditions did not follow actual unit CURRENT");
    bool fired_reset{};
    for(unsigned tick=34;tick<=39;++tick)
      fired_reset|=samples[tick].loaded<samples[33].loaded && samples[tick].source_zoom.current==-1 &&
          samples[tick].zoom_reset_serial>samples[33].zoom_reset_serial;
    check(fired_reset,"Actual prepared SMG shot failed to clear binocular/view scope");
    check(reload_step && *reload_step>=51 && *reload_step<=54,"Real SDL reload command did not arrive within the render event interval");
    const auto &reloaded=samples.at(*reload_step);
    check(samples[49].source_zoom.current==0 && reloaded.source_zoom.current==-1 &&
        (reloaded.source_action_word&1u) && reloaded.source_zoom_magazine_mode==1,
        "Actual manual reload did not perform source action/zoom reset");
    for(unsigned tick=*reload_step+1;tick<=*reload_step+6;++tick)
      check(samples[tick].source_zoom.current==-1 && delivered[tick].zoom==-1,
          "RMB bypassed real magazine/action admission during reload");
    check(samples[104].source_action_word==0 && samples[104].source_zoom_magazine_mode==0 &&
        samples[115].source_zoom.current==0,"Reload action did not release naturally before subsequent scope");
    const auto saw=[&](unsigned first,unsigned last,float fov,std::int16_t selector) {
      for(const auto &frame:frames)
        if(frame.tick>=first && frame.tick<=last && frame.selector==selector &&
            std::bit_cast<std::uint32_t>(frame.fov)==std::bit_cast<std::uint32_t>(fov))return true;
      return false;
    };
    check(saw(8,11,magnified,0) && saw(20,23,wide,-1) && saw(114,118,magnified,0),
        "Original FOV targets never reached the actual submitted render cameras");
    bool intermediate{};
    for(const auto &frame:frames)intermediate|=frame.fov>magnified && frame.fov<wide;
    check(intermediate,"Actual observer failed to render source transition frames");
    if(const auto *path=std::getenv("HALO2_SCOPE_TRACE")) {
      std::ofstream output(path);output<<"tick horizontal_fov view_selector\n"<<std::setprecision(9);
      for(const auto &frame:frames)output<<frame.tick<<' '<<frame.fov<<' '<<frame.selector<<'\n';
      check(bool(output),"Write actual render scope observations");
    }
    std::cout<<"PASS actual SDL RMB, independent source unit/view/HUD selectors, submitted FOV transitions, prepared-shot reset, manual reload/reset/admission/release\n";
    std::cout<<"LIMIT other weapon action managers, original six-channel camera coupling and Xbox raster coverage remain separate\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
