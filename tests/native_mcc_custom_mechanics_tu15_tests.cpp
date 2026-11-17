#include "halo2/frontend.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/object_initialization.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/player_action_edges.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numeric>
#include <thread>

using namespace halo2;
namespace {
void need(bool ok,const std::string &why) {if(!ok)throw FormatError(why);}
unsigned row(const MenuState &menu,int value,bool action=false) {
  const auto &entries=menu.frame().entries;
  const auto at=std::ranges::find_if(entries,[&](const auto &e){return (action?e.destination:e.value)==value;});
  need(at!=entries.end(),"Custom mechanics lost its real menu row");return unsigned(at-entries.begin());
}
void choose(MenuState &menu,unsigned selected,unsigned screen) {
  menu.select(selected);menu.accept();need(menu.frame().screen==screen,"Custom mechanics missed menu screen");
}
FrontendLaunch start(const FrontendContent &content,unsigned preset,unsigned players,bool lan) {
  MenuState menu(content);menu.accept();choose(menu,0,167);
  menu.text("Mechanics 1");menu.commit_text();menu.back();
  choose(menu,lan?3:2,30);choose(menu,0,14);
  need(menu.controller_slot(100,true)==0,"Custom host controller lost its owner");
  for(unsigned i=1;i<players;++i) {
    need(menu.controller_slot(100+i,true)==i,"Custom local controller failed to join");
    choose(menu,0,167);menu.text("Mechanics "+std::to_string(i+1));menu.commit_text();menu.back();
  }
  const auto &variant=content.bundled_variants.at(preset);
  choose(menu,row(menu,23,true),23);choose(menu,row(menu,207,true),207);
  choose(menu,row(menu,int(variant.engine)),208);
  choose(menu,row(menu,int(content.variants.size()+preset)),14);
  const auto lockout=std::ranges::find(content.multiplayer_maps,50,&UiLobbyMap::map_id);
  need(lockout!=content.multiplayer_maps.end(),"Custom lift fixture has no authored Lockout catalog entry");
  choose(menu,row(menu,23,true),23);choose(menu,row(menu,206,true),206);
  choose(menu,row(menu,int(lockout-content.multiplayer_maps.begin())),14);
  if(bomb_defusal_variant(variant)) {menu.auxiliary();menu.navigate(1,0);menu.accept();}
  menu.select(row(menu,-4,true));menu.accept();
  need(menu.launch.has_value(),"Actual custom menu Start was refused: "+menu.frame().description);
  return *menu.launch;
}
Vec3 intake(const Cache &cache,const LevelContent &content) {
  const auto tag=std::ranges::find_if(cache.tags(),[](const auto &t){
    return t.name=="scenarios\\objects\\multi\\lockout\\lockout_lift_tube\\lockout_lift_tube";
  });
  need(tag!=cache.tags().end(),"Lockout source lift definition absent");
  const auto placement=std::ranges::find_if(content.objects,[&](const auto &o){return o.definition==tag->datum;});
  need(placement!=content.objects.end(),"Lockout source lift placement absent");
  const auto object=cache.load_source_tag_metadata(tag->datum);
  const auto model=cache.load_source_tag_metadata(Bytes(object.bytes_at(object.source_address(),object.size())).u32(0x38));
  const auto physics=cache.load_source_tag_metadata(Bytes(model.bytes_at(model.source_address(),model.size())).u32(0x24));
  const Bytes p(physics.bytes_at(physics.source_address(),physics.size()));
  const auto basis=authored_basis(placement->rotation);
  const auto root=source_object_transform(placement->position,basis.forward,basis.up,placement->scale,false);
  bool found{};Vec3 selected;
  for(unsigned n=0;n<p.u32(0x108);++n) {
    const Bytes phantom(physics.bytes_at(p.u32(0x10c)+n*0x20u,0x20));
    need(phantom.u16(0xc)==4,"Lockout authored lift changed its source hull owner");
    const Bytes hull(physics.bytes_at(p.u32(0x74)+phantom.u16(0xe)*0x100u,0x100));
    Transform local;local.position={hull.f32(0x40),hull.f32(0x44),hull.f32(0x48)};
    const auto point=compose_transform(root,local).position;
    if(!found || point.z<selected.z) {selected=point;found=true;}
  }
  need(found,"Lockout has no authored lift hull");return selected;
}
std::uint64_t scalars(const auto &activity) {
  return std::accumulate(activity.native_lift_scalar_calls.begin(),activity.native_lift_scalar_calls.end(),std::uint64_t{});
}
float damage_policy(PhysicsLiveWorldTu15 &world,std::uint32_t attacker,std::uint32_t victim) {
  // Whole TU1.5 160900 (original retail15F520) consumes these source players. This checks
  // policy inputs and native output, independently of projectile hit finding.
  auto call=world.prepare_source_call();call.gp[0]=victim;call.gp[1]=attacker;
  world.run_actual(0x160900u,call);return std::bit_cast<float>(call.xmm[0][0]);
}
SourceGameVariant mechanical_settings(PhysicsLiveWorldTu15 &world) {
  const auto &memory=world.memory();const auto globals=Bytes(memory.view(0x4f3218u,4)).u32(0);
  SourceGameVariant variant;memory.read(globals+0x13cu,std::as_writable_bytes(std::span(variant.bytes)));
  return variant;
}
nlohmann::json native_life_probe(MultiplayerPreparedWorld &prepared,unsigned slot) {
  auto &match=prepared.match;auto &objects=*prepared.source_objects;
  const auto &player=match.player(slot);const auto &unit=match.unit(slot);
  const auto source=objects.tu15_world().multiplayer_engine_state();
  nlohmann::json result={{"tick",match.simulation().ticks()},{"slot",slot},
      {"mirror_current",player.source_current_object},{"mirror_unit",unit.source_datum},
      {"mirror_deaths",player.deaths},{"native_deaths",source.statistics.players[slot].fields[3]},
      {"mirror_dead",unit.vitality.dead},{"mirror_death_counted",player.death_counted},
      {"membership",player.source_membership},{"profile_color",(*player.source_configuration)[0x40]}};
  const DatumPool native_players(objects.memory(),objects.player_pool());
  const Bytes raw(std::as_bytes(native_players.lookup(player.source_datum)));
  const auto current=raw.u32(0x2c);
  result["native_current"]=current;result["native_previous"]=raw.u32(0x30);
  result["native_flags"]=raw.u16(2);result["native_respawn_countdown"]=std::bit_cast<std::int32_t>(raw.u32(0x164));
  result["native_lives"]=std::bit_cast<std::int16_t>(raw.u16(0x1ac));
  if(current!=null_datum) {
    // Authenticate the full currently published generation before querying
    // its body. An obsolete corpse is never used as a substitute body.
    const DatumPool headers(objects.memory(),objects.object_pool());
    const auto header=headers.lookup(current);
    if(!header.empty()) {
      const auto vitality=objects.object_vitality(current);
      result["native_body"]=vitality.body;result["native_shield"]=vitality.shield;
      result["native_weapon_definitions"]=objects.player_equipment(current).definitions;
    }
  }
  return result;
}
template<class Predicate>void until(MultiplayerMatch &match,unsigned ticks,Predicate predicate,const char *why) {
  for(unsigned n=0;n<ticks;++n) {if(predicate())return;match.step({});}
  need(predicate(),why);
}
nlohmann::json native_rows(MultiplayerPreparedWorld &prepared,bool exercise_respawn) {
  auto &match=prepared.match;auto &objects=*prepared.source_objects;auto &world=objects.tu15_world();
  const auto variant=mechanical_settings(world);
  need(source_game_variant_type(variant)==2 && source_game_variant_get(variant,2)==0,
      "Custom mechanics did not retain genuine unlimited mechanical Slayer settings");
  std::vector<unsigned> weapon_rows;
  for(unsigned row=0;row<prepared.content.netgame_equipment.size() && row<100;++row) {
    const auto &authored=prepared.content.netgame_equipment[row];
    if(authored.classification==0 && std::ranges::any_of(authored.items,[&](const auto &item){
        return item.group==0x77656170u && prepared.content.weapons.contains(item.definition);}))weapon_rows.push_back(row);
  }
  need(!weapon_rows.empty(),"Custom row regression has no actual authored map weapon rows");
  if(bomb_defusal_variant(*match.session_configuration())) {
    need(source_game_variant_get(variant,83)==1,"Defusal omitted the original NoWeapons input");
    for(const auto [parameter,none]:std::array<std::pair<int,int>,7>{{
        {76,7},{77,7},{78,4},{79,4},{80,4},{81,6},{82,6}}})
      need(source_game_variant_get(variant,parameter)==none,"Defusal omitted an original NoVehicles input");
    for(unsigned tick=0;tick<60;++tick)match.step({});
    unsigned checked{};
    for(unsigned row=0;row<prepared.content.netgame_equipment.size() && row<100;++row) {
      const auto &authored=prepared.content.netgame_equipment[row];
      if(std::ranges::find(weapon_rows,row)!=weapon_rows.end() ||
          (authored.classification>=1 && authored.classification<=7)) {
        need(world.netgame_object(row)==null_datum,"Defusal native row owner created a forbidden map weapon/vehicle");++checked;
      }
    }
    return {{"passed",true},{"native_no_weapon_or_vehicle_rows",checked},{"mechanical_score_limit",0}};
  }
  unsigned selected=100;std::uint32_t original=null_datum;
  for(unsigned tick=0;tick<90 && original==null_datum;++tick) {
    for(const auto row:weapon_rows) {
      const auto &authored=prepared.content.netgame_equipment[row];
      if(authored.timer_start!=0 || authored.respawn_seconds<0 || authored.respawn_seconds>60)continue;
      const auto object=world.netgame_object(row);
      if(object!=null_datum && objects.object_parent(object)==null_datum) {selected=row;original=object;break;}
    }
    if(original==null_datum)match.step({});
  }
  need(original!=null_datum,"Custom World never created an authored weapon through its native row owner");
  nlohmann::json result={{"passed",true},{"row",selected},{"native_row_object",original},
      {"automatic_native_row_birth",true},{"mechanical_score_limit",0}};
  if(!exercise_respawn)return result;
  const auto owner=std::ranges::find_if(prepared.local_slots,[&](const auto slot){
    return custom_slayer(*match.session_configuration())!=CustomSlayer::infection ||
        (*match.player(slot).source_configuration)[0x40]!=6;
  });
  need(owner!=prepared.local_slots.end(),"Native row carry control lacks a live eligible participant");
  const auto unit=match.player(*owner).source_current_object;
  need(unit!=null_datum && objects.object_vitality(unit).body>0,
      "Native row carry fixture lost its actual living unit");
  // A declared inventory action enters the genuine carry owner used by
  // ordinary native equipment. First drop its existing loadout through the
  // original owner: a full incompatible inventory correctly refuses carry.
  // This tests row respawn, not pickup hit finding or replacement selection.
  const auto previous_weapons=world.player_equipment(unit).weapons;
  world.call_actual(0xcf4d0u,std::array{unit});
  need(std::ranges::all_of(world.player_equipment(unit).weapons,[](auto weapon){return weapon==null_datum;}),
      "Original clear/drop owner did not supply the native carry fixture's empty inventory");
  const auto carried=world.call_actual(0xcf5a0u,std::array{unit,original,1u});
  const auto equipment=world.player_equipment(unit);
  need((carried&255u)!=0 && std::ranges::find(equipment.weapons,original)!=equipment.weapons.end(),
      "Original carry owner did not admit the actual map weapon generation");
  const auto &authored=prepared.content.netgame_equipment[selected];
  const int delay=authored.respawn_seconds?authored.respawn_seconds:
      authored.collection_delay?authored.collection_delay:30;
  need(delay>0 && delay<=60,"Native row respawn fixture lacks a bounded authored interval");
  std::uint32_t replacement=null_datum;unsigned elapsed{};
  for(;elapsed<unsigned(delay*30+120);++elapsed) {
    match.step({});replacement=world.netgame_object(selected);
    if(replacement!=null_datum && replacement!=original)break;
  }
  need(replacement!=null_datum && replacement!=original && objects.object_parent(replacement)==null_datum,
      "Native custom row timer did not construct a distinct replacement generation");
  result["native_carry_input"]=true;result["replacement_object"]=replacement;
  result["native_clear_drop_input"]=true;result["previous_held_weapons"]=previous_weapons;
  result["actual_respawn_ticks"]=elapsed+1;result["authored_respawn_seconds"]=delay;
  return result;
}
void source_death_event(PhysicsLiveWorldTu15 &world,std::uint32_t victim,std::uint32_t credited) {
  // Authentic1F2280 order: engine death callback15F8B0, then the separate
  // whole1F2A50 victim-stat owner. Neither call supplies a recorded outcome.
  world.call_actual(0x15f8b0u,std::array{credited,victim,0u,63u});
  const auto engine=Bytes(world.memory().view(0x4f63b8u,4)).u32(0);
  auto capture=world.prepare_source_call(std::array{engine+0x304u,credited,0u,0u});
  capture.gp[7]=victim;capture.gp[1]=63u;
  world.run_actual(0x1f2a50u,capture);
}
nlohmann::json score_boundary(const FrontendContent &content,unsigned preset,const std::filesystem::path &maps) {
  const auto launch=start(content,preset,2,false);
  MultiplayerOptions options;options.map=maps/launch.map;options.shared_maps=maps;
  options.variant=launch.variant;options.variant.values.set(2,1);options.local_players=2;
  options.selected_profiles=launch.selected_profiles;options.input_instances=launch.input_instances;
  options.mute=true;options.source_object_world=true;
  options.source_object_savegame_directory=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/"score-boundary";
  // One declared, legal custom score-goal input makes the real native win
  // threshold observable in one death. No score/status bank is supplied.
  MultiplayerPreparedWorld prepared(options);auto &match=prepared.match;auto &objects=*prepared.source_objects;
  auto &world=objects.tu15_world();match.step({});
  need(source_game_variant_get(source_game_variant(*match.session_configuration()),2)==1 &&
      source_game_variant_get(mechanical_settings(world),2)==0 && !match.finished(),
      "Custom score boundary lost its independent advertised/mechanical input owners");
  const auto attacker=prepared.local_slots[0],victim=prepared.local_slots[1];
  const auto victim_unit=match.player(victim).source_current_object;
  need(victim_unit!=null_datum,"Score boundary lost the genuine victim body");
  source_death_event(world,match.player(victim).source_datum,match.player(attacker).source_datum);
  const auto native=world.multiplayer_engine_state();
  need(native.statistics.players[attacker].fields[0]>=1 && !native.game_over.active &&
      !native.finalized && !match.finished(),
      "Native death score callback ended the private mechanical game at the custom score boundary");
  objects.kill_biped(victim_unit);match.step({});
  const auto completed=world.multiplayer_engine_state();
  need(match.finished() && match.player(attacker).score==1 && completed.game_over.active &&
      completed.round.phase==2 && completed.round.status==2,
      "Custom winner did not complete its native mechanical lifecycle at the completed World boundary");
  // A completed custom match must not leave the private native scheduler
  // active. Supply a genuine fatal physical input after completion, then
  // cross the ordinary respawn delay without any replacement generation.
  const auto survivor=match.player(attacker).source_current_object;
  need(survivor!=null_datum,"Completion control lost the surviving native unit");
  objects.kill_biped(survivor);
  const auto delay=source_game_variant_get(source_game_variant(*match.session_configuration()),17);
  const auto post_finish_ticks=unsigned(std::max(300,delay*30+120));
  for(unsigned tick=0;tick<post_finish_ticks;++tick) {
    match.step({});
    for(unsigned slot:{attacker,victim})
      need(match.player(slot).source_current_object==null_datum ||
          match.player(slot).source_current_object==(slot==attacker?survivor:victim_unit),
          "Completed custom match admitted a new native player generation");
  }
  const auto inactive=world.multiplayer_engine_state();
  need(match.finished() && match.player(attacker).score==1 && inactive.round.phase==2 &&
      inactive.round.status==2 && inactive.game_over.active,
      "Custom completion lost its native inactive acknowledgement or custom winner");
  return {{"passed",true},{"declared_custom_score_goal",1},{"private_native_score_limit",0},
      {"native_death_callback",true},{"native_score",native.statistics.players[attacker].fields[0]},
      {"custom_finished",true},{"native_game_over",true},{"native_requested_phase",2},
      {"native_applied_status",2},{"post_completion_no_rebirth_ticks",post_finish_ticks},
      {"scope","Actual native death/score event plus physical cleanup, independently of projectile hit finding"}};
}
void fatal(MultiplayerPreparedWorld &prepared,unsigned slot) {
  auto &objects=*prepared.source_objects;const auto &player=prepared.match.player(slot);
  need(player.source_current_object!=null_datum,"Fatal control lost its actual source unit");
  // The same genuine engine-event/physical-cleanup boundary used by the
  // canonical player-life test. This does not claim projectile hit finding.
  source_death_event(objects.tu15_world(),player.source_datum,null_datum);
  objects.kill_biped(player.source_current_object);
}
nlohmann::json zero_delay_infection(const FrontendContent &content,unsigned preset,const std::filesystem::path &maps) {
  const auto launch=start(content,preset,4,false);
  MultiplayerOptions options;options.map=maps/launch.map;options.shared_maps=maps;
  options.variant=launch.variant;options.variant.values.set(17,0);options.local_players=4;
  options.selected_profiles=launch.selected_profiles;options.input_instances=launch.input_instances;
  options.mute=true;options.source_object_world=true;
  options.source_object_savegame_directory=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/"zero-delay-infection";
  MultiplayerPreparedWorld prepared(options);auto &match=prepared.match;auto &objects=*prepared.source_objects;
  for(unsigned tick=0;tick<3;++tick)match.step({});
  const auto human=std::ranges::find_if(prepared.local_slots,[&](const auto slot){
    return (*match.player(slot).source_configuration)[0x40]!=6;
  });
  need(human!=prepared.local_slots.end(),"Zero-delay native control has no actual human participant");
  const auto slot=*human,previous=match.player(slot).source_current_object;
  const auto deaths=match.player(slot).deaths;
  need(previous!=null_datum && source_game_variant_get(mechanical_settings(objects.tu15_world()),17)==0,
      "Zero-delay native control lost its source body or declared native interval");
  nlohmann::json trace=nlohmann::json::array();
  trace.push_back({{"edge","before-declared-fatal-input"},{"state",native_life_probe(prepared,slot)}});
  fatal(prepared,slot);
  trace.push_back({{"edge","after-native-fatal-before-native-frame"},{"state",native_life_probe(prepared,slot)}});
  for(unsigned tick=0;tick<120;++tick) {
    match.step({});const auto current=match.player(slot).source_current_object;
    const bool replacement=current!=null_datum && current!=previous;
    if(tick<4 || replacement || tick==119)
      trace.push_back({{"edge","after-native-frame"},{"state",native_life_probe(prepared,slot)}});
    if(replacement)break;
  }
  const auto current=match.player(slot).source_current_object;
  need(current!=null_datum && current!=previous,
      "Original zero-delay scheduler did not supply a replacement unit: "+trace.dump());
  const auto equipment=objects.player_equipment(current);
  auto choices=prepared.cache->multiplayer_weapon_choices();std::uint32_t seed{};
  const auto sword=source_starting_weapon(null_datum,17,null_datum,choices,seed);
  need(!match.finished() && match.player(slot).deaths==deaths+1 && match.player(slot).source_membership==1 &&
      (*match.player(slot).source_configuration)[0x40]==6 && equipment.definitions[0]==sword &&
      equipment.grenades==std::array<std::uint8_t,2>{0,0} && objects.object_vitality(current).shield==0.f,
      "Zero-delay source birth outran custom death conversion or lost native zombie inputs: "+trace.dump());
  return {{"passed",true},{"declared_respawn_seconds",0},{"previous_unit",previous},
      {"replacement_unit",current},{"native_source_deaths",match.player(slot).deaths-deaths},
      {"native_zombie_profile",true},{"host_factory_requested_by_test",false},{"native_life_trace",trace}};
}

nlohmann::json infection_pickups(const FrontendContent &content,unsigned preset,const std::filesystem::path &maps) {
  nlohmann::json report={{"passed",false},{"cases",nlohmann::json::array()},
      {"scope","Actual menu Infection, genuine native loose-weapon/profile inputs and ordinary pickup/dual/switch commands. No carried-item, target-query, inventory or command outcomes supplied."}};
  for(const auto probe:{"human-pickup","zombie-pickup","zombie-dual","human-switch","zombie-switch"}) {
    auto &result=report["cases"].emplace_back(nlohmann::json{{"case",probe},{"passed",false},{"samples",nlohmann::json::array()}});
    try {
      const bool zombie=std::string_view(probe).starts_with("zombie"),switching=std::string_view(probe).ends_with("switch");
      const bool dual=std::string_view(probe)=="zombie-dual";
      const auto launch=start(content,preset,2,false);
      MultiplayerOptions options;options.map=maps/launch.map;options.shared_maps=maps;
      options.variant=launch.variant;options.local_players=2;options.mute=true;
      options.selected_profiles=launch.selected_profiles;options.input_instances=launch.input_instances;
      options.source_object_savegame_directory=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/("infection-"+std::string(probe));
      select_multiplayer_mode_authority(options);MultiplayerPreparedWorld prepared(options);
      need(prepared.source_objects && prepared.source_objects->owns_physics() && !prepared.match.source_engine_owned() &&
          prepared.rules.custom==CustomSlayer::infection,"Pickup policy control lost its actual native custom World");
      auto &match=prepared.match;auto &objects=*prepared.source_objects;auto &world=objects.tu15_world();
      for(unsigned tick=0;tick<15;++tick)match.step({});
      const auto selected=std::ranges::find_if(prepared.local_slots,[&](const auto slot){return ((*match.player(slot).source_configuration)[0x40]==6)==zombie;});
      need(selected!=prepared.local_slots.end(),"Pickup policy control lacks its actual faction participant");
      const auto slot=*selected,player=match.player(slot).source_datum,unit=match.player(slot).source_current_object;
      need(unit!=null_datum && objects.object_vitality(unit).body>0,"Pickup policy control has no living native actor");
      std::uint32_t seed{};const auto choices=prepared.cache->multiplayer_weapon_choices();
      const auto sword=source_starting_weapon(null_datum,17,null_datum,choices,seed);
      const auto magnum=source_starting_weapon(null_datum,4,null_datum,choices,seed);
      const auto smg=prepared.cache->slayer_starting_weapon(5);
      const auto weapon_input=[&](std::uint32_t definition) {
        const auto &weapon=prepared.content.weapons.at(definition);const auto loaded=std::min(std::max(0,weapon.capacity),std::max(0,weapon.initial_rounds));
        return SourceUnitEquipmentWeaponInputTu15{definition,std::int16_t(loaded),std::int16_t(loaded),0x77656170u};
      };
      if(switching) {
        // A sword-only inventory makes switch refusal vacuous. Declare a real
        // second carried weapon through CF2E0; original birth/carry owns both
        // full generations. The ordinary button must still obey the faction.
        SourceUnitEquipmentProfileInputTu15 alternate;
        alternate.weapons[0]=weapon_input(zombie?sword:smg);alternate.weapons[1]=weapon_input(magnum);
        const auto supplied=world.apply_equipment_profile(unit,alternate);
        need(supplied.weapons[0]!=null_datum && supplied.weapons[1]!=null_datum && supplied.weapons[0]!=supplied.weapons[1] &&
            supplied.definitions[0]==alternate.weapons[0].definition && supplied.definitions[1]==magnum,
            "Declared alternate profile did not birth/carry two actual weapon generations");
        const DatumPool headers(world.memory(),world.source_object_pool());
        for(unsigned index=0;index<2;++index)need(!headers.lookup(supplied.weapons[index]).empty(),
            "Declared alternate profile lost its genuine full weapon generation");
        result["declared_alternate_native_profile"]={{"weapons",supplied.weapons},{"definitions",supplied.definitions}};
        for(unsigned tick=0;tick<6;++tick)match.step({});
      }
      const auto initial=world.player_equipment(unit);const auto initial_weapon=match.unit(slot).weapon.source_datum;
      need(initial_weapon!=null_datum && initial.current[0]>=0 && unsigned(initial.current[0])<initial.definitions.size() &&
          (!zombie || initial.definitions[unsigned(initial.current[0])]==sword),
          "Pickup policy actor did not begin with its actual selected native loadout");
      result.update({{"slot",slot},{"source_player",player},{"source_unit",unit},{"zombie",zombie},
          {"initial_weapons",initial.weapons},{"initial_definitions",initial.definitions},{"initial_selected",initial_weapon}});
      std::uint32_t pickup=null_datum;unsigned query_samples{};bool admitted{},admitted_before_input{};
      if(!switching) {
        const auto pose=objects.biped_motion(unit);const auto forward=pose.forward;
        const auto point=pose.position+Vec3{forward.x*.5f,forward.y*.5f,.2f};const auto before_tick=match.state().tick;
        pickup=world.create_weapon(smg,point);
        need(pickup!=null_datum && !objects.object_body(pickup).is_null(),"Pickup fixture failed its genuine loose weapon birth");
        const auto loose=match.state();const auto found=std::ranges::find(loose.world_weapons,pickup,&WorldWeaponPresentation::handle);
        need(found!=loose.world_weapons.end() && found->definition==smg && objects.object_parent(pickup)==null_datum &&
            loose.tick==before_tick && match.player(slot).source_current_object==unit,
            "Declared loose pickup did not publish its exact native generation without advancing its actor/clock");
        result["declared_loose_weapon"]={{"object",pickup},{"definition",smg},{"position",{point.x,point.y,point.z}}};
      }
      const auto threshold=source_pickup_hold_threshold(prepared.content.pickup_hold_seconds);
      need(threshold>=0 && threshold<2000,"Pickup policy fixture lacks a bounded authored button hold");
      std::array<Controls,multiplayer_capacity> controls{};
      for(unsigned tick=0;tick<48;++tick) {
        need(match.player(slot).source_current_object==unit && objects.object_vitality(unit).body>0,
            "Pickup policy fixture changed or killed its genuine actor");
        controls={};controls[slot].yaw=match.unit(slot).yaw;controls[slot].pitch=match.unit(slot).pitch;
        // First obtain completed ordinary target queries. Then use hold and
        // release edges; no explicit target or carry request enters the test.
        if(switching)controls[slot].switch_weapon=tick==2;
        else if(tick>=2 && tick<40) {
          controls[slot].pickup=!dual;controls[slot].dual_wield=dual;
          const auto held=std::uint16_t(std::min(65000,threshold+int((tick-2)*1000/30)));
          controls[slot].pickup_milliseconds=dual?0:held;controls[slot].dual_wield_milliseconds=dual?held:0;
        }
        const auto target=world.player_action_target(player,unit);
        if(target && target->object==pickup && (target->action==1 || target->action==7))++query_samples;
        match.step(controls);const auto equipment=world.player_equipment(unit);
        const auto current=match.unit(slot).weapon.source_datum;
        const bool has=pickup!=null_datum && std::ranges::find(equipment.weapons,pickup)!=equipment.weapons.end();
        const bool changed=switching?current!=initial_weapon:has;
        admitted=admitted || changed;admitted_before_input=admitted_before_input || (tick<2 && changed);
        result["samples"].push_back({{"tick",match.state().tick},{"pickup_down",controls[slot].pickup},
            {"dual_down",controls[slot].dual_wield},{"switch_down",controls[slot].switch_weapon},
            {"target_object",target?target->object:null_datum},{"target_action",target?target->action:0},
            {"target_index",target?target->index:-1},{"selected",current},{"weapons",equipment.weapons}});
      }
      const auto completed=world.player_equipment(unit);
      result.update({{"authored_hold_threshold_ms",threshold},{"exact_native_weapon_query_samples",query_samples},
          {"ordinary_input_admitted",admitted},{"completed_weapons",completed.weapons},
          {"admitted_before_button_input",admitted_before_input},
          {"completed_selected",match.unit(slot).weapon.source_datum}});
      if(!switching)need(query_samples>0,"Pickup fixture never exposed its exact loose generation through the original native weapon query");
      if(zombie)need(!admitted && completed.weapons==initial.weapons && match.unit(slot).weapon.source_datum==initial_weapon,
          "Native Infection zombie admitted an ordinary weapon pickup/dual/switch request");
      else need(admitted && !admitted_before_input && (switching || query_samples>0),
          "Eligible human never completed the ordinary native pickup/switch positive control");
      if(!switching && !zombie) {
        const auto body=world.source_object_body(pickup);const Bytes item(world.memory().view(body+0x12cu,0x2cu));
        need(item.u32(0x28)==unit && (item.u32(0)&5u)==5u,
            "Human positive pickup lacks original item owner/held flags");
      }
      result["passed"]=true;
    }catch(const std::exception &error){result["error"]=error.what();}
  }
  report["passed"]=std::ranges::all_of(report["cases"],[](const auto &row){return row["passed"].template get<bool>();});
  return report;
}
nlohmann::json custom_camouflage(const FrontendContent &content,unsigned preset,const std::filesystem::path &maps) {
  nlohmann::json report={{"passed",false},{"cases",nlohmann::json::array()},
      {"scope","Legal original variant parameter21 is the sole camouflage input. Ordinary native frames own grant, timers and body fade; all body/player observations are read-only. No powerup grant API, position input or bank outcomes supplied."}};
  const auto launch=start(content,preset,2,false);
  for(const bool custom:{false,true})for(const bool enabled:{false,true}) {
    const auto name=std::string(custom?"fiesta":"classic")+(enabled?"-camouflage":"-no-camouflage");
    auto &result=report["cases"].emplace_back(nlohmann::json{{"case",name},{"passed",false},{"samples",nlohmann::json::array()}});
    try {
      UiVariant variant=launch.variant;
      if(!custom){variant=UiVariant{};variant.name="Classic Slayer";variant.engine=0;}
      variant.values.set(21,enabled?1:0);
      const auto requested=source_game_variant(variant);
      need(source_game_variant_valid(requested) && source_game_variant_get(requested,21)==int(enabled),
          "Camouflage control is not a legal original variant flag input");
      MultiplayerOptions options;options.map=maps/launch.map;options.shared_maps=maps;options.variant=variant;
      options.local_players=2;options.mute=true;options.selected_profiles=launch.selected_profiles;
      options.input_instances=launch.input_instances;
      options.source_object_savegame_directory=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/("global-"+name);
      select_multiplayer_mode_authority(options);MultiplayerPreparedWorld prepared(options);
      need(prepared.source_objects && prepared.source_objects->owns_physics() &&
          prepared.match.source_engine_owned()!=custom,"Global camouflage lost its selected native mechanical frame owner");
      auto &match=prepared.match;auto &objects=*prepared.source_objects;auto &world=objects.tu15_world();
      need(source_game_variant_get(mechanical_settings(world),21)==int(enabled),
          "Native mechanical variant lost its declared global camouflage input");
      const auto slot=prepared.local_slots.front(),player=match.player(slot).source_datum,unit=match.player(slot).source_current_object;
      need(player!=null_datum && unit!=null_datum,"Global camouflage has no genuine initial player/unit generations");
      const DatumPool players(world.memory(),world.source_player_pool());
      const DatumPool headers(world.memory(),world.source_object_pool());
      const auto pool=Bytes(world.memory().view(world.source_player_pool(),0x4cu));
      const auto player_row=pool.u32(0x44u)+(player&65535u)*pool.u32(0x24u),timer=player_row+0x198u;
      need(!players.lookup(player).empty() && !headers.lookup(unit).empty(),"Global camouflage full source generations are absent");
      std::array<std::uint64_t,2> frame_calls{},refresh_calls{},postpass_calls{};
      std::vector<bool> refresh_stack;
      std::uint64_t grant_stores{};nlohmann::json grants=nlohmann::json::array();
      SourcePhysicsEntryObserverBinding entries(world.memory(),[&](bool entering,std::uint32_t owner,const SourcePhysicsWorldRuntimeAbi &abi) {
        if(owner==0x15eba0u)++frame_calls[entering?0:1];
        if(owner==0x1503d0u)++postpass_calls[entering?0:1];
        if(owner==0x15dc90u) {
          if(entering){const bool selected=abi.gp[6]==player;refresh_stack.push_back(selected);if(selected)++refresh_calls[0];}
          else {need(!refresh_stack.empty(),"Native camouflage refresh returned without its entry");
            if(refresh_stack.back())++refresh_calls[1];refresh_stack.pop_back();}
        }
      });
      world.observe_physics_stores_read_only([&](std::uint32_t pc,const auto &store) {
        // 154BD0 is an internal direct child of generated15DC90. Its exact
        // selected-player timer store witnesses execution without pretending
        // it reenters the public entry/return observer boundary.
        if((pc!=0x154c14u && pc!=0x154c0eu) || store.address!=timer)return;
        need(store.written.size()==2,"Native camouflage grant changed its original timer store width");
        ++grant_stores;if(grants.size()<8)grants.push_back({{"instruction",pc},{"before",Bytes(store.before).u16(0)},
            {"after",Bytes(store.written).u16(0)}});
      });
      struct Cleanup {PhysicsLiveWorldTu15 &world;~Cleanup(){world.observe_physics_stores_read_only({});}} cleanup{world};
      bool active{},inactive=true;
      for(unsigned tick=0;tick<30;++tick) {
        match.step({});
        need(match.player(slot).source_datum==player && match.player(slot).source_current_object==unit &&
            !players.lookup(player).empty() && !headers.lookup(unit).empty() && objects.object_vitality(unit).body>0,
            "Global camouflage fixture lost its living native player/unit generation");
        const Bytes raw_player(std::as_bytes(players.lookup(player)));
        need(raw_player.u32(0x2cu)==unit,"Global camouflage readback is not the native player's current full unit generation");
        const auto ticks=std::bit_cast<std::int16_t>(raw_player.u16(0x198u));
        const auto body=world.source_object_body(unit);const Bytes raw_body(world.memory().view(body+0x2b0u,12));
        const auto primary=raw_body.f32(0),secondary=raw_body.f32(4),fade=raw_body.f32(8);
        need(std::isfinite(primary) && std::isfinite(secondary) && std::isfinite(fade),"Native global camouflage fade became nonfinite");
        const auto state=match.state();const auto &adopted=state.players.at(slot).source_camouflage;
        need(match.player(slot).source_powerup_timers[0]==ticks &&
            std::bit_cast<std::uint32_t>(adopted.primary)==raw_body.u32(0) &&
            std::bit_cast<std::uint32_t>(adopted.secondary)==raw_body.u32(4) &&
            std::bit_cast<std::uint32_t>(adopted.fade_speed)==raw_body.u32(8),
            "Global camouflage completed native timers/body fade differ from their adopted view");
        active=active || (ticks>0 && primary>0.f);inactive=inactive && ticks==0 && primary==0.f;
        result["samples"].push_back({{"tick",state.tick},{"timer",ticks},{"primary",primary},{"secondary",secondary},{"fade_speed",fade}});
      }
      result.update({{"source_player",player},{"source_unit",unit},{"native_frame_entries_returns",frame_calls},
          {"selected_refresh_entries_returns",refresh_calls},{"native_postpass_entries_returns",postpass_calls},
          {"selected_native154BD0_timer_stores",grant_stores},{"grant_store_examples",grants}});
      need(refresh_stack.empty() && postpass_calls[0]>0 && postpass_calls[0]==postpass_calls[1] &&
          (custom?(refresh_calls[0]>0 && refresh_calls[0]==refresh_calls[1]):(frame_calls[0]>0 && frame_calls[0]==frame_calls[1])),
          "Global camouflage did not complete its selected genuine frame/refresh/postpass owners");
      need(enabled?(active && grant_stores>0):(inactive && grant_stores==0),
          "Global camouflage flag did not select its original grant/timer/body fade behavior");
      if(custom && enabled) {
        const auto fingerprint=custom_slayer_fingerprint(multiplayer_fingerprint(options.map,maps,0,
            prepared.network_starting_weapon_identity()),variant);
        LanHost host(match,0,fingerprint,variant,options.map.stem().string());
        const auto description=query_lan_session(host.join_address());
        need(description.fingerprint==fingerprint && description.variant.values==variant.values &&
            source_game_variant_get(source_game_variant(description.variant),21)==1,
            "Global camouflage LAN discovery changed its actual custom variant flag");
        const std::array<std::uint8_t,9> appearance{};
        const std::array<SourcePlayerConfiguration,1> profiles{
            native_selected_player_configuration("Camouflage Peer",appearance,0)};
        LanClient client(host.join_address(),fingerprint,1,{},profiles);
        std::array<Controls,multiplayer_capacity> controls{};std::array<Controls,1> input{};
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        while(!client.ready()) {
          client.send(input);host.receive(controls);host.broadcast();client.receive();
          need(client.error().empty(),"Global camouflage LAN admission failed: "+client.error());
          need(std::chrono::steady_clock::now()<deadline,"Global camouflage LAN admission timed out");
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        nlohmann::json snapshots=nlohmann::json::array();
        for(unsigned tick=0;tick<5;++tick) {
          client.send(input);host.receive(controls);match.step(controls);host.broadcast();client.receive();
          const auto authority=match.state();const auto &received=client.state();
          const auto &sent=authority.players.at(slot),&decoded=received.players.at(slot);
          const auto raw=Bytes(std::as_bytes(players.lookup(player)));
          const auto timer_value=std::bit_cast<std::int16_t>(raw.u16(0x198u));
          need(raw.u32(0x2cu)==unit && !headers.lookup(unit).empty(),
              "LAN camouflage readback lost the selected current native unit generation");
          const Bytes source_fade(world.memory().view(world.source_object_body(unit)+0x2b0u,12));
          need(client.error().empty() && received.tick==authority.tick && sent.source_player==player &&
              sent.source_unit==unit && decoded.source_player==player && decoded.source_unit==unit &&
              timer_value>0 && match.player(slot).source_powerup_timers[0]==timer_value && sent.source_camouflage.primary>0.f &&
              std::bit_cast<std::uint32_t>(sent.source_camouflage.primary)==source_fade.u32(0) &&
              std::bit_cast<std::uint32_t>(sent.source_camouflage.secondary)==source_fade.u32(4) &&
              std::bit_cast<std::uint32_t>(sent.source_camouflage.fade_speed)==source_fade.u32(8) &&
              std::bit_cast<std::uint32_t>(decoded.source_camouflage.primary)==std::bit_cast<std::uint32_t>(sent.source_camouflage.primary) &&
              std::bit_cast<std::uint32_t>(decoded.source_camouflage.secondary)==std::bit_cast<std::uint32_t>(sent.source_camouflage.secondary) &&
              std::bit_cast<std::uint32_t>(decoded.source_camouflage.fade_speed)==std::bit_cast<std::uint32_t>(sent.source_camouflage.fade_speed),
              "Actual LAN decode changed the current native actor's completed camouflage fade bits");
          snapshots.push_back({{"tick",received.tick},{"source_player",player},{"source_unit",unit},
              {"native_host_timer",timer_value},{"decoded_primary",decoded.source_camouflage.primary},
              {"decoded_secondary",decoded.source_camouflage.secondary},{"decoded_fade_speed",decoded.source_camouflage.fade_speed}});
        }
        const auto &received=client.state();const auto origins=client.received_origins();
        need(!origins.empty() && origins.back().tick()==received.tick && origins.back().encoded_body_bytes()>0 &&
            received.source_body_poses_authoritative && received.source_scene_roots_authoritative,
            "Global camouflage LAN state lost its authenticated source snapshot origin");
        auto replica_options=options;replica_options.host_port=0;replica_options.join=host.join_address();
        replica_options.local_players=1;replica_options.selected_profiles.assign(profiles.begin(),profiles.end());
        replica_options.selected_teams.clear();replica_options.input_instances={-1};replica_options.variant=description.variant;
        select_multiplayer_mode_authority(replica_options);MultiplayerPreparedWorld replica(replica_options);
        need(!replica.source_objects && replica.local_slots.empty() && replica.rules.custom==CustomSlayer::fiesta &&
            replica.match.session_configuration() &&
            source_game_variant_get(source_game_variant(*replica.match.session_configuration()),21)==1 &&
            std::ranges::none_of(replica.match.state().players,[](const auto &participant){return participant.connected;}),
            "Global camouflage replica created physical authority/local units or lost its source flag");
        replica.match.replicate_devices(received.devices);
        replica.match.replicate_source_scene(received.source_scene_roots,received.source_body_poses);
        replica.match.replicate_prop_removals(received.prop_removals);replica.match.replicate_prop_regions(received.prop_regions);
        const auto &scene=replica.match.simulation().scene();
        need(scene.source_replica() && scene.objects().size()==received.source_scene_roots.size(),
            "Global camouflage replica did not consume the actual decoded source scene");
        for(const auto &root:received.source_scene_roots) {
          const auto &object=scene.get(root.object);
          need(object.placement==root.placement && object.solved_transform &&
              std::bit_cast<std::array<std::uint32_t,3>>(object.position)==
                  std::bit_cast<std::array<std::uint32_t,3>>(root.transform.position),
              "Global camouflage replica changed a decoded full generation/placement/pose");
        }
        const auto replica_tick=replica.match.state().tick;bool refused{};
        try {replica.match.step({});}
        catch(const FormatError &error){refused=std::string_view(error.what())=="Source replica cannot advance a locally invented physical world";}
        need(refused && replica.match.state().tick==replica_tick && !replica.source_objects,
            "Global camouflage replica advanced an invented native authority or timer");
        result["lan_replica"]={{"passed",true},{"decoded_samples",snapshots},{"additional_native_host_ticks",5},
            {"source_origin_bytes",origins.back().encoded_body_bytes()},{"applied_source_roots",received.source_scene_roots.size()},
            {"replica_no_world_or_local_units",true},{"replica_step_refused",true},
            {"scope","Host native timer is read-only source evidence. The wire carries only three completed body camouflage floats; decoded bits match, and the replica never simulates timers."}};
      }
      result["passed"]=true;
    }catch(const std::exception &error){result["error"]=error.what();}
  }
  // Keep the historical synthetic adapter refusing this unsupported mechanic.
  // It has no native bank, current player allocation or original fade owner.
  try {
    LevelContent synthetic;synthetic.player.datum=1;synthetic.player.vitality.health=100;
    synthetic.player.height=1;synthetic.player.crouch_height=.6f;synthetic.player.radius=.2f;
    synthetic.player.camera_height=.8f;synthetic.player.crouch_camera_height=.5f;
    WeaponDefinition weapon;weapon.datum=3;weapon.capacity=20;weapon.initial_rounds=20;synthetic.weapons.emplace(3,weapon);
    auto variant=launch.variant;variant.values.set(21,1);auto rules=slayer_rules(variant);rules.weapon=3;
    const std::array<Spawn,1> starts{};bool refused{};
    try {MultiplayerMatch unsupported(synthetic,0,starts,rules);}
    catch(const FormatError &error){refused=std::string_view(error.what())=="Global camouflage requires its original timer, fade and render consumers";}
    need(refused,"Synthetic custom adapter admitted global camouflage without its native owners");
    report["synthetic_adapter_refusal"]={{"passed",true}};
  }catch(const std::exception &error){report["synthetic_adapter_refusal"]={{"passed",false},{"error",error.what()}};}
  report["passed"]=report["synthetic_adapter_refusal"]["passed"].get<bool>() &&
      std::ranges::all_of(report["cases"],[](const auto &item){return item["passed"].template get<bool>();});
  return report;
}
nlohmann::json lifecycle(MultiplayerPreparedWorld &prepared,const std::vector<Vec3> &canonical) {
  auto &match=prepared.match;auto &objects=*prepared.source_objects;auto &world=objects.tu15_world();
  const auto variant=*match.session_configuration();const auto kind=custom_slayer(variant);
  if(!bomb_defusal_variant(variant)) {
    auto slot=prepared.local_slots.front();
    if(kind==CustomSlayer::infection) {
      const bool human=prepared.local_slots.size()==4;
      const auto at=std::ranges::find_if(prepared.local_slots,[&](auto candidate){
        return ((*match.player(candidate).source_configuration)[0x40]!=6)==human;
      });
      need(at!=prepared.local_slots.end(),"Infection lifecycle omitted its real faction participant");slot=*at;
    }
    const auto previous=match.player(slot).source_current_object;
    const auto deaths=match.player(slot).deaths;
    fatal(prepared,slot);match.step({});
    need(match.player(slot).deaths==deaths+1,"Native custom fatal event was not credited exactly once");
    until(match,600,[&]{const auto unit=match.player(slot).source_current_object;
      return unit!=null_datum && unit!=previous;},"Custom native player factory did not respawn a full new generation");
    const auto current=match.player(slot).source_current_object;
    need(match.player(slot).deaths==deaths+1 && match.unit(slot).source_datum==current &&
        objects.object_vitality(current).body>0,"Native custom respawn lost death count/body readback");
    const auto equipment=objects.player_equipment(current);
    need(equipment.definitions[0]==match.unit(slot).weapon.definition && equipment.weapons[0]!=null_datum,
        "Native respawn inventory differs from its actual carried objects");
    if(kind==CustomSlayer::infection)
      need((*match.player(slot).source_configuration)[0x40]==6 && match.player(slot).source_membership==1 &&
          equipment.grenades==std::array<std::uint8_t,2>{0,0} && objects.object_vitality(current).shield==0.f,
          "Infection conversion/respawn lost its native faction/loadout/vitality");
    return {{"passed",true},{"previous_unit",previous},{"replacement_unit",current},
        {"native_engine_fatal_event",true},{"deaths_before",deaths},{"deaths_after",match.player(slot).deaths},
        {"infection_human_conversion",kind==CustomSlayer::infection && prepared.local_slots.size()==4}};
  }
  const auto state=match.state();need(state.defusal && state.defusal->phase==BombDefusalPhase::live,"Defusal lifecycle did not reach live play");
  need(world.multiplayer_engine_entity().type==2 && source_game_variant_type(source_game_variant(variant))==9,
      "Custom Assault rules did not retain their distinct genuine mechanical Slayer owner");
  const auto requested_team=std::int8_t(1-state.defusal->attacking_team);
  const std::array<std::uint8_t,9> pending_appearance{};
  const auto requested_profile=native_selected_player_configuration("Pending Round",pending_appearance,requested_team);
  const auto pending_slot=match.join("Pending Round",requested_team,requested_profile);
  const auto pending_player=match.player(pending_slot).source_datum;
  need(match.player(pending_slot).source_configuration && (*match.player(pending_slot).source_configuration)[0x7c]==0xff &&
      match.player(pending_slot).source_membership==-1 && match.player(pending_slot).source_profile_request==requested_profile,
      "Defusal late join did not retain its requested input separately from accepted native NONE membership");
  const DatumPool raw_players(objects.memory(),objects.player_pool());
  for(unsigned tick=0;tick<120;++tick) {
    match.step({});const Bytes raw(std::as_bytes(raw_players.lookup(pending_player)));
    const auto round=match.state().defusal;
    need(raw.u32(0x2c)==null_datum && (raw.u16(0xc0)&255u)==0xffu &&
        match.player(pending_slot).source_current_object==null_datum && match.player(pending_slot).deaths==0 &&
        std::ranges::find(round->roster,pending_player)==round->roster.end(),
        "Pending Defusal participant acquired a body/current-round role/death before its custom reset");
  }
  std::uint32_t definition=null_datum;
  for(const auto &[datum,weapon]:prepared.content.weapons)if(weapon.pickup_priority==1){definition=datum;break;}
  std::vector<std::uint32_t> bombs;
  for(const auto &object:objects.live_objects())if(object.definition==definition)bombs.push_back(object.object);
  need(bombs.size()==1,"Defusal spawned a parallel stock Assault objective or omitted its genuine bomb body");
  const auto bomb=bombs.front();auto carrier=state.defusal->carrier;
  need(carrier!=null_datum,"Defusal has no actual custom bomb carrier");
  auto carrier_slot=carrier&0xffffu;auto unit=match.player(carrier_slot).source_current_object;
  need(objects.object_parent(bomb)==unit,"Defusal carrier did not own a genuine native hierarchy child");
  const auto before=objects.object_transform(bomb).position-objects.biped_motion(unit).position;
  const auto moved=objects.biped_motion(unit).position+Vec3{.2f,0,0};
  world.set_object_position(unit,moved);match.step({});match.step({});
  const auto after=objects.object_transform(bomb).position-objects.biped_motion(unit).position;
  need(length(after-before)<.02f,"Native bomb hierarchy did not follow the genuine moving carrier");
  bool dropped{};
  if(prepared.local_slots.size()==4) {
    // Move the other real attackers to the defender's captured canonical
    // start, so the death drop has a separately observable free-body phase.
    for(const auto slot:prepared.local_slots)if(slot!=carrier_slot && match.player(slot).source_membership==state.defusal->attacking_team)
      world.set_object_position(match.player(slot).source_current_object,canonical.front());
    fatal(prepared,carrier_slot);match.step({});match.step({});
    need(source_game_variant_get(mechanical_settings(world),16)==1,
        "Defusal native mechanics lost its actual one-life input");
    const auto dead_slot=carrier_slot;
    need(match.state().defusal->phase==BombDefusalPhase::live && match.state().defusal->carrier==null_datum &&
        objects.object_parent(bomb)==null_datum,"Native bomb did not detach into an actual dropped body");
    dropped=true;const auto position=objects.object_transform(bomb).position;
    const auto replacement=std::ranges::find_if(prepared.local_slots,[&](auto slot){return slot!=carrier_slot &&
        match.player(slot).source_membership==state.defusal->attacking_team && match.player(slot).source_current_object!=null_datum;});
    need(replacement!=prepared.local_slots.end(),"Defusal drop omitted its surviving attacker");
    world.set_object_position(match.player(*replacement).source_current_object,position);
    until(match,90,[&]{return match.state().defusal->carrier!=null_datum;},"Native dropped bomb was not recovered by a genuine attacker");
    carrier=match.state().defusal->carrier;carrier_slot=carrier&0xffffu;unit=match.player(carrier_slot).source_current_object;
    need(objects.object_parent(bomb)==unit,"Recovered bomb did not enter the replacement native hierarchy");
    for(unsigned tick=0;tick<120;++tick) {
      match.step({});
      need(match.player(dead_slot).source_current_object==null_datum,
          "Original one-life Defusal player respawned before its custom round reset");
    }
  }
  const auto site=match.state().defusal->sites.front().position;
  world.set_object_position(unit,site);
  match.step({}); // Adopt the completed native position input before testing its motion.
  until(match,120,[&]{const auto &body=match.unit(carrier_slot);return body.body.grounded &&
      std::hypot(body.body.velocity.x,body.body.velocity.y)<.05f;},"Native bomb carrier did not settle on the authored plant site");
  std::array<Controls,multiplayer_capacity> controls{};controls[carrier_slot].pickup=true;
  const auto plant_ticks=match.state().defusal_rules->plant_ticks;
  for(unsigned tick=0;tick<plant_ticks+30 && match.state().defusal->phase==BombDefusalPhase::live;++tick)match.step(controls);
  need(match.state().defusal->phase==BombDefusalPhase::planted && objects.object_parent(bomb)==null_datum,
      "Planting did not detach/place the genuine native bomb body");
  const auto defender=std::ranges::find_if(prepared.local_slots,[&](auto slot){return
      match.player(slot).source_membership!=state.defusal->attacking_team && match.player(slot).source_current_object!=null_datum;});
  need(defender!=prepared.local_slots.end(),"Defusal omitted a real defending participant");
  const auto defending_team=unsigned(match.player(*defender).source_membership);
  world.set_object_position(match.player(*defender).source_current_object,objects.object_transform(bomb).position);
  match.step({}); // The previous mirrored grounded state belongs to the old position.
  until(match,120,[&]{const auto &body=match.unit(*defender);return body.body.grounded &&
      std::hypot(body.body.velocity.x,body.body.velocity.y)<.05f;},"Native defender did not settle beside the planted source body");
  const auto scores=match.state().defusal->scores;controls={};controls[*defender].pickup=true;
  const auto defuse_ticks=match.state().defusal_rules->defuse_ticks;
  std::vector<std::uint32_t> old_units,old_items;
  for(const auto slot:prepared.local_slots) {
    old_units.push_back(match.unit(slot).source_datum);
    if(match.player(slot).source_current_object!=null_datum)
      for(const auto weapon:objects.player_equipment(match.player(slot).source_current_object).weapons)
        if(weapon!=null_datum)old_items.push_back(weapon);
  }
  for(unsigned tick=0;tick<defuse_ticks+30 && match.state().defusal->phase==BombDefusalPhase::planted;++tick)match.step(controls);
  need(match.state().defusal->phase==BombDefusalPhase::ended &&
      match.state().defusal->scores[defending_team]==scores[defending_team]+1,
      "Native body defuse did not retain the custom round score");
  const auto next_round=match.state().defusal->round+1;
  until(match,300,[&]{return match.state().defusal->round==next_round;},"Custom Defusal did not reset its next source round");
  const auto live=objects.live_objects();
  for(const auto previous:old_units)need(std::ranges::none_of(live,[&](const auto &object){return object.object==previous;}),
      "Defusal round reset retained an old native body generation");
  for(const auto previous:old_items)need(std::ranges::none_of(live,[&](const auto &object){return object.object==previous;}),
      "Defusal round reset retained an old native loadout generation");
  for(const auto slot:prepared.local_slots) {
    const auto current=match.player(slot).source_current_object;
    need(current!=null_datum && std::ranges::find(old_units,current)==old_units.end() &&
        objects.player_equipment(current).grenades==std::array<std::uint8_t,2>{1,0},
        "Defusal replacement body/loadout did not come from the genuine native factory");
  }
  const auto &joined=match.player(pending_slot);const auto pending_unit=joined.source_current_object;
  const auto joined_round=match.state().defusal;
  need(pending_unit!=null_datum && joined.source_membership==requested_team && joined.source_configuration &&
      std::bit_cast<std::int8_t>((*joined.source_configuration)[0x7c])==requested_team &&
      joined.source_profile_request==requested_profile && match.unit(pending_slot).source_datum==pending_unit &&
      objects.object_vitality(pending_unit).body>0 &&
      std::ranges::find(joined_round->roster,pending_player)!=joined_round->roster.end(),
      "Custom next round did not restore the accepted native team and construct its pending participant body");
  return {{"passed",true},{"source_bomb",bomb},{"native_hierarchy_follow",true},{"drop_recovery",dropped},
      {"planted_detached_body",true},{"defused",true},{"retired_units",old_units},{"retired_items",old_items},
      {"pending_peer_player",pending_player},{"pending_native_none_frames",120},
      {"pending_next_round_unit",pending_unit},{"pending_requested_team_preserved",true},
      {"custom_round",next_round},{"custom_round_score",match.state().defusal->scores[defending_team]}};
}
nlohmann::json exercise(const FrontendContent &content,unsigned preset,unsigned players,bool lan,
    const std::filesystem::path &maps,unsigned ordinal) {
  const auto launch=start(content,preset,players,lan);
  MultiplayerOptions options;options.map=maps/launch.map;options.shared_maps=maps;
  options.variant=launch.variant;options.local_players=players;options.host_port=launch.host_port;
  options.selected_profiles=launch.selected_profiles;options.selected_teams=launch.selected_teams;
  options.input_instances=launch.input_instances;options.mute=true;options.source_object_world=true;
  options.source_object_savegame_directory=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/std::to_string(ordinal);
  select_multiplayer_mode_authority(options);
  MultiplayerPreparedWorld prepared(options);
  need(prepared.source_objects && prepared.source_objects->owns_physics(),
      "Custom menu Start bypassed the canonical source physical World");
  auto &match=prepared.match;auto &world=*prepared.source_objects;
  need(!match.source_engine_owned() && match.session_configuration() &&
      match.session_configuration()->values==launch.variant.values,
      "Custom rules must retain score authority and exact settings independently of physical ownership");
  need(prepared.local_slots.size()==players,"Custom physical World lost local players");
  const auto mechanical_type=source_game_variant_type(mechanical_settings(world.tu15_world()));
  need(mechanical_type==2 && match.source_engine()==(bomb_defusal_variant(launch.variant)?9:2),
      "Custom advertised engine identity replaced its separate native mechanical owner");
  unsigned mechanical_scene_rows{};
  for(const auto &object:match.simulation().scene().objects())if(object.handle!=null_datum) {
    need(source_scenario_factory_game_allowed(prepared.content.objects.at(object.placement),2,mechanical_type),
        "Scene membership used the advertised custom engine instead of its actual native factory context");
    ++mechanical_scene_rows;
  }
  std::vector<Vec3> canonical;
  nlohmann::json starting_grenades=nlohmann::json::array();
  for(unsigned i=0;i<players;++i) {
    const auto slot=prepared.local_slots[i];const auto &p=match.player(slot);
    need(p.source_current_object!=null_datum && !world.object_body(p.source_current_object).is_null(),
        "Custom participant has no full source body allocation");
    need(match.unit(slot).source_datum==p.source_current_object,"Custom source unit identity differs");
    canonical.push_back(world.biped_motion(p.source_current_object).position);
    const auto equipment=world.player_equipment(p.source_current_object);
    need(equipment.weapons[0]!=null_datum && equipment.definitions[0]==match.unit(slot).weapon.definition,
        "Custom weapon inventory did not come from its actual source objects");
    if(bomb_defusal_variant(launch.variant)) {
      const std::array<std::uint8_t,2> expected{std::uint8_t(prepared.rules.starting_grenades?1:0),0};
      need(equipment.grenades==expected,"Defusal native birth profile lost its declared starting grenade budget");
      starting_grenades.push_back({{"slot",slot},{"unit",p.source_current_object},{"grenades",equipment.grenades}});
    }
  }
  for(unsigned tick=0;tick<(bomb_defusal_variant(launch.variant)?161u:3u);++tick)match.step({});
  unsigned zombies{},same_faction_pairs{},enemy_pairs{};
  if(custom_slayer(launch.variant)==CustomSlayer::infection) {
    const DatumPool raw_players(world.memory(),world.player_pool());
    const auto choices=prepared.cache->multiplayer_weapon_choices();
    std::uint32_t seed{};
    const auto sword=source_starting_weapon(null_datum,17,null_datum,choices,seed);
    for(const auto slot:prepared.local_slots) {
      const auto &p=match.player(slot);const bool zombie=(*p.source_configuration)[0x40]==6;
      zombies+=zombie;
      need(p.source_membership==(zombie?1:0),"Custom faction differs from its actual native membership");
      const Bytes raw(std::as_bytes(raw_players.lookup(p.source_datum)));
      need((raw.u16(0xc0)&255u)==unsigned(p.source_membership) &&
          (raw.u16(0x84)&255u)==unsigned(zombie?6:2),"Native player owner lost its faction/color input");
      const auto equipment=world.player_equipment(p.source_current_object);
      if(zombie)need(match.unit(slot).weapon.definition==sword && equipment.grenades==std::array<std::uint8_t,2>{0,0} &&
          world.object_vitality(p.source_current_object).shield==0.f,"Zombie traits did not reach native inventory/vitality");
      for(const auto other:prepared.local_slots)if(slot!=other) {
        const auto &victim=match.player(other);const auto scale=damage_policy(world.tu15_world(),p.source_datum,victim.source_datum);
        if(p.source_membership==victim.source_membership) {need(scale==0.f,"Native damage permits same-faction infection damage");++same_faction_pairs;}
        else {need(std::isfinite(scale) && scale>0.f,"Native damage rejected opposite infection factions");++enemy_pairs;}
      }
    }
    need(zombies==1 && enemy_pairs>0 && (players!=4 || same_faction_pairs>0),"Native infection policy omitted an actual faction control");
  }
  if(bomb_defusal_variant(launch.variant))for(const auto slot:prepared.local_slots) {
    const auto grenades=world.player_equipment(match.player(slot).source_current_object).grenades;
    const auto &memory=world.tu15_world().memory();const auto globals=Bytes(memory.view(0x4ecc1cu,4)).u32(0);
    const Bytes grenade_table(memory.view(globals+0x100u,8));
    need(grenade_table.u32(0)>=2,"Native grenade budget requires its actual authored type table");
    bool bounded=true;
    for(unsigned type=0;type<2;++type)
      bounded=bounded && grenades[type]<=Bytes(memory.view(grenade_table.u32(4)+type*0x2cu,2)).u16(0);
    // Starting counts are asserted at birth above. Native CF080 pickup
    // legitimately increments them later at CF134; preserve that mechanic.
    need(bounded && match.unit(slot).grenades==grenades,
        "Defusal native grenade pickup lost its authored capacity or completed mirror: "+nlohmann::json{
          {"slot",slot},{"actual_grenades",grenades},{"declared_starting_grenades",prepared.rules.starting_grenades},
          {"advertised_starting_grenades",source_game_variant_get(source_game_variant(launch.variant),88)},
          {"native_starting_grenades",source_game_variant_get(mechanical_settings(world.tu15_world()),88)},
          {"native_life",native_life_probe(prepared,slot)}}.dump());
  }
  const auto slot=prepared.local_slots.front(),unit=match.player(slot).source_current_object;
  need(unit!=null_datum,"Custom participant died before contact setup");
  const auto activity_before=world.tu15_world().native_physics_frame_activity();
  const auto before=scalars(activity_before);
  const auto entrance=intake(*prepared.cache,prepared.content);
  world.tu15_world().set_object_position(unit,entrance);
  float peak=entrance.z;
  for(unsigned tick=0;tick<60;++tick) {
    match.step({});need(match.player(slot).source_current_object==unit,"Custom lift contact changed full unit generation");
    const auto motion=world.biped_motion(unit);
    need(std::isfinite(length(motion.velocity)) && length(match.unit(slot).body.position-motion.position)<.001f,
        "Custom body readback differs from completed native physics");
    peak=std::max(peak,motion.position.z);
  }
  const auto activity_after=world.tu15_world().native_physics_frame_activity();
  const auto calls=scalars(activity_after)-before;
  const auto velocity=std::accumulate(activity_after.native_lift_velocity_completed.begin(),
      activity_after.native_lift_velocity_completed.end(),std::uint64_t{})-
      std::accumulate(activity_before.native_lift_velocity_completed.begin(),
      activity_before.native_lift_velocity_completed.end(),std::uint64_t{});
  need(calls>0 && velocity>0 && activity_after.native_lift_velocity_refusals==activity_before.native_lift_velocity_refusals &&
      peak>entrance.z+.25f,"Custom rules bypassed native authored lift velocity/travel");
  world.tu15_world().set_object_position(unit,canonical.front());
  for(unsigned tick=0;tick<30;++tick)match.step({});
  need(match.player(slot).source_current_object==unit,"Custom canonical leave changed the source unit generation");
  const auto rows=native_rows(prepared,!lan && players==2);
  const auto life=lan?nlohmann::json{{"scope","Physical/menu/LAN case; deep lifecycle is covered by both local roster sizes"}}:
      lifecycle(prepared,canonical);
  return {{"passed",true},{"mode",launch.variant.name},{"players",players},{"lan",lan},
      {"source_unit",unit},{"native_lift_scalar_calls",calls},{"rise",peak-entrance.z},
      {"native_lift_velocity_completed",velocity},
      {"actual_mechanical_engine",mechanical_type},{"advertised_rule_engine",match.source_engine()},
      {"native_factory_admitted_scene_rows",mechanical_scene_rows},
      {"defusal_native_starting_grenades",starting_grenades},
      {"same_faction_native_policy_pairs",same_faction_pairs},{"opposite_faction_native_policy_pairs",enemy_pairs},
      {"native_netgame_rows",rows},
      {"lifecycle",life},
      {"custom_scoring_authority",!match.source_engine_owned()},{"source_physical_world",true}};
}
}
int main(int argc,char **argv)try {
  need(argc==3 || (argc==4 && (std::string_view(argv[3])=="zero-delay" || std::string_view(argv[3])=="defusal-loadout" ||
      std::string_view(argv[3])=="infection-pickups" || std::string_view(argv[3])=="custom-camouflage")),
      "Supply MCC_MAPS REPORT [zero-delay|defusal-loadout|infection-pickups|custom-camouflage]");const std::filesystem::path maps=argv[1];
  if(!std::filesystem::is_regular_file(maps/"mainmenu.map"))return 77;
  need(std::getenv("HALO2_TEST_SCRATCH"),"Custom mechanics requires tools/run_test.py owned scratch");
  Cache cache(maps/"mainmenu.map");need(cache.is_mcc(),"Custom mechanics requires genuine MCC maps");
  const auto content=cache.load_frontend();nlohmann::json report={{"passed",false},{"cases",nlohmann::json::array()}};
  if(argc==4) {
    if(std::string_view(argv[3])=="custom-camouflage") {
      const auto fiesta=std::ranges::find_if(content.bundled_variants,[](const auto &variant){return custom_slayer(variant)==CustomSlayer::fiesta;});
      need(fiesta!=content.bundled_variants.end(),"Global camouflage control lacks its actual bundled Fiesta definition");
      report["diagnostic_only"]=true;report["case"]="custom-camouflage";
      report["custom_camouflage_controls"]=custom_camouflage(content,unsigned(fiesta-content.bundled_variants.begin()),maps);
      report["passed"]=report["custom_camouflage_controls"]["passed"];
      std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return report["passed"].get<bool>()?0:1;
    }
    if(std::string_view(argv[3])=="defusal-loadout") {
      const auto defusal=std::ranges::find_if(content.bundled_variants,bomb_defusal_variant);
      need(defusal!=content.bundled_variants.end(),"Defusal diagnostic lacks the real bundled profile");
      report["diagnostic_only"]=true;report["case"]="defusal-loadout2";
      try {report["case_result"]=exercise(content,unsigned(defusal-content.bundled_variants.begin()),2,false,maps,0);report["passed"]=true;}
      catch(const std::exception &error) {report["error"]=error.what();}
      std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return report["passed"].get<bool>()?0:1;
    }
    const auto infection=std::ranges::find_if(content.bundled_variants,[](const auto &variant){
      return custom_slayer(variant)==CustomSlayer::infection;
    });
    need(infection!=content.bundled_variants.end(),"Zero-delay diagnostic lacks the real bundled Infection profile");
    if(std::string_view(argv[3])=="infection-pickups") {
      report["diagnostic_only"]=true;report["case"]="infection-pickups";
      report["infection_pickup_controls"]=infection_pickups(content,unsigned(infection-content.bundled_variants.begin()),maps);
      report["passed"]=report["infection_pickup_controls"]["passed"];
      std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return report["passed"].get<bool>()?0:1;
    }
    report["diagnostic_only"]=true;report["case"]="zero-delay-infection4";
    try {report["zero_delay_infection_control"]=zero_delay_infection(content,unsigned(infection-content.bundled_variants.begin()),maps);report["passed"]=true;}
    catch(const std::exception &error) {report["error"]=error.what();}
    std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return report["passed"].get<bool>()?0:1;
  }
  unsigned type_controls{};
  for(const auto &[name,campaign]:std::array<std::pair<const char*,bool>,3>{{
      {"mainmenu.map",false},{"mainmenu.map",true},{"lockout.map",true}}}) {
    MultiplayerOptions invalid;invalid.map=maps/name;invalid.shared_maps=maps;invalid.mute=true;
    if(campaign)invalid.campaign_difficulty=0;
    bool refused{};
    try {MultiplayerPreparedWorld wrong_kind(invalid);}
    catch(const FormatError &error) {
      refused=std::string_view(error.what())=="MCC gameplay session kind differs from its authored scenario type";
    }
    need(refused,"MCC API accepted a caller role in place of its actual scenario type");++type_controls;
  }
  report["authored_scenario_type_negative_controls"]=type_controls;
  const auto reference=maps.parent_path().parent_path().parent_path().parent_path()/"disc/default.xbe";
  if(std::filesystem::is_regular_file(reference)) {
    MultiplayerOptions invalid;invalid.map=maps/"lockout.map";invalid.shared_maps=maps;invalid.gameplay_xbe=reference;invalid.mute=true;
    bool refused{},world_born{};invalid.source_world_observer=[&](const SourceRuntimeMemory &){world_born=true;};
    try {MultiplayerPreparedWorld wrong_source(invalid);}
    catch(const FormatError &error) {
      refused=std::string_view(error.what())=="MCC gameplay requires its genuine TU1.5 source owner";
    }
    need(refused && !world_born,"MCC API admitted a non-TU1.5 source fallback");
    report["wrong_source_image_negative_control"]=true;
  } else report["wrong_source_image_negative_control"]="Reference Base image not present; no game source substituted";
  unsigned ordinal{},failures{};
  try {
    MultiplayerOptions replay;replay.map=maps/"lockout.map";replay.shared_maps=maps;
    replay.local_players=1;replay.replay=true;replay.mute=true;
    replay.source_object_savegame_directory=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/"stock-replay";
    // Replay supplies bot controls. It must select the same genuine native
    // authority without requiring an extra caller source-world flag.
    need(!replay.source_object_world,"Replay route control accidentally forces native authority");
    MultiplayerPreparedWorld prepared(replay);
    need(prepared.source_objects && prepared.source_objects->owns_physics() && prepared.match.source_engine_owned(),
        "MCC bot replay selected a host mechanical fallback");
    std::array<Controls,multiplayer_capacity> input{};input[0].forward=.5f;
    for(unsigned tick=0;tick<3;++tick)prepared.match.step(input);
    const auto unit=prepared.match.player(prepared.local_slots[0]).source_current_object;
    need(unit!=null_datum && unit>>16 && !prepared.source_objects->object_body(unit).is_null(),
        "MCC bot replay did not retain its actual native player generation/body");
    report["mcc_bot_replay_native_route"]={{"passed",true},{"source_world_flag",false},
        {"native_engine_authority",true},{"native_player_unit",unit},{"native_frames",3}};
  } catch(const std::exception &error) {
    ++failures;report["mcc_bot_replay_native_route"]={{"passed",false},{"error",error.what()}};
  }
  for(unsigned preset=0;preset<content.bundled_variants.size();++preset) {
    const auto &variant=content.bundled_variants[preset];
    if(custom_slayer(variant)==CustomSlayer::none && !bomb_defusal_variant(variant))continue;
    for(bool lan:{false,true})for(unsigned players:{2u,4u}) {
      try {report["cases"].push_back(exercise(content,preset,players,lan,maps,ordinal++));}
      catch(const std::exception &e) {
        ++failures;report["cases"].push_back({{"passed",false},{"mode",variant.name},{"players",players},{"lan",lan},{"error",e.what()}});
      }
      std::ofstream(argv[2])<<report.dump(2)<<'\n';
    }
  }
  need(report["cases"].size()==12,"Custom mechanics did not exercise all actual menu Starts");
  const auto fiesta=std::ranges::find_if(content.bundled_variants,[](const auto &variant){
    return custom_slayer(variant)==CustomSlayer::fiesta;
  });
  need(fiesta!=content.bundled_variants.end(),"Custom score boundary lacks its genuine bundled Fiesta profile");
  try {report["score_boundary_control"]=score_boundary(content,unsigned(fiesta-content.bundled_variants.begin()),maps);}
  catch(const std::exception &error) {++failures;report["score_boundary_control"]={{"passed",false},{"error",error.what()}};}
  const auto infection=std::ranges::find_if(content.bundled_variants,[](const auto &variant){
    return custom_slayer(variant)==CustomSlayer::infection;
  });
  need(infection!=content.bundled_variants.end(),"Zero-delay source control lacks the real bundled Infection profile");
  try {report["zero_delay_infection_control"]=zero_delay_infection(content,unsigned(infection-content.bundled_variants.begin()),maps);}
  catch(const std::exception &error) {++failures;report["zero_delay_infection_control"]={{"passed",false},{"error",error.what()}};}
  report["infection_pickup_controls"]=infection_pickups(content,unsigned(infection-content.bundled_variants.begin()),maps);
  if(!report["infection_pickup_controls"]["passed"].get<bool>())++failures;
  report["custom_camouflage_controls"]=custom_camouflage(content,unsigned(fiesta-content.bundled_variants.begin()),maps);
  if(!report["custom_camouflage_controls"]["passed"].get<bool>())++failures;
  report["passed"]=failures==0;std::ofstream(argv[2])<<report.dump(2)<<'\n';
  std::cout<<report.dump()<<'\n';return failures?1:0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
