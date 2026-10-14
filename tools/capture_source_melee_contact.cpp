#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/math.hpp"
#include "halo2/source_decal_vertex_cache.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <set>
using namespace halo2;
namespace {void check(bool ok,const char *why){if(!ok)throw FormatError(why);}}
// Declared native placement beside a real map wall. The source world owns
// action admission, posed collision, damage, contact and animation frames.
// No fabricated geometry, source state, hit result or lower return is supplied.
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;
  check(argc==5 || argc==6,"Supply MAP SHARED TU15 OUTPUT_DIRECTORY [sword|primary|grenade|plasma|target|sword-target|primary-target|lunge|fire-N]");
  const auto mode=argc==6?std::string_view(argv[5]):std::string_view("melee");
  const bool firing=mode.starts_with("fire-");
  const bool lunge=mode=="lunge";
  const bool target=mode=="target" || mode=="sword-target" || mode=="primary-target" || lunge;
  const bool strict_decal_cache=mode=="plasma-cache";
  const bool plasma=mode=="plasma" || strict_decal_cache;
  const bool grenade=mode=="grenade" || plasma,special=mode=="primary" || mode=="special" || mode=="primary-target" || lunge;
  const bool sword=mode=="sword" || mode=="sword-target" || special || plasma;
  check(grenade || sword || target || mode=="melee" || firing,"Invalid source combat acceptance mode");
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
  options.source_object_savegame_directory=std::filesystem::absolute(argv[4])/"source-savegames";
  Cache cache(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe));
  const auto content=cache.load_content(0,true);CollisionWorld geometry(content.collision);
  auto spawn=cache.spawns().front();const auto original=spawn.position;float nearest=1e9f;bool found=grenade || target || firing;
  if(lunge) {
    found=false;
    for(const auto &candidate:cache.spawns()) {
      for(unsigned direction_=0;direction_<4 && !found;++direction_) {
        const float yaw=float(direction_)*1.5707963267948966f;
        const Vec3 d{std::cos(yaw),std::sin(yaw),0};
        const auto eye=candidate.position+Vec3{0,0,content.player.camera_height};
        if(geometry.raycast(eye,d,3.f) || geometry.raycast(eye,d*-1.f,3.f))continue;
        bool floor=true;
        for(const auto distance:{-2.5f,-1.25f,0.f,1.25f,2.5f}) {
          const auto hit=geometry.raycast(candidate.position+d*distance+Vec3{0,0,.5f},{0,0,-1},1.f);
          floor&=hit && hit->normal.z>.9f;
        }
        if(floor){spawn=candidate;spawn.facing=yaw;found=true;}
      }
      if(found)break;
    }
    check(found,"Lunge fixture has no clear authored start and real floor corridor");
  }
  if(!grenade && !target && !firing)for(const auto &triangle:content.collision) {
    const auto n=normalized(triangle.source_normal);if(std::abs(n.z)>.1f)continue;
    const auto midpoint=(triangle.a+triangle.b+triangle.c)/3.f;
    const auto side=normalized(Vec3{n.x,n.y,0});const auto outside=midpoint+side*.3f;
    const auto floor=geometry.raycast(outside+Vec3{0,0,1},{0,0,-1},3.f);
    if(!floor || floor->normal.z<.9f)continue;
    const auto feet=floor->position+Vec3{0,0,.002f},eye=feet+Vec3{0,0,content.player.camera_height};
    const auto hit=geometry.raycast(eye,side*-1.f,.35f);if(!hit || hit->distance<.2f)continue;
    const float distance=length(feet-original);if(distance>=nearest)continue;
    nearest=distance;spawn.position=feet;spawn.facing=std::atan2(-side.y,-side.x);found=true;
  }
  check(found,"No admitted original wall/floor placement for melee acceptance");
  options.native_spawn_inputs={spawn};options.defer_initial_player_spawn=true;options.starting_weapon=sword?17:5;
  if(firing)options.starting_weapon=std::stoul(std::string(mode.substr(5)));
  if(target) {
    auto victim=spawn;victim.position=victim.position+Vec3{std::cos(spawn.facing),std::sin(spawn.facing),0}*(lunge?2.5f:.65f);
    const auto floor=geometry.raycast(victim.position+Vec3{0,0,.5f},{0,0,-1},1.f);
    check(bool(floor) && floor->normal.z>.9f,"Target placement has no real map floor");
    victim.position=floor->position+Vec3{0,0,.002f};victim.facing+=3.14159265358979323846f;
    options.native_spawn_inputs.push_back(victim);options.local_players=2;
  }
  if(plasma)options.variant.values.set(83,17); // Authored Sword weapon-set rule owns plasma conversion.
  options.session_random_seed=0x12345678;
  std::vector<std::uint32_t> players;
  options.player_birth_owner=[&](MultiplayerPlayer &p,unsigned){players.push_back(p.source_datum);p.source_flags=8;};
  options.pre_player_update_owner=[&](MultiplayerMatch &match) {
    for(const auto player:players) {
      const auto *p=match.player_from_datum(player);check(p,"Source melee player disappeared");
      if(!(p->source_flags&1u))match.apply_source_player_activation(player,p->source_flags,p->source_flags|1u);
    }
  };
  MultiplayerPreparedWorld world(options);auto &match=world.match;
  unsigned firing_requests{},allocated_effects{},source_colors{};
  if(firing)match.configure_weapon_effect_owner([&](const Unit&,const WeaponEffectInput &input) {
    ++firing_requests;
    const auto effect=world.source_objects->request_weapon_effect(input);
    if(effect.instance!=null_datum)++allocated_effects;
    source_colors+=effect.colors_observed;
    std::cout<<"Source weapon effect="<<std::hex<<input.effect<<" instance="<<effect.instance
      <<" colors="<<effect.colors.a<<','<<effect.colors.b<<std::dec<<'\n';
    return effect.instance;
  });
  std::array<std::uint8_t,2> initial_grenades{};
  unsigned admitted{},contacts{},detonations{};std::uint64_t serial{},detonation_serial{};std::set<std::uint64_t> poses;
  float victim_vitality=2.f;
  unsigned shield_effects{};std::uint64_t effect_serial{};
  bool special_activity{};float attack_distance{};
  Vec3 attack_origin{},idle_previous{};
  float idle_motion{},lunge_motion{},closest_distance=1e9f;
  std::array<Controls,16> input{};input[0].yaw=spawn.facing;
  std::set<std::uint32_t> source_glows;
  std::set<std::uint32_t> observed_decal_definitions;
  bool source_glow_faded=false;unsigned peak_source_decals{};
  for(unsigned tick=0;tick<(strict_decal_cache?240u:180u);++tick) {
    if(strict_decal_cache && tick==79) {
      const std::array<unsigned,1> views{0};
      match.configure_local_views(views,world.cache->load_first_person_binding(match.unit(0).weapon.definition,2));
    }
    if(strict_decal_cache && tick>=79)
      world.source_objects->publish_native_observer_position(0,match.unit(0).body.position+Vec3{0,0,content.player.camera_height});
    if(target && tick>1) {
      const auto delta=match.unit(1).body.position-match.unit(0).body.position;
      input[0].yaw=std::atan2(delta.y,delta.x);input[1].yaw=input[0].yaw+3.14159265358979323846f;
    }
    // The original spawn chooser can choose the same declared start twice.
    // Drive the actual victim through delivered movement to establish a
    // distant target; never overwrite a source body or action selection.
    input[1].forward=lunge && tick>=25 && tick<65?-1.f:0.f;
    input[0].melee=tick==80 && !special && !grenade && !firing;input[0].fire=tick==80 && special;
    input[0].fire|=firing && tick>=80 && tick<92;
    input[0].grenade=tick==80 && grenade;input[0].pitch=plasma?-1.2f:grenade?.3f:0.f;match.step(input);
    const auto state=match.state();const auto &unit=match.unit(0);
    if(plasma) {
      const auto decals=source_decal_vertex_cache_tu15(world.source_objects->memory());
      peak_source_decals=std::max(peak_source_decals,unsigned(decals.size()));
      for(const auto &decal:decals)if(observed_decal_definitions.insert(decal.definition).second)
        std::cout<<"Source cached decal definition="<<std::hex<<decal.definition<<std::dec
                 <<" name="<<world.cache->query_tag({.handle=TagHandle{decal.definition}})->name
                 <<" layer="<<unsigned(decal.layer)<<" lifetime="<<decal.lifetime
                 <<" decay="<<decal.decay<<" birth="<<decal.tick<<" observed="<<state.tick
                 <<" alpha="<<(decal.tint>>24)<<" quads="<<decal.quads.size()<<'\n';
      for(const auto &decal:decals)if(decal.layer==5 && decal.lifetime>0) {
        source_glows.insert(decal.datum);
        const auto alpha=decal.tint>>24;
        source_glow_faded|=alpha>0 && alpha<255;
      }
    }
    if(lunge && tick>=65 && tick<=79) {
      if(tick>65)idle_motion=std::max(idle_motion,length(unit.body.position-idle_previous));
      idle_previous=unit.body.position;
    }
    if(tick==1) {
      initial_grenades=unit.grenades;
      if(plasma)check(initial_grenades[1]>0,"Original Sword starting equipment did not supply plasma grenades");
    }
    if(lunge && tick%20==0) {
      const auto p=match.unit(1).body.position;
      std::cout<<"Source moving target tick="<<tick<<" position="<<p.x<<','<<p.y<<','<<p.z<<'\n';
    }
    if(target && tick==79) {
      attack_distance=length(match.unit(1).body.position-unit.body.position);
      attack_origin=unit.body.position;
      std::cout<<"Source preattack target distance="<<attack_distance<<'\n';
      const auto body=world.source_objects->object_body(unit.source_datum);
      const Bytes targeting(world.source_objects->memory().view(body+0x1c8u,0x24));
      std::cout<<"Source aim target="<<std::hex<<targeting.u32(0)<<std::dec
        <<" confidence="<<targeting.f32(0x1c)<<'\n';
    }
    if(lunge && tick>80) {
      lunge_motion=std::max(lunge_motion,length(unit.body.position-attack_origin));
      closest_distance=std::min(closest_distance,length(match.unit(1).body.position-unit.body.position));
    }
    for(const auto &event:state.unit_damage_effects)if(event.serial>effect_serial) {
      effect_serial=event.serial;
      if(target && event.target==match.unit(1).source_datum) {
        ++shield_effects;
        std::cout<<"Source shield effect tick="<<event.tick<<" definition="<<std::hex<<event.effect.datum<<std::dec<<'\n';
      }
    }
    if(target && tick>80)victim_vitality=std::min(victim_vitality,match.unit(1).vitality.body+match.unit(1).vitality.shield);
    if(tick==80)check((firing || bool(content.weapons.at(unit.weapon.definition).inventory_flags&(1u<<24))==sword) &&
        world.source_objects->object_definition(unit.weapon.source_datum)==unit.weapon.definition,
        "Original initial equipment did not select the requested weapon kind");
    if(unit.melee_action.active()) {
      const auto activity=unit.melee_action.activity;
      special_activity|=activity==0x0a0005bbu || activity==0x130005bcu || activity==0x0b0005b2u || activity==0x140005b3u;
      ++admitted;const auto nodes=world.source_objects->world_nodes(unit.source_datum);
      check(!nodes.empty(),"Actual melee body lost its WORLD nodes");std::uint64_t hash=1469598103934665603ull;
      const auto root=inverse_transform(nodes.front());
      for(const auto &node:nodes) {
        const auto relative=compose_transform(root,node);
        for(const auto &axis:relative.basis)for(float value:{axis.x,axis.y,axis.z}) {
          check(std::isfinite(value),"Source melee pose contains invalid matrices");
          hash=(hash^std::bit_cast<std::uint32_t>(value))*1099511628211ull;
        }
      }
      poses.insert(hash);
      std::cout<<"Source melee tick="<<state.tick<<" activity="<<std::hex<<unit.melee_action.activity<<std::dec
          <<" progress="<<int(unit.melee_action.progress)<<" impact="<<int(unit.melee_action.impact)<<'\n';
    }
    for(const auto &impact:state.melee_impacts)if(impact.serial>serial) {
      serial=impact.serial;++contacts;check(impact.material>=0,"Source melee lost actual material");
      std::cout<<"Source contact tick="<<impact.tick<<" material="<<impact.material<<'\n';
    }
    for(const auto &event:state.grenade_detonations)if(event.serial>detonation_serial) {
      detonation_serial=event.serial;++detonations;
      check(!event.source_locations.empty(),"Source explosion lost its completed named locations");
      std::cout<<"Source detonation tick="<<event.tick<<" locations="<<event.source_locations.size()<<'\n';
      std::cout<<"Source explosion controls="<<event.controls.a<<','<<event.controls.b<<'\n';
    }
  }
  if(firing) {
    check(firing_requests>0 && source_colors>0,"Prepared shot never reached its actual TU1.5 color owner");
    std::cout<<"PASS actual TU1.5 prepared-shot creation/colors; retained allocations="<<allocated_effects<<'\n';return 0;
  }
  if(grenade) {
    if(strict_decal_cache) {
      const auto remaining=source_decal_vertex_cache_tu15(world.source_objects->memory());
      check(!source_glows.empty() && source_glow_faded && peak_source_decals>0,
          "Actual plasma source-cache glow never created/faded");
      check(std::none_of(remaining.begin(),remaining.end(),[&](const auto &decal){return source_glows.contains(decal.datum);}),
          "Actual plasma source-cache glow outlived its original lifetime");
      std::cout<<"Source cached decal peak="<<peak_source_decals<<" glow identities="<<source_glows.size()
               <<"; actual packed alpha faded and original cache allocations retired\n";
    }
    const auto counts=match.unit(0).grenades;
    check(detonations==1 && int(counts[0])+counts[1]==int(initial_grenades[0])+initial_grenades[1]-1,
      "Live TU1.5 grenade detonation/count acceptance incomplete");
    LanHost host(match,0,0x545531354558504cull);
    LanClient peer("127.0.0.1:"+std::to_string(host.port()),0x545531354558504cull,1);
    for(unsigned n=0;n<64 && !peer.ready();++n) {peer.send(std::span(input).first(1));host.receive(input);host.broadcast();peer.receive();}
    check(peer.ready() && peer.error().empty(),"Source explosion LAN handshake incomplete");
    host.broadcast();peer.receive();
    const auto &actual=peer.state().grenade_detonations;const auto expected=match.state().grenade_detonations;
    check(actual.size()==expected.size() && !actual.empty(),"Source explosion LAN journal differs");
    const auto &a=actual.back(),&e=expected.back();
    check(a.colors.a==e.colors.a && a.colors.b==e.colors.b && a.controls.a==e.controls.a && a.controls.b==e.controls.b && a.source_locations.size()==e.source_locations.size(),
        "Source explosion LAN lost colors/locations");
    for(unsigned n=0;n<a.source_locations.size();++n) {
      const auto &x=a.source_locations[n],&y=e.source_locations[n];
      check(x.name==y.name && x.frame.scale==y.frame.scale && x.frame.position.x==y.frame.position.x &&
          x.frame.position.y==y.frame.position.y && x.frame.position.z==y.frame.position.z,"Source explosion LAN location differs");
      for(unsigned k=0;k<3;++k)check(x.frame.basis[k].x==y.frame.basis[k].x && x.frame.basis[k].y==y.frame.basis[k].y &&
          x.frame.basis[k].z==y.frame.basis[k].z,"Source explosion LAN basis differs");
    }
    const auto expected_state=match.state();
    for(const auto &player:expected_state.players)if(player.connected && player.source_weapon!=null_datum) {
      const auto pose=std::find_if(expected_state.source_body_poses.begin(),expected_state.source_body_poses.end(),
          [&](const auto &p){return p.object==player.source_weapon;});
      check(pose!=expected_state.source_body_poses.end(),"Source snapshot omitted the actual held weapon WORLD bank");
      const auto received=std::find_if(peer.state().source_body_poses.begin(),peer.state().source_body_poses.end(),
          [&](const auto &p){return p.object==player.source_weapon;});
      check(received!=peer.state().source_body_poses.end() && received->nodes.size()==pose->nodes.size(),
          "Source held weapon WORLD bank was lost across LAN");
      for(unsigned n=0;n<pose->nodes.size();++n) {
        const auto &x=pose->nodes[n],&y=received->nodes[n];
        check(x.scale==y.scale && x.position.x==y.position.x && x.position.y==y.position.y && x.position.z==y.position.z,
            "Source held weapon WORLD position changed across LAN");
        for(unsigned k=0;k<3;++k)check(x.basis[k].x==y.basis[k].x && x.basis[k].y==y.basis[k].y && x.basis[k].z==y.basis[k].z,
            "Source held weapon WORLD orientation changed across LAN");
      }
    }
    std::cout<<"PASS actual TU1.5 grenade detonation and source count debit\n";return 0;
  }
  if(target) {
    check(admitted>2 && poses.size()>2 && victim_vitality<2.f,"Live TU1.5 unit melee damage/pose acceptance incomplete");
    if(sword)check(shield_effects>0,"Live TU1.5 sword lost its source shield effect request");
    if(lunge) {
      std::cout<<"Source lunge movement="<<lunge_motion<<" idle_step="<<idle_motion
        <<" closest_target_distance="<<closest_distance<<'\n';
      check(attack_distance>1.5f && special_activity,"Live TU1.5 target admission did not select the original lunge activity");
      check(lunge_motion>idle_motion && closest_distance<attack_distance,
          "Source lunge activity did not produce real closing movement");
    }
    std::cout<<"PASS actual TU1.5 unit melee: target minimum vitality="<<victim_vitality<<'\n';return 0;
  }
  check(admitted>2 && poses.size()>2 && contacts==1,"Live TU1.5 melee action/pose/contact acceptance incomplete");
  std::cout<<"PASS actual TU1.5 melee: "<<admitted<<" active frames, "<<poses.size()<<" relative poses, "<<contacts<<" real map contact\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
