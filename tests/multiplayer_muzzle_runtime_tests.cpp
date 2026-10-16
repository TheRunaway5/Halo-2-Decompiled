#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/particle_draw.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/animation_presentation.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <SDL.h>
#include <bit>
#include <cmath>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>

using namespace halo2;
namespace {
void require(bool condition,const char *why){if(!condition)throw FormatError(why);}
void push(SDL_Event event){require(SDL_PushEvent(&event)==1,"Actual SDL event queue rejected input");}
void button(unsigned window,bool held,unsigned mouse_button=SDL_BUTTON_LEFT){
  SDL_Event event{};event.type=held?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
  event.button.windowID=window;event.button.button=mouse_button;
  event.button.state=held?SDL_PRESSED:SDL_RELEASED;push(event);
}
struct Host {
  Cache cache;LevelContent content;MultiplayerMatch match;LanHost host;
  std::exception_ptr error;std::mutex mutex;std::jthread worker;
  static SlayerRules rules(const Cache &c){auto r=slayer_rules(UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}});r.weapon=c.slayer_starting_weapon(5);return r;}
  Host(const MultiplayerOptions &o):cache(o.map,o.shared_maps,OriginalData::load_xbe(o.gameplay_xbe)),
    content(cache.load_content(0,true)),match(content,0,cache.spawns(),rules(cache)),host(match,0,
      multiplayer_fingerprint(o.map,cache.shared_directory(),0,cache.slayer_starting_weapon(5),o.gameplay_xbe)){
    match.configure_replicated_first_person(cache.load_first_person_binding(cache.slayer_starting_weapon(5),2));
    worker=std::jthread([this](std::stop_token stop){try{std::array<Controls,16> controls{};auto next=std::chrono::steady_clock::now();
      while(!stop.stop_requested()){host.receive(controls);
        const auto roster=match.state();
        if(std::any_of(roster.players.begin(),roster.players.end(),[](const auto&p){return p.connected;}))match.step(controls);
        host.broadcast();next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);}}
      catch(...){std::lock_guard lock(mutex);error=std::current_exception();}});
  }
  void finish(){worker.request_stop();worker.join();std::lock_guard lock(mutex);if(error)std::rethrow_exception(error);}
};
}
int main(int argc,char **argv) {
  try {
    if(argc<4 || argc>8)throw FormatError("Supply MAP SHARED_MAPS UPDATE_XBE [PLAYERS] [SCREENSHOT] [--lan|--dual]");
    if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]))return 77;
    SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
    MultiplayerOptions options;
    options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
    options.local_players=argc>=5?unsigned(std::stoul(argv[4])):1;
    options.width=960;options.height=720;options.starting_weapon=5;options.mute=true;
    Cache definitions(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe));
    const auto content=definitions.load_content(0,true);
    const auto smg=content.weapons.at(definitions.slayer_starting_weapon(5));
    std::set<std::uint32_t> firing_effects;
    for(const auto &row:smg.firing_effects)
      firing_effects.insert(row.effects.begin(),row.effects.end());
    options.validation=std::getenv("HALO2_MUZZLE_VALIDATION");
    bool lan{},dual{};
    for(int at=5;at<argc;++at)if(std::string(argv[at])=="--lan")lan=true;
      else if(std::string(argv[at])=="--dual")dual=true;
      else {options.screenshot=argv[at];options.capture_tick=36;}
    require(!lan || options.screenshot.empty(),"LAN muzzle regression uses live expiry rather than a local capture");
    require(!dual || (!lan && options.local_players==1 && options.screenshot.empty()),
        "Dual SDL regression requires one offline live view");
    // A retail30Hz source step per submitted frame makes the visual-owner
    // regression independent of host shader compilation or background load.
    // Raw100ms frames can legitimately age a100ms muzzle to its transparent
    // endpoint before submission. This is a declared clock input, with real
    // SDL controls and unchanged original effect birth/retirement routines.
    options.reference_tick_cadence=!lan && options.screenshot.empty();
    if(options.reference_tick_cadence)options.frames=500;
    std::vector<std::uint32_t> born_players;
    if(dual) {
      CollisionWorld geometry(content.collision);bool placed{};
      for(const auto &equipment:content.netgame_equipment) {
        if(std::none_of(equipment.items.begin(),equipment.items.end(),[&](const auto &item){
            return item.definition==definitions.slayer_starting_weapon(5);}))continue;
        const auto floor=geometry.raycast(equipment.position+Vec3{0,0,.5f},{0,0,-1},3.f);
        if(!floor || floor->normal.z<.9f)continue;
        Spawn spawn;spawn.position=floor->position+Vec3{0,0,.002f};spawn.facing=0;
        options.native_spawn_inputs={spawn};placed=true;break;
      }
      require(placed,"Dual SDL regression has no authored SMG pickup floor");
      options.defer_initial_player_spawn=true;
      options.player_birth_owner=[&](MultiplayerPlayer &player,unsigned){
        born_players.push_back(player.source_datum);player.source_flags=8;
      };
      options.pre_player_update_owner=[&](MultiplayerMatch &match){
        for(const auto handle:born_players)if(const auto *player=match.player_from_datum(handle);
            player && !(player->source_flags&1u))
          match.apply_source_player_activation(handle,player->source_flags,player->source_flags|1u);
      };
    }
    std::unique_ptr<Host> host;if(lan){host=std::make_unique<Host>(options);options.join="127.0.0.1:"+std::to_string(host->host.port());}
    options.desktop=std::make_shared<DesktopSession>();
    auto *window=options.desktop->window(options.width,options.height,false);
    const auto window_id=SDL_GetWindowID(window);
    SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
    SDL_Event focus{};focus.type=SDL_WINDOWEVENT;focus.window.windowID=window_id;
    focus.window.event=SDL_WINDOWEVENT_FOCUS_GAINED;push(focus);
    require(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Actual relative mouse capture failed");
    unsigned positive{},transparent{},empty_after{},submitted{},input_tick{};
    std::set<std::uint32_t> effect_ticks;bool fired{};
    std::array<std::uint32_t,2> weapons{null_datum,null_datum};
    std::array<unsigned,2> hand_particles{},hand_models{};
    std::array<std::array<unsigned,2>,3> phase_hand_particles{};
    std::array<std::set<std::pair<std::uint32_t,std::uint32_t>>,2> hand_frames;
    unsigned rendered_frames{},stage{},stage_frame{},stage_tick{},released_tick{};
    options.frame_observer=[&](const MultiplayerFrameTiming &frame){rendered_frames+=frame.submitted;};
    options.animation_observer=[&](unsigned view,unsigned,const AnimationRenderObservation &draw){
      if(view || !draw.first_person || stage<2 || stage>6)return;
      for(unsigned hand=0;hand<2;++hand)if(draw.weapon==weapons[hand]) {
        require(!draw.vertices.empty() && !draw.weapon_nodes.empty(),"Dual hand lost its actual posed GPU model");
        ++hand_models[hand];hand_frames[hand].insert({draw.animation,std::bit_cast<std::uint32_t>(draw.frame)});
      }
    };
    std::set<std::uint32_t> first_person_locations;
    options.source_particle_phase_observer=[&](bool entering,const auto &,const auto &memory,const auto &,const auto &){
      if(entering)return;
      const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
      const auto pool=word(0x52d71cu),first=word(pool+0x44),high=word(pool+0x38);
      require(word(pool+0x24)==52 && high<=65536,"Original particle location pool is invalid");
      for(unsigned n=0;n<high;++n)if(word(word(pool+0x48)+n/32u*4u)&(1u<<(n%32u))) {
        const auto record=first+n*52u;
        if(std::to_integer<unsigned>(memory.view(record+2,1)[0]))
          first_person_locations.insert(std::uint32_t(Bytes(memory.view(record,2)).u16(0))*65536u+n);
      }
    };
    options.input_observer=[&](std::uint32_t tick,std::span<const Controls> controls,const MultiplayerState &state){
      input_tick=tick;
      const auto key=[&](SDL_Keycode code,bool held){SDL_Event event{};
        event.type=held?SDL_KEYDOWN:SDL_KEYUP;event.key.windowID=window_id;
        event.key.keysym.sym=code;event.key.state=held?SDL_PRESSED:SDL_RELEASED;push(event);};
      weapons={state.players[0].source_weapon,state.players[0].secondary_weapon.datum};
      if(!options.screenshot.empty()) {
        if(tick==18)button(window_id,true);
        if(tick==40)button(window_id,false);
        if(tick==60){SDL_Event quit{};quit.type=SDL_QUIT;push(quit);}
      }else {
        // Shader/pipeline warmup can batch many game ticks into one frame.
        // Keep real SDL inputs held through submitted frames as well as ticks.
        const auto next=[&](unsigned value){stage=value;stage_frame=rendered_frames;stage_tick=tick;};
        const auto elapsed=[&](unsigned frames,unsigned ticks){return rendered_frames>=stage_frame+frames && tick>=stage_tick+ticks;};
        require(tick<3000,"Actual SDL firing regression did not complete its rendered stages");
        if(stage==0 && rendered_frames>=10) {
          if(dual){key(SDLK_x,true);next(1);}else {button(window_id,true);next(2);}
        }else if(stage==1) {
          require(tick<stage_tick+300,"Actual SDL hold did not pick up the authored offhand SMG");
          if(weapons[1]!=null_datum){key(SDLK_x,false);button(window_id,true);next(2);}
        }else if(stage==2 && elapsed(20,30)) {
          button(window_id,false);
          if(dual){button(window_id,true,SDL_BUTTON_RIGHT);next(3);}
          else {released_tick=tick;next(6);}
        }else if(stage==3 && elapsed(20,20)){button(window_id,true);next(4);}
        else if(stage==4 && elapsed(20,20)) {
          button(window_id,false);button(window_id,false,SDL_BUTTON_RIGHT);
          released_tick=tick;key(SDLK_r,true);next(5);
        }else if(stage==5 && elapsed(2,2)){key(SDLK_r,false);next(6);}
        else if(stage==6 && elapsed(20,60)){SDL_Event quit{};quit.type=SDL_QUIT;push(quit);next(7);}
        if(dual && stage>=2)require(weapons[1]!=null_datum,"Live dual firing dropped its actual offhand");
      }
      fired|=state.players[0].loaded<60;
      for(unsigned i=1;i<controls.size();++i)require(!controls[i].fire,"Mouse fire leaked into another local view");
    };
    options.effect_observer=[&](unsigned view,std::uint32_t tick,std::span<const ParticleDraw> draws){
      ++submitted;
      if(view!=0 && !lan)for(const auto &draw:draws)
        if(draw.scene_sort_owner)require(!first_person_locations.contains(draw.scene_sort_owner->location),
            "Another local view received the shooter's original FP particles");
      if(view==0 && !draws.empty()) {
        bool visible=false;
        for(const auto &draw:draws) {
          // This observer also receives WORLD impacts. Concrete debris is
          // legitimately class8; only the stock SMG's own FP muzzle is class7.
          if(!firing_effects.contains(draw.effect_definition))continue;
          require(draw.material && draw.material->vertex.classification==7,"Actual authored muzzle material missing");
          if(!lan)require(draw.scene_sort_owner && draw.scene_sort_owner->system>65535u,
              "Local firing bypassed the actual original particle pool");
          for(float value:draw.vertex.attributes[4])require(std::isfinite(value),"Actual muzzle marker is not finite");
          // Original224550/2265C0 quantizes the authored age fade to zero
          // before and at age1. A retained endpoint is a valid submission.
          const auto alpha=draw.vertex.attributes[8][3];
          require(std::isfinite(alpha)&&alpha>=0&&alpha<=1,"Actual muzzle sprite has invalid packed alpha");
          visible|=alpha>0;transparent+=alpha==0;
          for(unsigned hand=0;hand<2;++hand)if(draw.effect_owner==weapons[hand] && alpha>0) {
            ++hand_particles[hand];
            if(stage>=2 && stage<=4)++phase_hand_particles[stage-2][hand];
          }
        }
        if(visible){++positive;effect_ticks.insert(tick);}
      }
      if(view==0 && stage>=6 && input_tick>released_tick+30 && draws.empty())++empty_after;
    };
    const auto result=run_multiplayer(options);if(host)host->finish();
    require(result==0 && fired,"Actual mouse fire did not reach the stock SMG");
    require(submitted>10 && positive>0 && effect_ticks.size()>2,"Actual fire never reached native particle rendering");
    if(options.screenshot.empty())require(empty_after>0,"Authored muzzle particles did not expire after firing");
    if(dual)for(unsigned hand=0;hand<2;++hand)
      require(hand_particles[hand]>0 && hand_models[hand]>5 && hand_frames[hand].size()>3,
          "Actual dual SDL fire did not animate/render both source hands and their particles");
    if(dual)require(phase_hand_particles[0][0] && phase_hand_particles[1][1] &&
        phase_hand_particles[2][0] && phase_hand_particles[2][1],
        "Primary-only, offhand-only or simultaneous SDL firing lost its original hand particles");
    std::cout<<"PASS actual SDL SMG fire: "<<positive<<" visible particle submissions on "<<effect_ticks.size()
        <<" ticks; "<<transparent<<" authored zero-alpha endpoints; "<<options.local_players
        <<" independent local views; native LAN="<<lan<<" dual="<<dual
        <<" hand_particles="<<hand_particles[0]<<','<<hand_particles[1]
        <<" primary_only="<<phase_hand_particles[0][0]
        <<" offhand_only="<<phase_hand_particles[1][1]
        <<" simultaneous="<<phase_hand_particles[2][0]<<','<<phase_hand_particles[2][1]
        <<" reference_tick_cadence="<<options.reference_tick_cadence<<"\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
