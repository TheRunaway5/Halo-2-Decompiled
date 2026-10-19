#define SDL_MAIN_HANDLED
#include "halo2/animation_presentation.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/model.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <thread>
using namespace halo2;
namespace {
struct HistoryReport {
  const char *path=std::getenv("HALO2_TEST_DUAL_HISTORY_REPORT");
  nlohmann::json data{{"samples",nlohmann::json::array()},{"render_loop_completed",false},
      {"full_weapon_parity",false},{"original_position_owner",true},
      {"diagnostic_placement","Pinned at the current authored pickup row through original B95D0"}};
  ~HistoryReport(){if(path){std::ofstream file(path);file<<data.dump(2)<<'\n';}}
};
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
void key(Uint32 window,SDL_Scancode scan) {
  SDL_Event e{};e.type=SDL_KEYDOWN;e.key.windowID=window;e.key.keysym.scancode=scan;e.key.state=SDL_PRESSED;
  check(SDL_PushEvent(&e)==1,"Queue weapon key press");e.type=SDL_KEYUP;e.key.state=SDL_RELEASED;
  check(SDL_PushEvent(&e)==1,"Queue weapon key release");
}
void hold_key(Uint32 window,SDL_Scancode scan,bool down) {
  SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.windowID=window;e.key.keysym.scancode=scan;
  e.key.state=down?SDL_PRESSED:SDL_RELEASED;check(SDL_PushEvent(&e)==1,"Queue held interaction key");
}
void trigger(Uint32 window,Uint8 button,bool down) {
  SDL_Event e{};e.type=down?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;e.button.windowID=window;
  e.button.button=button;e.button.state=down?SDL_PRESSED:SDL_RELEASED;
  check(SDL_PushEvent(&e)==1,"Queue weapon trigger");
}
}
int main(int argc,char **argv){try {
  check(argc==4 || argc==5,"Supply MAP SHARED XBE [CAPTURE]");
  const bool mcc=std::string(argv[3])=="native-mcc";
  if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]) || (!mcc && !std::filesystem::exists(argv[3])))return 77;
  SDL_SetHintWithPriority(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0xffff/0xffff",SDL_HINT_OVERRIDE);
  SDL_SetMainReady();MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
  if(!mcc)options.gameplay_xbe=argv[3];
  options.width=800;options.height=600;options.local_players=1;options.starting_weapon=5;options.mute=false;options.input_instances={-1};
  options.frames=argc==5?0u:320u;options.reference_tick_cadence=true;options.reference_render_fraction=.5f;
  const bool verify_world_poses=std::getenv("HALO2_TEST_DUAL_WORLD_POSES")!=nullptr;
  if(verify_world_poses)options.reference_render_fraction=1.f; // Exact completed-tick original palette comparison.
  const auto *representation_option=std::getenv("HALO2_TEST_REPRESENTATION");
  const unsigned representation=representation_option?unsigned(std::stoul(representation_option)):2u;
  check(representation==2 || representation==3,"Unsupported actual multiplayer representation");
  if(representation==3){auto profile=source_default_player_configuration();profile[0x44]=1;options.selected_profiles={profile};}
  options.validation=std::getenv("HALO2_ANIMATION_VALIDATION");
  if(const auto *artwork=std::getenv("HALO2_TEST_HUD_UI_ASSETS"))options.ui_assets=artwork;
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  const auto &world=*options.prepared_world;
  check(!mcc || world.cache->is_mcc(),"Weapon runtime testing requires an MCC cache");
  HistoryReport history;history.data["representation"]=representation;
  history.data["expected_unit_definition"]=world.content.player_representations.at(representation).biped.datum;
  const NetgameEquipment *row=nullptr,*primary_row=nullptr;
  for(const auto &r:world.content.netgame_equipment)
    if(!r.classification && source_netgame_equipment_selector(r,true,world.rules.source_engine)>=0 &&
        r.items.size()==1) {
      if(!row && r.items[0].definition==world.rules.weapon)row=&r;
      const auto w=world.content.weapons.find(r.items[0].definition);
      if(!primary_row && w!=world.content.weapons.end() && w->second.name.find("battle_rifle")!=std::string::npos)
        primary_row=&r;
    }
  check(row && primary_row,"Actual map has no SMG/battle-rifle pickup rows");
  // Only initial diagnostic placement is supplied. Item allocation, transfer,
  // clocks, trigger/reload/grenade outcomes and poses come from the live loop.
  const auto location=row->position;const auto slot=world.local_slots.at(0);
  options.desktop=std::make_shared<DesktopSession>();auto *window=options.desktop->window(800,600,false);
  std::cout<<"Native SDL video driver: "<<SDL_GetCurrentVideoDriver()<<'\n';
  const auto window_id=SDL_GetWindowID(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  std::optional<unsigned> start,carried_at,dual_at,grenade_at;MultiplayerState current;unsigned step{},draws{};
  std::uint32_t primary_target=null_datum,starting_datum=null_datum;
  bool map_pickup{},switched{},dual_requested{};
  bool raised{},dual{},right_fired{},left_fired{},reloaded{},thrown{},dropped{};
  bool offhand_radar{},primary_radar{},grenade_radar_phase{};
  bool reload_received{},lost_focus{};
  unsigned firing_particles{},firing_sounds{},fired_rounds{};
  std::optional<std::array<int,2>> previous_ammunition;
  const auto firing_sound=world.content.weapons.at(world.rules.weapon).sound;
  std::set<std::uint32_t> posed_weapons;std::set<unsigned> clips;
  unsigned history_checks{},history_mismatches{};
  unsigned world_pose_publications{},world_pose_draws{},dropped_pose_draws{},world_pose_mismatches{},world_palette_matrices{};
  history.data["world_pose_metrics"]={{"enabled",verify_world_poses},{"render_fraction",options.reference_render_fraction}};
  const bool follow_drop=std::getenv("HALO2_TEST_DUAL_DROP_LIFETIME")!=nullptr;
  std::uint32_t offhand_datum=null_datum;
  unsigned drop_checks{},drop_mismatches{},drop_preview_differences{};
  std::optional<unsigned> drop_tick,retired_tick;
  const auto dropped_model=follow_drop?std::optional{world.cache->load_model(world.rules.weapon,false)}:std::nullopt;
  const auto dropped_idle=follow_drop?world.cache->load_initialized_object_idle(world.rules.weapon):std::nullopt;
  const auto observe_drop=[&](const MultiplayerState &state,unsigned tick,bool rendered) {
    if(!follow_drop || offhand_datum==null_datum || state.players[slot].secondary_weapon.datum!=null_datum)return;
    auto *source=options.prepared_world->match.source_object_runtime();
    check(source && source->owns_physics(),"Drop lifetime requires the original world owner");
    const auto objects=source->live_objects();
    const auto object=std::find_if(objects.begin(),objects.end(),[&](const auto &o){return o.object==offhand_datum;});
    const auto count=std::count_if(state.world_weapons.begin(),state.world_weapons.end(),
        [&](const auto &w){return w.handle==offhand_datum;});
    const auto &inventory=options.prepared_world->match.unit(slot).source_inventory.slots;
    const bool owned=std::find(inventory.begin(),inventory.end(),offhand_datum)!=inventory.end();
    const auto &memory=source->memory();
    const auto word=[&](std::uint32_t address){return Bytes(memory.view(address,4)).u32(0);};
    if(!drop_tick)drop_tick=tick;
    ++drop_checks;
    if(object==objects.end()) {
      if(!retired_tick)retired_tick=tick;
      drop_mismatches+=count!=0 || owned;
      history.data["drop_retired_tick"]=*retired_tick;
      return;
    }
    const auto body=source->object_body(offhand_datum);
    const auto root=source->object_transform(offhand_datum);
    const auto item=std::find_if(state.world_weapons.begin(),state.world_weapons.end(),
        [&](const auto &w){return w.handle==offhand_datum;});
    const auto loaded=std::bit_cast<std::int16_t>(Bytes(memory.view(body+0x22cu,2)).u16(0));
    const auto reserve=std::bit_cast<std::int16_t>(Bytes(memory.view(body+0x22au,2)).u16(0));
    const bool matches=!owned && object->parent==null_datum && object->type==2 &&
        object->definition==world.rules.weapon && count==1 && item!=state.world_weapons.end() &&
        item->definition==object->definition && item->loaded==loaded && item->reserve==reserve &&
        !std::memcmp(&item->position,&root.position,sizeof(Vec3));
    drop_mismatches+=!matches;
    const auto nodes=source->world_nodes(offhand_datum);
    const auto preview=pose_model_nodes_world(*dropped_model,
        source_object_transform(root.position,root.basis[0],root.basis[2],1,false),
        dropped_idle?&*dropped_idle:nullptr,float(tick)/30.f);
    const bool same=nodes.size()==preview.size() &&
        (nodes.empty() || !std::memcmp(nodes.data(),preview.data(),nodes.size()*sizeof(Transform)));
    drop_preview_differences+=!same;
    // Source and preview are compared at one completed simulation tick here,
    // not claimed as an interpolated GPU/framebuffer comparison.
    if(rendered || tick-*drop_tick<4 || (tick-*drop_tick)%30==0) {
      std::array<std::uint32_t,19> object_words{};std::array<std::uint32_t,57> weapon_words{};
      auto bytes=memory.view(body+0x120u,sizeof(object_words));std::memcpy(object_words.data(),bytes.data(),bytes.size());
      bytes=memory.view(body+0x16cu,sizeof(weapon_words));std::memcpy(weapon_words.data(),bytes.data(),bytes.size());
      nlohmann::json row{{"tick",tick},{"source_tick",word(word(0x51df54u)+8u)},
          {"object",offhand_datum},{"definition",object->definition},{"parent",object->parent},
          {"inventory_owned",owned},{"world_publications",count},{"publication_matches",matches},
          {"loaded",loaded},{"reserve",reserve},{"position",{root.position.x,root.position.y,root.position.z}},
          {"published_loaded",item==state.world_weapons.end()?-1:item->loaded},
          {"published_reserve",item==state.world_weapons.end()?-1:item->reserve},
          {"object_words_120",object_words},{"weapon_words_16c",weapon_words},
          {"source_nodes",nodes.size()},{"preview_nodes",preview.size()},
          {"completed_source_and_preview_nodes_equal",same},{"render_loop_sample",rendered}};
      if(tick==*drop_tick || (tick-*drop_tick)%120==0) {
        std::vector<std::array<std::uint32_t,13>> bank(nodes.size());
        static_assert(sizeof(Transform)==52);
        if(!nodes.empty())std::memcpy(bank.data(),nodes.data(),nodes.size()*sizeof(Transform));
        row["source_world_node_words"]=bank;
      }
      history.data["drop_samples"].push_back(std::move(row));
    }
  };
  std::optional<SourcePhysicsEntryObserverBinding> retirement_observer;
  std::vector<std::size_t> retirement_pending;
  if(follow_drop) {
    auto *source=options.prepared_world->match.source_object_runtime();
    check(source && source->owns_physics(),"Drop retirement trace requires original storage");
    const auto &memory=source->memory();
    retirement_observer.emplace(memory,[&](bool before,std::uint32_t entry,const auto &abi) {
      if(entry!=0xba490u)return;
      const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
      if(before) {
        const auto object=word(abi.gp[4]+4u);
        if(object!=offhand_datum || object==null_datum){retirement_pending.push_back(SIZE_MAX);return;}
        auto &rows=history.data["source_retirement_requests"];
        if(rows.is_null())rows=nlohmann::json::array();retirement_pending.push_back(rows.size());
        rows.push_back({{"object",object},{"source_tick",word(word(0x51df54u)+8u)},
            {"caller_return",word(abi.gp[4])},{"completed",false}});
      }else {
        check(!retirement_pending.empty(),"Unpaired original drop retirement observation");
        const auto index=retirement_pending.back();retirement_pending.pop_back();
        if(index!=SIZE_MAX)history.data["source_retirement_requests"].at(index)["completed"]=true;
      }
    });
  }
  const auto report_observations=[&] {
    history.data["observations"]={{"map_pickup",map_pickup},{"switched",switched},{"dual",dual},
        {"right_fired",right_fired},{"left_fired",left_fired},{"reloaded",reloaded},
        {"thrown",thrown},{"dropped",dropped},{"offhand_radar",offhand_radar},{"primary_radar",primary_radar},
        {"fired_rounds",fired_rounds},{"firing_sounds",firing_sounds},{"firing_particles",firing_particles},
        {"posed_weapons",posed_weapons},{"clips",clips},{"posed_draws",draws},
        {"start_tick",start.value_or(0)},{"dual_tick",dual_at.value_or(0)},{"grenade_tick",grenade_at.value_or(0)}};
  };
  std::optional<Spawn> diagnostic_pose;
  const auto place=[&](const Spawn &pose) {
    options.prepared_world->match.debug_request({DebugGameRequest::Teleport,slot},&pose);
    auto *source=options.prepared_world->match.source_object_runtime();
    check(source && source->owns_physics(),"Dual placement requires the actual source pose owner");
    check(current.players[slot].source_unit!=null_datum,"Dual placement has no actual source unit");
    source->tu15_world().set_object_position(current.players[slot].source_unit,pose.position);
  };
  options.input_observer=[&](unsigned tick,std::span<const Controls> controls,const MultiplayerState &state){
    step=tick;current=state;const auto &p=state.players[slot];
    if(verify_world_poses) {
      auto *source=options.prepared_world->match.source_object_runtime();
      check(source && source->owns_physics(),"World pose comparison requires original source objects");
      for(const auto &weapon:state.world_weapons) {
        if(weapon.handle==null_datum)continue;
        const auto nodes=source->world_nodes(weapon.handle);if(nodes.empty())continue;
        const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),
            [&](const auto &p){return p.object==weapon.handle;});
        ++world_pose_publications;
        world_pose_mismatches+=pose==state.source_body_poses.end() || pose->nodes.size()!=nodes.size() ||
            (pose->nodes.size()==nodes.size() && std::memcmp(pose->nodes.data(),nodes.data(),nodes.size()*sizeof(Transform)));
      }
      history.data["world_pose_metrics"]["publication_checks"]=world_pose_publications;
      if(dropped && offhand_datum!=null_datum) {
        const auto root=source->object_transform(offhand_datum);
        const auto delta=root.position-(p.position+Vec3{0,0,world.content.player.camera_height});
        const float desired_yaw=std::atan2(delta.y,delta.x);
        const float desired_pitch=std::atan2(delta.z,std::sqrt(delta.x*delta.x+delta.y*delta.y));
        const auto &settings=options.desktop->keyboard_settings;
        const float gain=.0025f*float(settings.sensitivity)/3.f; // The existing SDL runtime mapping.
        SDL_Event motion{};motion.type=SDL_MOUSEMOTION;motion.motion.windowID=window_id;
        motion.motion.xrel=int(std::lround(-std::remainder(desired_yaw-p.yaw,6.283185307179586f)/gain));
        motion.motion.yrel=int(std::lround(-(desired_pitch-p.pitch)/gain*(settings.invert?-1.f:1.f)));
        check(SDL_PushEvent(&motion)==1,"Queue actual SDL look toward the detached source root");
      }
    }
    if(history.path) {
      auto *source=options.prepared_world->match.source_object_runtime();
      check(source && source->owns_physics(),"Dual history requires actual original source storage");
      const auto &memory=source->memory();
      const auto word=[&](std::uint32_t address){return Bytes(memory.view(address,4)).u32(0);};
      nlohmann::json sample{{"tick",tick},{"source_tick",word(word(0x51df54u)+8u)},
          {"fire",controls[0].fire},{"fire_secondary",controls[0].fire_secondary},
          {"reload",controls[0].reload},{"source_unit",p.source_unit},
          {"motion_sensor_control",p.motion_sensor_control},{"hands",nlohmann::json::array()}};
      sample["hud_storage"]=word(0x50eeccu);sample["hud_definition_address"]=word(0x51df94u);
      sample["unit_definition"]=p.source_unit==null_datum?null_datum:source->object_definition(p.source_unit);
      check(p.source_unit==null_datum || sample["unit_definition"]==history.data["expected_unit_definition"],
          "Actual dual source unit lost its selected authored species");
      for(unsigned hand=0;hand<2;++hand) {
        const auto datum=hand?p.secondary_weapon.datum:p.source_weapon;if(datum==null_datum)continue;
        const auto body=source->object_body(datum);
        std::array<std::uint32_t,57> raw{};
        const auto bytes=memory.view(body+0x16cu,sizeof(raw));std::memcpy(raw.data(),bytes.data(),bytes.size());
        const auto published_tick=hand?p.secondary_weapon.shot_tick:p.shot_tick;
        const auto definition=hand?p.secondary_weapon.definition:p.weapon;
        ++history_checks;
        history_mismatches+=published_tick!=raw[55] || source->object_definition(datum)!=definition;
        sample["hands"].push_back({{"hand",hand},{"datum",datum},{"definition",definition},
            {"source_definition",source->object_definition(datum)},{"source_words_16c",raw},
            {"published_shot_tick",published_tick},
            {"published_reload_tick",hand?p.secondary_weapon.reload_tick:p.reload_tick},
            {"published_loaded",hand?p.secondary_weapon.loaded:p.loaded},
            {"published_reserve",hand?p.secondary_weapon.reserve:p.reserve}});
      }
      history.data["samples"].push_back(std::move(sample));
      history.data["publication_checks"]=history_checks;history.data["publication_mismatches"]=history_mismatches;
    }
    reload_received|=controls[0].reload;
    lost_focus|=start.has_value() && SDL_GetKeyboardFocus()!=window;
    const auto primary_item=std::find_if(state.world_weapons.begin(),state.world_weapons.end(),[&](const auto &item) {
      const auto d=item.position-primary_row->position;
      return item.definition==primary_row->items[0].definition && d.x*d.x+d.y*d.y+d.z*d.z<1e-8f;
    });
    if(!start && primary_item!=state.world_weapons.end() && SDL_GetKeyboardFocus()==window) {
      start=tick;primary_target=primary_item->handle;starting_datum=p.source_weapon;
      Spawn at;at.position=primary_row->position;at.position.x-=.2f;at.bsp=0;at.facing=0;
      diagnostic_pose=at;place(at);hold_key(window_id,SDL_SCANCODE_E,true);
      std::cout<<"Actual map battle rifle available at tick="<<tick<<'\n';
    }
    check(tick<120 || start,"Weapon window never received actual keyboard focus");
    if(start && !map_pickup && p.source_weapon==primary_target) {
      check(p.weapon==primary_row->items[0].definition,"Map pickup changed its authored definition");
      check(std::none_of(state.world_weapons.begin(),state.world_weapons.end(),[&](const auto &item){return item.handle==primary_target;}),
          "Held map pickup still has a world owner");
      map_pickup=true;carried_at=tick;hold_key(window_id,SDL_SCANCODE_E,false);
    }
    if(carried_at && tick==*carried_at+10)key(window_id,SDL_SCANCODE_TAB);
    if(carried_at && tick>*carried_at+10 && !dual_requested && p.source_weapon==starting_datum) {
      switched=true;dual_requested=true;
      Spawn at;at.position=location;at.position.x-=.2f;at.bsp=0;at.facing=0;
      diagnostic_pose=at;place(at);hold_key(window_id,SDL_SCANCODE_X,true);
    }
    if(diagnostic_pose)place(*diagnostic_pose);
    if(p.secondary_weapon.datum!=null_datum) {
      if(offhand_datum==null_datum)offhand_datum=p.secondary_weapon.datum;
      check(offhand_datum==p.secondary_weapon.datum,"Dual history changed its full offhand datum");
      dual=true;if(!dual_at){dual_at=tick;hold_key(window_id,SDL_SCANCODE_X,false);}
      if(previous_ammunition) {
        fired_rounds+=unsigned(std::max(0,(*previous_ammunition)[0]-p.loaded));
        fired_rounds+=unsigned(std::max(0,(*previous_ammunition)[1]-p.secondary_weapon.loaded));
      }
      previous_ammunition=std::array{p.loaded,p.secondary_weapon.loaded};
      right_fired|=p.loaded<60;left_fired|=p.secondary_weapon.loaded<60;
      reloaded|=right_fired && left_fired && p.loaded==60 && p.secondary_weapon.loaded==60;
    }
    if(dual_at) {
      const auto elapsed=tick-*dual_at;
      if(elapsed==20)trigger(window_id,SDL_BUTTON_RIGHT,true);
      if(elapsed>20 && elapsed<26) {
        check(p.loaded==60,"Offhand-only SDL trigger consumed primary ammunition");
        offhand_radar|=p.motion_sensor_control==0x20;
      }
      if(elapsed==26){trigger(window_id,SDL_BUTTON_RIGHT,false);trigger(window_id,SDL_BUTTON_LEFT,true);}
      if(elapsed>26 && elapsed<32)primary_radar|=p.motion_sensor_control==1;
      if(elapsed==32){trigger(window_id,SDL_BUTTON_LEFT,false);trigger(window_id,SDL_BUTTON_RIGHT,false);}
      if(elapsed==40)key(window_id,SDL_SCANCODE_R);
      if(reloaded && !(p.source_action_word&1u) && !grenade_at && argc!=5)grenade_at=tick;
      if(grenade_at && tick==*grenade_at)key(window_id,SDL_SCANCODE_Q);
      if(grenade_at && tick>*grenade_at && p.grenade_action.animation_active)thrown=true;
      if(grenade_at && tick>*grenade_at && (p.grenade_action.phase()==1 || p.grenade_action.phase()==2)) {
        check(!p.motion_sensor_control,"SDL grenade request published a weapon trigger bit");
        grenade_radar_phase=true;
      }
      if(grenade_at && tick>*grenade_at && p.secondary_weapon.datum==null_datum)dropped=true;
      if(grenade_at && tick==*grenade_at+40){SDL_Event quit{};quit.type=SDL_QUIT;check(SDL_PushEvent(&quit)==1,"Exit weapon probe");}
    }
    observe_drop(state,tick,true);
    report_observations();
    check(!start || tick<*start+250,"Live dual pickup/animation sequence timed out");
  };
  options.animation_observer=[&](unsigned,unsigned,const AnimationRenderObservation &sample){
    if(!raised){SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);raised=true;}
    if(!sample.first_person)return;
    for(const auto &v:sample.vertices)check(std::isfinite(v.position.x)&&std::isfinite(v.position.y)&&std::isfinite(v.position.z),"Nonfinite posed weapon vertex");
    if(current.players[slot].secondary_weapon.datum!=null_datum) {
      posed_weapons.insert(sample.weapon);clips.insert(sample.animation);++draws;
    }
  };
  options.audio_observer=[&](const MultiplayerAudioSubmission &s) {
    if(s.sound==firing_sound){check(s.frames>0,"SMG firing sound submitted empty PCM");++firing_sounds;}
  };
  options.effect_observer=[&](unsigned,unsigned,std::span<const ParticleDraw> particles) {
    if(dual_at)firing_particles+=unsigned(particles.size());
  };
  std::map<std::uint32_t,SourceModelPaletteDefinition> palette_definitions;
  if(verify_world_poses)options.world_weapon_source_draw_observer=[&](unsigned,unsigned tick,
      std::uint32_t owner,std::uint32_t definition,std::span<const Transform> nodes,
      std::span<const Vertex> vertices,std::span<const SourceLiveModelBaseDrawTu15> draws) {
    ++world_pose_draws;if(dropped && owner==offhand_datum)++dropped_pose_draws;
    auto *source=options.prepared_world->match.source_object_runtime();
    check(source && source->owns_physics(),"World draw lost its original owner");
    const auto raw=source->rendered_world_nodes(owner);
    bool matches=raw && raw->size()==nodes.size() && !nodes.empty() && !vertices.empty() && !draws.empty();
    if(matches)matches=!std::memcmp(raw->data(),nodes.data(),nodes.size()*sizeof(Transform));
    const auto &memory=source->memory();
    const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    for(const auto &draw:draws) {
      check(draw.object==owner && draw.definition==definition && draw.lod<6,"World draw changed original selection identity");
      const auto model=draw.unit.words[1];
      auto found=palette_definitions.find(model);
      if(found==palette_definitions.end())found=palette_definitions.emplace(model,world.cache->load_source_model_palette_definition(model)).first;
      auto expected=source_model_skin_palette(nodes,found->second.inverse_binds);
      append_source_model_compounds(expected,found->second.compounds[draw.lod]);
      const auto region=draw.unit.words[3]&15u,section=(draw.unit.words[3]>>9)&511u;
      check(region<draw.geometry.palette.regions.size(),"World draw has no original region palette");
      const auto range=draw.geometry.palette.regions[region];
      const auto metadata=word(word(0x4f0414u)+(model&65535u)*16u+8u);
      const auto resident=word(word(metadata+0x28u)+section*92u+0x34u);
      const auto mapped=word(resident+0x64u),mapping=word(resident+0x68u);
      for(unsigned n=0;n<range.count;++n) {
        const auto node=mapped?std::to_integer<unsigned>(memory.view(mapping+n,1)[0]):n;
        check(node<expected.size() && unsigned(range.first)+n<draw.geometry.palette.matrices.size(),"Original region map escapes its WORLD palette");
        ++world_palette_matrices;
        matches&=!std::memcmp(expected[node].data(),draw.geometry.palette.matrices[range.first+n].data(),48);
      }
    }
    world_pose_mismatches+=!matches;
    auto &report=history.data["world_pose_metrics"];
    report["draws"]=world_pose_draws;report["dropped_draws"]=dropped_pose_draws;
    report["original_palette_matrices"]=world_palette_matrices;report["mismatches"]=world_pose_mismatches;
    if(!matches && !report.contains("first_mismatch"))report["first_mismatch"]={{"tick",tick},{"object",owner},{"source_nodes",raw?raw->size():0},{"submitted_nodes",nodes.size()}};
  };
  if(argc==5){options.screenshot=argv[4];options.capture_tick=240;}
  check(run_multiplayer(options)==0,"Weapon runtime failed");
  history.data["render_loop_completed"]=true;
  if(verify_world_poses) {
    check(world_pose_publications && world_pose_draws && dropped_pose_draws && world_palette_matrices && !world_pose_mismatches,
        "Loose weapon source publication/draw/palette comparison failed");
    const auto original=options.prepared_world->match.state();
    for(const auto &event:original.grenade_detonations)for(const auto &location:event.source_locations)
      history.data["world_pose_metrics"]["original_explosion_locations"].push_back({
          {"serial",event.serial},{"name",location.name},
          {"transform_words",std::bit_cast<std::array<std::uint32_t,13>>(location.frame)}});
    LanHost host(options.prepared_world->match,0,0xd770u);
    LanClient client("127.0.0.1:"+std::to_string(host.port()),0xd770u,1);
    std::array<Controls,multiplayer_capacity> incoming{};std::array<Controls,1> neutral{};
    for(unsigned n=0;n<128 && (!client.ready() || !client.state().source_body_poses_authoritative);++n) {
      client.send(neutral);host.receive(incoming);host.broadcast();client.receive();
      if(!client.ready() || !client.state().source_body_poses_authoritative)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    history.data["world_pose_metrics"]["lan_admission"]={{"ready",client.ready()},
        {"error",client.error()},{"rejected_packets",client.rejected_packet_count()},
        {"last_packet_rejection",client.last_packet_rejection()},{"host_peers",host.peers()}};
    check(client.ready() && client.error().empty(),"Loose weapon native LAN admission failed");
    const auto sent=options.prepared_world->match.state();const auto &received=client.state();
    unsigned transported{};
    for(const auto &weapon:sent.world_weapons) {
      const auto a=std::find_if(sent.source_body_poses.begin(),sent.source_body_poses.end(),[&](const auto &p){return p.object==weapon.handle;});
      if(a==sent.source_body_poses.end())continue;
      const auto b=std::find_if(received.source_body_poses.begin(),received.source_body_poses.end(),[&](const auto &p){return p.object==weapon.handle;});
      check(b!=received.source_body_poses.end() && a->nodes.size()==b->nodes.size() &&
          !std::memcmp(a->nodes.data(),b->nodes.data(),a->nodes.size()*sizeof(Transform)),"Native LAN changed a loose weapon WORLD bank");
      ++transported;
    }
    check(transported>0,"Native LAN omitted every loose weapon pose");
    unsigned explosion_locations{};
    for(const auto &event:sent.grenade_detonations) {
      const auto remote=std::find_if(received.grenade_detonations.begin(),received.grenade_detonations.end(),
          [&](const auto &e){return e.serial==event.serial;});
      check(remote!=received.grenade_detonations.end() && remote->source_locations.size()==event.source_locations.size(),
          "Native LAN omitted original explosion locations");
      for(unsigned n=0;n<event.source_locations.size();++n) {
        const auto &a=event.source_locations[n],&b=remote->source_locations[n];
        check(a.name==b.name && !std::memcmp(&a.frame,&b.frame,sizeof(Transform)),
            "Native LAN changed an original explosion transform");++explosion_locations;
      }
    }
    history.data["world_pose_metrics"]["native_lan_explosion_locations"]=explosion_locations;
    history.data["world_pose_metrics"]["native_lan_poses"]=transported;
    history.data["world_pose_metrics"]["native_lan_protocol"]=native_multiplayer_protocol;
  }
  if(follow_drop) {
    check(drop_tick.has_value(),"Actual offhand never entered the detached lifetime probe");
    std::array<Controls,multiplayer_capacity> neutral{};
    unsigned continued{};
    for(;continued<1024 && !retired_tick;++continued) {
      options.prepared_world->match.step(neutral);
      const auto state=options.prepared_world->match.state();
      observe_drop(state,state.tick,false);
    }
    history.data["drop_lifetime"]={{"object",offhand_datum},{"first_drop_tick",*drop_tick},
        {"headless_neutral_ticks",continued},{"publication_checks",drop_checks},
        {"publication_mismatches",drop_mismatches},{"completed_source_preview_differences",drop_preview_differences},
        {"source_retirement_observed",retired_tick.has_value()},
        {"original_retirement_time_not_assumed",true},{"gpu_or_framebuffer_comparison",false}};
    check(drop_checks>0 && drop_mismatches==0,"Detached source identity/ammunition/root differs from world publication");
  }
  report_observations();
  check(!history.path || (history_checks>0 && history_mismatches==0),"Independent hand publication differs from actual source storage");
  check(map_pickup && switched,"Actual SDL map pickup and carried-weapon switch failed");
  if(!(dual && right_fired && left_fired && reloaded)) {
    const auto &p=current.players[slot];
    std::cerr<<"Weapon runtime state: dual="<<dual<<" right_fired="<<right_fired
        <<" left_fired="<<left_fired<<" reloaded="<<reloaded<<" dead="<<p.dead
        <<" reload_received="<<reload_received<<" lost_focus="<<lost_focus
        <<" primary_ammo="<<p.loaded<<'/'<<p.reserve<<" secondary_ammo="<<p.secondary_weapon.loaded
        <<'/'<<p.secondary_weapon.reserve<<" reload_seconds="<<p.reload_seconds
        <<'/'<<p.secondary_weapon.reload_seconds<<'\n';
  }
  check(dual && right_fired && left_fired && reloaded,"Live dual pickup, independent triggers or reload failed");
  check(offhand_radar && primary_radar,"Actual independent SDL triggers did not retain their original radar bits");
  check(posed_weapons.size()==2 && clips.size()>=3 && draws>20,"Both real weapon datums did not submit animated poses");
  std::cout<<"SMG firing observations: sounds="<<firing_sounds<<" rounds="<<fired_rounds<<'\n';
  check(firing_sounds==fired_rounds && fired_rounds>1 && firing_particles>0,
      "Live dual SMGs did not submit exactly one authored sound per fired round");
  if(argc!=5) {
    std::cout<<"SMG grenade observations: request_tick="<<grenade_at.value_or(0)<<" thrown="<<thrown
        <<" dropped="<<dropped<<" radar_phase="<<grenade_radar_phase<<'\n';
    check(thrown && dropped && grenade_radar_phase,"Grenade animation/phase did not release the actual offhand");
  }
  std::cout<<"PASS actual SDL map pickup and switch; dual pickup, two independently firing/reloading weapon datums, "<<draws<<" posed draws, "<<clips.size()<<" clips, "<<firing_particles<<" particle draws, "<<firing_sounds<<" firing sounds";
  if(argc!=5)std::cout<<", grenade animation and offhand drop";
  std::cout<<'\n';
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
