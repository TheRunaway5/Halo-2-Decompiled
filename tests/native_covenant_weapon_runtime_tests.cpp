#include "helpers/gameplay_fixture.hpp"
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
#include "halo2/particle_draw.hpp"
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
  const bool reload_cycle=argc==7 && std::string(argv[6])=="reload";
  check(!reload_cycle || shotgun,"Staged reload probe requires shotgun");
  const bool pistol_tap=argc==7 && std::string(argv[6])=="tap";
  check(!charge_cycle || plasma_pistol,"Charge probe requires plasma pistol");
  check(!pistol_tap || plasma_pistol,"Tap probe requires plasma pistol");
  const bool needler=std::string(argv[4])=="needler";
  const bool rocket=std::string(argv[4])=="rocket_launcher";
  const bool material_only=std::getenv("HALO2_TEST_WEAPON_MATERIAL_ONLY")!=nullptr;
  const bool native_motion=std::getenv("HALO2_TEST_ALLOW_NATIVE_MOTION")!=nullptr;
  const auto *source_trace_path=std::getenv("HALO2_TEST_WEAPON_STATE_REPORT");
  const bool verify_source_shot_tick=std::getenv("HALO2_TEST_VERIFY_SOURCE_SHOT_TICK")!=nullptr;
  const bool clear_firing_lane=std::getenv("HALO2_TEST_CLEAR_FIRING_LANE")!=nullptr;
  const bool needler_clear_lane=std::getenv("HALO2_TEST_NEEDLER_CLEAR_LANE")!=nullptr;
  check(!needler_clear_lane || (needler && mcc),"Needler clear-lane diagnostic requires the prepared MCC case");
  nlohmann::json source_trace=nlohmann::json::array();
  const std::set<std::string> ordinary{"magnum","battle_rifle","covenant_carbine",
      "sniper_rifle","brute_shot","rocket_launcher","plasma_pistol","plasma_rifle","brute_plasma_rifle","beam_rifle"};
  check(sword||shotgun||needler||ordinary.contains(argv[4]),"Unknown weapon probe");
  const bool ammunition=argc==7 && std::string(argv[6])=="ammo";
  const bool heat_cycle=argc==7 && std::string(argv[6])=="heat";
  check(!ammunition || needler,"Ammo probe requires Needler");
  const bool firing_capture=argc==7 && std::string(argv[6])=="fire";
  const bool rocket_flight_capture=argc==7 && std::string(argv[6])=="flight";
  check(!rocket_flight_capture || rocket,"Flight capture requires the rocket launcher");
  check(!firing_capture || needler,"Firing capture requires Needler");
  const bool world=argc==7 && std::string(argv[6])=="world";
  const bool idle=argc==7 && std::string(argv[6])=="idle";
  const bool ignite=argc==7 && std::string(argv[6])=="ignite";
  const bool lit=argc==7 && std::string(argv[6])=="lit";
  const bool ready=argc==7 && (std::string(argv[6])=="ready" || ignite || lit);
  check(argc!=7 || idle || world || ready || ammunition || firing_capture || rocket_flight_capture || heat_cycle || charge_cycle || pistol_tap || reload_cycle,"Unknown capture phase");
  // This probe supplies keyboard/mouse events. Physical controller input is
  // independent host activity and must not switch/fire the tested inventory.
  SDL_SetHintWithPriority(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0xffff/0xffff",SDL_HINT_OVERRIDE);
  SDL_SetMainReady();MultiplayerOptions o;o.map=argv[1];o.shared_maps=argv[2];
  if(!mcc)o.gameplay_xbe=argv[3];
  o.width=800;o.height=600;o.local_players=local_players;o.starting_weapon=5;o.mute=false;
  if(const auto *width=std::getenv("HALO2_TEST_VIEWPORT_WIDTH"))o.width=unsigned(std::stoul(width));
  if(const auto *height=std::getenv("HALO2_TEST_VIEWPORT_HEIGHT"))o.height=unsigned(std::stoul(height));
  // Finite native SDL probe: one game tick and a declared mid-tick sample
  // per rendered frame. Short authored clips must not depend on host load.
  o.frames=heat_cycle?700u:(ammunition || reload_cycle)?450u:charge_cycle?320u:240u;
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
  // Material callbacks consume the owning cache's authored identifiers.
  // Independently check each paired name instead of applying an index delta
  // or requesting an Xbox identifier from an MCC shader input callback.
  const auto material_symbol=[&](std::uint32_t original,std::uint32_t imported,std::string_view name) {
    const auto id=cache.is_mcc()?imported:original;
    check(cache.string_id(id)==name,"Material fixture symbol differs from its owning cache name");
    return id;
  };
  const auto material_one=material_symbol(0x030005a6u,0x030005b2u,"one");
  const auto material_turned_on=material_symbol(0x09000568u,0x09000574u,"turned_on");
  const auto material_heat=material_symbol(0x04000567u,0x04000573u,"heat");
  const auto material_age=material_symbol(0x03000577u,0x03000583u,"age");
  const auto material_ammunition=material_symbol(0x1200056au,0x12000576u,"primary_ammunition");
  const auto material_alive=material_symbol(0x0500055bu,0x05000567u,"alive");
  std::uint32_t authored_ammo_alias=null_datum;
  if(needler) {
    for(const auto &function:cache.load_source_world_weapon_definition(target).functions)
      if(cache.string_id(function.output)=="ammo") {
        check(authored_ammo_alias==null_datum && function.input==material_ammunition,
            "Needler ammunition alias differs from its actual authored producer");
        authored_ammo_alias=function.output;
      }
    check(authored_ammo_alias!=null_datum,"Needler has no authored ammo material function");
  }
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
  nlohmann::json projectile_birth_requests=nlohmann::json::array();
  nlohmann::json weapon_effect_requests=nlohmann::json::array();
  std::vector<std::size_t> weapon_effect_pending;
  nlohmann::json source_sound_submissions=nlohmann::json::array();
  std::vector<std::size_t> sound_submit_pending;
  nlohmann::json sword_selection_calls=nlohmann::json::array();
  std::vector<std::size_t> sword_selection_pending;
  unsigned sword_selection_depth{};
  std::optional<std::uint32_t> original_sword_selected_activity;
  const bool trace_sword_selection=std::getenv("HALO2_TEST_TRACE_SWORD_SELECTION")!=nullptr;
  const bool trace_reload_timing=std::getenv("HALO2_TEST_TRACE_RELOAD_TIMING")!=nullptr;
  nlohmann::json reload_timing_calls=nlohmann::json::array();
  std::vector<std::size_t> reload_timing_pending;
  unsigned reload_timing_depth{};
  struct SoundObservationLifetime {
    MultiplayerObjectRuntime *world{};
    ~SoundObservationLifetime(){if(world)world->observe_sound_submissions({});}
  } sound_observation;
  std::set<std::uint32_t> traced_projectiles;
  std::vector<std::size_t> projectile_birth_pending;
  std::optional<SourcePhysicsEntryObserverBinding> projectile_birth_observer;
  if(std::getenv("HALO2_TEST_TRACE_PROJECTILE_BIRTHS") || std::getenv("HALO2_TEST_TRACE_WEAPON_EFFECT_REQUESTS") || trace_sword_selection || trace_reload_timing) {
    auto *source=o.prepared_world->match.source_object_runtime();
    check(source && source->owns_physics(),"Projectile birth trace requires actual source storage");
    const auto &memory=source->memory();
    const auto word=[&memory](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    if(std::getenv("HALO2_TEST_TRACE_WEAPON_EFFECT_REQUESTS")) {
      source->observe_sound_submissions([&](bool before,std::uint32_t sound,std::uint32_t request,
          const auto &abi,const auto &bank,auto instructions,auto) {
        std::array<std::uint32_t,40> words{};
        const auto bytes=bank.view(request,sizeof(words));std::memcpy(words.data(),bytes.data(),bytes.size());
        if(before) {
          sound_submit_pending.push_back(source_sound_submissions.size());
          source_sound_submissions.push_back({{"sound_argument",sound},{"request_address",request},
              {"before_request_words",words},{"before_gp",abi.gp},{"completed",false}});
          auto &row=source_sound_submissions.back();
          if(const auto tag=cache.query_tag({.handle=TagHandle{sound}})) {
            row["sound_tag_name"]=tag->name;row["sound_tag_group"]=tag->group;
          }
          const auto state=Bytes(bank.view(0x4f2c50u,4)).u32(0);
          row["source_sound_system"]=state;
          row["source_sound_device_initialized"]=std::to_integer<unsigned>(bank.view(state+0x78u,1)[0]);
          row["source_sound_system_enabled"]=std::to_integer<unsigned>(bank.view(state+0x79u,1)[0]);
          const auto clock=Bytes(bank.view(0x51df54u,4)).u32(0);
          row["source_tick"]=Bytes(bank.view(clock+8u,4)).u32(0);
        } else {
          check(!sound_submit_pending.empty(),"Unpaired original sound-submit return");
          auto &row=source_sound_submissions.at(sound_submit_pending.back());sound_submit_pending.pop_back();
          row["completed"]=true;row["after_request_words"]=words;row["after_gp"]=abi.gp;
          row["returned_handle"]=abi.gp[0];row["instruction_count"]=instructions.size();
          const auto count=std::min<std::size_t>(instructions.size(),4096);
          row["instruction_pcs"]=std::vector<std::uint32_t>(instructions.begin(),instructions.begin()+count);
          row["instruction_trace_truncated"]=count<instructions.size();
        }
      });
      sound_observation.world=source;
    }
    // Actual B9B70 takes its original C4-byte object request at ESP+4 and
    // returns the created full datum in EAX. The private validation snapshot
    // adds read-only entry/return observation inside its existing owners.
    projectile_birth_observer.emplace(memory,[&,word](bool before,std::uint32_t entry,const auto &abi) {
      if(trace_reload_timing && reload_cycle &&
          (entry==0x10ab50u || entry==0x16b44au || entry==0x16b31bu || entry==0x1d3d30u)) {
        const bool root_entry=entry==0x10ab50u;
        if(before && root_entry)++reload_timing_depth;
        if(!reload_timing_depth)return;
        if(before) {
          if(reload_timing_calls.size()>=256) {reload_timing_pending.push_back(SIZE_MAX);return;}
          reload_timing_pending.push_back(reload_timing_calls.size());
          reload_timing_calls.push_back({{"entry",entry},{"source_tick",word(word(0x51df54u)+8u)},
              {"before_gp",abi.gp},{"stack_words",{word(abi.gp[4]),word(abi.gp[4]+4u),word(abi.gp[4]+8u)}},
              {"completed",false}});
        }else {
          check(!reload_timing_pending.empty(),"Unpaired original magazine timing return");
          const auto index=reload_timing_pending.back();reload_timing_pending.pop_back();
          if(index!=SIZE_MAX) {
            auto &row=reload_timing_calls.at(index);
            check(row["entry"]==entry,"Original magazine timing nesting changed");
            row["completed"]=true;row["after_gp"]=abi.gp;row["returned_value"]=abi.gp[0];
          }
          if(root_entry)--reload_timing_depth;
        }
        return;
      }
      if(trace_sword_selection && sword &&
          (entry==0xebfa0u || entry==0xebf20u || entry==0x16b31bu || entry==0x1d3d30u ||
           entry==0xe13b0u || entry==0x169863u || entry==0x1d5320u || entry==0x1d46a0u || entry==0x1d3c80u)) {
        if(entry==0x169863u && !original_sword_selected_activity)return;
        const bool root_entry=entry==0xebfa0u || entry==0x169863u;
        if(before && root_entry)++sword_selection_depth;
        if(!sword_selection_depth)return;
        if(before) {
          if(sword_selection_calls.size()>=256) {sword_selection_pending.push_back(SIZE_MAX);return;}
          sword_selection_pending.push_back(sword_selection_calls.size());
          nlohmann::json row{{"entry",entry},{"source_tick",word(word(0x51df54u)+8u)},
              {"before_gp",abi.gp},{"stack_words",{word(abi.gp[4]),word(abi.gp[4]+4u),word(abi.gp[4]+8u)}},
              {"completed",false}};
          if(entry==0x1d3d30u) {
            // Actual1D3D30/1D3D80 consume this caller-owned manager.
            // A failed diagnostic read is reported without changing execution.
            try {
              row["manager_graph"]=word(abi.gp[6]+0x68u);
              row["manager_mode"]=word(abi.gp[6]+0x70u);
              row["manager_weapon_class"]=word(abi.gp[6]+0x74u);
              row["manager_weapon_type"]=word(abi.gp[6]+0x78u);
            }catch(const std::exception &error) {row["manager_read_error"]=error.what();}
          }
          if(entry==0x1d5320u) {
            // Literal169A47 passes the primary channel at hand+8. Keep the
            // canonical request and source channel readback independent.
            row["manager_address"]=abi.gp[1];
            try {
              row["manager_graph"]=word(abi.gp[1]);
              row["manager_packed_clip_before"]=word(abi.gp[1]+4u);
              row["manager_activity_before"]=word(abi.gp[1]+0x7cu);
            }catch(const std::exception &error) {row["manager_read_error"]=error.what();}
          }
          sword_selection_calls.push_back(std::move(row));
        }else {
          check(!sword_selection_pending.empty(),"Unpaired original Sword selector return");
          const auto index=sword_selection_pending.back();sword_selection_pending.pop_back();
          if(index!=SIZE_MAX) {
            auto &row=sword_selection_calls.at(index);
            check(row["entry"]==entry,"Original Sword selector nesting changed");
            row["completed"]=true;row["after_gp"]=abi.gp;row["returned_value"]=abi.gp[0];
            if(entry==0x1d5320u) {
              const auto manager=row["manager_address"].get<std::uint32_t>();
              try {
                row["manager_packed_clip_after"]=word(manager+4u);
                row["manager_activity_after"]=word(manager+0x7cu);
              }catch(const std::exception &error) {row["manager_return_read_error"]=error.what();}
            }
            if(entry==0xebf20u && abi.gp[0]) {
              const auto candidate=abi.gp[0];
              check(candidate>=0x44a8dcu && candidate<0x44a90cu && (candidate-0x44a8dcu)%12u==0,
                  "Original Sword selector returned a foreign candidate record");
              original_sword_selected_activity=word(candidate);
            }
          }
          if(root_entry)--sword_selection_depth;
        }
        return;
      }
      if(entry==0x108a70u) {
        if(before) {
          const auto *tag=abi.gp[0]==null_datum?nullptr:cache.query_tag({.handle=TagHandle{abi.gp[0]}});
          weapon_effect_pending.push_back(weapon_effect_requests.size());
          weapon_effect_requests.push_back({{"source_tick",word(word(0x51df54u)+8u)},
              {"caller_return",word(abi.gp[4])},{"effect",abi.gp[0]},{"weapon",abi.gp[1]},
              {"tag_group",tag?tag->group:""},{"tag_name",tag?tag->name:""},
              {"previous_handle",word(abi.gp[4]+4u)},{"a_bits",word(abi.gp[4]+8u)},
              {"b_bits",word(abi.gp[4]+12u)},{"completed",false}});
        } else {
          check(!weapon_effect_pending.empty(),"Unpaired original weapon-effect request return");
          auto &request=weapon_effect_requests.at(weapon_effect_pending.back());weapon_effect_pending.pop_back();
          request["completed"]=true;request["returned_handle"]=abi.gp[0];
        }
        return;
      }
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
        const auto requested=word(request);
        if(projectile_birth_requests.size()<192) {
          const auto *tag=cache.query_tag({.handle=TagHandle{requested}});
          if(tag && tag->group=="proj")projectile_birth_requests.push_back({
              {"source_tick",word(word(0x51df54u)+8u)},{"caller_return",word(abi.gp[4])},
              {"definition",requested},{"tag_name",tag->name},{"expected_projectile",fired_projectile},
              {"matches_expected",requested==fired_projectile}});
        }
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
  o.desktop=std::make_shared<DesktopSession>();
  if(const auto *fov=std::getenv("HALO2_TEST_FIELD_OF_VIEW"))
    o.desktop->display_settings.field_of_view=unsigned(std::stoul(fov));
  auto *window=o.desktop->window(o.width,o.height,false);
  std::cout<<"Native SDL video driver: "<<SDL_GetCurrentVideoDriver()<<'\n';
  SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);
  const auto window_id=SDL_GetWindowID(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  const auto slot=o.prepared_world->local_slots.at(0);
  // Isolate the weapon/renderer loop from the still-unfinished native mover.
  // This is explicit diagnostic placement; no original physics is claimed.
  Spawn at;at.position=row->position;at.position.x-=.2f;at.bsp=0;at.facing=0;
  std::optional<Spawn> clear_spawn;unsigned clear_spawn_index{};float selected_clearance{};
  if(clear_firing_lane || needler_clear_lane) {
    check(needler_clear_lane || rocket || std::string(argv[4])=="plasma_rifle",
        "Clear-lane acceptance requires its explicit plasma-rifle,rocket or Needler diagnostic");
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
  std::optional<unsigned> staged_reload_requested;
  int staged_reload_loaded{},staged_reload_reserve{};
  bool staged_reload_refilled{};
  nlohmann::json staged_reload_timeline=nlohmann::json::array(),staged_reload_poses=nlohmann::json::array();
  unsigned ignition_particles{},ready_sounds{},hidden_blade_draws{},lit_blade_draws{};
  unsigned firing_particles{},firing_sounds{},fired_rounds{};
  unsigned rocket_exhaust_draws{};
  std::uint32_t rocket_exhaust=null_datum;
  if(rocket)for(const auto &tag:cache.tags())
    if(tag.group=="effe" && tag.name.ends_with("\\rocket_exhaust"))rocket_exhaust=tag.datum;
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
  nlohmann::json source_charge_poses=nlohmann::json::array();
  nlohmann::json source_pose_observations=nlohmann::json::array();
  unsigned source_model_particle_draws{};
  nlohmann::json source_model_particle_observations=nlohmann::json::array();
  unsigned projectile_draws{};std::set<std::uint32_t> projectile_owners;
  nlohmann::json projectile_geometry_samples=nlohmann::json::array();
  std::set<std::uint32_t> flight_owners;unsigned live_flight_samples{},maximum_live_bolts{};
  float maximum_projectile_travel{};
  unsigned gauge_input_observations{};
  unsigned alive_input_observations{};
  nlohmann::json sword_material_timeline=nlohmann::json::array();
  nlohmann::json sword_pose_samples=nlohmann::json::array();
  std::optional<std::uint32_t> sword_material_clock;
  std::int16_t sword_material_countdown{};
  unsigned focus_samples{},pickup_input_samples{},maximum_pickup_milliseconds{};
  std::array<std::uint32_t,4> actual_unit_definitions{null_datum,null_datum,null_datum,null_datum};
  unsigned source_unit_definition_observations{};
  std::uint32_t owner=null_datum;bool posed{},shader{},blade{},carried_pose{};
  std::set<float> frames;MultiplayerState state;
  const auto place=[&](unsigned player,const Spawn &pose) {
    test::place_player(o.prepared_world->match,player,pose);
    if(auto *source=o.prepared_world->match.source_object_runtime();source && source->owns_physics() && (!native_motion || !picked)) {
      const auto object=state.players.at(player).source_unit;
      check(object!=null_datum,"Native fixture placement has no actual source unit");
      // B95D0 owns the original pose/physics mutation. Updating only the
      // presentation unit is overwritten by the next authoritative readback.
      source->tu15_world().set_object_position(object,pose.position);
    }
  };
  std::set<std::uint64_t> pose_hashes;
  const FirstPersonClockClip *selected_sword_attack{};
  // EBF20 uses the actual shared RNG. Match poses/audio to the canonical
  // EC450 action readback,including its selected second strike when authored.
  const auto observe_selected_sword_attack=[&] {
    if(!sword || ready || !picked)return;
    auto *source=o.prepared_world->match.source_object_runtime();
    check(source && source->owns_physics(),"Sword selection requires its actual source owner");
    const auto selected=source->melee_action(state.players.at(slot).source_unit);
    if(!selected.active())return;
    check(selected.activity>=0x0e000607u && selected.activity<=0x0e00060au,
        "Sword source action left the authenticated ordinary strike family");
    const auto clip=std::find_if(definition.clock_clips.begin(),definition.clock_clips.end(),
        [&](const auto &value){return value.activity==selected.activity;});
    check(clip!=definition.clock_clips.end(),"Selected source Sword strike has no authored clock clip");
    check(!selected_sword_attack || selected_sword_attack->activity==selected.activity,
        "Single Sword input selected a second source strike");
    if(original_sword_selected_activity)
      check(selected.activity==*original_sword_selected_activity,
          "Published Sword action differs from the original selected candidate");
    if(!selected_sword_attack) {
      selected_sword_attack=&*clip;attack_sounds.clear();
      for(const auto &event:clip->events)if(event.type==1)attack_sounds.insert(event.tag);
    }
  };
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
  // Original1E1020 follows the action's authored head/next chain. A clock
  // binding names its root; the completed source channel can select a variant.
  // Derive membership from immutable Cache metadata without choosing a result
  // or advancing either original RNG stream.
  std::set<std::uint32_t> authored_attack_variants;
  if(!sword) {
    const auto packed=first_person_clip_identifier(*action);
    const auto root_clip=cache.load_animation_identifier(definition.graph,packed);
    const auto graph=cache.load_source_tag_metadata(root_clip.source_graph);
    const Bytes header(graph.bytes_at(graph.source_address(),0x34));
    const auto count=header.u32(0x2c),rows=header.u32(0x30);
    const auto record=[&](unsigned index) {
      check(index<count,"Authored attack variant leaves its actual graph");
      return Bytes(graph.bytes_at(rows+index*0x6cu,0x34));
    };
    auto index=unsigned(packed>>16);
    const auto head=record(index).u16(0x30);
    if(head!=0xffffu)index=head;
    for(;;) {
      check(authored_attack_variants.insert((index<<16)|(packed&0xffffu)).second,
          "Authored attack variant chain cycles");
      const auto next=record(index).u16(0x32);
      if(next==0xffffu)break;
      index=next;
    }
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
    if(picked && (mcc || clear_firing_lane)) {
      auto *source=o.prepared_world->match.source_object_runtime();
      check(source && source->owns_physics(),"Species acceptance requires actual source objects");
      for(unsigned view=0;view<local_players;++view) {
        const auto unit=s.players.at(o.prepared_world->local_slots.at(view)).source_unit;
        check(unit!=null_datum,"Local view has no actual source unit");
        const auto body=source->object_body(unit);
        actual_unit_definitions[view]=Bytes(source->memory().view(body,4)).u32(0);
        check(actual_unit_definitions[view]==c.player_representations.at(representation).biped.datum,
            "Local view's source biped differs from its selected authored representation");
        ++source_unit_definition_observations;
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
        if(ammunition)o.capture_tick=400;else if(ready)o.capture_tick=t+(ignite?19:lit?24:10);else if(idle)o.capture_tick=t+48;else if(rocket_flight_capture)o.capture_tick=t+100;else if(charge_cycle)o.capture_tick=t+130;else if(pistol_tap)o.capture_tick=t+68;else o.capture_tick=t+(firing_capture?67:73);
      }
    }
    if(ready && (controls[0].fire || controls[0].fire_secondary))std::cerr<<"Unexpected input: tick="<<t<<" fire="<<controls[0].fire<<" secondary="<<controls[0].fire_secondary<<" controllers="<<SDL_NumJoysticks()<<'\n';
    if(ready)check(!controls[0].fire && !controls[0].fire_secondary,"Ready-only probe received an unrequested attack");
    if(picked) {
      check(!p.dead && p.weapon==target && p.source_weapon==owner,
          "Weapon probe lost its actual picked owner before the requested attack");
      observe_selected_sword_attack();
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
      if(source_trace_path && (elapsed==0 || (elapsed>=56 && elapsed<=((charge_cycle || pistol_tap)?130u:86u)))) {
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
          if(rocket) {
            auto &attachments=live_projectiles.back()["source_attachments"]=nlohmann::json::array();
            for(const auto &a:source->attached_effects(projectile.datum))
              attachments.push_back({{"slot",a.slot},{"instance",a.instance},{"definition",a.definition},
                  {"flags",a.flags},{"scale_a",a.scale_a},{"scale_b",a.scale_b}});
          }
        }
        source_trace.push_back({{"tick",t},{"elapsed",elapsed},{"fire",controls[0].fire},
            {"legacy_shot_tick",p.shot_tick},{"weapon",owner},{"effect_serial",p.weapon_effect.serial},
            {"published_fp_activity",p.first_person_clock.current.activity},
            {"effect",p.weapon_effect.effect},{"projectiles",s.weapon_projectiles.size()},
            {"live_projectiles",live_projectiles},
            {"source_position",{motion.position.x,motion.position.y,motion.position.z}},
            {"weapon_16c_words",words}});
        if(sword && trace_sword_selection) {
          const auto selected=source->melee_action(p.source_unit);
          const auto view=source->tu15_world().first_person_view(0);
          source_trace.back()["actual_source_melee"]={{"activity",selected.activity},
              {"progress",selected.progress},{"impact",selected.impact},
              {"recovery",selected.recovery},{"duration",selected.duration}};
          source_trace.back()["actual_source_first_person"]={{"unit",view.unit},
              {"weapon",view.hands[0].weapon},{"graph",view.hands[0].graph},
              {"activity",view.hands[0].activity},{"packed_clip",view.hands[0].animation_identifier},
              {"frame",view.hands[0].frame},{"channel_flags",view.hands[0].channel_flags}};
        }
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
      if(reload_cycle && elapsed==90) {
        check(p.loaded>0 && p.loaded<w.capacity && p.reserve>0,"Partial staged reload requires actual consumed ammunition and reserve");
        staged_reload_loaded=p.loaded;staged_reload_reserve=p.reserve;staged_reload_requested=t;
        key(window_id,SDL_SCANCODE_R,true);
      }
      if(reload_cycle && staged_reload_requested) {
        if(t==*staged_reload_requested+1u)key(window_id,SDL_SCANCODE_R,false);
        auto *source=o.prepared_world->match.source_object_runtime();
        check(source && source->owns_physics(),"Staged reload needs the actual source owner");
        const auto view=source->tu15_world().first_person_view(0);
        if(staged_reload_timeline.size()<256)staged_reload_timeline.push_back({{"tick",t},
            {"activity",view.hands[0].activity},{"packed_clip",view.hands[0].animation_identifier},
            {"frame",view.hands[0].frame},{"loaded",p.loaded},{"reserve",p.reserve}});
        if(t>*staged_reload_requested+90u && p.loaded==w.capacity && view.hands[0].activity==0x0400000cu) {
          staged_reload_refilled=true;SDL_Event q{};q.type=SDL_QUIT;check(SDL_PushEvent(&q)==1,"Exit completed staged reload");
        }
      }
      if(!ammunition && !heat_cycle && !reload_cycle && elapsed==(material_only?30u:charge_cycle?160u:110u)){SDL_Event q{};q.type=SDL_QUIT;check(SDL_PushEvent(&q)==1,"Exit weapon probe");}
    }
    check(t<(heat_cycle?700u:(ammunition || reload_cycle)?450u:charge_cycle?320u:240u),"Actual SDL covenant pickup/attack timed out");
  };
  o.animation_observer=[&](unsigned,unsigned,const AnimationRenderObservation &sample){
    // Observe the completed original pose before any legacy observer assertion.
    // This supplies no source result and never reapplies a host overlay.
    if(source_trace_path && sample.first_person && sample.weapon==owner &&
        source_pose_observations.size()<512) {
      auto *source=o.prepared_world->match.source_object_runtime();
      check(source && source->owns_physics(),"Pose diagnostics require the actual source owner");
      const auto hand=source->tu15_world().first_person_view(0).hands[0];
      const auto &bank=source->memory();
      const auto view=Bytes(bank.view(0x4f6498u,4)).u32(0);
      const auto packed_ammunition=Bytes(bank.view(view+0xcu+0xe0u,4)).u32(0);
      nlohmann::json row{{"input_observer_tick",tick},{"weapon",hand.weapon},
          {"source_graph",hand.graph},{"source_activity",hand.activity},
          {"source_packed_clip",hand.animation_identifier},{"source_frame",hand.frame},
          {"source_packed_ammunition",packed_ammunition},{"rendered_animation",sample.animation},
          {"rendered_frame",sample.frame},{"expected_action_root",action->index},
          {"legacy_ammunition_animation",sample.ammunition_animation},
          {"legacy_ammunition_frame",sample.ammunition_frame},
          {"completed_node_count",sample.completed_weapon_nodes.size()},
          {"loaded",state.players[slot].loaded},{"vertices",sample.vertices.size()}};
      if(needler && needle_node && *needle_node<sample.completed_weapon_nodes.size())
        row["completed_needle_scale"]=sample.completed_weapon_nodes[*needle_node].scale;
      source_pose_observations.push_back(std::move(row));
    }
    if(sample.first_person && sample.weapon==owner)observe_selected_sword_attack();
    if(SDL_GetKeyboardFocus()!=window){SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);}
    if(sample.first_person && sample.weapon==owner && !sample.vertices.empty())carried_pose=true;
    if(reload_cycle && sample.first_person && sample.weapon==owner && staged_reload_poses.size()<512) {
      auto *source=o.prepared_world->match.source_object_runtime();
      check(source && source->owns_physics(),"Staged reload poses require their actual source owner");
      const auto hand=source->tu15_world().first_person_view(0).hands[0];
      for(const auto &clip:definition.clock_clips)if(clip.activity==hand.activity &&
          (clip.activity==0x0c0005aeu || clip.activity==0x150005afu || clip.activity==0x140005b0u || clip.activity==0x0b0005b1u)) {
        check(hand.weapon==owner && hand.graph==definition.graph &&
            hand.animation_identifier==first_person_clip_identifier(clip) && sample.animation==clip.index,
            "Rendered staged reload lost its actual source activity/packed clip/weapon owner");
        std::uint64_t hash=1469598103934665603ull;
        for(const auto &v:sample.vertices)for(float x:{v.position.x,v.position.y,v.position.z}) {
          hash^=std::bit_cast<std::uint32_t>(x);hash*=1099511628211ull;
        }
        staged_reload_poses.push_back({{"input_observer_tick",tick},{"activity",hand.activity},
            {"source_frame",hand.frame},{"source_packed_clip",hand.animation_identifier},{"animation_index",sample.animation},
            {"frame",sample.frame},{"vertices",sample.vertices.size()},{"vertex_position_hash",hash}});
      }
    }
    if(heat_cycle && sample.first_person && sample.weapon==owner && sample.animation==vent_clip->index)++vent_draws;
    if(sample.first_person && sample.weapon==owner && needler) {
      const auto &p=state.players[slot];
      const auto &ammo=*definition.ammunition[0];
      const auto expected=first_person_ammunition_frame(p.loaded,p.ammunition_available,w.capacity,
          std::bit_cast<std::int16_t>(std::uint16_t(state.tick-p.reload_tick)),w.weapon_type,
          state.players[slot].first_person_clock.current.activity,ammo.frame_count);
      if(mcc) {
        auto *source=o.prepared_world->match.source_object_runtime();
        check(source && source->owns_physics(),"Needler overlay requires its completed original owner");
        const auto &bank=source->memory();
        const auto view=Bytes(bank.view(0x4f6498u,4)).u32(0);
        const auto packed=Bytes(bank.view(view+0xcu+0xe0u,4)).u32(0);
        check(packed==ammo.source_identifier,"Original Needler hand lost its authored packed ammunition clip");
        check(sample.completed_weapon_nodes.size()==weapon_model.bones.size(),
            "Needler overlay has no completed original node bank");
        const auto node=*needle_node;
        const auto graph_node=std::find(ammo.nodes.begin(),ammo.nodes.end(),weapon_model.bones[node].name);
        check(graph_node!=ammo.nodes.end(),"Needler overlay node lost its authored graph identity");
        const auto reference=sample_animation(ammo,graph_node-ammo.nodes.begin(),expected,weapon_model.bones[node].bind);
        const auto parent=weapon_model.bones[node].parent;
        check(parent<0 || unsigned(parent)<sample.completed_weapon_nodes.size(),
            "Needler overlay node lost its actual parent");
        const auto parent_scale=parent<0?1.f:sample.completed_weapon_nodes[unsigned(parent)].scale;
        const auto actual=sample.completed_weapon_nodes[node].scale;
        check(std::abs(actual-reference.scale*parent_scale)<1e-5f,
            "Completed original Needler needle scale differs from its authored ammunition frame");
      }else {
        check(sample.ammunition_animation==ammo.source_index,
            "Live Needler pose lost its authored ammunition overlay");
        check(sample.ammunition_frame==expected,"Live Needler ammunition pose disagrees with authoritative magazine");
      }
      ammunition_frames.insert(expected);
      check(sample.weapon_nodes.size()==weapon_model.bones.size(),"Live Needler pose lost its shared node bank");
      if(p.loaded==w.capacity)full_needle_scale=sample.weapon_nodes[*needle_node].scale;
      if(p.loaded<w.capacity)used_needle_scale=used_needle_scale<0?sample.weapon_nodes[*needle_node].scale:
          std::min(used_needle_scale,sample.weapon_nodes[*needle_node].scale);
    }
    if(charge_cycle && sample.first_person && sample.weapon==owner) {
      const auto charged=std::find_if(definition.clock_clips.begin(),definition.clock_clips.end(),
          [](const auto &clip){return clip.activity==0x0b000067u;});
      check(charged!=definition.clock_clips.end(),"Actual overcharged clip absent");
      auto *source=o.prepared_world->match.source_object_runtime();
      check(source && source->owns_physics(),"Charged poses require their actual source owner");
      const auto hand=source->tu15_world().first_person_view(0).hands[0];
      if(hand.activity==charged->activity && !sample.vertices.empty()) {
        check(hand.weapon==owner && hand.graph==definition.graph &&
            hand.animation_identifier==first_person_clip_identifier(*charged) && sample.animation==charged->index,
            "Rendered charge lost its actual source activity/packed clip/weapon owner");
        ++charged_pose_draws;
        if(source_trace_path && source_charge_poses.size()<512) {
          std::uint64_t hash=1469598103934665603ull;
          for(const auto &v:sample.vertices)for(float x:{v.position.x,v.position.y,v.position.z}) {
            hash^=std::bit_cast<std::uint32_t>(x);hash*=1099511628211ull;
          }
          source_charge_poses.push_back({{"input_observer_tick",tick},{"activity",hand.activity},
              {"source_frame",hand.frame},{"source_packed_clip",hand.animation_identifier},
              {"animation_index",sample.animation},{"frame",sample.frame},{"vertices",sample.vertices.size()},
              {"vertex_position_hash",hash}});
        }
      }
    }
    if(source_trace_path && sword && sample.first_person && sample.weapon==owner && sword_pose_samples.size()<256) {
      std::uint64_t hash=1469598103934665603ull;
      for(const auto &v:sample.vertices)for(float x:{v.position.x,v.position.y,v.position.z}) {
        hash^=std::bit_cast<std::uint32_t>(x);hash*=1099511628211ull;
      }
      sword_pose_samples.push_back({{"input_observer_tick",tick},{"animation_index",sample.animation},
          {"frame",sample.frame},{"vertices",sample.vertices.size()},{"vertex_position_hash",hash}});
    }
    const auto expected_animation=sword && !ready?
        (selected_sword_attack?int(selected_sword_attack->index):-1):int(action->index);
    bool expected_pose=sample.animation==expected_animation;
    if(!sword && sample.first_person && sample.weapon==owner) {
      auto *source=o.prepared_world->match.source_object_runtime();
      check(source && source->owns_physics(),"Attack variants require their actual source channel");
      const auto hand=source->tu15_world().first_person_view(0).hands[0];
      expected_pose=hand.activity==activity && authored_attack_variants.contains(hand.animation_identifier);
      if(expected_pose)check(hand.graph==definition.graph && sample.animation==(hand.animation_identifier>>16),
          "Authored attack variant lost its source graph/packed clip/render correspondence");
    }
    if(sample.first_person && sample.weapon==owner && expected_pose) {
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
      check(input(material_turned_on)==1.f,"World sword lost original turned_on input");
      ++materials;shader=true;return;
    }
    ++materials;
    if(sword && (m.name.ends_with("\\energy_blade_top")||m.name.ends_with("\\energy_blade_bottom"))) {
      auto *source=o.prepared_world->match.source_object_runtime();
      check(source && source->owns_physics() && source->object_definition(handle)==target,
          "Sword alive input lost its actual full source weapon owner");
      const auto flags=Bytes(source->memory().view(source->object_body(handle)+0x10au,2)).u16(0);
      // Complete10DE40/BEC50's authenticated eight-row period, verified
      // independently for all65536 flag words by the retained original oracle.
      constexpr std::array<unsigned,8> alive_bits{0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u,0,0,0,0};
      check(std::bit_cast<unsigned>(input(material_alive))==alive_bits[flags%8],
          "Held sword material lost its actual object's original alive input");
      ++alive_input_observations;
      const auto body=source->object_body(handle);
      const auto &memory=source->memory();
      const auto clock=Bytes(memory.view(Bytes(memory.view(0x51df54u,4)).u32(0)+8u,4)).u32(0);
      const auto item_flags=Bytes(memory.view(body+0x12cu,4)).u32(0);
      const auto weapon_flags=Bytes(memory.view(body+0x16cu,4)).u32(0);
      const auto power=Bytes(memory.view(body+0x184u,4)).f32(0);
      const auto countdown=std::bit_cast<std::int16_t>(Bytes(memory.view(body+0x19eu,2)).u16(0));
      const auto actual=input(material_turned_on);
      // The input-observer pickup tick is a separate phase from original
      // 1040F0/1021D0. Check the actual source countdown progression against
      // the authenticated original sequence; do not assume a first decrement
      // already occurred merely because the fixture observed the pickup.
      const bool item_active=(item_flags&1u)!=0 && (item_flags&2u)==0;
      const bool on=(!item_active || (weapon_flags&0x20u)!=0) && power<=1.f && power!=1.f;
      const auto initial=std::int16_t(std::nearbyint(30.f*w.ready_seconds));
      if(!sword_material_clock || *sword_material_clock!=clock) {
        sword_material_timeline.push_back({{"source_tick",clock},{"input_observer_tick",tick},
            {"pickup_observer_tick",*picked},{"ready_countdown",countdown},
            {"item_flags",item_flags},{"weapon_flags",weapon_flags},
            {"power_bits",std::bit_cast<unsigned>(power)},{"turned_on_bits",std::bit_cast<unsigned>(actual)},
            {"blade_combiner_word",m.forward_program.words[43]}});
        if(const auto *path=std::getenv("HALO2_SWORD_MATERIAL_ACCEPTANCE_REPORT")) {
          const nlohmann::json diagnostic{{"passed",false},{"status","observing"},
              {"runtime_cache_source",mcc?"MCC":"Xbox reference"},{"weapon",owner},{"definition",target},
              {"authored_ready_ticks",initial},{"source_timeline",sword_material_timeline},
              {"rendered_Xbox_parity_verified",false},{"audio_or_full_weapon_sequence_verified",false}};
          std::ofstream out(path);out<<diagnostic.dump(2)<<'\n';check(bool(out),"Cannot retain sword timing diagnostics");
        }
        if(sword_material_clock) {
          const auto elapsed=clock-*sword_material_clock;
          check(elapsed<=31 && countdown==std::max(0,int(sword_material_countdown)-int(elapsed)),
              "Sword source countdown differs from original1021D0 progression");
        } else check(countdown==initial || countdown==initial-1,
            "First observed Sword ready timer differs from original1040F0 conversion");
        sword_material_clock=clock;sword_material_countdown=countdown;
      }
      check(item_active && countdown>=0 && countdown<=initial && bool(weapon_flags&0x20u)==(countdown==0),
          "Carried Sword ready fields differ from original1021D0 sequence");
      check(actual==float(on),"Carried Sword material differs from its actual original ready fields");
      check((m.forward_program.words[43]!=0)==on,"Sword blade combiner ignored ready state");
      if(on)++lit_blade_draws;else ++hidden_blade_draws;
      shader=true;blade|=m.forward_program.words[43]!=0;
    }
    if(energy && m.name.ends_with("\\plasma_rifle_gauge")) {
      check(input(material_one)==1.f,"Actual live gauge lost its cache-local named one input");
      ++gauge_input_observations;
    }
    if(energy) {
      const auto &source=state.players[slot].weapon_shader;
      check(input(material_heat)==std::clamp(source.heat,0.f,1.f),"Carried energy material lost its actual heat input");
      check(input(material_age)==std::clamp(source.power_fraction,0.f,1.f),"Carried energy material lost its actual age input");
      shader=true;
    } else if(!sword) {
      const auto expected=float(state.players[slot].loaded)/float(w.capacity);
      check(input(material_ammunition)==expected,"Carried Needler lost original primary_ammunition input");
      if(needler)check(input(authored_ammo_alias)==expected,"Carried Needler lost authored ammunition function");shader=true;
    }
  };
  o.world_weapon_shader_input_observer=[&](auto handle,auto datum,auto name,float value) {
    if(!world || datum!=target || name!=material_turned_on)return;
    check(std::any_of(state.world_weapons.begin(),state.world_weapons.end(),[&](const auto &w){return w.handle==handle;}),
        "World shader has no actual live pickup owner");
    check(value==1.f,"World sword lost original turned_on input");shader=true;++materials;
  };
  o.audio_observer=[&](const MultiplayerAudioSubmission &s){
    if(s.owner==owner)observe_selected_sword_attack();
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
    for(const auto &draw:particles)if(rocket && draw.effect_definition==rocket_exhaust)++rocket_exhaust_draws;
    for(const auto &draw:particles)if(draw.model) {
      ++source_model_particle_draws;
      if(source_trace_path && source_model_particle_observations.size()<32) {
        source_model_particle_observations.push_back({{"tick",t},{"effect_owner",draw.effect_owner},
            {"effect_definition",draw.effect_definition},{"vertex_class",draw.material->vertex.classification},
            {"shader",draw.model->shader},{"geometry_vertices",draw.model->vertices.size()},
            {"geometry_indices",draw.model->indices.size()},{"selected_first",draw.model_first},
            {"selected_count",draw.model_count}});
      }
    }
    if(!sword && picked && state.players[slot].shot_tick)firing_particles+=unsigned(particles.size());
    if(sword && picked && t-*picked<15)check(particles.empty(),"Sword ignition particles appeared before authored delay");
    if(sword && picked && t-*picked>=15 && t-*picked<40) {
      if(!ignition_particles && !particles.empty())std::cout<<"First ignition particle observation: tick="<<t<<" relative="<<t-*picked<<'\n';
      ignition_particles+=unsigned(particles.size());
    }
  };
  o.weapon_projectile_draw_observer=[&](unsigned view,std::uint32_t rendered_tick,std::uint32_t handle,
      std::uint32_t definition,std::span<const Vertex> vertices) {
    if((!needler && !charge_cycle && !rocket) || definition!=fired_projectile)return;
    check(!vertices.empty(),"Actual fired Needler shard submitted empty geometry");
    check(std::any_of(state.weapon_projectiles.begin(),state.weapon_projectiles.end(),
        [&](const auto &p){return p.datum==handle && p.definition==definition;}),
        "Actual fired shard draw has no live salted owner");
    ++projectile_draws;projectile_owners.insert(handle);
    if(source_trace_path && projectile_geometry_samples.size()<256) {
      std::uint64_t hash=1469598103934665603ull;
      for(const auto &v:vertices)for(float x:{v.position.x,v.position.y,v.position.z}) {
        hash^=std::bit_cast<std::uint32_t>(x);hash*=1099511628211ull;
      }
      projectile_geometry_samples.push_back({{"view",view},{"tick",rendered_tick},
          {"owner",handle},{"definition",definition},{"vertices",vertices.size()},
          {"vertex_position_hash",hash}});
    }
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
  try {
    check(run_multiplayer(o)==0,"Actual covenant weapon runtime failed");
  }catch(const std::exception &error) {
    // Keep already observed inputs and poses when a genuine source owner
    // fails before normal completion. This is a failure diagnostic, never
    // an accepted attack or a replacement for the normal assertions below.
    if(source_trace_path) {
      nlohmann::json diagnostic{{"render_loop_completed",false},{"runtime_error",error.what()},
          {"source_pose_observations",source_pose_observations},
          {"projectile_geometry_draws",projectile_draws},{"projectile_geometry_owners",projectile_owners},
          {"projectile_geometry_samples",projectile_geometry_samples},
          {"source_model_particle_draws",source_model_particle_draws},
          {"source_model_particle_observations",source_model_particle_observations},
          {"projectile_birth_requests",projectile_birth_requests},{"projectile_births",projectile_births},
          {"projectile_retirements",projectile_retirements},
          {"runtime_cache_source",mcc?"MCC":"Xbox reference"},
          {"runtime_cache_path",cache.path().string()},{"weapon",owner},{"definition",target},
          {"picked_tick",picked.value_or(0)},{"samples",source_trace},
          {"weapon_effect_requests",weapon_effect_requests},{"source_sound_submissions",source_sound_submissions},
          {"pending_weapon_effect_requests",weapon_effect_pending.size()},
          {"pending_sound_submissions",sound_submit_pending.size()},
          {"sword_selection_calls",sword_selection_calls},{"pending_sword_selection_calls",sword_selection_pending.size()},
          {"reload_timing_calls",reload_timing_calls},{"pending_reload_timing_calls",reload_timing_pending.size()},
          {"staged_reload_timeline",staged_reload_timeline},{"staged_reload_poses",staged_reload_poses},
          {"source_charge_poses",source_charge_poses},{"charged_live_owners",charged_live_owners},
          {"charged_live_samples",charged_live_samples},{"charged_pose_draws",charged_pose_draws},
          {"full_weapon_sequence_verified",false},{"rendered_Xbox_parity_verified",false}};
      if(sword) {
        diagnostic["expected_attack_activity"]=activity;
        diagnostic["expected_attack_animation_index"]=action->index;
        diagnostic["actual_pose_samples"]=sword_pose_samples;
      }
      std::ofstream report(source_trace_path);report<<diagnostic.dump(2)<<'\n';
      if(!report)std::cerr<<"Cannot retain bounded failed weapon-state trace\n";
    }
    throw;
  }
  std::cout<<"Pickup gate observations: requested="<<requested.value_or(0)<<" picked="<<picked.value_or(0)
      <<" focus="<<focus_samples<<" pickup_inputs="<<pickup_input_samples
      <<" maximum_pickup_ms="<<maximum_pickup_milliseconds<<'\n';
  if(source_trace_path) {
    check(weapon_effect_pending.empty(),"Original weapon-effect observation did not complete");
    check(sound_submit_pending.empty(),"Original sound-submit observation did not complete");
    check(sword_selection_pending.empty() && sword_selection_depth==0,"Original Sword selector observation did not complete");
    check(reload_timing_pending.empty() && reload_timing_depth==0,"Original magazine timing observation did not complete");
    nlohmann::json trace{{"render_loop_completed",true},{"native_motion_after_pickup",native_motion},
        {"source_pose_observations",source_pose_observations},
        {"projectile_geometry_draws",projectile_draws},{"projectile_geometry_owners",projectile_owners},
        {"projectile_geometry_samples",projectile_geometry_samples},
        {"rocket_exhaust_draws",rocket_exhaust_draws},{"rocket_exhaust_definition",rocket_exhaust},
        {"source_model_particle_draws",source_model_particle_draws},
        {"source_model_particle_observations",source_model_particle_observations},
        {"runtime_cache_source",cache.is_mcc()?"MCC":"Xbox reference"},
        {"runtime_cache_path",cache.path().string()},{"native_world_constructor",o.gameplay_xbe.empty()},
        {"cache_local_material_symbols",{{"one",material_one},{"turned_on",material_turned_on},
            {"heat",material_heat},{"age",material_age},{"primary_ammunition",material_ammunition},{"alive",material_alive}}},
        {"weapon_effect_requests",weapon_effect_requests},
        {"source_sound_submissions",source_sound_submissions},
        {"source_shot_tick_equality_checked",verify_source_shot_tick},
        {"picked_tick",picked.value_or(0)},{"legacy_fired_rounds",fired_rounds},
        {"firing_sound_submissions",firing_sounds},{"maximum_heat",maximum_heat},
        {"maximum_spent",maximum_spent},{"actual_gauge_material_observations",gauge_input_observations},
        {"original_source_position_owner",true},{"samples",source_trace}};
    trace["projectile_births"]=projectile_births;
    trace["projectile_birth_requests"]=projectile_birth_requests;
    trace["authored_attack_variants"]=authored_attack_variants;
    if(sword) {
      trace["sword_selection_calls"]=sword_selection_calls;
      trace["expected_attack_activity"]=selected_sword_attack?selected_sword_attack->activity:activity;
      trace["expected_attack_animation_index"]=selected_sword_attack?selected_sword_attack->index:action->index;
      trace["expected_strike_observed_from_actual_source_action"]=selected_sword_attack!=nullptr;
      trace["original_selected_candidate_activity"]=original_sword_selected_activity.value_or(null_datum);
      trace["actual_pose_samples"]=sword_pose_samples;
      nlohmann::json candidates=nlohmann::json::array();
      for(const auto &clip:definition.clock_clips)
        if(clip.activity>=0x0e000607u && clip.activity<=0x0e00060au)
          candidates.push_back({{"activity",clip.activity},{"animation_index",clip.index},{"frames",clip.frames}});
      trace["authored_strike_candidates"]=candidates;
    }
    trace["projectile_retirements"]=projectile_retirements;
    if(charge_cycle || pistol_tap) {
      trace["charge_input_phase"]=charge_cycle?"full_charge":"early_release";
      trace["source_charge_poses"]=source_charge_poses;
      trace["charged_pose_draws"]=charged_pose_draws;
      trace["charged_live_samples"]=charged_live_samples;
      trace["charged_live_owners"]=charged_live_owners;
      trace["expected_fired_projectile"]=fired_projectile;
      trace["expected_firing_sound"]=firing_sound;
    }
    if(reload_cycle) {
      trace["reload_timing_calls"]=reload_timing_calls;
      trace["pending_reload_timing_calls"]=reload_timing_pending.size();
      trace["staged_reload_requested_tick"]=staged_reload_requested.value_or(0);
      trace["staged_reload_refilled"]=staged_reload_refilled;
      trace["staged_reload_initial_loaded"]=staged_reload_loaded;
      trace["staged_reload_initial_reserve"]=staged_reload_reserve;
      trace["staged_reload_timeline"]=staged_reload_timeline;
      trace["staged_reload_poses"]=staged_reload_poses;
    }
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
    trace["source_unit_definitions_observed"]=source_unit_definition_observations>0;
    trace["source_unit_definition_observations"]=source_unit_definition_observations;
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
    if(rocket) {
      check(flight_owners.size()==1 && live_flight_samples>4 && maximum_projectile_travel>2.f && projectile_draws>4,
          "Clear firing lane did not submit the actual rocket in sustained flight");
    } else {
    check(flight_owners.size()==4 && live_flight_samples>4 && maximum_projectile_travel>2.f,
        "Clear firing lane did not expose all four source bolts in sustained flight");
    check(glow_draws>0 && glow_owners==flight_owners && glow_screen_views==((1u<<local_players)-1u),
        "Live source bolts did not submit their authored glow into the actual gameplay view");
    std::cout<<"Clear-lane observations: "<<live_flight_samples<<" live samples, "<<maximum_projectile_travel
        <<" maximum travel, "<<glow_draws<<" glow draws / "<<glow_owners.size()<<" owners\n";
    }
  }
  if(std::string(argv[4])=="plasma_rifle" || std::string(argv[4])=="brute_plasma_rifle") {
    check(gauge_input_observations>0,"Actual live gauge material was never observed");
    std::cout<<"Gauge one observations: "<<gauge_input_observations<<'\n';
    if(const auto *path=std::getenv("HALO2_GAUGE_ACCEPTANCE_REPORT")) {
      std::ofstream report(path);
      report<<"{\"passed\":true,\"component_scope\":\"held_gauge_input\",\"actual_gauge_material_observations\":"<<gauge_input_observations
          <<",\"runtime_cache_source\":\""<<(mcc?"MCC":"Xbox reference")<<"\",\"cache_local_one_id\":"<<material_one
          <<",\"rendered_Xbox_parity_verified\":false,\"full_weapon_sequence_verified\":false}\n";
      check(bool(report),"Cannot retain live gauge acceptance report");
    }
  }
  if(material_only) {
    if(sword) {
      check(alive_input_observations>0 && hidden_blade_draws>0 && lit_blade_draws>0,
          "Sword material-only case requires actual alive input and both original ready states");
      if(const auto *path=std::getenv("HALO2_SWORD_MATERIAL_ACCEPTANCE_REPORT")) {
        const nlohmann::json report{{"passed",true},{"component_scope","held_sword_alive_and_ready_inputs"},
            {"runtime_cache_source",mcc?"MCC":"Xbox reference"},{"weapon",owner},{"definition",target},
            {"alive_material_observations",alive_input_observations},{"cache_local_alive_id",material_alive},
            {"hidden_blade_draws",hidden_blade_draws},{"lit_blade_draws",lit_blade_draws},
            {"source_unit_definition_observations",source_unit_definition_observations},
            {"local_players",local_players},{"selected_representation",representation},
            {"actual_unit_definitions",std::vector(actual_unit_definitions.begin(),actual_unit_definitions.begin()+local_players)},
            {"source_timeline",sword_material_timeline},{"source_ready_countdown_progression_verified",true},
            {"pickup_observer_first_decrement_assumed",false},
            {"rendered_Xbox_parity_verified",false},
            {"audio_or_full_weapon_sequence_verified",false}};
        std::ofstream out(path);out<<report.dump(2)<<'\n';check(bool(out),"Cannot retain bounded sword material report");
      }
      std::cout<<"PASS actual SDL sword pickup,source owner,alive input and ready blade states\n";return 0;
    }
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
  if(rocket && std::getenv("HALO2_TEST_ROCKET_VISUALS")) {
    check(projectile_draws>4 && !projectile_owners.empty(),"Live rocket model did not reach the renderer");
    check(rocket_exhaust!=null_datum && rocket_exhaust_draws>4,
        "Live rocket exhaust never reached particle submission");
    std::cout<<"PASS actual rocket flight: "<<projectile_draws<<" model draws, "
        <<rocket_exhaust_draws<<" authored exhaust draws\n";
    return 0;
  }
  if(ammunition) {
    check(empty_tick && (refilled || !o.screenshot.empty()),"Live Needler did not complete its actual firing/empty/reload cycle");
    check(ammunition_frames.size()>1 && full_needle_scale>=0 && used_needle_scale>=0 && full_needle_scale!=used_needle_scale,
        "Live Needler consumed ammo without retracting needle geometry");
    std::cout<<"Needler ammunition observations: frames="<<ammunition_frames.size()<<" needle scale full="<<full_needle_scale<<" used="<<used_needle_scale<<'\n';
  }
  if(sword && !ready)check(selected_sword_attack,"Actual source never selected an authored Sword strike");
  if(sword && !ready && trace_sword_selection)
    check(original_sword_selected_activity && *original_sword_selected_activity==selected_sword_attack->activity,
        "Source Sword pose expectation lacks its original selected candidate");
  const auto *tested_action=sword && !ready?selected_sword_attack:&*action;
  const auto minimum_samples=unsigned(std::min(5,std::max(2,int(tested_action->frames)-2)));
  std::cout<<"Attack observations: "<<draws<<" draws, "<<frames.size()<<" frames, "<<pose_hashes.size()<<" distinct poses\n";
  check(picked&&posed&&frames.size()>=minimum_samples&&draws>=minimum_samples,
      "Actual attack did not submit advancing authored poses");
  check(pose_hashes.size()>=minimum_samples,"Authored attack clock advanced without moving the rendered weapon vertices");
  if(reload_cycle) {
    check(staged_reload_requested && staged_reload_refilled,"Actual shotgun did not complete its staged reload and return to idle");
    check(state.players[slot].loaded==w.capacity &&
        staged_reload_reserve-state.players[slot].reserve==w.capacity-staged_reload_loaded,
        "Staged reload did not conserve actual source magazine/reserve ammunition");
    for(const auto activity:{0x0c0005aeu,0x140005b0u,0x0b0005b1u}) {
      std::set<float> phase_frames;std::set<std::uint64_t> phase_poses;
      for(const auto &pose:staged_reload_poses)if(pose.at("activity").get<std::uint32_t>()==activity) {
        check(pose.at("vertices").get<unsigned>()>0,"Staged reload submitted an empty authored pose");
        phase_frames.insert(pose.at("frame").get<float>());phase_poses.insert(pose.at("vertex_position_hash").get<std::uint64_t>());
      }
      check(phase_frames.size()>=2 && phase_poses.size()>=2,"Staged reload lost advancing enter/continue/exit geometry");
    }
    std::cout<<"Staged reload: actual enter/continue/exit geometry and conserved source ammunition\n";
  }
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
  else if(shotgun)check(state.players[slot].shot_tick>0 && (reload_cycle || state.players[slot].loaded<w.capacity),"Actual shotgun did not fire or consume a shell");
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
    check(charged_pose_draws>10,"Actual full charge never submitted source-owned overcharged poses");
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
