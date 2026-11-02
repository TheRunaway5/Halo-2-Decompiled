#define SDL_MAIN_HANDLED
// Standalone native Ghost-destruction -> live renderer light probe.
//
// Scope: reproduce (old catalog) or clear (fixed catalog) the
// SourceModelEffectLights preload throw for ligh e7b648a9
// effects\generic_lights\plasma_explosion, created by the Ghost's own
// hull_destroyed_initial_damage response after an ACTUAL native death.
//
// Inputs supplied by this fixture (declared scene inputs only):
//   * the map's own rocket starting-weapon selector, derived exactly as
//     tests/destruction_runtime_tu15_tests.cpp does from
//     Cache::multiplayer_weapon_choices();
//   * the shooter's initial position (supported BSP floor, clear sight line),
//     applied through test::place_player + the original B95D0
//     set_object_position owner, as the existing destruction fixture does;
//   * ordinary Controls (yaw/pitch toward the live Ghost bound, fire, reload);
//   * for the presentation phase, all-neutral SDL input, reference tick
//     cadence and a finite capture camera aimed at the actual Ghost.
// Nothing supplies damage, events, death, visibility, light slots or any
// renderer admission. The CPU phase stops at the FIRST observed native dead
// flag (no settle), then the same MultiplayerPreparedWorld is adopted by
// run_multiplayer for a finite frame budget.
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/custom_slayer.hpp"
#include "halo2/collision.hpp"
#include "halo2/cache.hpp"
#include "halo2/math.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/renderer_timing.hpp"
#include "halo2/scene.hpp"
#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/source_draw_unit.hpp"
#include "helpers/gameplay_fixture.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char *message){if(!value)throw FormatError(message);}
constexpr std::uint32_t plasma_light=0xe7b648a9u; // effects\generic_lights\plasma_explosion
Json xyz(Vec3 v){return Json::array({v.x,v.y,v.z});}
unsigned env_unsigned(const char *name,unsigned fallback,unsigned low,unsigned high){
  const char *text=std::getenv(name);if(!text||!*text)return fallback;
  std::size_t used{};const auto value=std::stoul(text,&used);
  check(used==std::string_view(text).size() && value>=low && value<=high,"Invalid declared probe environment value");
  return unsigned(value);
}
} // namespace

int main(int argc,char **argv){
  Json report{{"probe","ghost_destruction_light_probe"},{"runtime_cache_source","MCC"},
      {"supplied_damage_events_or_outcomes",false},{"forced_death_visibility_or_light_slots",false},
      {"scene_placement_is_declared_test_input",true},{"cpu_phase",Json::object()},{"render_phase",Json::object()}};
  std::filesystem::path report_path;
  const auto save=[&]{if(!report_path.empty()){std::ofstream out(report_path);out<<report.dump(2)<<'\n';}};
  try {
    check(argc==5,"Supply MCC_MAP MCC_MAPS OUTPUT_DIR REPORT_JSON");
    report_path=argv[4];
    const std::filesystem::path output=argv[3];std::filesystem::create_directories(output);
    SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
    MultiplayerOptions o;o.map=std::filesystem::canonical(argv[1]);o.shared_maps=std::filesystem::canonical(argv[2]);
    o.local_players=1;o.source_object_world=true;o.mute=true;o.session_random_seed=0x22de570u;
    o.width=960;o.height=540;
    o.variant.values.set(2,0);o.variant.values.set(10,0); // Finite fixture owns exit, not game-over.
    // Default2 = the user's Gauss-Warthog route scene (Ghost rows remain authored).
    const unsigned light_vehicle_choice=env_unsigned("HALO2_PROBE_LIGHT_VEHICLE_CHOICE",2,0,6);
    o.variant.values.set(76,light_vehicle_choice);
    o.source_object_savegame_directory=output/"source-savegame";
    const unsigned frame_budget=env_unsigned("HALO2_PROBE_FRAMES",120,1,120);
    report["declared_inputs"]={{"map",o.map.string()},{"session_random_seed",0x22de570u},
        {"variant_writes",{{"2",0},{"10",0},{"76",light_vehicle_choice}}},{"frames",frame_budget},
        {"width",o.width},{"height",o.height}};
    {
      // Same derivation as destruction_runtime_tu15_tests: the map's own
      // multiplayer weapon choice table selects the rocket option.
      const Cache cache(o.map,o.shared_maps);check(cache.is_mcc(),"Probe requires an MCC cache");
      const auto content=cache.load_content(0,true,source_game_variant(o.variant));
      const auto choices=cache.multiplayer_weapon_choices();
      for(unsigned option=3;option<=20 && !o.starting_weapon;++option){std::uint32_t seed{};
        const auto weapon=source_starting_weapon(null_datum,option,null_datum,choices,seed);
        if(weapon!=null_datum && content.weapons.at(weapon).name.ends_with("\\rocket_launcher")) {
          o.starting_weapon=option;
          report["declared_inputs"]["starting_weapon"]={{"option",option},{"weapon",weapon},
              {"name",content.weapons.at(weapon).name},{"derived_from","Cache::multiplayer_weapon_choices"}};
        }
      }
      check(o.starting_weapon,"Map has no authored rocket starting-weapon selection");
    }
    auto world=std::make_shared<MultiplayerPreparedWorld>(o);
    auto &source=*world->source_objects;auto &native=source.tu15_world();
    const auto &cache=*world->cache;const auto &content=world->content;
    const auto slot=world->local_slots.at(0);
    auto &memory=source.memory();
    const auto word=[&](auto address){return Bytes(memory.view(address,4)).u32(0);};
    std::array<Controls,multiplayer_capacity> commands{};
    {
      const auto initial=world->match.state();
      commands[slot].yaw=initial.players[slot].yaw;commands[slot].pitch=initial.players[slot].pitch;
    }
    for(unsigned tick=0;tick<60;++tick)world->match.step(commands);
    std::vector<std::uint32_t> candidates;
    for(const auto &object:source.live_objects()) {
      if(object.parent!=null_datum || object.type!=1)continue;
      const auto tag=cache.query_tag({.handle=TagHandle{object.definition}});
      if(tag && tag->name.ends_with("\\ghost"))candidates.push_back(object.object);
    }
    check(!candidates.empty(),"No actual source-created Ghost in this scene");
    report["cpu_phase"]["ghost_candidates"]=candidates;
    // Declared clear firing lane: supported cached-BSP floor, no player
    // overlap, an unobstructed eye->bound-center BSP ray, and no OTHER live
    // object's actual bound sphere across that segment (read-only query).
    const CollisionWorld terrain(content.collision);
    const auto shooter_unit=world->match.state().players[slot].source_unit;
    const auto object_name=[&](std::uint32_t object)->std::string{
      const auto tag=cache.query_tag({.handle=TagHandle{source.object_definition(object)}});return tag?tag->name:"";
    };
    const auto object_blocker=[&](Vec3 from,Vec3 to,std::uint32_t target_object)->std::optional<std::uint32_t>{
      const auto segment=to-from;const auto reach=length(segment);if(!(reach>0))return {};
      const auto direction=segment/reach;
      for(const auto &object:source.live_objects()) {
        if(object.object==target_object || object.object==shooter_unit || object.parent==shooter_unit ||
            object.parent==target_object)continue;
        const auto bound=source.object_sphere(object.object);
        const auto along=std::clamp(dot(bound.center-from,direction),0.f,reach);
        if(length(bound.center-(from+direction*along))<bound.radius+.35f)return object.object;
      }
      return {};
    };
    std::map<std::string,unsigned> lane_object_rejections;
    std::uint32_t target=null_datum;Spawn spawn;
    for(const auto candidate:candidates) {
      const auto sphere=source.object_sphere(candidate);const auto root=source.object_transform(candidate);
      for(const float distance:{8.f,10.f,12.f,6.f}) {
        for(unsigned side=0;side<32 && target==null_datum;++side) {
          const float angle=float(side)*.19634954084936207f;
          auto origin=sphere.center-(root.basis[0]*std::cos(angle)+root.basis[1]*std::sin(angle))*distance+Vec3{0,0,10};
          float remaining=20;
          for(unsigned layer=0;layer<8 && remaining>0;++layer) {
            const auto floor=terrain.raycast(origin,{0,0,-1},remaining);if(!floor)break;
            const auto position=floor->position+Vec3{0,0,content.player.radius};
            const auto eye=position+Vec3{0,0,content.player.camera_height},delta=sphere.center-eye;
            const bool terrain_clear=floor->normal.z>=content.player.maximum_slope_cosine &&
                !terrain.overlaps(position,content.player.height,content.player.radius) &&
                !terrain.raycast(eye,normalized(delta),length(delta));
            const auto blocker=terrain_clear?object_blocker(eye,sphere.center-normalized(delta)*sphere.radius,candidate):
                std::optional<std::uint32_t>{};
            if(blocker)++lane_object_rejections[object_name(*blocker)];
            if(terrain_clear && !blocker) {
              target=candidate;spawn.position=position;spawn.facing=std::atan2(delta.y,delta.x);
              report["cpu_phase"]["declared_lane"]={{"target",candidate},{"distance",distance},{"side",side},
                  {"floor_layer",layer},{"floor_normal",xyz(floor->normal)},{"position",xyz(position)},
                  {"target_center",xyz(sphere.center)},{"target_radius",sphere.radius},
                  {"scope","Supported cached-BSP initial scene input; no impact, damage or death supplied"}};
              break;
            }
            const float step=content.player.radius*.1f;remaining-=floor->distance+step;
            origin=floor->position-Vec3{0,0,step};
          }
        }
        if(target!=null_datum)break;
      }
      if(target!=null_datum)break;
    }
    report["cpu_phase"]["lane_object_rejections"]=lane_object_rejections;
    check(target!=null_datum,"No supported terrain- and object-clear lane to any actual Ghost");
    const auto target_definition=source.object_definition(target);
    report["cpu_phase"]["target"]={{"object",target},{"definition",target_definition},
        {"name",cache.query_tag({.handle=TagHandle{target_definition}})->name}};
    const auto unit=world->match.state().players[slot].source_unit;
    test::place_player(world->match,slot,spawn);native.set_object_position(unit,spawn.position);
    Vec3 current_eye{},current_target{};
    const auto aim=[&]{
      // Existing destruction fixture: actual TU1.5 1582F0 unit eye query.
      constexpr std::uint32_t eye=0x6005b800u,velocity=eye+0x20u;
      auto abi=native.prepare_source_call(std::array{eye,velocity});abi.gp[0]=unit;
      native.run_actual(0x1582f0u,abi);
      current_eye={std::bit_cast<float>(word(eye)),std::bit_cast<float>(word(eye+4u)),std::bit_cast<float>(word(eye+8u))};
      current_target=source.object_sphere(target).center;
      const auto delta=current_target-current_eye;
      commands[slot].yaw=std::atan2(delta.y,delta.x);
      commands[slot].pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));
    };
    aim();for(unsigned tick=0;tick<30;++tick){world->match.step(commands);aim();}
    const auto target_body=source.source_object_body(target);
    check(!(word(target_body+0x108u)&0x40000u),"Ghost already carries the original dead flag");
    std::uint32_t store_tick{};
    Json dead_flag_stores=Json::array();std::map<std::uint32_t,unsigned> destruction_emitters;
    source.observe_physics_stores_read_only([&](std::uint32_t,const SourcePhysicsContactRecordStore &store){
      const auto flag=target_body+0x10au;
      if(store.address<=flag && flag-store.address<store.written.size()) {
        const auto offset=flag-store.address;
        if(!(std::to_integer<unsigned>(store.before.at(offset))&4u) && (std::to_integer<unsigned>(store.written.at(offset))&4u))
          dead_flag_stores.push_back({{"instruction",store.instruction},{"address",store.address},{"match_tick",store_tick}});
      }
      if(store.instruction==0xdc4a0u && store.written.size()==4)++destruction_emitters[Bytes(store.written).u32(0)];
    });
    // Native light pool (0x4ECBDC, 272-byte records, definition at +4).
    const auto plasma_lights=[&](Json *rows,std::map<std::uint32_t,unsigned> *all)->unsigned{
      const auto pool=word(0x4ecbdcu),data=word(pool+0x44u),bitmap=word(pool+0x48u),high=word(pool+0x38u);
      check(high<=350,"Native light pool escaped its capacity");
      unsigned plasma{};
      for(unsigned at_slot=0;at_slot<high;++at_slot)if(word(bitmap+(at_slot/32u)*4u)&(1u<<(at_slot&31u))) {
        const auto at=data+at_slot*272u;const auto definition=word(at+4u);
        if(all)++(*all)[definition];
        if(definition!=plasma_light)continue;
        ++plasma;
        if(rows)rows->push_back({{"slot",at_slot},{"flags",word(at)},
            {"position",xyz({std::bit_cast<float>(word(at+0x84u)),std::bit_cast<float>(word(at+0x88u)),std::bit_cast<float>(word(at+0x8cu))})}});
      }
      return plasma;
    };
    bool dead=false;unsigned shots{},cpu_ticks{};std::uint64_t frontier{};
    std::set<std::uint32_t> projectiles;Json samples=Json::array(),detonations=Json::array(),births=Json::array();
    const auto initial_vitality=source.object_vitality(target);
    report["cpu_phase"]["initial_body"]=initial_vitality.body;
    for(unsigned tick=0;tick<900 && !dead;++tick) {
      auto state=world->match.state();
      check(!state.players[slot].dead && state.players[slot].source_unit==unit,"Probe lost its actual live shooter");
      const auto live=source.live_objects();
      check(std::ranges::any_of(live,[&](const auto &object){return object.object==target;}),
          "Ghost retired before its first observed dead flag");
      aim();
      commands[slot].fire=tick%30==0;commands[slot].reload=state.players[slot].loaded==0;
      const auto previous_shot=state.players[slot].shot_tick;
      store_tick=state.tick+1;
      world->match.step(commands);source.advance_effect_frame(tick_seconds);++cpu_ticks;
      state=world->match.state();
      if(state.players[slot].shot_tick!=previous_shot)++shots;
      for(const auto &object:source.live_objects())if(object.type==5 && projectiles.insert(object.object).second && births.size()<32)
        births.push_back({{"object",object.object},{"definition",object.definition},{"tick",state.tick},
            {"position",xyz(source.object_transform(object.object).position)}});
      for(const auto &event:state.grenade_detonations)if(event.serial>frontier) {
        frontier=event.serial;
        if(detonations.size()<32) {
          // Read-only: nearest actual bounds to the original detonation.
          std::vector<std::pair<float,std::uint32_t>> near;
          for(const auto &object:source.live_objects())if(object.object!=event.projectile) {
            const auto bound=source.object_sphere(object.object);
            near.push_back({length(bound.center-event.position)-bound.radius,object.object});
          }
          std::ranges::sort(near);Json nearest=Json::array();
          for(std::size_t n=0;n<std::min<std::size_t>(3,near.size());++n)
            nearest.push_back({{"object",near[n].second},{"name",object_name(near[n].second)},{"surface_distance",near[n].first}});
          detonations.push_back({{"tick",event.tick},{"projectile",event.projectile},
              {"definition",event.definition},{"position",xyz(event.position)},{"target_center",xyz(current_target)},
              {"distance_to_target_center",length(event.position-current_target)},{"nearest_bounds",nearest}});
        }
      }
      const auto vitality=source.object_vitality(target);
      const auto flags=word(source.object_body(target)+0x108u)>>16u;
      dead=flags&4u;
      if(tick%30==0 || dead || vitality.body<initial_vitality.body)
        if(samples.size()<64)samples.push_back({{"tick",state.tick},{"body",vitality.body},{"shield",vitality.shield},
            {"damage_flags",flags},{"loaded",state.players[slot].loaded},{"shot_tick",state.players[slot].shot_tick},
            {"eye",xyz(current_eye)},{"target_center",xyz(current_target)}});
      if(dead) {
        Json rows=Json::array();std::map<std::uint32_t,unsigned> all;
        const auto plasma=plasma_lights(&rows,&all);
        Json definitions=Json::object();for(const auto &[d,n]:all)definitions[std::to_string(d)]=n;
        report["cpu_phase"]["death"]={{"match_tick",state.tick},{"cpu_firing_tick",tick},{"body",vitality.body},
            {"damage_flags",flags},{"plasma_lights_in_native_pool",plasma},{"plasma_rows",rows},
            {"native_pool_definitions",definitions}};
      }
    }
    source.observe_physics_stores({});
    commands[slot].fire=commands[slot].reload=false;
    report["cpu_phase"].update({{"firing_ticks",cpu_ticks},{"shots",shots},{"projectiles",projectiles.size()},
        {"projectile_births",births},{"detonations",detonations},{"samples",samples},
        {"dead",dead},{"original_dead_flag_stores",dead_flag_stores},
        {"original_destruction_emitter_stores",[&]{Json j=Json::object();for(const auto &[e,n]:destruction_emitters)j[std::to_string(e)]=n;return j;}()},
        {"stopped_at_first_death_without_settle",true}});
    save();
    check(shots>0,"Ordinary rocket commands produced no shots");
    check(dead,"No actual native Ghost death within the firing budget");
    check(!dead_flag_stores.empty(),"Dead flag was not observed through an original store");

    // --- Presentation: adopt the SAME prepared world in run_multiplayer. ---
    MultiplayerOptions r=o;r.prepared_world=world;r.frames=frame_budget;r.screenshot=output/"frame.ppm";
    r.reference_tick_cadence=true;r.reference_render_fraction=1.f;
    Camera camera;camera.position=current_eye;
    const auto forward=normalized(current_target-current_eye);
    const auto right=normalized(cross(forward,Vec3{0,0,1}));
    camera.forward=forward;camera.up=cross(right,forward);camera.oriented=true;
    r.capture_camera=camera;
    report["render_phase"]["capture_camera"]={{"position",xyz(camera.position)},{"forward",xyz(camera.forward)},
        {"up",xyz(camera.up)},{"target_center",xyz(current_target)},{"scope","Finite capture camera toward the actual Ghost bound"}};
    unsigned observed_ticks{},non_neutral_ticks{},frames{};
    std::optional<std::uint32_t> first_plasma_tick,last_plasma_tick,first_tick;
    Json plasma_samples=Json::array();
    std::map<unsigned,unsigned> observer_phases;std::map<std::uint32_t,unsigned> observed_definitions;
    std::map<std::uint32_t,unsigned> receiver_units_by_light;
    unsigned frame_plasma_units{},frame_plasma_callbacks{};
    Json frame_rows=Json::array();std::uint64_t prepared_total{};
    unsigned plasma_frames_with_units{},plasma_frames_with_prepared_draws{};
    r.input_observer=[&](std::uint32_t,std::span<const Controls> controls,const MultiplayerState &s) {
      ++observed_ticks;if(!first_tick)first_tick=s.tick;
      const auto &c=controls[slot];
      if(c.forward!=0 || c.right!=0 || c.fire || c.reload || c.jump || c.crouch || c.grenade || c.melee ||
          c.fire_secondary || c.switch_weapon || c.pickup)++non_neutral_ticks;
      Json rows=Json::array();
      if(const auto plasma=plasma_lights(&rows,nullptr)) {
        if(!first_plasma_tick)first_plasma_tick=s.tick;last_plasma_tick=s.tick;
        if(plasma_samples.size()<40)plasma_samples.push_back({{"tick",s.tick},{"lights",rows}});
      }
    };
    r.effect_light_command_observer=[&](unsigned,std::uint32_t,const SourceEffectLightCommandObservationTu15 &observed) {
      ++observer_phases[unsigned(observed.phase)];
      const auto &light_memory=observed.world.memory();
      const auto pool=Bytes(light_memory.view(0x4ecbdcu,4)).u32(0);
      const SourceRawDatumPoolServices storage{.read=[&](auto at,auto size){return light_memory.view(at,size);}};
      const auto light=source_raw_datum_lookup(pool,observed.light,storage);
      check(light!=0,"Light observation lost its actual salted pool owner");
      const auto definition=Bytes(light_memory.view(light+4u,4)).u32(0);
      ++observed_definitions[definition];
      if(observed.phase!=SourceEffectLightCommandPhaseTu15::culled)return;
      unsigned units{};
      for(unsigned n=0;n<observed.state.draw_count;++n)units+=observed.state.units.at(n).words[1]!=null_datum;
      receiver_units_by_light[definition]+=units;
      if(definition==plasma_light){++frame_plasma_callbacks;frame_plasma_units+=units;}
    };
    r.frame_observer=[&](const MultiplayerFrameTiming &f) {
      const auto prepared=f.renderer.world_native_light_prepared_draws;prepared_total+=prepared;
      if(frame_plasma_units)++plasma_frames_with_units;
      if(frame_plasma_units && prepared)++plasma_frames_with_prepared_draws;
      if(frame_rows.size()<120)frame_rows.push_back({{"frame",frames},{"light_prepared_draws",prepared},
          {"plasma_culled_callbacks",frame_plasma_callbacks},{"plasma_model_receiver_units",frame_plasma_units}});
      frame_plasma_units=frame_plasma_callbacks=0;++frames;
    };
    int result=-1;std::string run_error;
    try {result=run_multiplayer(r);}
    catch(const std::exception &error){run_error=error.what();}
    Json phases=Json::object();for(const auto &[p,n]:observer_phases)phases[std::to_string(p)]=n;
    Json definitions=Json::object();for(const auto &[d,n]:observed_definitions)definitions[std::to_string(d)]=n;
    Json units=Json::object();for(const auto &[d,n]:receiver_units_by_light)units[std::to_string(d)]=n;
    const bool fatal_key=run_error.find("selected model light pass was not admitted during preload")!=std::string::npos;
    const bool exact_light=run_error.find("light="+std::to_string(plasma_light)+" ")!=std::string::npos;
    const bool unknown=run_error.find("known_light=0")!=std::string::npos;
    report["render_phase"].update({{"run_result",result},{"run_error",run_error},{"frames",frames},
        {"observed_input_ticks",observed_ticks},{"first_observed_tick",first_tick?Json(*first_tick):Json(nullptr)},
        {"non_neutral_control_ticks",non_neutral_ticks},
        {"first_plasma_native_pool_tick",first_plasma_tick?Json(*first_plasma_tick):Json(nullptr)},
        {"last_plasma_native_pool_tick",last_plasma_tick?Json(*last_plasma_tick):Json(nullptr)},
        {"plasma_native_pool_samples",plasma_samples},
        {"light_observer_phases",phases},{"light_observer_definitions",definitions},
        {"model_receiver_units_by_light",units},
        {"plasma_frames_with_model_receiver_units",plasma_frames_with_units},
        {"plasma_frames_with_renderer_prepared_light_draws",plasma_frames_with_prepared_draws},
        {"renderer_world_native_light_prepared_draws_total",prepared_total},{"frame_rows",frame_rows}});
    report["verdict"]={{"red_exact_plasma_preload_fatal",fatal_key && exact_light && unknown},
        {"preload_fatal_any_light",fatal_key},
        {"green_no_throw_with_plasma_prepared_draws",run_error.empty() && result==0 &&
            plasma_frames_with_prepared_draws>0 && receiver_units_by_light[plasma_light]>0}};
    const bool passed=report["verdict"].at("green_no_throw_with_plasma_prepared_draws").get<bool>();
    report["passed"]=passed;save();
    std::cout<<"probe_verdict "<<report["verdict"].dump()<<std::endl;
    if(!passed)std::cerr<<"Late native light regression: "<<run_error<<std::endl;
    return passed?0:2;
  } catch(const std::exception &error) {
    report["passed"]=false;report["probe_error"]=error.what();save();
    std::cerr<<"probe_error: "<<error.what()<<std::endl;return 1;
  }
}
