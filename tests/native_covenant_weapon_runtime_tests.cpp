#define SDL_MAIN_HANDLED
#include "halo2/animation_presentation.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/collision.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/source_light_volume_draw.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/hud.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
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
  check(argc==5 || argc==6 || argc==7,"Supply MAP SHARED XBE WEAPON [CAPTURE [idle|ready|ignite|lit|world|fire]]");
  const bool mcc=std::string(argv[3])=="native-mcc";
  if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]))return 77;
  const auto *representation_option=std::getenv("HALO2_TEST_REPRESENTATION");
  const unsigned representation=representation_option?unsigned(std::stoul(representation_option)):2u;
  check(representation==2 || representation==3,"Unsupported actual multiplayer representation");
  const auto *players_option=std::getenv("HALO2_TEST_LOCAL_PLAYERS");
  const unsigned local_players=players_option?unsigned(std::stoul(players_option)):1u;
  check(local_players==1 || local_players==2 || local_players==4,"Unsupported diagnostic local-view count");
  const bool shotgun=std::string(argv[4])=="shotgun";
  const bool sword=std::string(argv[4])=="sword";
  const bool plasma_pistol=std::string(argv[4])=="plasma_pistol";
  const bool charge_cycle=argc==7 && std::string(argv[6])=="charge";
  const bool pistol_tap=argc==7 && std::string(argv[6])=="tap";
  check(!charge_cycle || plasma_pistol,"Charge probe requires plasma pistol");
  check(!pistol_tap || plasma_pistol,"Tap probe requires plasma pistol");
  const bool needler=std::string(argv[4])=="needler";
  const bool material_only=std::getenv("HALO2_TEST_WEAPON_MATERIAL_ONLY")!=nullptr;
  const bool native_motion=std::getenv("HALO2_TEST_ALLOW_NATIVE_MOTION")!=nullptr;
  const auto *source_trace_path=std::getenv("HALO2_TEST_WEAPON_STATE_REPORT");
  const bool verify_source_shot_tick=std::getenv("HALO2_TEST_VERIFY_SOURCE_SHOT_TICK")!=nullptr;
  const bool clear_firing_lane=std::getenv("HALO2_TEST_CLEAR_FIRING_LANE")!=nullptr;
  nlohmann::json source_trace=nlohmann::json::array();
  const std::set<std::string> ordinary{"magnum","battle_rifle","covenant_carbine",
      "sniper_rifle","brute_shot","rocket_launcher","plasma_pistol","plasma_rifle","brute_plasma_rifle","beam_rifle"};
  check(sword||shotgun||needler||ordinary.contains(argv[4]),"Unknown weapon probe");
  const bool ammunition=argc==7 && std::string(argv[6])=="ammo";
  const bool heat_cycle=argc==7 && std::string(argv[6])=="heat";
  check(!ammunition || needler,"Ammo probe requires Needler");
  const bool firing_capture=argc==7 && std::string(argv[6])=="fire";
  check(!firing_capture || needler,"Firing capture requires Needler");
  const bool world=argc==7 && std::string(argv[6])=="world";
  const bool idle=argc==7 && std::string(argv[6])=="idle";
  const bool ignite=argc==7 && std::string(argv[6])=="ignite";
  const bool lit=argc==7 && std::string(argv[6])=="lit";
  const bool ready=argc==7 && (std::string(argv[6])=="ready" || ignite || lit);
  check(argc!=7 || idle || world || ready || ammunition || firing_capture || heat_cycle || charge_cycle || pistol_tap,"Unknown capture phase");
  // This probe supplies keyboard/mouse events. Physical controller input is
  // independent host activity and must not switch/fire the tested inventory.
  SDL_SetHintWithPriority(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0xffff/0xffff",SDL_HINT_OVERRIDE);
  SDL_SetMainReady();MultiplayerOptions o;o.map=argv[1];o.shared_maps=argv[2];
  if(!mcc)o.gameplay_xbe=argv[3];
  o.width=800;o.height=600;o.local_players=local_players;o.starting_weapon=5;o.mute=false;
  // Finite native SDL probe: one game tick and a declared mid-tick sample
  // per rendered frame. Short authored clips must not depend on host load.
  o.frames=heat_cycle?700u:ammunition?450u:charge_cycle?320u:240u;
  o.reference_tick_cadence=true;o.reference_render_fraction=.5f;
  // The standalone multi-view path assigns keyboard input to view0 and
  // leaves absent controllers neutral. Only one view may own the keyboard.
  if(local_players==1)o.input_instances={-1};
  o.validation=std::getenv("HALO2_ANIMATION_VALIDATION");
  if(const auto *ui=std::getenv("HALO2_TEST_HUD_UI_ASSETS"))o.ui_assets=ui;
  if(representation==3) {
    auto profile=source_default_player_configuration();profile[0x44]=1;o.selected_profiles.assign(local_players,profile);
  }
  {
    Cache placements(o.map,o.shared_maps,mcc?std::shared_ptr<const OriginalData>{}:OriginalData::load_xbe(o.gameplay_xbe));
    check(!mcc || placements.is_mcc(),"Weapon runtime testing requires an MCC cache");
    o.native_spawn_inputs=placements.spawns();
    for(auto &spawn:o.native_spawn_inputs)spawn.facing=0; // Declared stationary multi-view input, before birth.
  }
  o.prepared_world=std::make_shared<MultiplayerPreparedWorld>(o);
  const auto &c=o.prepared_world->content;const auto &cache=*o.prepared_world->cache;
  const auto weapon=std::find_if(c.weapons.begin(),c.weapons.end(),[&](const auto &p){
    return p.second.name.ends_with(sword?std::string("\\energy_blade"):std::string("\\")+argv[4]);
  });check(weapon!=c.weapons.end(),"Authored target weapon absent");
  const auto target=weapon->first;const auto &w=weapon->second;
  const bool energy=w.energy.has_value();
  check(!heat_cycle || energy,"Heat cycle requires a recovered energy definition");
  const auto row=std::find_if(c.netgame_equipment.begin(),c.netgame_equipment.end(),[&](const auto &r){
    return r.items.size()==1 && r.items[0].definition==target;
  });check(row!=c.netgame_equipment.end(),"Authored target pickup row absent");
  const auto definition=cache.load_first_person_binding(target,int(representation));
  const auto activity=ready?0x05000024u:sword?0x0e000607u:
      (charge_cycle || (w.barrel_timing && w.barrel_timing->index==1))?0x06000007u:0x06000006u;
  const auto firing_sound=charge_cycle?w.barrel_views.at(1)->sound:w.sound;
  const auto fired_projectile=charge_cycle?w.barrel_views.at(1)->projectile:w.projectile;
  nlohmann::json projectile_births=nlohmann::json::array();
  nlohmann::json projectile_retirements=nlohmann::json::array();
  std::set<std::uint32_t> traced_projectiles;
  std::vector<std::size_t> projectile_birth_pending;
  std::optional<SourcePhysicsEntryObserverBinding> projectile_birth_observer;
  if(std::getenv("HALO2_TEST_TRACE_PROJECTILE_BIRTHS")) {
    auto *source=o.prepared_world->match.source_object_runtime();
    check(source && source->owns_physics(),"Projectile birth trace requires actual source storage");
    const auto &memory=source->memory();
    const auto word=[&memory](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    // Actual B9B70 takes its original C4-byte object request at ESP+4 and
    // returns the created full datum in EAX. The private validation snapshot
    // adds read-only entry/return observation inside its existing owners.
    projectile_birth_observer.emplace(memory,[&,word](bool before,std::uint32_t entry,const auto &abi) {
      if(entry==0xba490u) {
        if(!before)return;
        const auto object=word(abi.gp[4]+4u);if(!traced_projectiles.contains(object))return;
        const auto pool=word(0x4ecbd0u),headers=word(pool+0x44u),stride=word(pool+0x24u);
        check(stride==12,"Actual retirement pool stride changed");
        const auto header=headers+(object&65535u)*stride,body=word(header+8u);
        check((word(header)&65535u)==(object>>16) && body,
            "Actual retirement request lost its full projectile generation");
        projectile_retirements.push_back({{"source_tick",word(word(0x51df54u)+8u)},
            {"object",object},{"caller_return",word(abi.gp[4])},{"parent",word(body+0x14u)},
            {"definition",word(body)},{"flags",word(body+0x12cu)},
            {"timer_bits",word(body+0x158u)},{"arming_bits",word(body+0x160u)},
            {"traveled_bits",word(body+0x168u)}});
        return;
      }
      if(entry!=0xb9b70u)return;
      if(before) {
        const auto request=word(abi.gp[4]+4u);
        if(word(request)!=fired_projectile){projectile_birth_pending.push_back(SIZE_MAX);return;}
        std::array<std::uint32_t,49> descriptor{};
        const auto bytes=memory.view(request,sizeof(descriptor));
        std::memcpy(descriptor.data(),bytes.data(),bytes.size());
        projectile_birth_pending.push_back(projectile_births.size());
        projectile_births.push_back({{"source_tick",word(word(0x51df54u)+8u)},
            {"caller_return",word(abi.gp[4])},{"request_words",descriptor},{"completed",false}});
      }else {
        check(!projectile_birth_pending.empty(),"Unpaired actual object construction return");
        const auto index=projectile_birth_pending.back();projectile_birth_pending.pop_back();if(index==SIZE_MAX)return;
        auto &row=projectile_births.at(index);row["completed"]=true;row["returned_object"]=abi.gp[0];
        if(abi.gp[0]==null_datum)return;
        traced_projectiles.insert(abi.gp[0]);
        const auto pool=word(0x4ecbd0u),stride=word(pool+0x24u),headers=word(pool+0x44u);
        check(stride==12,"Actual object constructor pool stride changed");
        const auto header=headers+(abi.gp[0]&65535u)*stride,body=word(header+8u);
        check((word(header)&65535u)==(abi.gp[0]>>16) && body,
            "Completed actual object constructor returned an uncommitted full datum");
        row["definition"]=word(body);row["parent"]=word(body+0x14u);
        row["type"]=std::to_integer<unsigned>(memory.view(header+3u,1)[0]);
        std::array<std::uint32_t,3> position{},velocity{};
        const auto position_bytes=memory.view(body+0x64u,sizeof(position));
        const auto velocity_bytes=memory.view(body+0x88u,sizeof(velocity));
        std::memcpy(position.data(),position_bytes.data(),position_bytes.size());
        std::memcpy(velocity.data(),velocity_bytes.data(),velocity_bytes.size());
        row["position_words"]=position;row["velocity_words"]=velocity;
      }
    });
  }
  const auto action=std::find_if(definition.clock_clips.begin(),definition.clock_clips.end(),
      [&](const auto &clip){return clip.activity==activity;});
  check(action!=definition.clock_clips.end(),"Authored attack clip absent");
  std::set<std::uint32_t> attack_sounds;
  for(const auto &event:action->events)if(event.type==1)attack_sounds.insert(event.tag);
  o.desktop=std::make_shared<DesktopSession>();auto *window=o.desktop->window(800,600,false);
  std::cout<<"Native SDL video driver: "<<SDL_GetCurrentVideoDriver()<<'\n';
  SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);
  const auto window_id=SDL_GetWindowID(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  const auto slot=o.prepared_world->local_slots.at(0);
  // Isolate the weapon/renderer loop from the still-unfinished native mover.
  // This is explicit diagnostic placement; no original physics is claimed.
  Spawn at;at.position=row->position;at.position.x-=.2f;at.bsp=0;at.facing=0;
  std::optional<Spawn> clear_spawn;unsigned clear_spawn_index{};float selected_clearance{};
  if(clear_firing_lane) {
    check(std::string(argv[4])=="plasma_rifle","Clear-lane acceptance currently owns the plasma-rifle fixture");
    const CollisionWorld selection_geometry(c.collision);const auto spawns=cache.spawns();
    // Host rays select an input scene only. Original object/projectile owners
    // still decide actual birth, movement, contacts and retirement.
    for(unsigned n=0;n<spawns.size();++n) {
      const auto &candidate=spawns[n];if(candidate.bsp!=0)continue;
      float clearance=20.f;
      for(float side:{-c.player.radius,0.f,c.player.radius})
        for(float height:{c.player.height*.25f,c.player.height*.5f,c.player.camera_height,c.player.height})
          if(const auto hit=selection_geometry.raycast(candidate.position+Vec3{0,side,height},{1,0,0},20.f))
            clearance=std::min(clearance,hit->distance);
      if(clearance>selected_clearance){selected_clearance=clearance;clear_spawn=candidate;clear_spawn_index=n;}
    }
    check(clear_spawn && selected_clearance>=8.f,"No authored spawn has the requested clear firing lane");
    clear_spawn->facing=0;
  }
  std::optional<unsigned> requested,picked,empty_tick;bool refilled{};unsigned tick{},draws{},materials{},sounds{};
  unsigned ignition_particles{},ready_sounds{},hidden_blade_draws{},lit_blade_draws{};
  unsigned firing_particles{},firing_sounds{},fired_rounds{};
  bool overheated{},vented{};std::optional<unsigned> cooled_tick;unsigned vent_draws{};
  float maximum_heat{},maximum_spent{};
  std::optional<unsigned> ready_sound_tick;
  unsigned glow_draws{},glow_vertices{},glow_views{},glow_screen_views{};std::set<std::uint32_t> glow_owners;
  std::ofstream glow_records;
  if(const auto *path=std::getenv("HALO2_TEST_GLOW_RECORDS")) {
    glow_records.open(path,std::ios::binary);check(bool(glow_records),"Cannot open actual glow GPU input records");
    glow_records.write("LVG2",4);
  }
  unsigned charged_pose_draws{},charged_live_samples{};std::set<std::uint32_t> charged_live_owners;
  unsigned projectile_draws{};std::set<std::uint32_t> projectile_owners;
  std::set<std::uint32_t> flight_owners;unsigned live_flight_samples{},maximum_live_bolts{};
  float maximum_projectile_travel{};
  unsigned gauge_input_observations{};
  unsigned focus_samples{},pickup_input_samples{},maximum_pickup_milliseconds{};
  std::array<std::uint32_t,4> actual_unit_definitions{null_datum,null_datum,null_datum,null_datum};
  std::uint32_t owner=null_datum;bool posed{},shader{},blade{},carried_pose{};
  std::set<float> frames;MultiplayerState state;
  const auto place=[&](unsigned player,const Spawn &pose) {
    o.prepared_world->match.debug_request({DebugGameRequest::Teleport,player},&pose);
    if(auto *source=o.prepared_world->match.source_object_runtime();source && source->owns_physics() && (!native_motion || !picked)) {
      const auto object=state.players.at(player).source_unit;
      check(object!=null_datum,"Native fixture placement has no actual source unit");
      // B95D0 owns the original pose/physics mutation. Updating only the
      // presentation unit is overwritten by the next authoritative readback.
      source->tu15_world().set_object_position(object,pose.position);
    }
  };
  std::set<std::uint64_t> pose_hashes;
  std::set<float> ammunition_frames;float full_needle_scale=-1,used_needle_scale=-1;
  const auto weapon_model=cache.load_model(definition.weapon_model,false);
  std::optional<unsigned> needle_node;
  if(needler){
    const auto &ammo=*definition.ammunition[0];
    for(unsigned n=0;n<weapon_model.bones.size();++n){
      const auto at=std::find(ammo.nodes.begin(),ammo.nodes.end(),weapon_model.bones[n].name);
      if(at!=ammo.nodes.end() && (ammo.components[at-ammo.nodes.begin()]&4u)){needle_node=n;break;}
    }
    check(needle_node.has_value(),"Actual Needler ammunition has no scaled needle node");
  }
  std::optional<std::uint64_t> observed_shot;
  const auto vent_clip=std::find_if(definition.clock_clips.begin(),definition.clock_clips.end(),
      [](const auto &clip){return clip.activity==0x08000071u;});
  if(heat_cycle)check(vent_clip!=definition.clock_clips.end(),"Actual vent clip absent");
  o.input_observer=[&](unsigned t,std::span<const Controls> controls,const MultiplayerState &s){
    if(SDL_GetKeyboardFocus()!=window){SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);}
    focus_samples+=unsigned(SDL_GetKeyboardFocus()==window);
    pickup_input_samples+=unsigned(controls[0].pickup);
    maximum_pickup_milliseconds=std::max(maximum_pickup_milliseconds,unsigned(controls[0].pickup_milliseconds));
    tick=t;state=s;const auto &p=s.players[slot];
    if(clear_firing_lane && picked) {
      auto *source=o.prepared_world->match.source_object_runtime();
      check(source && source->owns_physics(),"Species acceptance requires actual source objects");
      for(unsigned view=0;view<local_players;++view) {
        const auto unit=s.players.at(o.prepared_world->local_slots.at(view)).source_unit;
        check(unit!=null_datum,"Local view has no actual source unit");
        const auto body=source->object_body(unit);
        actual_unit_definitions[view]=Bytes(source->memory().view(body,4)).u32(0);
        check(actual_unit_definitions[view]==c.player_representations.at(representation).biped.datum,
            "Local view's source biped differs from its selected authored representation");
      }
    }
    if(local_players>1 && picked)for(unsigned view=1;view<local_players;++view) {
      auto spectator=at;spectator.position.x-=.6f*float(view);
      place(o.prepared_world->local_slots.at(view),spectator);
    }
    if(charge_cycle || pistol_tap)for(const auto &projectile:s.weapon_projectiles)if(projectile.definition==fired_projectile) {
      check(projectile.datum!=null_datum,"Actual charged projectile lost its full live owner");
      charged_live_owners.insert(projectile.datum);++charged_live_samples;
    }
    if(world) {
      at.position.x=row->position.x;at.position.y=row->position.y-1.2f;at.facing=1.5707963267948966f;
      at.position.z=row->position.z-c.player.camera_height;
      place(slot,at);
      return;
    }
    if(!requested && t>=35 && SDL_GetKeyboardFocus()==window) {
      requested=t;
      place(slot,at);key(window_id,SDL_SCANCODE_E,true);
    }
    if(requested && !picked)
      place(slot,at);
    if(!picked && p.weapon==target) {picked=t;owner=p.source_weapon;key(window_id,SDL_SCANCODE_E,false);
      std::cout<<"Pickup observation: tick="<<t<<" loaded="<<p.loaded<<" shot_tick="<<p.shot_tick
          <<" rate="<<w.rounds_per_second<<" recovery="<<w.fire_recovery<<" trigger="<<w.trigger_behavior
          <<" effect="<<std::hex<<p.weapon_effect.effect<<" effect_weapon="<<p.weapon_effect.weapon<<std::dec
          <<" effect_tick="<<p.weapon_effect.tick<<" effect_serial="<<p.weapon_effect.serial<<'\n';
      if(!o.screenshot.empty()) {
        if(ammunition)o.capture_tick=400;else if(ready)o.capture_tick=t+(ignite?19:lit?24:10);else if(idle)o.capture_tick=t+48;else if(charge_cycle)o.capture_tick=t+130;else if(pistol_tap)o.capture_tick=t+68;else o.capture_tick=t+(firing_capture?67:73);
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
      const auto elapsed=t-*picked;
      if(clear_spawn && elapsed==20) {
        at=*clear_spawn;
        auto *source=o.prepared_world->match.source_object_runtime();
        check(source && source->owns_physics(),"Clear-lane placement requires the actual source pose owner");
        source->tu15_world().set_object_position(p.source_unit,at.position);
      }
      place(slot,at);
      unsigned live_bolts{};
      for(const auto &projectile:s.weapon_projectiles)if(projectile.definition==fired_projectile) {
        ++live_bolts;++live_flight_samples;flight_owners.insert(projectile.datum);
        maximum_projectile_travel=std::max(maximum_projectile_travel,projectile.traveled);
      }
      maximum_live_bolts=std::max(maximum_live_bolts,live_bolts);
      if(source_trace_path && (elapsed==0 || (elapsed>=56 && elapsed<=86))) {
        auto *source=o.prepared_world->match.source_object_runtime();
        check(source && source->owns_physics(),"Weapon-state trace needs the actual source bank");
        const auto body=source->object_body(owner);
        std::array<std::uint32_t,57> words{};
        const auto bytes=source->memory().view(body+0x16c,sizeof(words));
        std::memcpy(words.data(),bytes.data(),bytes.size());
        if(verify_source_shot_tick)
          check(p.shot_tick==words[55],"Published shot tick differs from original108607 source store");
        const auto motion=source->biped_motion(p.source_unit);
        nlohmann::json live_projectiles=nlohmann::json::array();
        for(const auto &projectile:s.weapon_projectiles)if(projectile.definition==fired_projectile) {
          const auto projectile_body=source->object_body(projectile.datum);
          const auto source_travel=Bytes(source->memory().view(projectile_body+0x168u,4)).u32(0);
          check(std::bit_cast<std::uint32_t>(projectile.traveled)==source_travel,
              "Published projectile travel differs from its actual source bank");
          live_projectiles.push_back({{"object",projectile.datum},{"definition",projectile.definition},
              {"position",{projectile.position.x,projectile.position.y,projectile.position.z}},
              {"velocity",{projectile.velocity.x,projectile.velocity.y,projectile.velocity.z}},
              {"traveled_bits",source_travel}});
        }
        source_trace.push_back({{"tick",t},{"elapsed",elapsed},{"fire",controls[0].fire},
            {"legacy_shot_tick",p.shot_tick},{"weapon",owner},{"effect_serial",p.weapon_effect.serial},
            {"effect",p.weapon_effect.effect},{"projectiles",s.weapon_projectiles.size()},
            {"live_projectiles",live_projectiles},
            {"source_position",{motion.position.x,motion.position.y,motion.position.z}},
            {"weapon_16c_words",words}});
      }
      if(energy) {
        maximum_heat=std::max(maximum_heat,p.weapon_shader.heat);
        maximum_spent=std::max(maximum_spent,p.weapon_shader.power_fraction);
        overheated|=(p.weapon_shader.weapon_flags&2u)!=0;
        vented|=(p.weapon_shader.weapon_flags&4u)!=0;
        if(overheated && !cooled_tick && !(p.weapon_shader.weapon_flags&2u) &&
            p.weapon_shader.heat<w.energy->heat.cooled_threshold)cooled_tick=t;
      }
      if(!ready && elapsed==60){
        std::cout<<"Request actual SDL fire: tick="<<t<<" focus="<<(SDL_GetKeyboardFocus()==window)
          <<" binding="<<o.desktop->keyboard_settings.bindings[unsigned(PcAction::Fire)]<<'\n';
        fire(window_id,true);
      }
      if(!ready && elapsed>60 && elapsed<74)
        std::cout<<"Fire input observation: tick="<<t<<" fire="<<controls[0].fire
          <<" focus="<<(SDL_GetKeyboardFocus()==window)<<'\n';
      // Needler repeats every four source ticks, restarting its longer attack
      // clip. Let the first shot finish before the held empty/refill sequence,
      // so this probe separately verifies the complete authored attack pose.
      if(ammunition && elapsed==61)fire(window_id,false);
      if(pistol_tap && elapsed==61)fire(window_id,false);
      if(ammunition && elapsed==60+unsigned(action->frames)+5)fire(window_id,true);
      if(heat_cycle) {
        if(w.trigger_behavior==1 && (elapsed==61 || elapsed==71))fire(window_id,false);
        if(w.trigger_behavior==1 && elapsed==70)fire(window_id,true);
        if(overheated)fire(window_id,false);
        if(cooled_tick && t>*cooled_tick+20){SDL_Event q{};q.type=SDL_QUIT;check(SDL_PushEvent(&q)==1,"Exit completed native heat cycle");}
      } else if(!ready && !ammunition && elapsed==(charge_cycle?120u:72u))fire(window_id,false);
      if(ammunition && !empty_tick && p.loaded==0) {
        empty_tick=t;fire(window_id,false);key(window_id,SDL_SCANCODE_R,true);
        if(!o.screenshot.empty())o.capture_tick=s.tick;
      }
      if(ammunition && empty_tick && t==*empty_tick+1)key(window_id,SDL_SCANCODE_R,false);
      if(ammunition && empty_tick && t>*empty_tick+70 && p.loaded==w.capacity) {
        refilled=true;SDL_Event q{};q.type=SDL_QUIT;check(SDL_PushEvent(&q)==1,"Exit completed Needler refill probe");
      }
      if(!ammunition && !heat_cycle && elapsed==(material_only?30u:charge_cycle?160u:110u)){SDL_Event q{};q.type=SDL_QUIT;check(SDL_PushEvent(&q)==1,"Exit weapon probe");}
    }
    check(t<(heat_cycle?700u:ammunition?450u:charge_cycle?320u:240u),"Actual SDL covenant pickup/attack timed out");
  };
  o.animation_observer=[&](unsigned,unsigned,const AnimationRenderObservation &sample){
    if(SDL_GetKeyboardFocus()!=window){SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);}
    if(sample.first_person && sample.weapon==owner && !sample.vertices.empty())carried_pose=true;
    if(heat_cycle && sample.first_person && sample.weapon==owner && sample.animation==vent_clip->index)++vent_draws;
    if(sample.first_person && sample.weapon==owner && needler) {
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
    if(charge_cycle && sample.first_person && sample.weapon==owner) {
      const auto charged=std::find_if(definition.clock_clips.begin(),definition.clock_clips.end(),
          [](const auto &clip){return clip.activity==0x0b000067u;});
      check(charged!=definition.clock_clips.end(),"Actual charged_1 clip absent");
      if(sample.animation==charged->index && !sample.vertices.empty())++charged_pose_draws;
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
    if(energy && m.name.ends_with("\\plasma_rifle_gauge")) {
      check(input(0x030005a6u)==1.f,"Actual live gauge lost the original one input");
      ++gauge_input_observations;
    }
    if(energy) {
      const auto &source=state.players[slot].weapon_shader;
      check(input(0x04000567u)==std::clamp(source.heat,0.f,1.f),"Carried energy material lost its actual heat input");
      check(input(0x03000577u)==std::clamp(source.power_fraction,0.f,1.f),"Carried energy material lost its actual age input");
      shader=true;
    } else if(!sword) {
      const auto expected=float(state.players[slot].loaded)/float(w.capacity);
      check(input(0x1200056au)==expected,"Carried Needler lost original primary_ammunition input");
      if(needler)check(input(0x04000b93u)==expected,"Carried Needler lost authored ammunition function");shader=true;
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
    if(!sword && s.sound==firing_sound) {
      check(s.owner==owner,"Firing sound bypassed its actual picked effect owner");
      check(s.frames>0,"Firing sound submitted an empty PCM packet");++firing_sounds;
    }
    if(s.owner==owner && s.sound==0xe2fc0187u) {
      ++ready_sounds;ready_sound_tick=tick;
      check(picked && tick-*picked==15,"Sword ignition sound did not follow original15-tick event delay");
    }
  };
  o.effect_observer=[&](unsigned,unsigned t,std::span<const ParticleDraw> particles) {
    if(!sword && picked && state.players[slot].shot_tick)firing_particles+=unsigned(particles.size());
    if(sword && picked && t-*picked<15)check(particles.empty(),"Sword ignition particles appeared before authored delay");
    if(sword && picked && t-*picked>=15 && t-*picked<40) {
      if(!ignition_particles && !particles.empty())std::cout<<"First ignition particle observation: tick="<<t<<" relative="<<t-*picked<<'\n';
      ignition_particles+=unsigned(particles.size());
    }
  };
  o.weapon_projectile_draw_observer=[&](unsigned,std::uint32_t,std::uint32_t handle,
      std::uint32_t definition,std::span<const Vertex> vertices) {
    if((!needler && !charge_cycle) || definition!=fired_projectile)return;
    check(!vertices.empty(),"Actual fired Needler shard submitted empty geometry");
    check(std::any_of(state.weapon_projectiles.begin(),state.weapon_projectiles.end(),
        [&](const auto &p){return p.datum==handle && p.definition==definition;}),
        "Actual fired shard draw has no live salted owner");
    ++projectile_draws;projectile_owners.insert(handle);
  };
  o.weapon_light_volume_observer=[&](unsigned view,std::uint32_t rendered_tick,const SourceLightVolumeDraw &draw) {
    if(!charge_cycle && !pistol_tap && !clear_firing_lane)return;
    const auto projectile=std::find_if(state.weapon_projectiles.begin(),state.weapon_projectiles.end(),
        [&](const auto &p){return p.datum==draw.object && p.definition==fired_projectile;});
    if(projectile==state.weapon_projectiles.end())return;
    if(glow_records.is_open()) {
      static_assert(sizeof(LightVolumeState)==68 && sizeof(LightVolumeVertex)==40);
      const std::array<std::uint32_t,5> identity{rendered_tick,view,draw.object,draw.definition,unsigned(draw.vertices.size())};
      const std::array<float,4> viewport{draw.viewport.left,draw.viewport.top,draw.viewport.width,draw.viewport.height};
      const auto write=[&](const auto &value){glow_records.write(reinterpret_cast<const char*>(&value),sizeof(value));};
      write(identity);write(draw.state);write(draw.camera);write(viewport);write(draw.actual_w_distance);
      glow_records.write(reinterpret_cast<const char*>(draw.vertices.data()),std::streamsize(draw.vertices.size()*sizeof(LightVolumeVertex)));
      write(std::uint32_t(draw.bitmap_materials.size()));
      for(const auto &binding:draw.bitmap_materials)write(std::array<std::uint32_t,2>{binding.bitmap,binding.material});
      write(std::uint32_t(draw.ranges.size()));
      for(const auto &range:draw.ranges)write(std::array<std::uint32_t,3>{range.first,range.count,range.material});
      check(bool(glow_records),"Cannot retain actual recorded glow inputs");
    }
    check(view<local_players && draw.command.callback==source_light_volume_callback,
        "Recorded bolt glow lost its actual view/callback");
    check(draw.vertices.size()>0 && draw.vertices.size()%6==0,"Recorded bolt glow has no complete sprite triangles");
    std::array<std::uint32_t,2> payload{};std::memcpy(payload.data(),draw.command.payload.data(),8);
    check(payload==std::array{draw.object,draw.definition},"Recorded bolt glow lost its actual attachment identities");
    const auto rect=hud_viewport(local_players,view,false).outer;
    const auto expected=full_surface_viewport({rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top},800,600);
    check(draw.viewport.left==expected.left && draw.viewport.top==expected.top &&
        draw.viewport.width==expected.width && draw.viewport.height==expected.height,
        "Recorded bolt glow lost the actual gameplay viewport");
    for(const auto &v:draw.vertices)check(v.screen[3]>0 && std::isfinite(v.screen[0]) && std::isfinite(v.screen[1]),
        "Recorded bolt glow has invalid original projected vertices");
    float left=draw.vertices[0].screen[0],right=left,top=draw.vertices[0].screen[1],bottom=top;
    bool colored{};
    for(const auto &v:draw.vertices) {
      left=std::min(left,v.screen[0]);right=std::max(right,v.screen[0]);
      top=std::min(top,v.screen[1]);bottom=std::max(bottom,v.screen[1]);
      colored|=v.color[3]>0 && (v.color[0]>0 || v.color[1]>0 || v.color[2]>0);
    }
    if(colored && right>expected.left && left<expected.left+expected.width &&
        bottom>expected.top && top<expected.top+expected.height)glow_screen_views|=1u<<view;
    if(!(glow_views&(1u<<view)) || (!o.screenshot.empty() && rendered_tick==o.capture_tick))
      std::cout<<"Bolt glow bounds: tick="<<rendered_tick<<" view="<<view<<" xy="<<left<<','<<top<<','<<right<<','<<bottom
          <<" root="<<draw.state.marker.position.x<<','<<draw.state.marker.position.y<<','<<draw.state.marker.position.z
          <<" eye="<<draw.camera.camera_to_world.position.x<<','<<draw.camera.camera_to_world.position.y<<','<<draw.camera.camera_to_world.position.z<<'\n';
    ++glow_draws;glow_views|=1u<<view;glow_vertices+=unsigned(draw.vertices.size());glow_owners.insert(draw.object);
  };
  if(argc>=6 && *argv[5]){o.frames=0;o.screenshot=argv[5];o.capture_tick=(idle||world)?90:115;}
  check(run_multiplayer(o)==0,"Actual covenant weapon runtime failed");
  std::cout<<"Pickup gate observations: requested="<<requested.value_or(0)<<" picked="<<picked.value_or(0)
      <<" focus="<<focus_samples<<" pickup_inputs="<<pickup_input_samples
      <<" maximum_pickup_ms="<<maximum_pickup_milliseconds<<'\n';
  if(source_trace_path) {
    nlohmann::json trace{{"render_loop_completed",true},{"native_motion_after_pickup",native_motion},
        {"source_shot_tick_equality_checked",verify_source_shot_tick},
        {"picked_tick",picked.value_or(0)},{"legacy_fired_rounds",fired_rounds},
        {"firing_sound_submissions",firing_sounds},{"maximum_heat",maximum_heat},
        {"maximum_spent",maximum_spent},{"actual_gauge_material_observations",gauge_input_observations},
        {"original_source_position_owner",true},{"samples",source_trace}};
    trace["projectile_births"]=projectile_births;
    trace["projectile_retirements"]=projectile_retirements;
    trace["live_flight_samples"]=live_flight_samples;
    trace["flight_owners"]=flight_owners;
    trace["maximum_live_bolts"]=maximum_live_bolts;
    trace["maximum_projectile_travel"]=maximum_projectile_travel;
    trace["glow_draws"]=glow_draws;trace["glow_vertices"]=glow_vertices;
    trace["glow_owners"]=glow_owners;trace["glow_views"]=glow_views;
    trace["glow_screen_views"]=glow_screen_views;
    trace["local_players"]=local_players;trace["selected_representation"]=representation;
    trace["input_assignment"]="keyboard_view0; other views have no controller owner";
    trace["expected_unit_definition"]=c.player_representations.at(representation).biped.datum;
    trace["actual_unit_definitions"]=std::vector(actual_unit_definitions.begin(),actual_unit_definitions.begin()+local_players);
    if(clear_spawn)trace["clear_firing_lane"]={{"authored_spawn_index",clear_spawn_index},
        {"position",{clear_spawn->position.x,clear_spawn->position.y,clear_spawn->position.z}},
        {"facing",clear_spawn->facing},{"host_selection_ray_minimum_clearance",selected_clearance},
        {"host_geometry_used_only_for_scene_selection",true},{"original_position_owner",true}};
    trace["published_source_contacts"]=nlohmann::json::array();
    for(const auto &event:state.grenade_detonations)if(traced_projectiles.contains(event.projectile))
      trace["published_source_contacts"].push_back({{"tick",event.tick},{"serial",event.serial},
          {"projectile",event.projectile},{"definition",event.definition},{"effect",event.effect},
          {"source_effect_instance",event.source_effect_instance},
          {"position",{event.position.x,event.position.y,event.position.z}}});
    std::ofstream report(source_trace_path);report<<trace.dump(2)<<'\n';
    check(bool(report),"Cannot retain bounded source weapon-state trace");
  }
  if(!o.screenshot.empty())check(std::filesystem::exists(o.screenshot),"Requested weapon capture was not written");
  if(world) {
    check(shader && materials>10,"World sword never received its own source shader input");
    std::cout<<"PASS sword world: actual pickup owner, "<<materials<<" material observations\n";return 0;
  }
  check(picked&&carried_pose,"Actual pickup did not submit its carried pose");
  check(shader&&materials>10,"Actual carried materials did not receive source inputs");
  if(clear_firing_lane) {
    check(flight_owners.size()==4 && live_flight_samples>4 && maximum_projectile_travel>2.f,
        "Clear firing lane did not expose all four source bolts in sustained flight");
    check(glow_draws>0 && glow_owners==flight_owners && glow_screen_views==((1u<<local_players)-1u),
        "Live source bolts did not submit their authored glow into the actual gameplay view");
    std::cout<<"Clear-lane observations: "<<live_flight_samples<<" live samples, "<<maximum_projectile_travel
        <<" maximum travel, "<<glow_draws<<" glow draws / "<<glow_owners.size()<<" owners\n";
  }
  if(std::string(argv[4])=="plasma_rifle" || std::string(argv[4])=="brute_plasma_rifle") {
    check(gauge_input_observations>0,"Actual live gauge material was never observed");
    std::cout<<"Gauge one observations: "<<gauge_input_observations<<'\n';
    if(const auto *path=std::getenv("HALO2_GAUGE_ACCEPTANCE_REPORT")) {
      std::ofstream report(path);
      report<<"{\"passed\":true,\"component_scope\":\"held_gauge_input\",\"actual_gauge_material_observations\":"<<gauge_input_observations
          <<",\"rendered_Xbox_parity_verified\":false,\"full_weapon_sequence_verified\":false}\n";
      check(bool(report),"Cannot retain live gauge acceptance report");
    }
  }
  if(material_only) {
    check(gauge_input_observations>0,"Material-only fixture requires its actual held gauge");
    std::cout<<"PASS actual SDL pickup,carried pose and original one input on held gauge\n";
    return 0;
  }
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
  const auto minimum_samples=unsigned(std::min(5,std::max(2,int(action->frames)-2)));
  std::cout<<"Attack observations: "<<draws<<" draws, "<<frames.size()<<" frames, "<<pose_hashes.size()<<" distinct poses\n";
  check(picked&&posed&&frames.size()>=minimum_samples&&draws>=minimum_samples,
      "Actual attack did not submit advancing authored poses");
  check(pose_hashes.size()>=minimum_samples,"Authored attack clock advanced without moving the rendered weapon vertices");
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
  else if(needler) {
    check(projectile_draws>4 && projectile_owners.size()>1,"Fired Needler shards never reached actual renderer submission");
    check(firing_particles>0,"Actual Needler shot never submitted its authored muzzle/smoke particles");
    check(firing_sounds==fired_rounds && fired_rounds>1,"Needler did not submit exactly one authored firing sound per shot");
    std::cout<<"Needler live effects: "<<firing_particles<<" particle draws, "<<firing_sounds<<" owned sounds / "<<fired_rounds<<" fired rounds\n";
  }
  if(!sword && !needler) {
    check(w.barrel_timing.has_value(),"Actual ordinary firing timing is unavailable");
    std::cout<<"Ordinary firing observations: "<<fired_rounds<<" rounds, "<<firing_sounds<<" owned sounds\n";
    check(fired_rounds>0 && firing_sounds==fired_rounds,"Ordinary weapon did not submit one owned firing sound per shot");
    if(!energy)check(fired_rounds==unsigned(std::max<std::int16_t>(1,w.barrel_timing->definition.burst_minimum)),
        "Actual held SDL trigger repeated or interrupted its authored burst");
    std::cout<<"Ordinary firing observations: "<<fired_rounds<<" rounds, "<<firing_sounds<<" owned sounds\n";
  }
  if(charge_cycle) {
    check(charged_pose_draws>10,"Actual full charge never submitted advancing charged_1 poses");
    check(fired_rounds==1 && maximum_spent>=.15f,"Full charge lost its secondary barrel age/shot stores");
    const auto metadata=cache.load_source_tag_metadata(fired_projectile);
    const Bytes projectile(metadata.bytes_at(metadata.source_address(),0x9c));
    check(charged_live_samples>0 && charged_live_owners.size()==1,"Full charge lost its actual live secondary projectile");
    if(projectile.u32(0x38)!=null_datum)
      check(projectile_draws>0 && projectile_owners.size()==1,"Actual charged bolt model never reached renderer submission");
    else {
      check(glow_draws>0 && glow_vertices>=84 && glow_owners==charged_live_owners && glow_views==((1u<<local_players)-1),
          "Actual charged bolt MGS2 glow never reached GPU recording");
      std::cout<<"Bolt glow observations: "<<glow_draws<<" recorded draws, "<<glow_vertices<<" projected vertices, "<<local_players<<" actual views\n";
      std::cout<<"Bolt glow screen bounds: intersected view mask="<<glow_screen_views<<'\n';
      std::cout<<"LIMIT bolt MGS2 is recorded; remaining light/effect/looping-sound attachment owners need validation\n";
    }
    std::cout<<"Charge observations: "<<charged_pose_draws<<" charged poses, "<<projectile_draws<<" charged bolt draws\n";
  }
  if(pistol_tap) {
    check(fired_rounds==1 && maximum_spent>=.004f && maximum_spent<.15f,
        "Actual early release lost its primary barrel age/shot stores");
    check(charged_live_samples>0 && charged_live_owners.size()==1 && glow_owners==charged_live_owners &&
        glow_draws>0 && glow_views==((1u<<local_players)-1),"Actual primary bolt glow did not reach each gameplay view");
    std::cout<<"Bolt glow observations: "<<glow_draws<<" primary recorded draws, "<<glow_vertices<<" projected vertices, "<<local_players<<" actual views\n";
    std::cout<<"Bolt glow screen bounds: intersected view mask="<<glow_screen_views<<'\n';
  }
  if(heat_cycle) {
    check(overheated && vented && cooled_tick && vent_draws>4 && maximum_spent>0.f && maximum_heat==1.f,
        "Actual native heat cycle lost overheat/vent/cooling, battery use or advancing vent poses");
    std::cout<<"Energy heat observations: maximum="<<maximum_heat<<" spent="<<maximum_spent
        <<" vent draws="<<vent_draws<<" cooled tick="<<*cooled_tick
        <<" firing particle draws="<<firing_particles<<'\n';
  }
  std::cout<<"PASS "<<argv[4]<<": actual SDL pickup/fire, "<<draws<<" attack draws, "<<frames.size()
      <<" frames, "<<materials<<" material observations, "<<sounds<<" authored attack sounds, "
      <<projectile_draws<<" shard draws / "<<projectile_owners.size()<<" owners\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
