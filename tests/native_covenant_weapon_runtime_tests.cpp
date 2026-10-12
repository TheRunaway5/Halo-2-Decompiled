#define SDL_MAIN_HANDLED
#include "halo2/animation_presentation.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer_audio.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <set>
using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
void key(Uint32 window,SDL_Scancode scan,bool down) {
  SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.windowID=window;
  e.key.keysym.scancode=scan;e.key.state=down?SDL_PRESSED:SDL_RELEASED;
  check(SDL_PushEvent(&e)==1,"Queue actual SDL pickup input");
}
void fire(Uint32 window,bool down) {
  SDL_Event e{};e.type=down?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
  e.button.windowID=window;e.button.button=SDL_BUTTON_LEFT;
  e.button.state=down?SDL_PRESSED:SDL_RELEASED;
  check(SDL_PushEvent(&e)==1,"Queue actual SDL fire input");
}
}
int main(int argc,char **argv)try {
  check(argc==5 || argc==6 || argc==7,"Supply MAP SHARED XBE needler|sword|shotgun [CAPTURE [idle|ready|ignite|lit|world|fire]]");
  if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]))return 77;
  const bool shotgun=std::string(argv[4])=="shotgun";
  const bool sword=std::string(argv[4])=="sword";
  check(sword||shotgun||std::string(argv[4])=="needler","Unknown weapon probe");
  const bool ammunition=argc==7 && std::string(argv[6])=="ammo";
  check(!ammunition || (!sword && !shotgun),"Ammo probe requires Needler");
  const bool firing_capture=argc==7 && std::string(argv[6])=="fire";
  check(!firing_capture || (!sword && !shotgun),"Firing capture requires Needler");
  const bool world=argc==7 && std::string(argv[6])=="world";
  const bool idle=argc==7 && std::string(argv[6])=="idle";
  const bool ignite=argc==7 && std::string(argv[6])=="ignite";
  const bool lit=argc==7 && std::string(argv[6])=="lit";
  const bool ready=argc==7 && (std::string(argv[6])=="ready" || ignite || lit);
  check(argc!=7 || idle || world || ready || ammunition || firing_capture,"Unknown capture phase");
  // This probe supplies keyboard/mouse events. Physical controller input is
  // independent host activity and must not switch/fire the tested inventory.
  SDL_SetHintWithPriority(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0xffff/0xffff",SDL_HINT_OVERRIDE);
  SDL_SetMainReady();MultiplayerOptions o;o.map=argv[1];o.shared_maps=argv[2];o.gameplay_xbe=argv[3];
  o.width=800;o.height=600;o.local_players=1;o.starting_weapon=5;o.mute=false;
  o.input_instances={-1}; // This SDL input probe has no controller owner.
  o.validation=std::getenv("HALO2_ANIMATION_VALIDATION");
  if(const auto *ui=std::getenv("HALO2_TEST_HUD_UI_ASSETS"))o.ui_assets=ui;
  o.prepared_world=std::make_shared<MultiplayerPreparedWorld>(o);
  const auto &c=o.prepared_world->content;const auto &cache=*o.prepared_world->cache;
  const auto weapon=std::find_if(c.weapons.begin(),c.weapons.end(),[&](const auto &p){
    return p.second.name.ends_with(sword?"\\energy_blade":shotgun?"\\shotgun":"\\needler");
  });check(weapon!=c.weapons.end(),"Authored target weapon absent");
  const auto target=weapon->first;const auto &w=weapon->second;
  const auto row=std::find_if(c.netgame_equipment.begin(),c.netgame_equipment.end(),[&](const auto &r){
    return r.items.size()==1 && r.items[0].definition==target;
  });check(row!=c.netgame_equipment.end(),"Authored target pickup row absent");
  const auto definition=cache.load_first_person_binding(target,2);
  const auto activity=ready?0x05000024u:sword?0x0e000607u:0x06000006u;
  const auto action=std::find_if(definition.clock_clips.begin(),definition.clock_clips.end(),
      [&](const auto &clip){return clip.activity==activity;});
  check(action!=definition.clock_clips.end(),"Authored attack clip absent");
  std::set<std::uint32_t> attack_sounds;
  for(const auto &event:action->events)if(event.type==1)attack_sounds.insert(event.tag);
  o.desktop=std::make_shared<DesktopSession>();auto *window=o.desktop->window(800,600,false);
  const auto window_id=SDL_GetWindowID(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  const auto slot=o.prepared_world->local_slots.at(0);
  // Isolate the weapon/renderer loop from the still-unfinished native mover.
  // This is explicit diagnostic placement; no original physics is claimed.
  Spawn at;at.position=row->position;at.position.x-=.2f;at.bsp=0;at.facing=0;
  std::optional<unsigned> requested,picked,empty_tick;bool refilled{};unsigned tick{},draws{},materials{},sounds{};
  unsigned ignition_particles{},ready_sounds{},hidden_blade_draws{},lit_blade_draws{};
  unsigned firing_particles{},firing_sounds{},fired_rounds{};
  std::optional<unsigned> ready_sound_tick;
  unsigned projectile_draws{};std::set<std::uint32_t> projectile_owners;
  std::uint32_t owner=null_datum;bool posed{},shader{},blade{},carried_pose{};
  std::set<float> frames;MultiplayerState state;
  std::set<std::uint64_t> pose_hashes;
  std::set<float> ammunition_frames;float full_needle_scale=-1,used_needle_scale=-1;
  const auto weapon_model=cache.load_model(definition.weapon_model,false);
  std::optional<unsigned> needle_node;
  if(!sword && !shotgun){
    const auto &ammo=*definition.ammunition[0];
    for(unsigned n=0;n<weapon_model.bones.size();++n){
      const auto at=std::find(ammo.nodes.begin(),ammo.nodes.end(),weapon_model.bones[n].name);
      if(at!=ammo.nodes.end() && (ammo.components[at-ammo.nodes.begin()]&4u)){needle_node=n;break;}
    }
    check(needle_node.has_value(),"Actual Needler ammunition has no scaled needle node");
  }
  std::optional<std::uint64_t> observed_shot;
  o.input_observer=[&](unsigned t,std::span<const Controls> controls,const MultiplayerState &s){
    tick=t;state=s;const auto &p=s.players[slot];
    if(world) {
      at.position.x=row->position.x;at.position.y=row->position.y-1.2f;at.facing=1.5707963267948966f;
      at.position.z=row->position.z-c.player.camera_height;
      o.prepared_world->match.debug_request({DebugGameRequest::Teleport,slot},&at);
      return;
    }
    if(!requested && t>=35 && SDL_GetKeyboardFocus()==window) {
      requested=t;
      o.prepared_world->match.debug_request({DebugGameRequest::Teleport,slot},&at);key(window_id,SDL_SCANCODE_E,true);
    }
    if(requested && !picked)
      o.prepared_world->match.debug_request({DebugGameRequest::Teleport,slot},&at);
    if(!picked && p.weapon==target) {picked=t;owner=p.source_weapon;key(window_id,SDL_SCANCODE_E,false);
      std::cout<<"Pickup observation: tick="<<t<<" loaded="<<p.loaded<<" shot_tick="<<p.shot_tick
          <<" rate="<<w.rounds_per_second<<" recovery="<<w.fire_recovery<<" trigger="<<w.trigger_behavior
          <<" effect="<<std::hex<<p.weapon_effect.effect<<" effect_weapon="<<p.weapon_effect.weapon<<std::dec
          <<" effect_tick="<<p.weapon_effect.tick<<" effect_serial="<<p.weapon_effect.serial<<'\n';
      if(!o.screenshot.empty()) {
        if(ammunition)o.capture_tick=400;else if(ready)o.capture_tick=t+(ignite?19:lit?24:10);else if(idle)o.capture_tick=t+48;else o.capture_tick=t+(firing_capture?67:73);
      }
    }
    if(ready && (controls[0].fire || controls[0].fire_secondary))std::cerr<<"Unexpected input: tick="<<t<<" fire="<<controls[0].fire<<" secondary="<<controls[0].fire_secondary<<" controllers="<<SDL_NumJoysticks()<<'\n';
    if(ready)check(!controls[0].fire && !controls[0].fire_secondary,"Ready-only probe received an unrequested attack");
    if(picked) {
      check(!p.dead && p.weapon==target && p.source_weapon==owner,
          "Weapon probe lost its actual picked owner before the requested attack");
      if(!observed_shot || *observed_shot!=p.shot_tick) {
        if(observed_shot && p.shot_tick!=0)++fired_rounds;
        observed_shot=p.shot_tick;
        std::cout<<"Shot observation: tick="<<t<<" shot_tick="<<p.shot_tick<<" loaded="<<p.loaded
            <<" fire="<<controls[0].fire<<'\n';
      }
      o.prepared_world->match.debug_request({DebugGameRequest::Teleport,slot},&at);
      const auto elapsed=t-*picked;
      if(!ready && elapsed==60){
        std::cout<<"Request actual SDL fire: tick="<<t<<" focus="<<(SDL_GetKeyboardFocus()==window)
          <<" binding="<<o.desktop->keyboard_settings.bindings[unsigned(PcAction::Fire)]<<'\n';
        fire(window_id,true);
      }
      if(!ready && elapsed>60 && elapsed<74)
        std::cout<<"Fire input observation: tick="<<t<<" fire="<<controls[0].fire
          <<" focus="<<(SDL_GetKeyboardFocus()==window)<<'\n';
      if(!ready && !ammunition && elapsed==72)fire(window_id,false);
      if(ammunition && !empty_tick && p.loaded==0) {
        empty_tick=t;fire(window_id,false);key(window_id,SDL_SCANCODE_R,true);
        if(!o.screenshot.empty())o.capture_tick=s.tick;
      }
      if(ammunition && empty_tick && t==*empty_tick+1)key(window_id,SDL_SCANCODE_R,false);
      if(ammunition && empty_tick && t>*empty_tick+70 && p.loaded==w.capacity) {
        refilled=true;SDL_Event q{};q.type=SDL_QUIT;check(SDL_PushEvent(&q)==1,"Exit completed Needler refill probe");
      }
      if(!ammunition && elapsed==110){SDL_Event q{};q.type=SDL_QUIT;check(SDL_PushEvent(&q)==1,"Exit weapon probe");}
    }
    check(t<(ammunition?450u:240u),"Actual SDL covenant pickup/attack timed out");
  };
  o.animation_observer=[&](unsigned,unsigned,const AnimationRenderObservation &sample){
    if(SDL_GetKeyboardFocus()!=window){SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);}
    if(sample.first_person && sample.weapon==owner && !sample.vertices.empty())carried_pose=true;
    if(sample.first_person && sample.weapon==owner && !sword && !shotgun) {
      const auto &p=state.players[slot];
      check(sample.ammunition_animation==definition.ammunition[0]->source_index,
          "Live Needler pose lost its authored ammunition overlay");
      const auto expected=first_person_ammunition_frame(p.loaded,p.ammunition_available,w.capacity,
          std::bit_cast<std::int16_t>(std::uint16_t(state.tick-p.reload_tick)),w.weapon_type,
          state.players[slot].first_person_clock.current.activity,definition.ammunition[0]->frame_count);
      check(sample.ammunition_frame==expected,"Live Needler ammunition pose disagrees with authoritative magazine");
      ammunition_frames.insert(sample.ammunition_frame);
      check(sample.weapon_nodes.size()==weapon_model.bones.size(),"Live Needler pose lost its shared node bank");
      if(p.loaded==w.capacity)full_needle_scale=sample.weapon_nodes[*needle_node].scale;
      if(p.loaded<w.capacity)used_needle_scale=used_needle_scale<0?sample.weapon_nodes[*needle_node].scale:
          std::min(used_needle_scale,sample.weapon_nodes[*needle_node].scale);
    }
    if(sample.first_person && sample.weapon==owner && sample.animation==action->index) {
      posed=true;frames.insert(sample.frame);++draws;check(!sample.vertices.empty(),"Actual attack pose has no vertices");
      std::uint64_t hash=1469598103934665603ull;
      for(const auto &v:sample.vertices)for(float x:{v.position.x,v.position.y,v.position.z}) {
        hash^=std::bit_cast<std::uint32_t>(x);hash*=1099511628211ull;
      }
      pose_hashes.insert(hash);
    }
  };
  o.weapon_material_observer=[&](auto handle,auto datum,const Material &m,const ShaderInput &input){
    if(datum!=target || (!world && (!picked || handle!=owner)))return;
    if(world) {
      check(std::any_of(state.world_weapons.begin(),state.world_weapons.end(),[&](const auto &w){return w.handle==handle;}),"World shader has no actual live pickup owner");
      check(input(0x09000568u)==1.f,"World sword lost original turned_on input");
      ++materials;shader=true;return;
    }
    ++materials;
    if(sword && (m.name.ends_with("\\energy_blade_top")||m.name.ends_with("\\energy_blade_bottom"))) {
      // The pickup callback observes the completed creation tick, including
      // the first original1021D0 decrement21->20. Compare against the
      // independent executable timeline, not the native state producer.
      const bool on=tick-*picked>=20;
      check(input(0x09000568u)==float(on),"Carried Sword did not follow original21-tick ready gate");
      check((m.forward_program.words[43]!=0)==on,"Sword blade combiner ignored ready state");
      if(on)++lit_blade_draws;else ++hidden_blade_draws;
      shader=true;blade|=m.forward_program.words[43]!=0;
    }
    if(!sword) {
      const auto expected=float(state.players[slot].loaded)/float(w.capacity);
      check(input(0x1200056au)==expected,"Carried Needler lost original primary_ammunition input");
      if(!shotgun)check(input(0x04000b93u)==expected,"Carried Needler lost authored ammunition function");shader=true;
    }
  };
  o.world_weapon_shader_input_observer=[&](auto handle,auto datum,auto name,float value) {
    if(!world || datum!=target || name!=0x09000568u)return;
    check(std::any_of(state.world_weapons.begin(),state.world_weapons.end(),[&](const auto &w){return w.handle==handle;}),
        "World shader has no actual live pickup owner");
    check(value==1.f,"World sword lost original turned_on input");shader=true;++materials;
  };
  o.audio_observer=[&](const MultiplayerAudioSubmission &s){
    if(s.owner==owner && attack_sounds.contains(s.sound))++sounds;
    if(!sword && !shotgun && s.sound==w.sound) {
      check(s.owner==owner,"Needler firing sound bypassed its actual picked effect owner");
      check(s.frames>0,"Needler firing sound submitted an empty PCM packet");++firing_sounds;
    }
    if(s.owner==owner && s.sound==0xe2fc0187u) {
      ++ready_sounds;ready_sound_tick=tick;
      check(picked && tick-*picked==15,"Sword ignition sound did not follow original15-tick event delay");
    }
  };
  o.effect_observer=[&](unsigned,unsigned t,std::span<const ParticleDraw> particles) {
    if(!sword && !shotgun && picked && state.players[slot].shot_tick)firing_particles+=unsigned(particles.size());
    if(sword && picked && t-*picked<15)check(particles.empty(),"Sword ignition particles appeared before authored delay");
    if(sword && picked && t-*picked>=15 && t-*picked<40) {
      if(!ignition_particles && !particles.empty())std::cout<<"First ignition particle observation: tick="<<t<<" relative="<<t-*picked<<'\n';
      ignition_particles+=unsigned(particles.size());
    }
  };
  o.weapon_projectile_draw_observer=[&](unsigned,std::uint32_t,std::uint32_t handle,
      std::uint32_t definition,std::span<const Vertex> vertices) {
    if(definition!=w.projectile)return;
    check(!vertices.empty(),"Actual fired Needler shard submitted empty geometry");
    check(std::any_of(state.weapon_projectiles.begin(),state.weapon_projectiles.end(),
        [&](const auto &p){return p.datum==handle && p.definition==definition;}),
        "Actual fired shard draw has no live salted owner");
    ++projectile_draws;projectile_owners.insert(handle);
  };
  if(argc>=6 && *argv[5]){o.screenshot=argv[5];o.capture_tick=(idle||world)?90:115;}
  check(run_multiplayer(o)==0,"Actual covenant weapon runtime failed");
  if(!o.screenshot.empty())check(std::filesystem::exists(o.screenshot),"Requested weapon capture was not written");
  if(world) {
    check(shader && materials>10,"World sword never received its own source shader input");
    std::cout<<"PASS sword world: actual pickup owner, "<<materials<<" material observations\n";return 0;
  }
  check(picked&&carried_pose,"Actual pickup did not submit its carried pose");
  check(shader&&materials>10,"Actual carried materials did not receive source inputs");
  if(idle) {
    if(sword)check(blade,"Actual idle sword blade combiner remained zero");
    std::cout<<"PASS "<<argv[4]<<" idle: actual SDL pickup, carried vertices, "
        <<materials<<" material observations\n";
    return 0;
  }
  if(ammunition) {
    check(empty_tick && (refilled || !o.screenshot.empty()),"Live Needler did not complete its actual firing/empty/reload cycle");
    check(ammunition_frames.size()>1 && full_needle_scale>=0 && used_needle_scale>=0 && full_needle_scale!=used_needle_scale,
        "Live Needler consumed ammo without retracting needle geometry");
    std::cout<<"Needler ammunition observations: frames="<<ammunition_frames.size()<<" needle scale full="<<full_needle_scale<<" used="<<used_needle_scale<<'\n';
  }
  check(picked&&posed&&frames.size()>4&&draws>10,"Actual attack did not submit advancing authored poses");
  check(pose_hashes.size()>4,"Authored attack clock advanced without moving the rendered weapon vertices");
  if(ready){
    std::cout<<"Ready observations: hidden="<<hidden_blade_draws<<" lit="<<lit_blade_draws
        <<" sounds="<<ready_sounds<<" particles="<<ignition_particles<<'\n';
    if(sword) {
      check(hidden_blade_draws>0,"Sword ready sequence never hid the blade");
      const bool full=o.screenshot.empty() || lit;
      if(full)check(lit_blade_draws>0 && ready_sounds==1 && ignition_particles>0,
          "Sword ready sequence lost ignition sound/particles or revealed blade");
      else if(ignite)check(!lit_blade_draws && ready_sounds==1 && ignition_particles>0,
          "Sword ignition capture lost its authored burst or sound");
      else check(!lit_blade_draws && !ready_sounds && !ignition_particles,
          "Sword hilt capture played ignition too early");
    }
    std::cout<<"PASS "<<argv[4]<<" ready: "<<frames.size()<<" authored frames, "<<pose_hashes.size()
        <<" distinct poses, "<<hidden_blade_draws<<" hidden / "<<lit_blade_draws<<" lit blade draws, "
        <<ready_sounds<<" ready sound(s), "<<ignition_particles<<" ignition particle draws\n";return 0;
  }
  if(sword){check(blade,"Actual sword blade combiner remained zero");check(sounds>0,"Actual sword strike sound was not accepted by SDL audio");}
  else if(shotgun)check(state.players[slot].shot_tick>0 && state.players[slot].loaded<w.capacity,"Actual shotgun did not fire or consume a shell");
  else {
    check(projectile_draws>4 && projectile_owners.size()>1,"Fired Needler shards never reached actual renderer submission");
    check(firing_particles>0,"Actual Needler shot never submitted its authored muzzle/smoke particles");
    check(firing_sounds==fired_rounds && fired_rounds>1,"Needler did not submit exactly one authored firing sound per shot");
    std::cout<<"Needler live effects: "<<firing_particles<<" particle draws, "<<firing_sounds<<" owned sounds / "<<fired_rounds<<" fired rounds\n";
  }
  std::cout<<"PASS "<<argv[4]<<": actual SDL pickup/fire, "<<draws<<" attack draws, "<<frames.size()
      <<" frames, "<<materials<<" material observations, "<<sounds<<" authored attack sounds, "
      <<projectile_draws<<" shard draws / "<<projectile_owners.size()<<" owners\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
