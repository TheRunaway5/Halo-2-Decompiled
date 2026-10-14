#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/math.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/particle_draw.hpp"
#include "halo2/particle_render_inputs.hpp"
#include "halo2/multiplayer_audio.hpp"
#include <SDL.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <iomanip>
#include <set>

using namespace halo2;
namespace {
void check(bool ok,const char *message) {if(!ok)throw FormatError(message);}
Vec3 eye(const Unit &unit) {return unit.body.position+Vec3{0,0,unit.definition.camera_height};}
void aim(Controls &control,const Unit &shooter,const Unit &target) {
  const auto delta=target.body.position+Vec3{0,0,target.body.height*.5f}-eye(shooter);
  control.yaw=std::atan2(delta.y,delta.x);
  control.pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));
}
bool same_particle_material(const ParticleMaterial &a,const ParticleMaterial &b) {
  if(a.vertex.template_datum!=b.vertex.template_datum || a.vertex.pass_datum!=b.vertex.pass_datum ||
      a.vertex.vertex_datum!=b.vertex.vertex_datum || a.vertex.classification!=b.vertex.classification)return false;
  for(unsigned n=0;n<a.bitmaps.size();++n) {
    if(bool(a.bitmaps[n])!=bool(b.bitmaps[n]))return false;
    if(a.bitmaps[n] && (a.bitmaps[n]->datum!=b.bitmaps[n]->datum || a.bitmaps[n]->index!=b.bitmaps[n]->index))return false;
  }
  return true;
}
}
int main(int argc,char **argv)try {
  check(argc==4 || argc==5,"Supply imported assets directory, gameplay executable, output directory and optional capture stage");
  const std::string stage=argc==5?argv[4]:"full";
  check(stage=="full" || stage=="elite-pop" || stage=="spartan-pop" ||
      stage=="elite-recharge" || stage=="spartan-recharge","Unknown shield capture stage");
  if(!std::getenv("DISPLAY"))return 77;
  SDL_SetMainReady();
  const std::filesystem::path assets=argv[1],output=argv[3];
  std::filesystem::create_directories(output);
  MultiplayerOptions options;
  options.map=assets/"disc/maps/lockout.map";
  options.shared_maps=options.map.parent_path();options.gameplay_xbe=argv[2];
  options.width=1280;options.height=720;options.frames=1;
  options.local_players=2;options.starting_weapon=5;
  options.hidden=true;options.mute=false;options.validation=true;
  options.reference_auxiliary_seed=0x5821b36du;
  const std::array<std::array<std::uint8_t,9>,2> appearance{{
      {8,12,9,1,0,26,9,0,0},{3,4,3,4,1,5,31,0,0}}};
  for(unsigned i=0;i<2;++i)options.selected_profiles.push_back(
      native_selected_player_configuration(i?"Elite":"Spartan",appearance[i],-1));
  // Select two unchanged authored start records with a measured clear native
  // firing line. Placement is an explicit offline input before object birth;
  // no unit position, health, shield, readiness or feedback is ever assigned.
  std::array<std::uint32_t,2> hands_models{},body_models{};
  {
    Cache cache(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe));
    const auto content=cache.load_content(0,true);
    for(unsigned player=0;player<2;++player) {
      const auto &representation=content.player_representations.at(player?3:2);
      hands_models[player]=representation.hands;
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
  std::vector<std::array<Controls,multiplayer_capacity>> plan;
  const auto step=[&] {plan.push_back(controls);match.step(controls);};
  // Original start scoring may legitimately select the same authored point.
  // Separate only through ordinary walking toward the other authored point.
  for(unsigned tick=0;tick<180 && length(match.unit(0).body.position-match.unit(1).body.position)<1.5f;++tick) {
    const auto position=match.unit(0).body.position;
    const auto &a=options.native_spawn_inputs[0].position;
    const auto &b=options.native_spawn_inputs[1].position;
    const auto goal=length(a-position)>length(b-position)?a:b;
    const auto delta=goal-position;
    controls={};controls[0].yaw=std::atan2(delta.y,delta.x);
    controls[0].forward=1;step();
  }
  controls={};
  for(unsigned tick=0;tick<90;++tick) {
    aim(controls[0],match.unit(0),match.unit(1));
    aim(controls[1],match.unit(1),match.unit(0));step();
  }
  print_roster("settled");
  check(length(match.unit(0).body.position-match.unit(1).body.position)>1,"Actual initial spawns overlap");

  std::array<std::uint32_t,2> damaged{},depleted{},recharge{};
  for(unsigned n=0;n<2;++n) {
    const auto &v=match.unit(n).definition.vitality;
    damaged[n]=v.shield_damaged_effect;depleted[n]=v.shield_depleted_effect;recharge[n]=v.shield_recharging_effect;
  }
  std::array<unsigned,2> pop_tick{},charge_tick{},death_tick{};
  for(unsigned victim:{1u,0u}) {
    const auto shooter=1u-victim;
    const auto tick=[&](bool fire) {
      controls={};aim(controls[shooter],match.unit(shooter),match.unit(victim));
      aim(controls[victim],match.unit(victim),match.unit(shooter));controls[shooter].fire=fire;
      if(match.unit(shooter).weapon.loaded==0)controls[shooter].reload=true;
      step();
    };
    for(unsigned n=0;n<500 && match.unit(victim).vitality.shield>0;++n)tick(true);
    check(match.unit(victim).vitality.shield==0 && !match.unit(victim).vitality.dead,
        "Planner did not produce a real nonlethal shield break");
    pop_tick[victim]=match.state().tick;
    for(unsigned n=0;n<500 && !match.unit(victim).vitality.shield_recharging;++n)tick(false);
    check(match.unit(victim).vitality.shield_recharging,"Planner did not reach actual recharge");
    charge_tick[victim]=match.state().tick;
    for(unsigned n=0;n<500 && match.unit(victim).vitality.shield<1;++n)tick(false);
    check(match.unit(victim).vitality.shield==1,"Planner did not finish actual recharge");
    // Both alive players retain their authored line of fire. Test deaths only
    // after the two complete pop/recharge cycles, so no teleport is needed.
  }
  for(unsigned victim:{1u,0u}) {
    const auto shooter=1u-victim;
    for(unsigned n=0;n<700 && !match.unit(victim).vitality.dead;++n) {
      controls={};aim(controls[shooter],match.unit(shooter),match.unit(victim));
      controls[shooter].fire=true;if(match.unit(shooter).weapon.loaded==0)controls[shooter].reload=true;
      step();
    }
    check(match.unit(victim).vitality.dead,"Planner did not reach genuine player death");
    death_tick[victim]=match.state().tick;
    // Stop after the ordinary respawn, and re-aim only through controls.
    for(unsigned n=0;n<240 && match.unit(victim).vitality.dead;++n){controls={};step();}
    check(!match.unit(victim).vitality.dead,"Planner did not reach genuine respawn");
  }
  for(unsigned n=0;n<30;++n){controls={};step();}
  const auto final_tick=match.state().tick;
  const unsigned capture_victim=stage.starts_with("elite")?1:0;
  const auto capture_limit=stage=="full"?final_tick:
      stage.ends_with("pop")?pop_tick[capture_victim]+1:charge_tick[capture_victim]+10;
  {std::ofstream timeline(output/"timeline.tsv");timeline<<"species\tpop\trecharge\tdeath\n";
    for(unsigned v=0;v<2;++v)timeline<<(v?"elite":"spartan")<<'\t'<<pop_tick[v]<<'\t'<<charge_tick[v]<<'\t'<<death_tick[v]<<'\n';}
  options.input_script=output/"shield-cycle-controls.tsv";
  {std::ofstream script(options.input_script);script<<std::setprecision(9);
    for(unsigned tick=0;tick<capture_limit;++tick)for(unsigned p=0;p<2;++p) {
      const auto &c=plan[tick][p];script<<tick<<' '<<p<<' '<<c.fire<<' '<<c.reload
          <<" 0 "<<c.yaw<<' '<<c.pitch<<" move "<<c.forward<<' '<<c.right<<'\n';
    }
  }
  options.prepared_world.reset();options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  auto &live=options.prepared_world->match;
  struct PopLeaf {std::uint32_t datum{};ParticleMaterial material;};
  std::array<std::vector<PopLeaf>,2> pop_leaves;
  std::array<std::array<std::vector<unsigned>,2>,2> leaf_draws;
  std::array<ParticleModelDefinition,2> charge_models;
  std::array<ParticleMaterial,2> charge_materials;
  for(unsigned victim=0;victim<2;++victim) {
    // Read the actual depletion graph, not a class shared with muzzle flashes
    // and impacts. Each authored leaf must reach both real local views.
    const auto graph=options.prepared_world->tags.get({depleted[victim]});
    const auto events=graph.block(0x14,0x38);
    for(unsigned event=0;event<events.size();++event) {
      const auto systems=events.element(event).block(0x30,0x38);
      for(unsigned system=0;system<systems.size();++system) {
        const auto definition=options.prepared_world->cache->load_particle_system_render_definition(
            depleted[victim],event,system);
        pop_leaves[victim].push_back({definition.particle_datum,
            options.prepared_world->cache->load_particle_material(definition.particle_datum,15,0,0)});
      }
    }
    check(!pop_leaves[victim].empty(),"Selected shield depletion graph has no authored particles");
    for(auto &view:leaf_draws[victim])view.resize(pop_leaves[victim].size());
    const auto charge_graph=options.prepared_world->tags.get({recharge[victim]});
    const auto charge_events=charge_graph.block(0x14,0x38);
    check(charge_events.size()==1 && charge_events.element(0).block(0x30,0x38).size()==1,
        "Selected recharge graph differs from the verified original single-system effect");
    const auto charge_system=options.prepared_world->cache->load_particle_system_render_definition(recharge[victim],0,0);
    check(charge_system.coordinates==1,"Recharge lost its authored marker-local coordinates");
    charge_models[victim]=load_particle_model(*options.prepared_world->cache,charge_system.particle_datum);
    charge_materials[victim]=options.prepared_world->cache->load_particle_material(charge_system.particle_datum,15,0,1);
  }
  options.frames=0;options.capture_tick=capture_limit;options.reference_tick_cadence=true;
  options.screenshot=output/(stage+".ppm");
  std::array<std::array<unsigned,2>,2> model_draws{},pop_draws{},perpendicular{};
  std::array<bool,2> popped{},charged{},dead{};
  std::set<std::uint32_t> shield_sounds,voices;
  std::ofstream audio(output/"audio.tsv"),particles(output/"particles.tsv");
  audio<<"tick\tkind\tsound\towner\tview\tframes\n";
  particles<<"tick\tview\tclass\tmodel_count\talpha\n";
  options.audio_observer=[&](const MultiplayerAudioSubmission &s) {
    audio<<live.state().tick<<'\t'<<unsigned(s.kind)<<'\t'<<std::hex<<s.sound<<'\t'<<s.owner<<std::dec<<'\t'<<s.view<<'\t'<<s.frames<<'\n';
    if(s.kind==MultiplayerAudioKind::Shield)shield_sounds.insert(s.sound);
    if(s.kind==MultiplayerAudioKind::DamageVoice)voices.insert(s.sound);
  };
  options.effect_observer=[&](unsigned view,std::uint32_t tick,std::span<const ParticleDraw> draws) {
    for(const auto &draw:draws) {
      const auto classification=draw.material->vertex.classification;
      particles<<tick<<'\t'<<view<<'\t'<<classification<<'\t'<<draw.model_count<<'\t'<<draw.vertex.attributes[2][3]<<'\n';
      for(unsigned victim=0;victim<2;++victim) {
        if(tick>=pop_tick[victim] && tick<pop_tick[victim]+30) {
          for(unsigned leaf=0;leaf<pop_leaves[victim].size();++leaf)
            if(same_particle_material(*draw.material,pop_leaves[victim][leaf].material) &&
                std::isfinite(draw.vertex.attributes[2][3]) && draw.vertex.attributes[2][3]>0) {
              ++leaf_draws[victim][view][leaf];++pop_draws[victim][view];
              if(classification==9)++perpendicular[victim][view];
            }
        }
        if(tick>=charge_tick[victim] && tick<charge_tick[victim]+30 && draw.model &&
            same_particle_material(*draw.material,charge_materials[victim])) {
          check(classification==1 && draw.model_count==504 && draw.model->vertices.size()==172,
              "Recharge draw did not retain the authored PRTM mesh");
          const auto &authored=charge_models[victim];
          check(draw.model->vertices==authored.vertices && draw.model->indices==authored.indices &&
              draw.model->spans==authored.spans && draw.model->shader==authored.shader,
              "Recharge replaced original Xbox mesh, index spans or shader");
          const auto &a=draw.vertex.attributes;
          const auto area=std::abs(a[12][0])+std::abs(a[12][1])+std::abs(a[12][2])+
              std::abs(a[13][0])+std::abs(a[13][1])+std::abs(a[13][2]);
          check(std::isfinite(area) && area>0,"Recharge mesh has no finite nondegenerate current transform");
          ++model_draws[victim][view];
        }
      }
    }
  };
  options.input_observer=[&](auto,std::span<const Controls>,const MultiplayerState &s) {
    for(unsigned victim=0;victim<2;++victim) {
      if(s.tick==pop_tick[victim]) {
        check(s.players[victim].shield==0 && !s.players[victim].dead,"Live break differs from actual planned controls");
        unsigned requests{};
        for(const auto &event:s.unit_damage_effects)
          if(event.tick==s.tick && event.target==s.players[victim].source_unit &&
              event.effect.kind==UnitDamageEffectKind::ShieldDepleted) {
            check(event.effect.datum==depleted[victim],"Shield break requested another species' pop effect");
            ++requests;
          }
        check(requests==1,"Real shield break did not publish exactly one authored depletion request");
        popped[victim]=true;
      }
      if(s.tick==charge_tick[victim]) {
        check(s.players[victim].shield_recharging,"Live recharge differs from actual planned controls");
        unsigned requests{};
        for(const auto &event:s.unit_damage_effects)
          if(event.tick==s.tick && event.target==s.players[victim].source_unit &&
              event.effect.kind==UnitDamageEffectKind::ShieldRecharging) {
            check(event.effect.datum==recharge[victim],"Recharge requested another species' effect");++requests;
          }
        check(requests==1,"Actual recharge start did not publish exactly one authored effect request");
        charged[victim]=true;
      }
      if(s.tick==death_tick[victim]){check(s.players[victim].dead,"Live death differs from actual planned controls");dead[victim]=true;}
    }
  };
  check(run_multiplayer(options)==0,"Continuous shield cycle renderer/audio failed");
  for(unsigned victim=0;victim<2;++victim)if(pop_tick[victim]<capture_limit)
    for(unsigned view=0;view<2;++view)for(unsigned leaf=0;leaf<pop_leaves[victim].size();++leaf) {
      check(leaf_draws[victim][view][leaf]>0,"Shield break omitted an authored particle leaf from a local view");
      std::cout<<"PASS "<<(victim?"Elite":"Spartan")<<" pop leaf="<<std::hex<<pop_leaves[victim][leaf].datum
          <<std::dec<<" view="<<view<<" visible_draws="<<leaf_draws[victim][view][leaf]<<'\n';
    }
  if(stage!="full") {
    for(unsigned view=0;view<2;++view)
      check(stage.ends_with("pop")?pop_draws[capture_victim][view]>0:model_draws[capture_victim][view]>0,
          "Requested shield capture lacks its actual particle submissions");
    std::cout<<"Captured actual "<<stage<<" at tick "<<options.capture_tick<<'\n';return 0;
  }
  for(unsigned victim=0;victim<2;++victim) {
    check(popped[victim] && charged[victim] && dead[victim],"A real damage transition was not observed");
    for(unsigned view=0;view<2;++view) {
      check(pop_draws[victim][view]>0 && model_draws[victim][view]>0,
          "Shield pop/recharge was not submitted to both actual local views");
      if(victim==1)check(perpendicular[victim][view]>0,"Elite shield pop lacks authored class9 sparks");
    }
  }
  for(const auto tag:{0xe8170692u,0xe8190694u,0xe81b0696u,0xe81d0698u,0xe81f069au,0xe821069cu,0xe823069eu})
    check(shield_sounds.contains(tag),"An authored shield hit/low/empty/recharge sound did not reach real SDL packets");
  for(const auto tag:{0xe96b07e6u,0xea91090cu})
    check(voices.contains(tag),"An authored player death voice did not reach real SDL packets");
  // The resident dialogue table intentionally has NONE for some pain choices.
  // A separate controlled original-selector test covers each species' pain9;
  // this natural run must preserve ordinary RNG admission, including silence.
  std::cout<<"Natural shield cycles passed: both species, two real views, PRTM recharge, class9 pop sparks, all shield sounds, pain/death and respawn. ticks="<<final_tick<<'\n';
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
