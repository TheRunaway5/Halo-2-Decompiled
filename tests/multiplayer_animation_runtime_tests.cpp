#define SDL_MAIN_HANDLED
#include "halo2/animation_presentation.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/native_player_profile.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <thread>
#include <mutex>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
void push(SDL_Event event){check(SDL_PushEvent(&event)==1,"Queue actual animation input");}
void button(Uint32 window,bool down) {
  SDL_Event e{};e.type=down?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
  e.button.windowID=window;e.button.button=SDL_BUTTON_LEFT;e.button.state=down?SDL_PRESSED:SDL_RELEASED;push(e);
}
void reload(Uint32 window) {
  SDL_Event e{};e.type=SDL_KEYDOWN;e.key.windowID=window;
  e.key.keysym.scancode=SDL_SCANCODE_R;e.key.keysym.sym=SDLK_r;e.key.state=SDL_PRESSED;push(e);
  e.type=SDL_KEYUP;e.key.state=SDL_RELEASED;push(e);
}
void key(Uint32 window,SDL_Scancode code,bool down) {
  SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.windowID=window;
  e.key.keysym.scancode=code;e.key.keysym.sym=SDL_GetKeyFromScancode(code);
  e.key.state=down?SDL_PRESSED:SDL_RELEASED;push(e);
}
struct Sample {unsigned view{},player{};std::uint32_t tick{},animation{};float fraction{},frame{};bool first_person{},interpolated{};std::uint64_t positions{};bool sneaking{};float body_height{};std::uint64_t legs{};};
struct NativeLanHost {
  Cache cache;
  LevelContent content;
  MultiplayerMatch match;
  LanHost host;
  std::mutex error_mutex;
  std::exception_ptr error;
  std::jthread worker;
  static SlayerRules rules(const Cache &cache) {
    auto rules=slayer_rules({"Slayer",0,{{0x59,5}}});rules.weapon=cache.slayer_starting_weapon(5);return rules;
  }
  NativeLanHost(const MultiplayerOptions &options):
      cache(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe)),
      content(cache.load_content(0,true)),match(content,0,cache.spawns(),rules(cache)),
      host(match,0,multiplayer_fingerprint(options.map,cache.shared_directory(),0,
          cache.slayer_starting_weapon(5),options.gameplay_xbe)) {
    for(unsigned representation:{2u,3u})
      match.configure_replicated_first_person(cache.load_first_person_binding(cache.slayer_starting_weapon(5),representation));
    if(cache.original_data()->grenade_constants)
      match.configure_grenades(std::shared_ptr<const Cache>(&cache,[](const Cache*){}),*cache.original_data()->grenade_constants);
    if(cache.original_data()->melee_constants)
      match.configure_melee_animation(std::shared_ptr<const Cache>(&cache,[](const Cache*){}),*cache.original_data()->melee_constants);
    worker=std::jthread([this](std::stop_token stop) {
      try {
        std::array<Controls,16> commands{};auto next=std::chrono::steady_clock::now();
        bool started=false;
        while(!stop.stop_requested()) {
          host.receive(commands);
          // Resource preparation precedes the client's actual hello. As in
          // the native dedicated loop, admit a participant before asking the
          // original active-engine end query about this session.
          started=started || host.peers()!=0;
          if(started)match.step(commands);
          host.broadcast();
          next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
        }
      }catch(...) {std::lock_guard lock(error_mutex);error=std::current_exception();}
    });
  }
  void finish() {
    worker.request_stop();if(worker.joinable())worker.join();
    std::lock_guard lock(error_mutex);if(error)std::rethrow_exception(error);
  }
};
}
int main(int argc,char **argv) {
  try {
    if(argc<4 || argc>7)throw FormatError("Supply MAP SHARED_MAPS UPDATE_XBE [--split4] [--lan] [--elite]");
    if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
    SDL_SetMainReady();MultiplayerOptions options;
    options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
    options.width=640;options.height=480;options.local_players=1;options.starting_weapon=5;options.mute=true;
    bool lan{},elite{};
    for(int at=4;at<argc;++at) {
      if(std::string(argv[at])=="--split4")options.local_players=4;
      else if(std::string(argv[at])=="--lan")lan=true;
      else if(std::string(argv[at])=="--elite")elite=true;
      else throw FormatError("Unknown animation probe mode");
    }
    const unsigned representation=elite?3:2;
    for(unsigned i=0;i<options.local_players;++i) {
      const std::array<std::uint8_t,9> appearance{10,0,0,10,std::uint8_t(i?i%2:elite),0,0,0,0};
      options.selected_profiles.push_back(native_selected_player_configuration("Animation "+std::to_string(i),appearance,-1));
    }
    const Cache pose_cache(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe));
    const auto pose_content=pose_cache.load_content(0,true);
    const auto pose_model=pose_cache.load_model(pose_content.player_representations[representation].biped.datum,false);
    const auto ids=pose_cache.load_weapon_animation_identifiers(pose_cache.slayer_starting_weapon(5));
    const auto standing_idle=pose_cache.load_unit_weapon_action(pose_model.animation,0x06000086,ids[0],ids[1],0x0400000c);
    const auto sneaking_idle=pose_cache.load_unit_weapon_action(pose_model.animation,0x06000087,ids[0],ids[1],0x0400000c);
    const auto jump_clip=pose_cache.load_unit_weapon_action(pose_model.animation,0x06000086,ids[0],ids[1],0x0800001e);
    const auto landing_clip=pose_cache.load_unit_weapon_action(pose_model.animation,0x06000086,ids[0],ids[1],0x0900001f);
    std::array<AnimationClip,4> sneaking_gaits;
    constexpr std::array<std::uint32_t,4> activities{0x0a000014,0x09000015,0x09000016,0x0a000017};
    for(unsigned n=0;n<activities.size();++n)
      sneaking_gaits[n]=pose_cache.load_unit_weapon_action(pose_model.animation,0x06000087,ids[0],ids[1],activities[n]);
    std::vector<unsigned> leg_nodes,feet;std::optional<unsigned> head;
    for(unsigned n=0;n<pose_model.bones.size();++n) {
      const auto name=pose_cache.string_id(pose_model.bones[n].name);
      if(name.ends_with("thigh") || name.ends_with("calf") || name.ends_with("foot"))leg_nodes.push_back(n);
      if(name.ends_with("foot"))feet.push_back(n);
      if(name=="head")head=n;
    }
    check(head && feet.size()==2 && leg_nodes.size()>=6,"Actual body lost authored head/leg/foot nodes");
    std::unique_ptr<NativeLanHost> server;
    if(lan) {
      server=std::make_unique<NativeLanHost>(options);
      options.join="127.0.0.1:"+std::to_string(server->host.port());
    }
    options.validation=std::getenv("HALO2_ANIMATION_VALIDATION");
    options.desktop=std::make_shared<DesktopSession>();
    auto *window=options.desktop->window(options.width,options.height,false);const auto window_id=SDL_GetWindowID(window);
    SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
    std::vector<Sample> samples;MultiplayerState current;bool fired{},reloaded{},raised{},reload_requested{};
    bool melee{},melee_finished{};
    bool ran{},airborne{},crouched{},aimed_up{},aimed_down{},threw_grenade{},drew_throw_overlay{};
    std::set<unsigned> moving_throw_frames;
    std::set<std::uint64_t> melee_body_poses;
    std::set<std::uint64_t> airborne_leg_poses,landing_leg_poses;
    std::set<unsigned> airborne_frames,landing_frames;
    bool returned_from_landing{};
    std::array<std::set<std::uint64_t>,4> sneak_leg_poses;
    std::array<std::set<unsigned>,4> sneak_frames;
    std::vector<float> standing_heights,sneaking_heights;
    bool drew_sneaking_idle{},stood_up{};
    std::optional<unsigned> held_start;unsigned input_step{};
    options.input_observer=[&](std::uint32_t step,std::span<const Controls> controls,const MultiplayerState &state) {
      check(controls.size()==options.local_players,"Animation input changed local participants");current=state;input_step=step;
      if(std::getenv("HALO2_ANIMATION_INPUT_TRACE"))std::cerr<<"Animation input step="<<step<<" start="<<(held_start?int(*held_start):-1)
          <<" focus="<<(SDL_GetKeyboardFocus()==window)<<" fire="<<controls[0].fire<<" reload="<<controls[0].reload
          <<" forward="<<controls[0].forward<<" jump="<<controls[0].jump<<" crouch="<<controls[0].crouch
          <<" pitch="<<controls[0].pitch<<" tick="<<state.tick<<" connected="<<state.players[0].connected<<'\n';
      check(step<120 || held_start,"Animation test window never obtained actual SDL keyboard focus");
      if(held_start)check(SDL_GetKeyboardFocus()==window,"Animation test lost actual SDL keyboard focus during its stimulus");
      if(held_start && step==*held_start+132)key(window_id,SDL_SCANCODE_F,true);
      if(held_start && step==*held_start+133)key(window_id,SDL_SCANCODE_F,false);
      if(held_start && step==*held_start+6)button(window_id,true);
      if(held_start && step==*held_start+20)button(window_id,false);
      if(held_start && step==*held_start+24)reload(window_id);
      if(held_start && step==*held_start+2)key(window_id,SDL_SCANCODE_W,true);
      if(held_start && step==*held_start+7)key(window_id,SDL_SCANCODE_W,false);
      if(held_start && step==*held_start+10)key(window_id,SDL_SCANCODE_SPACE,true);
      if(held_start && step==*held_start+11)key(window_id,SDL_SCANCODE_SPACE,false);
      if(held_start && step==*held_start+60)key(window_id,SDL_SCANCODE_LSHIFT,true);
      if(held_start && step==*held_start+68)key(window_id,SDL_SCANCODE_LSHIFT,false);
      if(held_start && step==*held_start+94)key(window_id,SDL_SCANCODE_W,true);
      if(held_start && step==*held_start+95)key(window_id,SDL_SCANCODE_Q,true);
      if(held_start && step==*held_start+96)key(window_id,SDL_SCANCODE_Q,false);
      if(held_start && step==*held_start+102)key(window_id,SDL_SCANCODE_W,false);
      if(held_start && step==*held_start+200)key(window_id,SDL_SCANCODE_LSHIFT,true);
      if(held_start && step==*held_start+225)key(window_id,SDL_SCANCODE_W,true);
      if(held_start && step==*held_start+250){key(window_id,SDL_SCANCODE_W,false);key(window_id,SDL_SCANCODE_S,true);}
      if(held_start && step==*held_start+275){key(window_id,SDL_SCANCODE_S,false);key(window_id,SDL_SCANCODE_A,true);}
      if(held_start && step==*held_start+300){key(window_id,SDL_SCANCODE_A,false);key(window_id,SDL_SCANCODE_D,true);}
      if(held_start && step==*held_start+325)key(window_id,SDL_SCANCODE_D,false);
      if(held_start && step==*held_start+340)key(window_id,SDL_SCANCODE_LSHIFT,false);
      if(held_start && (step==*held_start+55 || step==*held_start+70)) {
        SDL_Event e{};e.type=SDL_MOUSEMOTION;e.motion.windowID=window_id;
        e.motion.yrel=step==*held_start+55?-300:600;push(e);
      }
      const auto &p=state.players[0];
      ran|=p.connected&&p.native_grounded&&p.native_forward>0;
      airborne|=p.connected&&!p.native_grounded;
      crouched|=p.connected&&p.native_grounded&&p.native_crouching;
      aimed_up|=p.connected&&p.pitch>.25f;aimed_down|=p.connected&&p.pitch<-.25f;
      threw_grenade|=p.connected&&p.grenade_action.animation_active;
      melee|=p.melee_action.active();melee_finished|=melee&&!p.melee_action.active();
      reload_requested|=controls[0].reload;
      fired|=state.players[0].connected && state.players[0].shot_tick && state.players[0].loaded<60;
      // A native LAN request and its authoritative snapshot arrive on distinct
      // simulation steps. Observe the actual later result, not same-step echo.
      reloaded|=reload_requested && state.players[0].connected && state.players[0].source_zoom_magazine_mode==1;
      for(unsigned i=1;i<options.local_players;++i)
        check(!controls[i].fire && !controls[i].reload &&
            (!state.players[i].connected || state.players[i].loaded==60),"Animation stimulus leaked into another view");
      if(held_start && step==*held_start+370 && options.screenshot.empty()){SDL_Event e{};e.type=SDL_QUIT;push(e);}
    };
    options.animation_observer=[&](unsigned view,unsigned player,const AnimationRenderObservation &sample) {
      check(view<options.local_players && player<options.local_players && !sample.vertices.empty(),"Actual pose submission lost a local model");
      if(view==0 && sample.first_person) {
        if(!raised){SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);raised=true;}
        if(!held_start && SDL_GetKeyboardFocus()==window)held_start=input_step;
      }
      if(sample.tick && !lan)check(sample.unit==current.players[player].source_unit && sample.tick==current.tick,"Pose submission used a stale unit/tick");
      if(lan && current.players[player].connected)check(sample.unit==current.players[player].source_unit &&
          sample.tick>=current.tick,"LAN pose submission used a stale current binding");
      if(player==0 && !sample.first_person && current.players[0].grenade_action.animation_active)
        drew_throw_overlay|=sample.animation!=current.players[0].grenade_action.clip;
      if(player==0 && !sample.first_person && current.players[0].grenade_action.animation_active && current.players[0].native_forward>0)
        moving_throw_frames.insert(unsigned(sample.frame));
      std::uint64_t hash=14695981039346656037ull;
      for(const auto &vertex:sample.vertices)for(float component:{vertex.position.x,vertex.position.y,vertex.position.z}) {
        check(std::isfinite(component),"Actual interpolated draw vertex is not finite");
        hash^=std::bit_cast<std::uint32_t>(component);hash*=1099511628211ull;
      }
      if(player==0 && !sample.first_person && current.players[0].melee_action.active())melee_body_poses.insert(hash);
      float body_height=0;std::uint64_t legs=14695981039346656037ull;
      if(!sample.first_person && player==0) {
        check(sample.body_nodes.size()>=pose_model.bones.size(),"Third-person draw lost its actual skinning node bank");
        float floor=sample.body_nodes[feet[0]].position.z;
        for(auto foot:feet)floor=std::min(floor,sample.body_nodes[foot].position.z);
        body_height=sample.body_nodes[*head].position.z-floor;
        // Vertical rotation components are independent of world translation
        // and root yaw. A moving capsule cannot fake an animated leg bank.
        for(auto node:leg_nodes)for(const auto axis:sample.body_nodes[node].basis) {
          legs^=std::uint32_t(std::int32_t(std::lround(axis.z*100000.f)));legs*=1099511628211ull;
        }
        const auto &p=current.players[0];
        const auto matches=[&](const AnimationClip &clip){return sample.graph==clip.source_graph && sample.animation==clip.source_index;};
        if(!p.native_grounded && matches(jump_clip)) {
          airborne_leg_poses.insert(legs);airborne_frames.insert(unsigned(sample.frame));
        }
        if(p.native_grounded && p.landing.kind==0 && matches(landing_clip)) {
          landing_leg_poses.insert(legs);landing_frames.insert(unsigned(sample.frame));
        }
        if(!landing_frames.empty() && p.native_grounded && p.landing.kind==-1 && matches(standing_idle))returned_from_landing=true;
        if(held_start && input_step>=*held_start+185 && input_step<*held_start+199 && !p.native_crouching && matches(standing_idle))
          standing_heights.push_back(body_height);
        if(held_start && input_step>=*held_start+218 && input_step<*held_start+225 && p.crouch==1 && matches(sneaking_idle)) {
          sneaking_heights.push_back(body_height);drew_sneaking_idle=true;
        }
        if(held_start && input_step>=*held_start+230 && input_step<*held_start+325 && p.native_grounded && p.native_crouching)
          for(unsigned n=0;n<sneaking_gaits.size();++n)if(matches(sneaking_gaits[n])) {
            sneak_leg_poses[n].insert(legs);sneak_frames[n].insert(unsigned(sample.frame));
          }
        if(held_start && input_step>=*held_start+360 && !p.native_crouching && matches(standing_idle))stood_up=true;
      }
      samples.push_back({view,player,sample.tick,sample.animation,sample.fraction,sample.frame,sample.first_person,sample.interpolated,hash,
          current.players[player].native_crouching,body_height,legs});
    };
    if(const auto *path=std::getenv("HALO2_ANIMATION_CAPTURE");path && !lan){options.screenshot=path;options.capture_tick=374;}
    const auto result=run_multiplayer(options);
    if(server)server->finish();
    if(const auto *path=std::getenv("HALO2_ANIMATION_TRACE")) {
      std::ofstream output(path);output<<"view player tick clip fraction frame first_person interpolated positions_hash sneaking body_height legs_hash\n"<<std::setprecision(9);
      for(const auto &sample:samples)output<<sample.view<<' '<<sample.player<<' '<<sample.tick<<' '<<sample.animation<<' '
          <<sample.fraction<<' '<<sample.frame<<' '<<sample.first_person<<' '<<sample.interpolated<<' '<<std::hex<<sample.positions<<std::dec
          <<' '<<sample.sneaking<<' '<<sample.body_height<<' '<<std::hex<<sample.legs<<std::dec<<'\n';
      check(bool(output),"Write actual submitted animation trace");
    }
    if(!fired || !reloaded)std::cerr<<"Actual animation inputs fired="<<fired<<" requested_reload="<<reload_requested
        <<" observed_reload="<<reloaded<<" loaded="<<current.players[0].loaded<<" tick="<<current.tick<<'\n';
    check(result==0 && fired && reloaded && samples.size()>20,"Actual SDL fire/reload and draw sequence missing");
    if(!(ran&&airborne&&crouched&&aimed_up&&aimed_down))std::cerr<<"Actual movement stimulus run="<<ran<<" airborne="<<airborne
        <<" crouch="<<crouched<<" aim_up="<<aimed_up<<" aim_down="<<aimed_down<<'\n';
    check(ran&&airborne&&crouched&&aimed_up&&aimed_down,"Actual SDL run/jump/crouch/aim stimulus did not reach the unit");
    check(melee&&melee_finished,"Actual SDL ordinary melee never started/finished its retained action");
    check(threw_grenade,"Actual SDL grenade request did not reach the admitted unit animation");
    if(options.local_players>1)check(drew_throw_overlay,"Grenade body overlay replaced the locomotion base channel");
    if(options.local_players>1)check(moving_throw_frames.size()>2,"Run cycle stopped while the grenade overlay was active");
    const Cache melee_cache(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe));
    const auto melee_binding=melee_cache.load_first_person_binding(melee_cache.slayer_starting_weapon(5),representation);
    bool drew_melee=false;
    for(const auto &clip:melee_binding.clock_clips)if(clip.activity==0x0e000607)
      for(const auto &s:samples)if(s.player==0&&s.first_person&&s.animation==clip.index)drew_melee=true;
    check(drew_melee,"Admitted melee never reached submitted first-person vertices");
    if(options.local_players>1)check(melee_body_poses.size()>3,"Admitted melee never advanced its submitted body overlay");
    unsigned intermediate{},body{};std::set<unsigned> views;
    for(const auto &sample:samples)if(sample.interpolated && sample.fraction>0 && sample.fraction<1) {
      if(sample.first_person){++intermediate;views.insert(sample.view);}else ++body;
    }
    check(intermediate>10 && views.size()==options.local_players,"Actual first-person draw still samples whole tick frames");
    if(options.local_players>1)check(body>10,"Actual remote body/hand-marker draw stayed on whole ticks");
    if(options.local_players>1) {
      const auto cache=Cache(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe));
      const auto content=cache.load_content(0,true);
      const auto model=cache.load_model(content.player_representations[representation].biped.datum,false);
      const auto ids=cache.load_weapon_animation_identifiers(cache.slayer_starting_weapon(5));
      const auto run=cache.load_unit_weapon_action(model.animation,0x06000086,ids[0],ids[1],0x0a000014);
      const auto jump=cache.load_unit_weapon_action(model.animation,0x06000086,ids[0],ids[1],0x0800001e);
      const auto land=cache.load_unit_weapon_action(model.animation,0x06000086,ids[0],ids[1],0x0900001f);
      bool drew_run=false,drew_jump=false,drew_land=false;
      for(const auto &s:samples)if(s.player==0&&!s.first_person){drew_run|=s.animation==run.source_index;drew_jump|=s.animation==jump.source_index;drew_land|=s.animation==land.source_index;}
      check(drew_run&&drew_jump,"Run/jump input reached simulation but body renderer submitted idle");
      check(drew_land,"Actual jump contact never selected the authored landing channel");
      if(airborne_frames.size()<=8 || airborne_leg_poses.size()<=8 || landing_frames.size()<=1 || landing_leg_poses.size()<=1)
        std::cerr<<"Jump frames="<<airborne_frames.size()<<" leg_poses="<<airborne_leg_poses.size()
            <<" landing frames="<<landing_frames.size()<<" leg_poses="<<landing_leg_poses.size()<<'\n';
      check(airborne_frames.size()>8 && airborne_leg_poses.size()>8,
          "Third-person jump submitted a frozen airborne body pose");
      check(landing_frames.size()>1 && landing_leg_poses.size()>1 && returned_from_landing,
          "Third-person landing did not animate and recover to the standing body");
      std::cout<<"PASS representation="<<representation<<" jump frames="<<airborne_frames.size()
          <<" animated leg poses="<<airborne_leg_poses.size()<<" landing frames="<<landing_frames.size()
          <<" animated leg poses="<<landing_leg_poses.size()<<", recovery to idle\n";
      check(drew_sneaking_idle && stood_up && !standing_heights.empty() && !sneaking_heights.empty(),
          "Sneaking/standing input never reached its actual third-person stance");
      check(*std::max_element(sneaking_heights.begin(),sneaking_heights.end())<*std::min_element(standing_heights.begin(),standing_heights.end()),
          "Third-person sneaking retained standing body height");
      for(unsigned n=0;n<sneaking_gaits.size();++n) {
        if(sneak_frames[n].size()<=8 || sneak_leg_poses[n].size()<=8)
          std::cerr<<"Sneak direction="<<n<<" frames="<<sneak_frames[n].size()<<" leg_poses="<<sneak_leg_poses[n].size()<<'\n';
        check(sneak_frames[n].size()>8 && sneak_leg_poses[n].size()>8,
            "Directional third-person sneak clock/legs remained idle");
      }
      std::cout<<"PASS representation="<<representation<<" rendered sneak height="<<sneaking_heights.front()
          <<" standing height="<<standing_heights.front()<<", four directional sneak leg cycles and stand-up\n";
    }
    std::cout<<"PASS actual SDL "<<(lan?"native UDP ":"")<<"fire/reload: "<<samples.size()<<" posed draw submissions, "<<intermediate
        <<" fractional first-person submissions, "<<body<<" fractional body submissions, "<<views.size()<<" independent local views\n";
    std::cout<<"PASS actual run/jump/landing/crouch/aim, moving grenade and ordinary melee inputs and submitted poses; physical Xbox/framebuffer parity remains separate\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
