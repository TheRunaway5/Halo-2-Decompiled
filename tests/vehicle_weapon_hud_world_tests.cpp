#include "halo2/multiplayer_world.hpp"
#include "halo2/hud.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "helpers/gameplay_fixture.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer_kbm.hpp"
#include "halo2/custom_slayer.hpp"
#include "halo2/source_shadow.hpp"
#ifdef HALO2_VEHICLE_HUD_GPU
#include "halo2/desktop_session.hpp"
#include "halo2/source_light_volume_draw.hpp"
#include "halo2/particle_draw.hpp"
#endif
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <vector>
using namespace halo2;
namespace {
struct VehicleHudReport {
  const char* path=std::getenv("HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT");
  nlohmann::json data{{"passed",false},{"full_weapon_parity",false},
      {"GPU_or_Xbox_pixels_compared",false},{"source_results_or_ready_flags_supplied",false},
      {"cases",nlohmann::json::array()}};
  ~VehicleHudReport(){if(path){std::ofstream output(path);output<<data.dump(2)<<'\n';}}
};
void check(bool yes,const char *why){if(!yes)throw FormatError(why);}
struct Entrance {Spawn spawn;unsigned seat{};std::size_t placement{};};
Entrance entrance(const Cache &cache,const LevelContent &content,std::string_view name,bool child) {
  const auto &assets=*content.unit_interactions;
  for(const auto &[row,placement]:assets.netgame_placements) {
    const auto &object=content.objects.at(placement);
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto &t){return t.datum==object.definition;});
    if(tag==cache.tags().end() || tag->name.find("\\"+std::string(name)+"\\")==std::string::npos)continue;
    const auto basis=authored_basis(object.rotation);
    auto root=source_object_transform(object.position,basis.forward,basis.up,object.scale,false);
    auto target=placement;
    if(child) {
      const auto found=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto &p){return p.model_attachment && p.model_attachment->parent==placement;});
      if(found==content.objects.end())continue;
      target=std::size_t(found-content.objects.begin());const auto &attachment=*found->model_attachment;
      const auto &parent=assets.units.at(object.definition),&definition=assets.units.at(found->definition);
      const auto nodes=pose_model_nodes_world(parent.hierarchy,root);
      const auto markers=pose_model_markers(parent.markers.at(attachment.parent_marker),nodes,parent.permutations,{},1);
      check(!markers.empty(),"Missing authored turret attachment marker");Transform local;
      if(attachment.child_marker) {
        const auto child_nodes=pose_model_nodes_world(definition.hierarchy,Transform{});
        const auto child_markers=pose_model_markers(definition.markers.at(attachment.child_marker),child_nodes,definition.permutations,{},1);
        if(!child_markers.empty())local=child_markers.front().world;
      }
      root=compose_transform(markers.front().world,inverse_transform(local));
    }
    const auto &definition=assets.units.at(content.objects.at(target).definition);
    const auto nodes=pose_model_nodes_world(definition.hierarchy,root);
    for(unsigned index=0;index<definition.seats.size();++index) {
      const auto &seat=definition.seats[index];if(child?index!=0:!(seat.flags&4u))continue;
      const auto marker=definition.markers.find(seat.entry_marker);if(marker==definition.markers.end())continue;
      const auto entrances=pose_model_markers(marker->second,nodes,definition.permutations,{},1);if(entrances.empty())continue;
      Spawn spawn;spawn.position=entrances.front().world.position+entrances.front().world.basis[0]*(seat.radius*.5f);
      CollisionWorld terrain(content.collision);const auto floor=terrain.raycast(spawn.position+Vec3{0,0,2*content.player.height},{0,0,-1},4*content.player.height);
      if(!floor)continue;spawn.position.z=floor->position.z+content.player.radius;
      const auto look=entrances.front().world.position-(spawn.position+Vec3{0,0,content.player.camera_height});
      spawn.facing=std::atan2(look.y,look.x);return {spawn,index,target};
    }
  }
  throw FormatError("Missing authored vehicle HUD approach: "+std::string(name));
}
void rocket_lock_on(MultiplayerPreparedWorld &world,
    std::array<Controls,multiplayer_capacity> &controls,unsigned driver,
    const Cache &cache,const LevelContent &content,nlohmann::json &report) {
  const auto shooter=world.local_slots.at(1);
  auto target=world.match.state().players[driver].source_parent;
  check(target!=null_datum,"Rocket lock acceptance requires the occupied vehicle");
  // A gunner's immediate parent is the attached turret. Aim at the actual
  // chassis so both Warthog variants exercise a rocket/chassis collision.
  while(world.source_objects->object_parent(target)!=null_datum)
    target=world.source_objects->object_parent(target);
  const bool empty_target=std::getenv("HALO2_TEST_ROCKET_EMPTY_TARGET")!=nullptr;
  if(empty_target) {
    controls[driver].pickup=false;controls[driver].pickup_milliseconds=0;
    world.match.step(controls);
    for(unsigned tick=0;tick<150 && world.match.state().players[driver].source_parent!=null_datum;++tick) {
      controls[driver].pickup=true;controls[driver].pickup_milliseconds=900;
      world.match.step(controls);
    }
    controls[driver].pickup=false;controls[driver].pickup_milliseconds=0;
    check(world.match.state().players[driver].source_parent==null_datum,
        "Normal source exit did not empty the rocket target");
    for(unsigned tick=0;tick<30;++tick)world.match.step(controls);
  }
  const auto sphere=world.source_objects->object_sphere(target);
  const auto root=world.source_objects->object_transform(target);
  CollisionWorld terrain(content.collision);
  Spawn spawn;bool clear_scene=false;
  for(unsigned side=0;side<16;++side) {
    const float angle=float(side)*.39269908169872414f;
    auto candidate=sphere.center-(root.basis[0]*std::cos(angle)+root.basis[1]*std::sin(angle))*10.f;
    const auto floor=terrain.raycast(candidate+Vec3{0,0,10},{0,0,-1},20);
    if(!floor)continue;
    candidate.z=floor->position.z+content.player.radius;
    const auto eye=candidate+Vec3{0,0,content.player.camera_height},delta=sphere.center-eye;
    if(const auto hit=terrain.raycast(eye,normalized(delta),length(delta));hit)continue;
    spawn.position=candidate;clear_scene=true;break;
  }
  check(clear_scene,"Rocket lock scene has no terrain-supported clear line of sight");
  const auto unit=world.match.state().players[shooter].source_unit;
  test::place_player(world.match,shooter,spawn);
  world.source_objects->tu15_world().set_object_position(unit,spawn.position);
  const auto &memory=world.source_objects->memory();
  const auto word=[&](auto at){return Bytes(memory.view(at,4)).u32(0);};
  const auto aim=[&] {
    // Query the original posed eye instead of substituting a camera height.
    auto &source=world.source_objects->tu15_world();
    constexpr std::uint32_t eye=0x6005b800u,velocity=eye+0x20u;
    auto abi=source.prepare_source_call(std::array{eye,velocity});abi.gp[0]=unit;
    source.run_actual(0x1582f0u,abi);
    const Vec3 position{std::bit_cast<float>(word(eye)),std::bit_cast<float>(word(eye+4)),
        std::bit_cast<float>(word(eye+8))};
    const auto delta=world.source_objects->object_sphere(target).center-position;
    controls[shooter].yaw=std::atan2(delta.y,delta.x);
    controls[shooter].pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));
  };
  auto &result=report["rocket_lock_on"];
  result={{"passed",false},{"target",target},{"empty_target",empty_target},{"scene_inputs_only",true},
      {"frames",nlohmann::json::array()},{"contacts",nlohmann::json::array()},
      {"target_queries",nlohmann::json::array()}};
  std::vector<std::pair<std::uint32_t,std::size_t>> queries;
  SourcePhysicsEntryObserverBinding targeting(memory,[&](bool before,std::uint32_t entry,const auto &abi) {
    if(entry!=0x1adf60u)return;
    auto &rows=result["target_queries"];
    if(before) {
      if(rows.size()>=8){queries.push_back({0,SIZE_MAX});return;}
      const auto parameters=word(abi.gp[4]+4u),look=word(abi.gp[4]+8u),output=word(abi.gp[4]+20u);
      std::array<std::uint32_t,12> parameters_words{};
      std::memcpy(parameters_words.data(),memory.view(parameters,sizeof(parameters_words)).data(),sizeof(parameters_words));
      queries.push_back({output,rows.size()});
      rows.push_back({{"parameters",parameters_words},{"look_words",{word(look),word(look+4),word(look+8)}},
          {"ignored_root",word(abi.gp[4]+12u)},{"team",word(abi.gp[4]+16u)}});
    } else {
      check(!queries.empty(),"Unpaired original targeting query");
      const auto [output,index]=queries.back();queries.pop_back();if(index==SIZE_MAX)return;
      rows[index]["result"]=abi.gp[0]&255u;
      std::array<std::uint32_t,24> words{};
      std::memcpy(words.data(),memory.view(output,sizeof(words)).data(),sizeof(words));
      rows[index]["output_words"]=words;
    }
  });
  aim();
  for(unsigned tick=0;tick<60;++tick){world.match.step(controls);aim();}
  auto state=world.match.state();const auto weapon=state.players[shooter].source_weapon;
  check(weapon!=null_datum,"Rocket shooter has no original weapon object");
  const auto definition=world.source_objects->object_definition(weapon);
  check(content.weapons.at(definition).name.ends_with("\\rocket_launcher"),
      "Original starting equipment did not select the rocket launcher");
  const auto projectile=content.weapons.at(definition).projectile;
  const auto body=world.source_objects->object_body(weapon);
  const auto hud_content=cache.load_hud(content.player_representations.at(2).biped.datum,definition);
  std::set<std::uint32_t> born;bool locking=false,locked=false,contact=false;
  std::uint64_t frontier{};
  for(const auto &event:state.grenade_detonations)frontier=std::max(frontier,event.serial);
  for(unsigned tick=0;tick<180;++tick) {
    if(!state.players[driver].dead)aim();
    controls[shooter].fire=tick<(empty_target?1u:60u);
    world.match.step(controls);world.source_objects->advance_effect_frame(tick_seconds);
    state=world.match.state();const auto &p=state.players[shooter];
    const auto hud=multiplayer_hud_weapon_state(p,hud_content,0);
    locking|=hud.locking;locked|=hud.locked;
    if(tick<65 || p.shot_tick==state.tick)
      result["frames"].push_back({{"tick",state.tick},{"mode",p.weapon_trigger.mode},
          {"target",word(body+0x194u)},{"locking",hud.locking},{"locked",hud.locked},
          {"lock_progress",hud.lock_progress},{"target_position",p.weapon_trigger.target_position.has_value()},
          {"loaded",p.loaded},{"shot_tick",p.shot_tick},{"yaw",p.yaw},{"pitch",p.pitch},
          {"input_yaw",controls[shooter].yaw},{"input_pitch",controls[shooter].pitch}});
    for(const auto &object:world.source_objects->live_objects())
      if(object.type==5 && object.definition==projectile)born.insert(object.object);
    for(const auto &event:state.grenade_detonations) {
      if(event.serial<=frontier)continue;frontier=event.serial;
      if(event.definition!=projectile)continue;
      check(event.projectile!=null_datum && event.source_effect_instance!=null_datum &&
          !event.source_locations.empty(),"Rocket contact lost its original effect provenance");
      born.insert(event.projectile);contact=true;
      result["contacts"].push_back({{"tick",event.tick},{"projectile",event.projectile},
          {"effect",event.effect},{"source_instance",event.source_effect_instance},
          {"locations",event.source_locations.size()}});
    }
    if(contact && std::getenv("HALO2_TEST_ROCKET_EXPLOSION_RENDER"))break;
  }
  result["observed_locking"]=locking;result["observed_locked"]=locked;
  result["projectiles"]=born;result["observed_contact"]=contact;
  check(empty_target || (locking && locked),"Actual rocket input did not reach original locking and locked HUD states");
  check(!born.empty() && contact,"Actual released rocket did not reach an original contact explosion");
  result["passed"]=true;
  std::cout<<"PASS original rocket lock-on, release and contact explosion\n";
}
}
int main(int argc,char **argv) {VehicleHudReport report;try {
#ifdef HALO2_VEHICLE_HUD_GPU
  check(argc==5,"Supply Coagulation MAP SHARED TU1.5_XBE_OR_native-mcc SCREENSHOT");
#else
  check(argc==4,"Supply Coagulation MAP SHARED TU1.5_XBE_OR_native-mcc");
#endif
  const bool mcc=std::string_view(argv[3])=="native-mcc";
  report.data["runtime_cache_source"]=mcc?"prepared MCC requested":"offline reference requested";
  report.data["map"]=argv[1];
  const auto selected_case=std::getenv("HALO2_TEST_VEHICLE_WEAPON_HUD_CASE");
  unsigned worlds{},seated{},weapon_huds{};
  for(const auto &[name,child,parameter,choice]:std::array<std::tuple<std::string_view,bool,unsigned,int>,7>{{
      {"ghost",false,76,3},{"banshee",false,76,1},{"scorpion",false,78,1},
      {"wraith",false,78,2},{"warthog",true,76,1},{"warthog",true,76,2},{"spectre",true,76,5}}}) {
    const auto case_name=name=="warthog"?std::string(choice==1?"warthog-mg":"warthog-gauss"):std::string(name);
    if(selected_case && case_name!=selected_case)continue;
#ifdef HALO2_VEHICLE_HUD_GPU
    const auto selected=std::getenv("HALO2_VEHICLE_HUD_GPU_NAME");
    if(name!=(selected?selected:"spectre"))continue;
#endif
    auto& row=report.data["cases"].emplace_back(nlohmann::json{{"case",case_name},
        {"variant_parameter",parameter},{"variant_choice",choice},{"passed",false},
        {"canonical_map_spawn_inputs_preserved",mcc},
        {"MCC_scene_placement","One original B95D0 input,then60 natural source ticks"}});
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
    if(!mcc)options.gameplay_xbe=argv[3];
    options.local_players=2;options.starting_weapon=5;options.source_object_world=true;
    options.variant.values.set(parameter,choice);options.session_random_seed=0x13c40au;
    Cache cache(options.map,options.shared_maps,mcc?std::shared_ptr<const OriginalData>{}:OriginalData::load_xbe(options.gameplay_xbe));
    check(cache.is_mcc()==mcc,"Vehicle weapon HUD received the wrong cache platform");
    row["cache_is_mcc"]=cache.is_mcc();report.data["runtime_cache_source"]=mcc?"MCC":"offline reference";
    const auto content=cache.load_content(0,true,source_game_variant(options.variant));
    if(std::getenv("HALO2_TEST_ROCKET_LOCK_ON")) {
      check(mcc,"Rocket lock acceptance requires a prepared MCC vehicle scene");
      const auto choices=cache.multiplayer_weapon_choices();bool selected=false;
      for(unsigned option=3;option<=20;++option) {
        std::uint32_t seed{};const auto weapon=source_starting_weapon(null_datum,option,null_datum,choices,seed);
        if(weapon==null_datum || !content.weapons.at(weapon).name.ends_with("\\rocket_launcher"))continue;
        options.starting_weapon=option;selected=true;break;
      }
      check(selected,"Authored multiplayer choices have no rocket launcher");
    }
    const auto approach=entrance(cache,content,name,child);
    if(!mcc)options.native_spawn_inputs={approach.spawn,cache.spawns().back()};
    options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
    auto &world=*options.prepared_world;world.bind_source_local_views(world.local_slots);
    row["source_world_constructed"]=true;row["authored_seat"]=approach.seat;
    std::array<Controls,multiplayer_capacity> controls{};const auto player=world.local_slots[0];
    if(mcc) {
      // Establish the genuine native look baseline at the map's actual spawn,
      // then turn through the ordinary input producer. Placement never chooses
      // an available seat or supplies any admission/ready result.
      const auto initial=world.match.state();
      for(const auto local:world.local_slots) {
        controls[local].yaw=initial.players[local].yaw;
        controls[local].pitch=initial.players[local].pitch;
      }
      world.match.step(controls);
    }
    controls[player].yaw=approach.spawn.facing;controls[player].pitch=0;
    const auto place=[&] {
      if(!mcc)return;
      const auto state=world.match.state();const auto& p=state.players[player];
      check(p.source_unit!=null_datum,"MCC vehicle HUD has no genuinely born player unit");
      if(p.source_parent!=null_datum)return;
      test::place_player(world.match,player,approach.spawn);
      world.source_objects->tu15_world().set_object_position(p.source_unit,approach.spawn.position);
    };
    place();
    for(unsigned tick=0;tick<60;++tick)world.match.step(controls);
    for(unsigned tick=0;tick<150;++tick) {
      controls[player].pickup=true;controls[player].pickup_milliseconds=900;
      world.match.step(controls);auto state=world.match.state();const auto &p=state.players[player];
      if(p.source_parent==null_datum)continue;
      check(!state.players[world.local_slots[1]].source_vehicle_hud.ownership.seated &&
          !state.players[world.local_slots[1]].source_vehicle_hud.weapon.present,"Vehicle HUD leaked into the other player");
      ++seated;check(p.source_vehicle_hud.ownership.seated,"Actual seated source unit lost vehicle HUD ownership");
      row["entry_tick"]=tick;row["source_unit"]=p.source_unit;row["source_parent"]=p.source_parent;
      const auto &actual=p.source_vehicle_hud;
      check(actual.ownership.vehicle_weapon!=null_datum && actual.weapon.present,"Actual armed seat lost its vehicle weapon HUD");
      check(p.source_seat==int(approach.seat),"Original owner selected another seat");
      check(actual.weapon.datum==actual.ownership.vehicle_weapon &&
          world.source_objects->object_definition(actual.weapon.datum)==actual.definition,"Vehicle HUD weapon identity differs from its original object");
      const auto body=world.source_objects->object_body(actual.weapon.datum);float age{},heat{};
      world.source_objects->memory().read(body+0x180u,std::as_writable_bytes(std::span(&age,1)));
      world.source_objects->memory().read(body+0x184u,std::as_writable_bytes(std::span(&heat,1)));
      check(actual.weapon.age==age && actual.weapon.heat==heat,"Vehicle HUD differs from actual source heat/age fields");
      row["source_weapon"]=actual.weapon.datum;row["source_weapon_definition"]=actual.definition;
      const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto& tag){return tag.datum==actual.definition;});
      if(tag!=cache.tags().end())row["weapon_name"]=tag->name;
      auto authored=cache.load_hud(content.player_representations.at(2).biped.datum,actual.definition);
      authored.action_messages.enabled=false;
      std::erase_if(authored.widgets,[](const auto &w){return w.weapon_type!=1;});
      for(auto &w:authored.widgets)w.weapon_type=4;
      Mesh mesh;MultiplayerHud hud(authored,options.shared_maps/"fonts",mesh,1,"Slayer",cache.original_data()->hud_bitmap_geometry_constants,{},2);
      hud.animate(state,player,0,0,0,state.tick*tick_seconds);
      auto vertices=hud.draw(state,player,0,1,1280,720,0,0);
      if(actual.ownership.parent_hud!=null_datum) {
        auto parent_content=cache.load_hud_definition(content.player_representations.at(2).biped.datum,actual.ownership.parent_hud);
        MultiplayerHud parent(parent_content,options.shared_maps/"fonts",mesh,1,"Slayer",cache.original_data()->hud_bitmap_geometry_constants);
        parent.animate(state,player,0,0,0,state.tick*tick_seconds);
        const auto parent_vertices=parent.draw(state,player,0,1,1280,720,0,0);
        vertices.insert(vertices.end(),parent_vertices.begin(),parent_vertices.end());
      }
      check(!vertices.empty(),"Live source vehicle and seat HUDs submitted no authored widgets");
      row["authored_HUD_vertices"]=vertices.size();row["actual_HUD_source_fields_match"]=true;
#ifdef HALO2_VEHICLE_HUD_GPU
      const bool ghost_fire=std::getenv("HALO2_TEST_GHOST_PROJECTILE_RENDER")!=nullptr;
      const bool rocket_explosion=std::getenv("HALO2_TEST_ROCKET_EXPLOSION_RENDER")!=nullptr;
      unsigned explosion_draws{};std::map<std::uint32_t,unsigned> particle_definitions;
      if(rocket_explosion) {
        controls[player].pickup=false;controls[player].pickup_milliseconds=0;
        rocket_lock_on(world,controls,player,cache,content,row);
        const auto effect=row.at("rocket_lock_on").at("contacts").at(0).at("effect").get<std::uint32_t>();
        options.effect_observer=[&,effect](unsigned,std::uint32_t,std::span<const ParticleDraw> draws) {
          for(const auto &draw:draws) {
            ++particle_definitions[draw.effect_definition];
            explosion_draws+=unsigned(draw.effect_definition==effect);
          }
        };
      }
      options.frames=ghost_fire?90u:rocket_explosion?24u:4u;options.mute=true;options.screenshot=argv[4];
      unsigned glow_draws{},glow_vertices{},fire_inputs{};
      std::set<std::uint32_t> projectile_owners;
      if(ghost_fire) {
        check(mcc && name=="ghost","Ghost projectile rendering requires its prepared MCC driver");
        options.reference_tick_cadence=true;options.reference_render_fraction=.5f;
        options.desktop=std::make_shared<DesktopSession>();
        auto *window=options.desktop->window(800,600,false);
        SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);
        const auto start=state.tick,projectile=content.weapons.at(actual.definition).projectile;
        options.input_observer=[&,start,window](unsigned tick,std::span<const Controls> input,const auto &) {
          if(tick==start+2 || tick==start+35) {
            SDL_Event event{};event.type=tick==start+2?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
            event.button.button=SDL_BUTTON_LEFT;event.button.windowID=SDL_GetWindowID(window);
            check(SDL_PushEvent(&event)==1,"Queue actual Ghost fire input");
          }
          fire_inputs+=unsigned(input[player].fire);
        };
        options.weapon_light_volume_observer=[&,projectile](unsigned,std::uint32_t,const SourceLightVolumeDraw &draw) {
          const auto current=world.match.state();
          const auto owner=std::find_if(current.weapon_projectiles.begin(),current.weapon_projectiles.end(),
              [&](const auto &p){return p.datum==draw.object && p.definition==projectile;});
          if(owner==current.weapon_projectiles.end())return;
          check(!draw.vertices.empty(),"Actual Ghost light volume submitted empty geometry");
          projectile_owners.insert(draw.object);++glow_draws;glow_vertices+=draw.vertices.size();
        };
      }
      unsigned submitted{};
      options.view_state_observer=[&](unsigned view,const MultiplayerState &frame,const Camera &) {
        if(view)return;
        check(rocket_explosion || (frame.players[player].source_vehicle_hud.weapon.present &&
            !frame.players[player].source_vehicle_hud.ownership.handheld_visible),
            "Rendered seated view lost its source weapon HUD");++submitted;
      };
      check(run_multiplayer(options)==0 && submitted>=4,"Actual vehicle HUD Vulkan submission stopped early");
      std::cout<<submitted<<" actual seated Vulkan frames submitted; screenshot="<<argv[4]<<'\n';
      row["actual_Vulkan_frames"]=submitted;
      if(ghost_fire) {
        row["projectile_render"]={{"fire_input_ticks",fire_inputs},{"glow_draws",glow_draws},
            {"glow_vertices",glow_vertices},{"actual_projectile_owners",projectile_owners}};
        check(fire_inputs>0 && projectile_owners.size()>1 && glow_draws>0,
            "Actual Ghost firing did not submit authored projectile light volumes");
      }
      if(rocket_explosion) {
        row["explosion_render"]={{"root_particle_draws",explosion_draws},
            {"particle_definitions",particle_definitions}};
        check(explosion_draws>0,"Original rocket explosion particles never reached Vulkan submission");
      }
#endif
      ++weapon_huds;std::cout<<"PASS "<<name<<(child?" turret":" driver")<<" choice="<<choice
          <<" weapon="<<actual.definition<<" handheld="<<actual.ownership.handheld_visible<<'\n'<<std::flush;
#ifdef HALO2_VEHICLE_HUD_GPU
      // The runtime releases its view/session callbacks on return. Source
      // entry/exit is verified by the separate world-only executable above.
      row["passed"]=true;report.data["passed"]=true;return 0;
#endif
      break;
    }
    ++worlds;check(weapon_huds==worlds,"Normal original vehicle entry did not reach its armed HUD");
    controls[player].pickup=false;controls[player].pickup_milliseconds=0;
    if(std::getenv("HALO2_TEST_ROCKET_LOCK_ON")) {
      rocket_lock_on(world,controls,player,cache,content,row);
      row["passed"]=true;report.data["passed"]=true;return 0;
    }
    if(std::getenv("HALO2_TEST_GHOST_BOOST")) {
      check(name=="ghost","Boost acceptance requires the actual Ghost driver case");
      for(unsigned tick=0;tick<30;++tick)world.match.step(controls);
      const auto vehicle=world.match.state().players[player].source_parent;
      check(vehicle!=null_datum,"Ghost boost lost the actual seated parent");
      const auto body=world.source_objects->object_body(vehicle);
      const auto charge=[&] {
        return std::bit_cast<float>(Bytes(world.source_objects->memory().view(body+0x2c4u,4)).u32(0));
      };
      const auto before=charge();MultiplayerKbm keyboard;
      SDL_Event button{};button.type=SDL_MOUSEBUTTONDOWN;button.button.button=SDL_BUTTON_RIGHT;
      keyboard.handle_event(button);
      for(unsigned tick=0;tick<30;++tick) {
        controls[player]=keyboard.consume(controls[player].yaw,controls[player].pitch).gameplay_controls(false,true,false);
        world.match.step(controls);
      }
      const auto held=charge();
      const auto flags=Bytes(world.source_objects->memory().view(body+0x134u,4)).u32(0);
      check(held>before && (flags&0x800000u),"Right-click did not activate original Ghost boost state");
      button.type=SDL_MOUSEBUTTONUP;keyboard.handle_event(button);
      for(unsigned tick=0;tick<30;++tick) {
        controls[player]=keyboard.consume(controls[player].yaw,controls[player].pitch).gameplay_controls(false,true,false);
        world.match.step(controls);
      }
      check(charge()<held,"Released right-click did not decay original Ghost boost state");
      row["ghost_boost"]={{"passed",true},{"before",before},{"held",held},{"released",charge()},
          {"source_vehicle",vehicle},{"held_source_flags",flags}};
      std::cout<<"PASS right-click activates and releases the original Ghost boost state\n";
    }
    if(std::getenv("HALO2_TEST_VEHICLE_WEAPON_FIRE")) {
      check(mcc,"Vehicle firing acceptance requires the prepared MCC cache");
      const auto weapon=row.at("source_weapon").get<std::uint32_t>();
      const auto definition=row.at("source_weapon_definition").get<std::uint32_t>();
      const auto &authored=content.weapons.at(definition);
      const bool secondary=std::getenv("HALO2_TEST_VEHICLE_WEAPON_SECONDARY")!=nullptr;
      std::int16_t trigger_input=0,selected_barrel=0;
      std::set<std::uint32_t> expected_projectiles;
      if(authored.projectile!=null_datum)expected_projectiles.insert(authored.projectile);
      for(const auto &barrel:authored.barrel_views)
        if(barrel && barrel->projectile!=null_datum)expected_projectiles.insert(barrel->projectile);
      if(secondary) {
        const auto trigger=std::find_if(authored.triggers.begin(),authored.triggers.end(),
            [](const auto &t){return t.input>0;});
        check(trigger!=authored.triggers.end() && trigger->input<=2 && trigger->primary_barrel>=0,
            "Vehicle has no supported authored additional trigger input");
        trigger_input=trigger->input;selected_barrel=trigger->primary_barrel;
        const auto &memory=world.source_objects->memory();
        const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
        const auto tag_row=word(0x4f0414u)+(definition&65535u)*16u;
        check(word(tag_row+4u)==definition,"Additional vehicle trigger lost its actual full weapon tag");
        const auto tag=word(tag_row+8u),count=word(tag+0x2d0u),barrels=word(tag+0x2d4u);
        check(unsigned(selected_barrel)<count,"Additional vehicle input selects an absent authored barrel");
        const auto projectile=word(barrels+unsigned(selected_barrel)*0xecu+0x90u);
        check(projectile!=null_datum,"Additional vehicle input has no authored projectile");
        expected_projectiles={projectile};
      }
      check(!expected_projectiles.empty(),"Armed vehicle has no authored projectile definition");
      const auto &source_memory=world.source_objects->memory();
      const auto game=Bytes(source_memory.view(0x4f3218u,4)).u32(0);
      const auto game_kind=Bytes(source_memory.view(game+8u,4)).u32(0);
      // Whole1889D0 skips input-record bit29 for multiplayer(kind2).
      // C7DD0 maps that flag to weapon+16E bit8, consumed as input2 by
      //1030F0. A retained bomb barrel does not enable a local MP trigger.
      const bool third_input_suppressed=secondary && trigger_input==2 && game_kind==2;
      auto &firing=row[secondary?"secondary_firing":"primary_firing"];
      firing={{"passed",false},{"neutral_ticks",0},{"held_ticks",0},{"release_ticks",0},
          {"prepared_subshots",0},{"live_projectiles",nlohmann::json::array()},
          {"completed_source_contacts",nlohmann::json::array()},
          {"constructor_calls",nlohmann::json::array()},
          {"retirement_calls",nlohmann::json::array()},
          {"expected_projectile_definitions",expected_projectiles},
          {"authored_projectile_instantaneous",authored.instantaneous},
          {"authored_trigger_input",trigger_input},{"selected_authored_barrel",selected_barrel},
          {"source_game_kind",game_kind},{"original_mode_expects_projectile",!third_input_suppressed},
          {"multiplayer_third_input_suppression_checks",0},
          {"native_effect_frame_after_each_game_tick",true},
          {"audio_or_Xbox_pixels_certified",false}};
      const auto read_prepared=[&] {
        std::array<std::uint16_t,2> counts{};
        const auto body=world.source_objects->object_body(weapon);
        for(unsigned barrel=0;barrel<2;++barrel)
          world.source_objects->memory().read(body+0x1aau+barrel*0x34u,
              std::as_writable_bytes(std::span(&counts[barrel],1)));
        return counts;
      };
      auto previous=read_prepared();unsigned preparations{};
      std::set<std::uint32_t> seen,constructed;
      for(const auto &object:world.source_objects->live_objects())if(object.type==5)seen.insert(object.object);
      const auto &bank=world.source_objects->memory();
      const auto word=[&](std::uint32_t at){return Bytes(bank.view(at,4)).u32(0);};
      const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return bank.view(p,n);}};
      std::vector<std::size_t> birth_pending,retirement_pending;
      // The owned MCC profile observes complete B9B70/BA490 entry/returns.
      // A bullet can be constructed and retired inside one game tick, before
      // either a live-object snapshot or a retained contact effect exists.
      SourcePhysicsEntryObserverBinding binding(bank,[&](bool before,std::uint32_t entry,const auto &abi) {
        const auto tick=word(word(0x51df54u)+8u);
        if(entry==0xb9b70u) {
          auto &rows=firing["constructor_calls"];
          if(before) {
            const auto request=word(abi.gp[4]+4u),definition=word(request);
            if(!expected_projectiles.contains(definition)){birth_pending.push_back(SIZE_MAX);return;}
            birth_pending.push_back(rows.size());
            rows.push_back({{"definition",definition},{"tick",tick},
                {"caller_return",word(abi.gp[4])},{"completed",false}});
          } else {
            check(!birth_pending.empty(),"Unpaired original projectile constructor observation");
            const auto index=birth_pending.back();birth_pending.pop_back();if(index==SIZE_MAX)return;
            auto &row=rows.at(index);row["completed"]=true;row["returned_object"]=abi.gp[0];
            if(abi.gp[0]==null_datum)return;
            const auto header=source_raw_datum_lookup(word(0x4ecbd0u),abi.gp[0],storage);
            check(header && bank.view(header+3u,1)[0]==std::byte{5},
                "Original projectile constructor returned no live full type5 generation");
            const auto definition=word(word(header+8u));
            check(definition==row.at("definition").get<std::uint32_t>(),
                "Original projectile constructor changed its authored definition");
            row["type"]=5;row["actual_definition"]=definition;
            constructed.insert(abi.gp[0]);
          }
        } else if(entry==0xba490u) {
          auto &rows=firing["retirement_calls"];
          if(before) {
            const auto object=word(abi.gp[4]+4u),header=source_raw_datum_lookup(word(0x4ecbd0u),object,storage);
            if(!header || bank.view(header+3u,1)[0]!=std::byte{5}){retirement_pending.push_back(SIZE_MAX);return;}
            const auto definition=word(word(header+8u));
            if(!expected_projectiles.contains(definition)){retirement_pending.push_back(SIZE_MAX);return;}
            retirement_pending.push_back(rows.size());rows.push_back({{"datum",object},{"definition",definition},
                {"tick",tick},{"caller_return",word(abi.gp[4])},{"type",5},{"completed",false}});
          } else {
            check(!retirement_pending.empty(),"Unpaired original projectile retirement observation");
            const auto index=retirement_pending.back();retirement_pending.pop_back();
            if(index!=SIZE_MAX)rows.at(index)["completed"]=true;
          }
        }
      });
      std::uint64_t contact_frontier{};
      for(const auto &event:world.match.state().grenade_detonations)
        contact_frontier=std::max(contact_frontier,event.serial);
      for(unsigned tick=0;tick<180;++tick) {
        const bool held=tick>=30 && tick<150;
        controls[player].fire=trigger_input==0 && held;
        controls[player].fire_secondary=trigger_input==1 && held;
        controls[player].melee=trigger_input==2 && held;
        world.match.step(controls);
        // Match the real native caller: game ticks precede13A9B0's
        // accepted effect/particle interval, shared by the local views.
        world.source_objects->advance_effect_frame(tick_seconds);
        if(third_input_suppressed) {
          const auto body=world.source_objects->object_body(weapon);
          check(!(Bytes(bank.view(body+0x16eu,2)).u16(0)&0x100u),
              "Local multiplayer input enabled the original mode-suppressed third trigger");
          firing["multiplayer_third_input_suppression_checks"]=tick+1;
        }
        const auto state=world.match.state();const auto &p=state.players[player];
        check(p.source_vehicle_hud.weapon.present &&
            p.source_vehicle_hud.ownership.vehicle_weapon==weapon,
            "Vehicle firing lost its genuine source weapon ownership");
        const auto counts=read_prepared();
        // Whole TU1.5 108080 adds prepared subshots at1085DF. This is
        // preparation evidence; only constructed type5 generations below
        // establish actual live projectile publication.
        for(unsigned barrel=0;barrel<2;++barrel)if(counts[barrel]>previous[barrel]) {
          check(tick>=30,"Neutral vehicle input prepared a weapon shot");
          preparations+=counts[barrel]-previous[barrel];
        }
        previous=counts;firing["prepared_subshots"]=preparations;
        for(const auto &object:world.source_objects->live_objects()) {
          if(object.type!=5 || !seen.insert(object.object).second)continue;
          check(expected_projectiles.contains(object.definition),
              "Vehicle firing constructed a projectile outside its selected authored trigger");
          constructed.insert(object.object);
          firing["live_projectiles"].push_back({{"datum",object.object},
              {"definition",object.definition},{"tick",state.tick}});
        }
        for(const auto &event:state.grenade_detonations) {
          if(event.serial<=contact_frontier)continue;
          contact_frontier=event.serial;
          if(!expected_projectiles.contains(event.definition))continue;
          check(event.projectile!=null_datum && event.source_effect_instance!=null_datum &&
              !event.source_locations.empty(),"Vehicle contact lacks its actual local source provenance");
          // The original contact observer validated this full type5 body
          // during its live update. It may retire before the tick snapshot.
          constructed.insert(event.projectile);
          firing["completed_source_contacts"].push_back({{"datum",event.projectile},
              {"definition",event.definition},{"tick",event.tick},
              {"source_effect_instance",event.source_effect_instance},
              {"source_locations",event.source_locations.size()},
              {"player",event.player},{"unit",event.unit}});
        }
        const auto phase=tick<30?"neutral_ticks":tick<150?"held_ticks":"release_ticks";
        firing[phase]=firing[phase].get<unsigned>()+1;
        firing["constructed_projectile_generations"]=constructed.size();
      }
      controls[player].fire=false;
      controls[player].fire_secondary=false;controls[player].melee=false;
      check(birth_pending.empty() && retirement_pending.empty(),"Original vehicle firing observation did not complete");
      if(third_input_suppressed) {
        check(preparations==0 && constructed.empty(),
            "Original mode-suppressed local vehicle input fired in multiplayer");
      } else {
        check(preparations>0,"Real vehicle trigger input prepared no source shot");
        check(!constructed.empty(),
            "Real vehicle trigger input has no validated constructor,live projectile or source contact");
      }
      firing["passed"]=true;
    }
    for(unsigned tick=0;tick<90;++tick)world.match.step(controls);
    bool exited=false;
    for(unsigned tick=0;tick<150;++tick) {
      controls[player].pickup=true;controls[player].pickup_milliseconds=900;world.match.step(controls);
      const auto state=world.match.state();const auto &p=state.players[player];
      if(p.source_parent!=null_datum)continue;
      check(!p.source_vehicle_hud.ownership.seated && p.source_vehicle_hud.ownership.handheld_visible &&
          !p.source_vehicle_hud.weapon.present && p.source_vehicle_hud.ownership.vehicle_weapon==null_datum &&
          p.source_vehicle_hud.ownership.parent_hud==null_datum,
          "Vehicle HUD did not release its weapon on actual source exit");exited=true;row["exit_tick"]=tick;break;
    }
    check(exited,"Normal original exit did not release vehicle HUD ownership");
    row["actual_source_exit_completed"]=exited;row["passed"]=true;
  }
  check(worlds>0,"No requested vehicle weapon HUD case was exercised");
  report.data["passed"]=true;report.data["worlds"]=worlds;report.data["selected_source_weapon_HUDs"]=weapon_huds;
  std::cout<<worlds<<" actual TU1.5 vehicle worlds, "<<weapon_huds<<" selected source weapon HUDs passed\n";
}catch(const std::exception &e){report.data["runtime_error"]=e.what();std::cerr<<e.what()<<'\n';return 1;}}
