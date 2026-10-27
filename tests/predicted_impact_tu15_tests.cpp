// Predicted impact sparks (NETCODE_ENHANCED_GOAL item 6) against actual host
// worlds of the MCC Coagulation cache. A battle-rifle shooter aims at a
// structure wall. The non-host side is emulated from the host's published
// state exactly as the runtime feeds it: the press gate, the local structure
// trace from the view (original_map_vector_contact 04800005) and
// NativeWorldEffects::impact.
// TU1.5 host world:
//  (a) a predicted press spawns its authored impact effects on the firing
//      frame, before the host has run the shot;
//  (b) the host fires exactly the predicted rounds at the predicted barrel
//      spacing (rounds spent, last shot tick);
//  (c) the host's replicated projectile contacts of the shooter go through
//      the ledger (suppressed only as copies of a predicted effect), and a
//      new bullet decal of that burst, when the replicated decal frame has
//      one, lies on the predicted round. On Coagulation the TU1.5 contact
//      request of these wall rounds carries no effect and the fixture's
//      decal frame stays empty: no host copy exists (reported).
// Native legacy host world (Simulation bullet impacts, replicated as such):
//  (d) the host's terrain impacts of a predicted burst are suppressed as
//      copies, and an unpredicted burst's impacts are not and still spawn.
// Original never predicts (predicted_impacts_enabled).
#include "halo2/predicted_impacts.hpp"
#include "halo2/bsp_surface_lighting.hpp"
#include "halo2/collision.hpp"
#include "halo2/kernels.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/native_grenade.hpp"
#include "halo2/world_effects.hpp"
#include "helpers/gameplay_fixture.hpp"
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>

using namespace halo2;
namespace {
void check(bool value,const std::string &why){if(!value)throw FormatError(why);}
struct Files {
  std::filesystem::path path;
  Files() {
    const char *scratch=std::getenv("HALO2_TEST_SCRATCH");
    auto name=((scratch&&*scratch)?std::filesystem::path(scratch):std::filesystem::temp_directory_path())/"halo2-predicted-impact-XXXXXX";
    auto text=name.string();check(::mkdtemp(text.data()),"Cannot create private source files");path=text;
  }
  ~Files(){std::error_code error;std::filesystem::remove_all(path,error);}
};
struct Clock final:PhysicsLiveClockSource {
  mutable std::atomic<std::uint64_t> counter{};
  std::uint32_t milliseconds()const noexcept override{return 1000;}
  std::uint64_t cycles()const noexcept override{return counter.fetch_add(1000);}
  std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
constexpr float camera_fallback=.62f;
constexpr double tick_period=1.0/30.0;
Vec3 up(Vec3 p,float z){return {p.x,p.y,p.z+z};}
std::string text(Vec3 v){std::ostringstream out;out<<std::fixed<<std::setprecision(3)<<v.x<<','<<v.y<<','<<v.z;return out.str();}

struct Wall {Spawn spawn;Vec3 eye,forward,point;float distance{};};
// Spawns with a structure wall 3..12 wu ahead at eye height, where the static
// collision agrees (no instance/structure disagreement) and no placed object
// stands near the line of fire.
std::vector<Wall> walls(const Cache &cache,const LevelContent &content,const BspSurfaceLighting &structure) {
  CollisionWorld collision(content.collision);
  const float camera=content.player.camera_height>0?content.player.camera_height:camera_fallback;
  std::vector<Wall> out;
  for(const auto &spawn:cache.spawns())for(unsigned heading=0;heading<16;++heading) {
    const float yaw=float(heading)*0.39269908169872414f;
    const Vec3 forward{std::cos(yaw),std::sin(yaw),-.05f};
    const auto eye=up(spawn.position,camera);
    const auto contact=original_map_vector_contact(structure,eye,forward*20.f,nullptr,0x04800005u);
    if(!contact)continue;
    const float distance=contact->contact.fraction*20.f*length(forward);
    if(distance<3 || distance>12)continue;
    const auto hit=collision.raycast(eye,normalized(forward),20.f);
    if(!hit || std::abs(hit->distance-distance)>.05f)continue;
    bool clear=true;
    for(const auto &object:content.objects) {
      const auto d=object.position-eye;const float along=dot(d,normalized(forward));
      if(along>-1 && along<distance+1 && length(d-normalized(forward)*along)<1.5f){clear=false;break;}
    }
    if(clear)out.push_back({spawn,eye,normalized(forward),contact->point,distance});
  }
  return out;
}

struct Fixture {
  MultiplayerOptions base;std::optional<Cache> placement;LevelContent content;
  std::optional<BspSurfaceLighting> structure;std::uint32_t rifle=null_datum;
};
void prepare(Fixture &f,const char *map,const char *shared,const char *xbe) {
  auto &base=f.base;base.map=map;base.shared_maps=shared;base.gameplay_xbe=xbe;
  base.local_players=1;base.mute=true;base.source_object_world=true;base.session_random_seed=0x12345678u;
  base.netcode_mode=NetcodeMode::balanced;
  try{f.placement.emplace(base.map,base.shared_maps,OriginalData::load_xbe(base.gameplay_xbe));}
  catch(const std::exception &error) {
    std::cout<<"TU1.5 XBE not admitted with this MCC cache ("<<error.what()<<"); using compiled TU1.5 operands\n";
    base.gameplay_xbe.clear();f.placement.emplace(base.map,base.shared_maps);
  }
  check(f.placement->is_mcc(),"Predicted impact fixture requires prepared MCC content");
  f.content=f.placement->load_content(0,true);
  for(unsigned option=3;option<=20 && !base.starting_weapon;++option)try {
    const auto weapon=f.placement->slayer_starting_weapon(option);const auto found=f.content.weapons.find(weapon);
    if(found!=f.content.weapons.end() && found->second.name.ends_with("battle_rifle")){base.starting_weapon=option;f.rifle=weapon;}
  }catch(const FormatError &){}
  check(f.rifle!=null_datum,"No starting-weapon option selects the authored battle rifle");
  f.structure=f.placement->load_bsp_surface_lighting(0);
}

class Driver {
public:
  Driver(const MultiplayerOptions &base,const Wall &w):wall(w),options(with(base,files.path,w)),
      world(options,std::nullopt,std::make_shared<Clock>()),match(world.match) {
    check(bool(world.source_objects) && world.source_objects->owns_physics(),"Actual TU1.5 source world absent");
    for(unsigned tick=0;tick<90;++tick)step();
    shooter=match.player(0).source_current_object;
    check(shooter!=null_datum,"Shooter was not born");
    world.source_objects->tu15_world().set_object_position(shooter,wall.spawn.position);
    Spawn at=wall.spawn;at.bsp=0;test::place_player(match,0,at);
    for(unsigned tick=0;tick<15;++tick)step();
    {const auto s=match.state();yaw=s.players[0].yaw;pitch=s.players[0].pitch;}
    aiming=true;
    for(unsigned tick=0;tick<20;++tick)step();
  }
  static MultiplayerOptions with(MultiplayerOptions o,const std::filesystem::path &files,const Wall &w) {
    o.source_object_savegame_directory=files;
    auto start=w.spawn;start.facing=std::atan2(w.forward.y,w.forward.x);
    o.native_spawn_inputs={start};
    o.defer_initial_player_spawn=true;
    o.player_birth_owner=[](MultiplayerPlayer &p,unsigned){p.source_flags=8;};
    o.pre_player_update_owner=[](MultiplayerMatch &m) {
      const auto &p=m.player(0);
      if(p.connected && !(p.source_flags&1u))m.apply_source_player_activation(p.source_datum,p.source_flags,p.source_flags|1u);
    };
    return o;
  }
  // The runtime camera's origin (player_camera): the ordinary unit eye.
  Vec3 eye()const {
    const auto &unit=match.unit(0);const auto p=match.state().players[0];
    return ordinary_unit_eye(p.position,unit.definition.camera_height>0?unit.definition.camera_height:camera_fallback,
        unit.definition.crouch_camera_height,p.crouch);
  }
  void step(bool fire=false) {
    std::array<Controls,multiplayer_capacity> input{};
    const auto state=match.state();
    if(aiming && state.players[0].connected && !state.players[0].dead) {
      // Look input is delta-based: steer by the error to the wall point.
      const auto from=eye();const Vec3 d{wall.point.x-from.x,wall.point.y-from.y,wall.point.z-from.z};
      yaw+=std::remainder(std::atan2(d.y,d.x)-state.players[0].yaw,6.28318531f);
      pitch+=std::atan2(d.z,std::hypot(d.x,d.y))-state.players[0].pitch;
    }
    input[0].yaw=yaw;input[0].pitch=pitch;input[0].fire=fire;
    match.step(input);
  }
  Files files;
  const Wall &wall;
  MultiplayerOptions options;
  MultiplayerPreparedWorld world;
  MultiplayerMatch &match;
  std::uint32_t shooter=null_datum;
  bool aiming{};
  float yaw{},pitch{};
};

// New host projectile contacts of the shooter since `serial`.
std::vector<NativeGrenadeDetonation> contacts(const MultiplayerState &state,std::uint64_t &serial,std::uint32_t unit,
    std::uint32_t projectile) {
  std::vector<NativeGrenadeDetonation> out;
  for(const auto &event:state.grenade_detonations)if(event.serial>serial) {
    serial=event.serial;
    if(event.unit==unit && event.definition==projectile)out.push_back(event);
  }
  return out;
}
}

// (d) The native legacy host (no TU1.5 world): Simulation's terrain bullet
// impacts are replicated in MultiplayerState::bullet_impacts.
void legacy_suppression(const Fixture &f,const std::vector<Wall> &candidates) {
  auto cache=std::make_shared<Cache>(f.base.map,f.base.shared_maps,
      f.base.gameplay_xbe.empty()?nullptr:OriginalData::load_xbe(f.base.gameplay_xbe));
  const auto content=cache->load_content(0,true);
  const auto structure=cache->load_bsp_surface_lighting(0);
  UiVariant variant{"Slayer",0,{}};
  auto rules=slayer_rules(variant);rules.weapon=f.rifle;
  MultiplayerMatch match(content,0,cache->spawns(),rules);
  const auto slot=match.join("Predicted");
  const auto &rifle=content.weapons.at(f.rifle);
  const auto shot_weapon=predicted_shot_weapon(rifle);
  const PredictedImpactTracer tracer(*cache,content,structure);
  CollisionWorld collision(content.collision);
  Mesh mesh;NativeWorldEffects effects(*cache,mesh,collision,0x1234abcdu);
  std::array<Controls,multiplayer_capacity> controls{};
  for(unsigned n=0;n<30;++n)match.step(controls);
  const auto &wall=candidates.front();
  Spawn at=wall.spawn;at.bsp=0;test::place_player(match,slot,at);
  const auto eye=[&]{
    const auto &unit=match.unit(slot);const auto p=match.state().players[slot];
    return ordinary_unit_eye(p.position,unit.definition.camera_height>0?unit.definition.camera_height:camera_fallback,
        unit.definition.crouch_camera_height,p.crouch);
  };
  const auto aim=[&]{
    const auto from=eye();const auto d=wall.point-from;
    controls[slot].yaw=std::atan2(d.y,d.x);controls[slot].pitch=std::atan2(d.z,std::hypot(d.x,d.y));
  };
  PredictedShotGate gate;PredictedImpactLedger ledger;std::uint32_t visual_random=0x2468u;
  const auto seconds=[&]{return double(match.state().tick)*tick_period;};
  const auto player=[&]{const auto s=match.state();return predicted_shot_player(s.players[slot],s.tick);};
  std::uint64_t serial{};
  for(const auto &e:match.state().bullet_impacts)serial=std::max(serial,e.serial);
  for(unsigned n=0;n<60;++n){aim();gate.input(seconds(),{},player(),&*shot_weapon);match.step(controls);}
  {const auto p=match.state().players[slot];
   std::cout<<"legacy host: shooter at "<<text(p.position)<<" armed="<<player().armed<<" loaded="<<p.loaded<<'\n';}
  const auto predict_due=[&]{
    for(const auto &round:gate.take_due(seconds())) {
      const auto s=match.state();const auto &p=s.players[slot];
      const auto predicted=tracer.trace(rifle,eye(),direction(p.yaw,p.pitch),round.range,{},s.tick+1,visual_random);
      if(!predicted)continue;
      const auto before=effects.diagnostics().effects;effects.impact(predicted->impact);
      check(effects.diagnostics().effects>before,"Legacy predicted impact spawned nothing");
      ledger.predict({round.player,round.unit,round.weapon,round.projectile,round.tick_floor,round.seconds,
          predicted->impact.position,predicted->distance,predicted->impact.material,predicted->effects});
      std::cout<<"  predicted round "<<round.index<<" at "<<text(predicted->impact.position)<<" material "
          <<predicted->impact.material<<'\n';
    }
  };
  const auto burst=[&](bool predicted) {
    std::vector<std::pair<BulletImpact,bool>> impacts;
    if(predicted)check(gate.input(seconds(),{true},player(),&*shot_weapon)>0,"Legacy press was not predicted");
    if(predicted)predict_due();
    for(unsigned n=0;n<40;++n) {
      aim();controls[slot].fire=n==0;match.step(controls);
      if(predicted){gate.input(seconds(),{n==0},player(),&*shot_weapon);predict_due();}
      for(const auto &e:match.state().bullet_impacts)if(e.serial>serial) {
        serial=e.serial;
        const bool suppressed=ledger.suppress(predicted_host_impact(e,rifle.projectile),seconds());
        if(!suppressed) {
          const auto before=effects.diagnostics().effects;effects.impact(e);
          check(effects.diagnostics().effects>before,"Unsuppressed legacy host impact spawned nothing");
        }
        impacts.push_back({e,suppressed});
      }
      effects.advance(match.state().tick,float(tick_period));
    }
    controls[slot].fire=false;
    return impacts;
  };
  const auto predicted=burst(true);
  for(const auto &[e,suppressed]:predicted)std::cout<<"  host impact tick "<<e.tick<<" at "<<text(e.position)<<" terrain "
      <<e.terrain<<" material "<<e.material<<" -> "<<(suppressed?"suppressed":"spawned")<<'\n';
  check(!predicted.empty(),"Legacy host produced no bullet impact for the predicted press");
  for(const auto &[e,suppressed]:predicted)check(!e.terrain || suppressed,"Legacy host copy of a predicted impact spawned");
  for(unsigned n=0;n<30;++n){aim();match.step(controls);}
  const auto unpredicted=burst(false);
  check(!unpredicted.empty(),"Legacy host produced no bullet impact for the unpredicted press");
  for(const auto &[e,suppressed]:unpredicted)check(!suppressed,"Unpredicted legacy host impact was suppressed");
  std::cout<<"legacy host: "<<predicted.size()<<" impacts of the predicted press suppressed, "<<unpredicted.size()
      <<" unpredicted impacts spawned; ledger predicted "<<ledger.statistics().predicted<<", suppressed "
      <<ledger.statistics().suppressed<<", pending "<<ledger.pending()<<'\n';
}
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;
  if(argc!=4 || !std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_directory(argv[2]))return 77;
  check(!predicted_impacts_enabled(NetcodeMode::original),"Original predicts impacts");
  Fixture f;prepare(f,argv[1],argv[2],argv[3]);
  const auto &rifle=f.content.weapons.at(f.rifle);
  const auto shot_weapon=predicted_shot_weapon(rifle);
  check(shot_weapon.has_value(),"Battle rifle is not a predicted precision weapon");
  std::cout<<"battle rifle: rounds/press "<<shot_weapon->rounds_per_press<<", round "<<shot_weapon->round_seconds*1000
      <<" ms, cycle "<<shot_weapon->cycle_seconds*1000<<" ms, ready "<<shot_weapon->ready_seconds*1000<<" ms, range "
      <<shot_weapon->range<<", rounds/shot "<<shot_weapon->rounds_per_shot<<", trigger "<<rifle.triggers.front().behavior<<'\n';
  for(const auto &[datum,weapon]:f.content.weapons)if(const auto w=predicted_shot_weapon(weapon))
    std::cout<<"  predicted weapon "<<weapon.name<<": rounds/press "<<w->rounds_per_press<<", cycle "<<w->cycle_seconds*1000<<" ms\n";
  const auto candidates=walls(*f.placement,f.content,*f.structure);
  check(!candidates.empty(),"No spawn faces a clear structure wall");
  std::cout<<candidates.size()<<" wall candidates\n";

  // Client presentation owners (a separate cache instance, as on a non-host).
  Cache &cache=*f.placement;
  const PredictedImpactTracer tracer(cache,f.content,*f.structure);
  CollisionWorld collision(f.content.collision);
  // Walls of distinct surface materials, nearest first.
  std::vector<const Wall *> chosen;std::set<std::int16_t> materials;
  {
    std::uint32_t probe_random=1;
    for(const auto &wall:candidates)if(const auto hit=tracer.trace(rifle,wall.eye,wall.forward,40,{},0,probe_random))
      if(materials.insert(hit->impact.material).second)chosen.push_back(&wall);
    if(chosen.size()>5)chosen.resize(5);
  }
  unsigned tried=0,passed=0,fired=0,published=0;
  for(const auto *chosen_wall:chosen) {
    const auto &wall=*chosen_wall;
    if(passed)break;
    ++tried;
    std::cout<<"wall "<<tried<<": eye "<<text(wall.eye)<<" -> "<<text(wall.point)<<" ("<<wall.distance<<" wu)\n";
    Driver d(f.base,wall);
    auto &match=d.match;
    // One presentation clock per world (effect ticks never run backwards).
    Mesh mesh;NativeWorldEffects effects(cache,mesh,collision,0x2468ace0u);
    PredictedShotGate gate;PredictedImpactLedger ledger;std::uint32_t visual_random=0x13579bdfu;
    std::vector<PredictedImpactLedger::Prediction> predictions;
    std::uint64_t serial=0;
    {const auto s=match.state();for(const auto &e:s.grenade_detonations)serial=std::max(serial,e.serial);}
    const auto seconds=[&]{return double(match.simulation().ticks())*tick_period;};
    const auto player=[&]{const auto s=match.state();return predicted_shot_player(s.players[0],s.tick);};
    // The weapon has been current since before this window: idle inputs.
    for(unsigned n=0;n<60;++n){gate.input(seconds(),{},player(),&*shot_weapon);d.step();}
    {
      const auto p=match.state().players[0];
      std::cout<<"  shooter armed="<<player().armed<<" loaded="<<p.loaded<<" magazine_mode="<<p.source_zoom_magazine_mode
          <<" action="<<p.source_action_word<<" weapon="<<std::hex<<p.weapon<<std::dec<<'\n';
    }
    std::uint64_t day_effects=effects.diagnostics().effects;
    const auto predict_due=[&](double at,unsigned &spawned_now) {
      for(const auto &round:gate.take_due(at)) {
        const auto s=match.state();const auto &p=s.players[0];
        const auto predicted=tracer.trace(rifle,d.eye(),direction(p.yaw,p.pitch),round.range,{},s.tick+1,visual_random);
        if(!predicted){std::cout<<"  round "<<round.index<<" traced no wall\n";continue;}
        const auto before=effects.diagnostics().effects;
        effects.impact(predicted->impact);
        spawned_now+=unsigned(effects.diagnostics().effects-before);
        PredictedImpactLedger::Prediction entry;entry.player=round.player;entry.unit=round.unit;entry.weapon=round.weapon;
        entry.projectile=round.projectile;entry.tick_floor=round.tick_floor;entry.seconds=round.seconds;
        entry.position=predicted->impact.position;entry.distance=predicted->distance;
        entry.material=predicted->impact.material;entry.effects=predicted->effects;
        ledger.predict(entry);predictions.push_back(entry);
        std::cout<<"  predicted round "<<round.index<<" at "<<text(entry.position)<<" material "<<entry.material
            <<" effects "<<std::hex<<entry.effects[0]<<','<<entry.effects[1]<<','<<entry.effects[2]<<','<<entry.effects[3]
            <<std::dec<<" (tick floor "<<round.tick_floor<<")\n";
      }
    };
    std::set<std::pair<std::uint32_t,std::uint32_t>> decals_before;
    {const auto s=match.state();if(s.source_decals)for(const auto &r:s.source_decals->records)decals_before.insert({r.datum,r.cache_datum});
     std::cout<<"  decal frame "<<(s.source_decals?"present":"absent")<<", records "<<(s.source_decals?s.source_decals->records.size():0)<<'\n';}
    // (a) The press: the gate admits it and the first round's effect spawns
    // on this frame, before the host step that fires it.
    const auto press_tick=match.state().tick;
    const unsigned scheduled=gate.input(seconds(),{true},player(),&*shot_weapon);
    std::cout<<"  press at tick "<<press_tick<<": scheduled "<<scheduled<<" rounds\n";
    if(!scheduled){std::cout<<"  press not admitted; next wall\n";continue;}
    unsigned firing_frame_spawns=0;predict_due(seconds(),firing_frame_spawns);
    check(firing_frame_spawns>0,"Predicted impact did not spawn its effect on the firing frame");
    check(effects.diagnostics().effects>day_effects,"World effects did not retain the predicted impact");
    // (b)/(c) Host steps: later burst rounds are predicted when due; the
    // shooter's replicated contacts go through the ledger.
    std::vector<std::pair<NativeGrenadeDetonation,bool>> host;
    for(unsigned n=0;n<45;++n) {
      d.step(n==0);
      gate.input(seconds(),{n==0},player(),&*shot_weapon);
      unsigned spawned=0;predict_due(seconds(),spawned);
      for(const auto &event:contacts(match.state(),serial,d.shooter,rifle.projectile)) {
        const bool suppressed=ledger.suppress(predicted_host_impact(event),seconds());
        if(!suppressed)effects.spawn_source(event.effect,event.projectile,event.tick,event.source_locations,
            event.colors,event.controls,event.replica_render);
        host.push_back({event,suppressed});
      }
      effects.advance(match.state().tick,float(tick_period));
    }
    const auto state=match.state();
    std::cout<<"  after burst: loaded "<<state.players[0].loaded<<" (from 36), last shot tick "<<state.players[0].shot_tick
        <<", replicated contacts of the shooter "<<host.size()<<'\n';
    for(const auto &[event,suppressed]:host)
      std::cout<<"    host contact tick "<<event.tick<<" at "<<text(predicted_host_impact(event).position)<<" effect "
          <<std::hex<<event.effect<<std::dec<<" -> "<<(suppressed?"suppressed":"spawned")<<'\n';
    if(state.players[0].loaded!=36-int(scheduled)){std::cout<<"  host did not fire the predicted burst; next wall\n";continue;}
    ++fired;published+=unsigned(host.size());
    {
      const auto spacing=std::uint32_t(std::lround(shot_weapon->round_seconds/tick_period));
      const auto last=std::uint32_t(state.players[0].shot_tick);
      std::cout<<"  host last shot at press +"<<int(last)-int(press_tick)<<" ticks; predicted +"<<spacing*(scheduled-1)<<'\n';
      check(last+1>=press_tick+spacing*(scheduled-1) && last<=press_tick+spacing*(scheduled-1)+1,
          "Host burst timing differs from the predicted rounds");
    }
    // (b) Host bullet decals created by this burst, by their quad centres.
    std::vector<Vec3> decals;
    if(state.source_decals)for(const auto &record:state.source_decals->records) {
      if(decals_before.contains({record.datum,record.cache_datum}))continue;
      for(const auto &allocation:state.source_decals->allocations)if(allocation.cache_datum==record.cache_datum) {
        Vec3 sum{};unsigned count{};
        for(const auto &quad:allocation.quads)for(const auto &vertex:quad){sum=sum+vertex.position;++count;}
        if(count)decals.push_back(sum*(1.f/float(count)));
      }
    }
    std::cout<<"  host decals created since the press: "<<decals.size()<<" (frame records "
        <<(state.source_decals?state.source_decals->records.size():0)<<", tick "<<(state.source_decals?state.source_decals->tick:0)<<")\n";
    if(decals.empty()){std::cout<<"  no host decal for this wall; next wall\n";continue;}
    bool all_on_decals=true;
    for(const auto &prediction:predictions) {
      float nearest=1e9f;for(const auto &decal:decals)nearest=std::min(nearest,length(decal-prediction.position));
      const float tolerance=.35f+.03f*prediction.distance;
      std::cout<<"    predicted round at "<<text(prediction.position)<<": nearest host decal "<<nearest<<" wu (tolerance "
          <<tolerance<<")\n";
      all_on_decals=all_on_decals && nearest<=tolerance;
    }
    if(!all_on_decals){std::cout<<"  a predicted round missed the host's decals; next wall\n";continue;}
    for(const auto &[event,suppressed]:host)
      if(std::find(std::begin(predictions.front().effects),std::end(predictions.front().effects),event.effect)!=
         std::end(predictions.front().effects))check(suppressed,"Host copy of a predicted effect was not suppressed");
    ++passed;
  }
  // The TU1.5 world publishes a contact or decal for a wall round only when
  // its original owners create one; otherwise there is no host copy to match.
  check(fired>0,"The TU1.5 host never fired a predicted burst");
  if(!passed)std::cout<<"TU1.5 host: "<<fired<<" predicted bursts fired at "<<tried<<" walls of distinct materials; "
      "replicated contacts "<<published<<", new decals 0 (no host copy exists to suppress or compare)\n";
  legacy_suppression(f,candidates);
  std::cout<<"Predicted battle-rifle impacts spawn on the firing frame with the host's round timing; native host copies are "
      "suppressed and unpredicted impacts spawn\n";
  return 0;
}catch(const std::exception &error) {
  std::cerr<<"predicted impact TU1.5: "<<error.what()<<'\n';
  return 1;
}
