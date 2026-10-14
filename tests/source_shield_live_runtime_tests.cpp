#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/math.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/model.hpp"
#include <SDL.h>
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <iomanip>
#include <map>
#include <set>
#include <tuple>

using namespace halo2;
namespace {
void check(bool ok,const char *message) {if(!ok)throw FormatError(message);}
Vec3 eye(const Unit &unit) {return unit.body.position+Vec3{0,0,unit.definition.camera_height};}
void aim(Controls &control,const Unit &shooter,const Unit &target) {
  const auto delta=target.body.position+Vec3{0,0,target.body.height*.5f}-eye(shooter);
  control.yaw=std::atan2(delta.y,delta.x);
  control.pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));
}
using SurfaceVertex=std::tuple<std::uint32_t,std::uint32_t,std::array<std::uint32_t,3>>;
SurfaceVertex surface_vertex(const Vertex &v) {
  return {v.source_section,v.source_part,{std::bit_cast<std::uint32_t>(v.source_local_position.x),
      std::bit_cast<std::uint32_t>(v.source_local_position.y),std::bit_cast<std::uint32_t>(v.source_local_position.z)}};
}
// Classify the actual authored skin, including every descendant of l_hand /
// r_hand. The shield observer must receive forearms, palms and finger geometry;
// merely seeing a nonempty first-person draw would miss a hand-only omission.
std::map<SurfaceVertex,unsigned> first_person_surfaces(const Cache &cache,std::uint32_t datum) {
  const auto model=cache.load_model(datum,false);
  std::vector<unsigned> masks(model.bones.size());
  for(unsigned i=0;i<model.bones.size();++i) {
    const auto name=cache.string_id(model.bones[i].name);
    if(name=="l_upperarm" || name=="l_forearm")masks[i]|=1;
    if(name=="r_upperarm" || name=="r_forearm")masks[i]|=2;
    for(int node=int(i);node>=0;node=model.bones.at(node).parent) {
      const auto ancestor=cache.string_id(model.bones.at(node).name);
      if(ancestor=="l_hand")masks[i]|=4|(node!=int(i)?16:0);
      if(ancestor=="r_hand")masks[i]|=8|(node!=int(i)?32:0);
    }
  }
  check(model.skin.size()==model.mesh.vertices.size(),"First-person authored skin coverage missing");
  std::map<SurfaceVertex,unsigned> result;
  for(unsigned i=0;i<model.skin.size();++i) {
    const auto &skin=model.skin[i];unsigned mask=0;
    for(unsigned n=0;n<4;++n)if(skin.weights[n]>0)mask|=masks.at(skin.bones[n]);
    result[surface_vertex(model.mesh.vertices[i])]|=mask;
  }
  return result;
}
}
int main(int argc,char **argv)try {
  check(argc==4,"Supply imported assets directory, gameplay executable and output directory");
  if(!std::getenv("DISPLAY"))return 77;
  SDL_SetMainReady();
  const std::filesystem::path assets=argv[1],output=argv[3];
  std::filesystem::create_directories(output);
  MultiplayerOptions options;
  options.map=assets/"disc/maps/lockout.map";
  options.shared_maps=options.map.parent_path();options.gameplay_xbe=argv[2];
  options.width=1280;options.height=720;options.frames=1;
  options.local_players=2;options.starting_weapon=5;
  options.hidden=true;options.mute=true;options.validation=true;
  const std::array<std::array<std::uint8_t,9>,2> appearance{{
      {8,12,9,1,0,26,9,0,0},{3,4,3,4,1,5,31,0,0}}};
  for(unsigned i=0;i<2;++i)options.selected_profiles.push_back(
      native_selected_player_configuration(i?"Elite":"Spartan",appearance[i],-1));
  // Select two unchanged authored start records with a measured clear native
  // firing line. Placement is an explicit offline input before object birth;
  // no unit position, health, shield, readiness or feedback is ever assigned.
  std::array<std::uint32_t,2> hands_models{},body_models{};
  std::array<std::map<SurfaceVertex,unsigned>,2> fp_surfaces;
  std::array<bool,2> reported_fp_surfaces{};
  {
    Cache cache(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe));
    const auto content=cache.load_content(0,true);
    for(unsigned player=0;player<2;++player) {
      const auto &representation=content.player_representations.at(player?3:2);
      hands_models[player]=representation.hands;
      fp_surfaces[player]=first_person_surfaces(cache,representation.hands);
      body_models[player]=cache.load_initial_model_definition(representation.biped.datum).render_model;
    }
    CollisionWorld collision(content.collision);
    float closest=std::numeric_limits<float>::infinity();
    for(const auto &a:cache.spawns())for(const auto &b:cache.spawns()) {
      if(!slayer_spawn(a,0,true) || !slayer_spawn(b,0,true))continue;
      const auto delta=b.position-a.position;const auto distance=length(delta);
      if(distance<2 || distance>=closest || std::abs(delta.z)>.1f)continue;
      const auto start=a.position+Vec3{0,0,content.player_representations.at(2).biped.camera_height};
      const auto end=b.position+Vec3{0,0,content.player_representations.at(3).biped.height*.5f};
      const auto line=end-start;
      if(collision.raycast(start,normalized(line),length(line)))continue;
      closest=distance;options.native_spawn_inputs={a,b};
    }
    check(options.native_spawn_inputs.size()==2,"No authored native acceptance spawn pair has a clear firing line");
    for(const auto &spawn:options.native_spawn_inputs)
      std::cout<<"authored spawn "<<spawn.position.x<<' '<<spawn.position.y<<' '<<spawn.position.z<<'\n';
  }
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  auto &match=options.prepared_world->match;
  check(options.prepared_world->local_slots==std::vector<unsigned>{0,1},"Unexpected real local roster");
  std::array<Controls,multiplayer_capacity> controls{};
  const auto print_roster=[&](const char *stage) {
    const auto state=match.state();
    for(unsigned i=0;i<2;++i) {
      const auto &p=state.players[i];
      std::cout<<stage<<" tick="<<state.tick<<" player="<<i<<" dead="<<p.dead
          <<" position="<<p.position.x<<' '<<p.position.y<<' '<<p.position.z
          <<" shield="<<p.shield<<" loaded="<<p.loaded<<'\n';
    }
    std::cout.flush();
  };
  print_roster("born");
  // Original start scoring may legitimately select the same authored point.
  // Separate only through ordinary walking toward the other authored point.
  for(unsigned tick=0;tick<180 && length(match.unit(0).body.position-match.unit(1).body.position)<1.5f;++tick) {
    const auto position=match.unit(0).body.position;
    const auto &a=options.native_spawn_inputs[0].position;
    const auto &b=options.native_spawn_inputs[1].position;
    const auto goal=length(a-position)>length(b-position)?a:b;
    const auto delta=goal-position;
    controls={};controls[0].yaw=std::atan2(delta.y,delta.x);
    controls[0].forward=1;match.step(controls);
  }
  controls={};
  for(unsigned tick=0;tick<90;++tick) {
    aim(controls[0],match.unit(0),match.unit(1));
    aim(controls[1],match.unit(1),match.unit(0));match.step(controls);
  }
  print_roster("settled");
  check(length(match.unit(0).body.position-match.unit(1).body.position)>1,"Actual initial spawns overlap");
  std::ofstream report(output/"live-layers.tsv");
  report<<"stage\ttick\tplayer\thealth\tshield\trecent\tworld_vertices\tfp_vertices\n";
  const auto check_fp_surfaces=[&](unsigned owner,std::span<const Vertex> vertices) {
    std::array<unsigned,6> counts{};
    for(const auto &vertex:vertices) {
      const auto found=fp_surfaces.at(owner).find(surface_vertex(vertex));
      check(found!=fp_surfaces.at(owner).end(),"FP shield submitted geometry outside its authored arms/hands model");
      for(unsigned n=0;n<counts.size();++n)if(found->second&(1u<<n))++counts[n];
    }
    for(const auto count:counts)check(count>0,"FP shield omitted an arm, hand or finger skin branch");
    if(!reported_fp_surfaces[owner]) {
      std::cout<<"PASS "<<(owner?"Elite":"Spartan")<<" FP shield skin: left_arm="<<counts[0]
          <<" right_arm="<<counts[1]<<" left_hand="<<counts[2]<<" right_hand="<<counts[3]
          <<" left_fingers="<<counts[4]<<" right_fingers="<<counts[5]<<'\n';
      reported_fp_surfaces[owner]=true;
    }
  };
  const auto capture=[&](const char *name,std::optional<unsigned> damaged) {
    std::array<std::size_t,2> world{},fp{};
    std::array<std::set<unsigned>,2> materials;
    options.shield_layer_observer=[&](unsigned view,unsigned owner,bool first_person,
        std::uint32_t shader,std::uint32_t render_model,std::span<const Vertex> vertices) {
      check(owner<2 && view<2 && !vertices.empty(),"Invalid actual shield draw");
      check(damaged && owner==*damaged,"Unhurt player acquired another player's shield layer");
      const auto &definition=match.unit(owner).definition.vitality;
      check(shader==(first_person?definition.shield_damaged_first_person_shader:
          definition.shield_damaged_shader),"Actual layer selected a different owner's authored shader");
      check(render_model==(first_person?hands_models[owner]:body_models[owner]),
          "Shield layer escaped its owned arms/body model onto held-weapon geometry");
      if(first_person) {
        check(view==owner,"First-person shield is in another player's view");
        check_fp_surfaces(owner,vertices);
      }
      (first_person?fp[owner]:world[owner])+=vertices.size();
      for(const auto &vertex:vertices) {
        check(std::isfinite(vertex.position.x) && std::isfinite(vertex.position.y) &&
            std::isfinite(vertex.position.z),"Shield pose contains nonfinite geometry");
        materials[owner].insert(vertex.material);
      }
    };
    options.screenshot=output/(std::string(name)+".ppm");
    check(run_multiplayer(options)==0,"Actual native shield renderer failed");
    const auto state=match.state();
    for(unsigned i=0;i<2;++i) {
      const auto &p=state.players[i];
      report<<name<<'\t'<<state.tick<<'\t'<<i<<'\t'<<p.health<<'\t'<<p.shield<<'\t'
          <<p.damage_feedback.shield.visible<<'\t'<<world[i]<<'\t'<<fp[i]<<'\n';
      if(damaged && i==*damaged)check(world[i]>0 && fp[i]>0,"Hit player lacks its actual world or first-person shield layer");
      else check(world[i]==0 && fp[i]==0,"Cold/faded player retained a shield layer");
    }
    check(std::filesystem::file_size(options.screenshot)>1280u*720u*3u,"Native GPU shield capture missing");
    report.flush();
  };
  capture("cold",{});
  for(unsigned victim:{1u,0u}) {
    const auto shooter=1u-victim;
    const auto initial=match.state().players[victim].shield;
    for(unsigned tick=0;tick<180 && match.state().players[victim].shield==initial;++tick) {
      controls={};aim(controls[shooter],match.unit(shooter),match.unit(victim));
      aim(controls[victim],match.unit(victim),match.unit(shooter));
      controls[shooter].fire=true;match.step(controls);
    }
    auto hit=match.state();
    check(hit.players[victim].shield<initial && !hit.players[victim].dead,
        "Actual controls did not produce an admitted nonlethal shield hit");
    check(hit.players[victim].damage_feedback.shield.visible>0,"Admitted real hit did not drive shield feedback");
    check(hit.players[shooter].damage_feedback.shield.visible==0,"Unhit shooter has recent shield damage");
    capture(victim?"elite-hit":"spartan-hit",victim);
    for(unsigned tick=0;tick<300 && match.state().players[victim].damage_feedback.shield.visible>0;++tick) {
      controls={};aim(controls[0],match.unit(0),match.unit(1));
      aim(controls[1],match.unit(1),match.unit(0));match.step(controls);
    }
    check(match.state().players[victim].damage_feedback.shield.visible==0,"Authored feedback failed to expire after firing stopped");
    capture(victim?"elite-faded":"spartan-faded",{});
  }
  // Let the actual registered FP ready/idle clocks advance in the running
  // renderer before the final hit. This verifies visible raised arms/weapons,
  // rather than treating admitted offscreen ready-frame geometry as pixels.
  for(unsigned victim:{1u,0u}) {
    const auto name=victim?"elite-live":"spartan-live";
    const auto first_tick=match.state().tick;
    const auto initial=match.state().players[victim].shield;
    constexpr unsigned updates=46;
    options.frames=0;options.capture_tick=first_tick+updates;
    options.reference_tick_cadence=true;
    options.input_script=output/(std::string(name)+"-controls.tsv");
    options.screenshot=output/(std::string(name)+".ppm");
    {
      std::ofstream script(options.input_script);script<<std::setprecision(9);
      for(unsigned tick=0;tick<updates;++tick)for(unsigned player=0;player<2;++player) {
        Controls input;aim(input,match.unit(player),match.unit(1-player));
        script<<tick<<' '<<player<<' '<<(tick==updates-1 && player!=victim)
            <<" 0 0 "<<input.yaw<<' '<<input.pitch<<'\n';
      }
    }
    std::array<std::size_t,2> final_world{},final_fp{};
    options.shield_layer_observer=[&](unsigned view,unsigned owner,bool fp,
        std::uint32_t shader,std::uint32_t render_model,std::span<const Vertex> vertices) {
      const auto state=match.state();
      check(owner<2 && view<2 && !vertices.empty(),"Invalid continuous shield draw");
      check(state.players[owner].damage_feedback.shield.visible>0,
          "Continuous renderer admitted an owner without current damage feedback");
      const auto &definition=match.unit(owner).definition.vitality;
      check(shader==(fp?definition.shield_damaged_first_person_shader:definition.shield_damaged_shader),
          "Continuous renderer lost authored shield identity");
      check(render_model==(fp?hands_models[owner]:body_models[owner]),
          "Continuous shield escaped its owned arms/body model onto held-weapon geometry");
      if(fp) {
        check(view==owner,"Continuous FP shield belongs to another view");
        check_fp_surfaces(owner,vertices);
      }
      if(state.tick==options.capture_tick)(fp?final_fp[owner]:final_world[owner])+=vertices.size();
    };
    check(run_multiplayer(options)==0,"Continuous native shield rendering failed");
    const auto state=match.state();
    check(state.tick==options.capture_tick && state.players[victim].shield<initial &&
        state.players[victim].damage_feedback.shield.visible>0,
        "Continuous final capture did not contain the admitted real shot");
    check(final_world[victim]>0 && final_fp[victim]>0 && final_world[1-victim]==0 && final_fp[1-victim]==0,
        "Continuous capture lost world/FP layers or owner isolation");
    for(unsigned player=0;player<2;++player) {
      const auto &p=state.players[player];
      report<<name<<'\t'<<state.tick<<'\t'<<player<<'\t'<<p.health<<'\t'<<p.shield<<'\t'
          <<p.damage_feedback.shield.visible<<'\t'<<final_world[player]<<'\t'<<final_fp[player]<<'\n';
    }
    report.flush();
  }
  std::cout<<"Natural native hits: Spartan and Elite world/first-person shield layers, cold/fade admission, owner isolation and held-weapon exclusion pass\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
