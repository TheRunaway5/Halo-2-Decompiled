#include "halo2/native_lag_compensation.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/collision.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/source_replay_batch.hpp"
#include "helpers/gameplay_fixture.hpp"
#include <openssl/evp.h>
#include <boost/pfr/core.hpp>
#include <boost/pfr/core_name.hpp>
#include <atomic>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sys/wait.h>
#include <spawn.h>
#include <fstream>
#include <set>
#include <sstream>

// Lag-compensated hitscan in an actual TU1.5 MCC world (NETCODE_ENHANCED_GOAL
// item 2). Slot 0 is a battle-rifle shooter treated as remote: before every
// step it reports a displayed snapshot time LATENCY ticks behind the host.
// Slot 1 is the target, placed by the original B95D0 position owner. Each
// scenario runs in an Original, a Balanced and a Full world:
//  (a) aiming where the target was 100 ms ago hits under Balanced, misses
//      under Original;
//  (b) a 200 ms shooter is rewound only to the 100 ms cap: aiming at the
//      200 ms position misses, aiming at the cap position hits (Full's higher
//      cap reaches the 200 ms position);
//  (c) a target behind structure longer than the veto window is not hit by a
//      rewound shot under Balanced (the rewound hit is vetoed), but is under
//      Full; a target exposed inside the window is still hit;
//  (d) a rewound shot that hits nothing leaves the whole source bank byte-
//      identical to the Original world's, and every rewound query restores
//      exactly the bytes it replaced.
// With --replay (replay-mode migration): a Balanced host records the ticks of
// a rewound hit as ordinary replay phases; a replay world that imported the
// host's cut, rewind history included, reproduces every tick byte for byte;
// and that world, built in Original mode, keeps Balanced rewind as the
// promoted host.
extern char **environ;
using namespace halo2;
namespace {
void check(bool value,const std::string &why){if(!value)throw FormatError(why);}
struct Files {
  std::filesystem::path path;
  Files() {
    const char *scratch=std::getenv("HALO2_TEST_SCRATCH");
    auto name=((scratch&&*scratch)?std::filesystem::path(scratch):std::filesystem::temp_directory_path())/"halo2-lag-compensation-XXXXXX";
    auto text=name.string();check(::mkdtemp(text.data()),"Cannot create private source files");path=text;
  }
  ~Files(){std::error_code error;std::filesystem::remove_all(path,error);}
};
// stride 0 (the replay test) keeps exported logical cycles exact between a
// host and a replay world; reads are still recorded as phase inputs.
struct Clock final:PhysicsLiveClockSource {
  explicit Clock(std::uint64_t s=1000):stride(s){}
  mutable std::atomic<std::uint64_t> counter{};
  const std::uint64_t stride;
  std::uint32_t milliseconds()const noexcept override{return 1000;}
  std::uint64_t cycles()const noexcept override{return counter.fetch_add(stride);}
  std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
constexpr float target_height=.7f,camera_fallback=.62f;
Vec3 up(Vec3 p,float z){return {p.x,p.y,p.z+z};}
float distance(Vec3 a,Vec3 b){return std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));}

struct Geometry {
  Spawn shooter;Vec3 eye,forward,right;
  Vec3 a,b,c;
  std::vector<Vec3> hidden; // Floor points behind structure near A.
};
struct Probe {
  const CollisionWorld &collision;const BspSurfaceLighting &structure;
  std::optional<Vec3> floor(Vec3 near)const {
    const auto hit=collision.raycast(up(near,.8f),{0,0,-1},2.f);
    if(!hit || hit->normal.z<.9f)return std::nullopt;
    if(collision.raycast(up(hit->position,.05f),{0,0,1},.9f))return std::nullopt; // Head room.
    return up(hit->position,.002f);
  }
  bool blocked(Vec3 from,Vec3 to,bool static_collision=true)const {
    const Vec3 d{to.x-from.x,to.y-from.y,to.z-from.z};const auto length=distance(from,to);
    if(original_map_vector_contact(structure,from,d,nullptr,0x04800005u))return true;
    return static_collision && collision.raycast(from,{d.x/length,d.y/length,d.z/length},length).has_value();
  }
  bool visible(Vec3 eye,Vec3 floor)const {
    for(float z:{.1f,.35f,.6f})if(blocked(eye,up(floor,z)))return false;
    return true;
  }
  bool hidden(Vec3 eye,Vec3 floor,Vec3 right)const {
    const Vec3 forward{-right.y,right.x,0};
    for(float side:{-.3f,0.f,.3f})for(float along:{-.3f,0.f,.3f})for(float z=.02f;z<=.9f;z+=.08f) {
      const Vec3 p{floor.x+right.x*side+forward.x*along,floor.y+right.y*side+forward.y*along,floor.z+z};
      if(!original_map_vector_contact(structure,eye,{p.x-eye.x,p.y-eye.y,p.z-eye.z},nullptr,0x04800005u))return false;
    }
    return true;
  }
};
Geometry find_geometry(const Cache &cache,const LevelContent &content,const BspSurfaceLighting &structure) {
  CollisionWorld collision(content.collision);Probe probe{collision,structure};
  const float camera=content.player.camera_height>0?content.player.camera_height:camera_fallback;
  std::optional<Geometry> best;
  for(const auto &spawn:cache.spawns())for(unsigned heading=0;heading<16;++heading) {
    const float yaw=float(heading)*0.39269908169872414f;
    const Vec3 forward{std::cos(yaw),std::sin(yaw),0},right{std::sin(yaw),-std::cos(yaw),0};
    const auto eye=up(spawn.position,camera);
    if(collision.raycast(eye,forward,7.f))continue;
    for(float range:{4.f,3.5f,5.f}) {
      const auto a=probe.floor(spawn.position+forward*range);
      if(!a || std::abs(a->z-spawn.position.z)>.5f || !probe.visible(eye,*a))continue;
      const auto b=probe.floor(*a+right*1.6f),c=probe.floor(*a-right*1.6f);
      if(!b || !c || std::abs(b->z-a->z)>.3f || std::abs(c->z-a->z)>.3f ||
         !probe.visible(eye,*b) || !probe.visible(eye,*c))continue;
      Geometry g{spawn,eye,forward,right,*a,*b,*c,{}};
      for(float along:{0.f,.5f,-.5f,1.f,1.5f,-1.f,2.f})for(float lateral=.6f;lateral<=3.01f;lateral+=.3f)
        for(float sign:{1.f,-1.f}) {
          const auto h=probe.floor(*a+right*(lateral*sign)+forward*along);
          if(h && std::abs(h->z-a->z)<.5f && probe.hidden(eye,*h,right) &&
             std::ranges::none_of(g.hidden,[&](Vec3 o){return distance(o,*h)<.25f;}))
            g.hidden.push_back(*h);
        }
      std::ranges::sort(g.hidden,{},[&](Vec3 h){return distance(h,*a);});
      if(g.hidden.size()>4)g.hidden.resize(4);
      if(!g.hidden.empty())return g;
      if(!best)best=g;
    }
  }
  check(best.has_value(),"No authored spawn has a clear visible target corridor");
  return *best;
}

using Record=NativeLagCompensation::QueryRecord;
struct Shot {
  bool damaged{};
  std::vector<Record> records;
  std::uint32_t fire_tick{};
  float shield_before{},health_before{},shield_after{},health_after{};
  int first_present_offset=-1000; // first considered query present - (F-1)
  Vec3 target_at; // Published target position just before the fire tick.
};
std::string describe(const Shot &s) {
  std::ostringstream out;out<<"damaged="<<s.damaged<<" shield "<<s.shield_before<<"->"<<s.shield_after
      <<" health "<<s.health_before<<"->"<<s.health_after<<" queries="<<s.records.size();
  for(const auto &r:s.records)out<<" [present="<<r.present<<" viewed="<<r.viewed<<" target="<<r.target_tick
      <<" rewind="<<r.rewind_ticks<<" capped="<<r.capped<<" rewound="<<r.rewound<<" rewound_hit="<<r.rewound_hit
      <<" vetoed="<<r.vetoed<<" hit="<<std::hex<<r.hit<<std::dec<<" targets="<<r.targets<<" type="<<r.result_type
      <<" origin="<<r.origin.x<<','<<r.origin.y<<','<<r.origin.z<<" point="<<r.point.x<<','<<r.point.y<<','<<r.point.z<<"]";
  out<<" target_at="<<s.target_at.x<<','<<s.target_at.y<<','<<s.target_at.z;
  return out.str();
}
// Bytes whose values differ between identical Original processes,
// independent of netcode (characterized over repeated Original-only runs):
// two 64-byte records at 4ECC40 whose slot order varies, and three flag bytes
// whose bit 6 varies. The Original-vs-Original check below proves the mask.
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,4> nondeterministic_bytes{{
    {0x4ecc40u,0x80u},{0x4ef1b8u,1u},{0x51db10u,1u},{0x6011ede8u,1u}}};
// 64-bit FNV-1a hash of every committed 4 KiB bank page (masked bytes as 0).
using Pages=std::map<std::uint32_t,std::uint64_t>;
Pages page_hashes(const SourceRuntimeMemory &bank) {
  Pages out;
  bank.visit_committed([&](std::uint32_t at,std::span<const std::byte> bytes) {
    for(std::size_t offset=0;offset<bytes.size();offset+=4096) {
      std::uint64_t h=1469598103934665603ull;
      const auto end=std::min(bytes.size(),offset+4096);
      for(std::size_t i=offset;i<end;++i) {
        const auto address=std::uint64_t(at)+i;
        const bool masked=std::ranges::any_of(nondeterministic_bytes,[&](const auto &r){
          return address>=r.first && address<std::uint64_t(r.first)+r.second;});
        h^=masked?0u:std::to_integer<unsigned>(bytes[i]);h*=1099511628211ull;
      }
      out[std::uint32_t(at+offset)]=h;
    }
  });
  return out;
}
std::vector<std::uint32_t> differing(const Pages &a,const Pages &b) {
  std::vector<std::uint32_t> out;
  for(const auto &[at,h]:a){const auto o=b.find(at);if(o==b.end()||o->second!=h)out.push_back(at);}
  for(const auto &[at,h]:b)if(!a.contains(at))out.push_back(at);
  return out;
}

class Driver {
public:
  // warm=false leaves the world as constructed (a replay world that imports
  // a host cut before it steps).
  Driver(const MultiplayerOptions &base,NetcodeMode mode,const Geometry &g,bool warm=true,std::uint64_t cycle_stride=1000):geometry(g),
      options(with(base,mode,files.path,g)),world(options,std::nullopt,std::make_shared<Clock>(cycle_stride)),match(world.match) {
    check(bool(world.source_objects) && world.source_objects->owns_physics(),"Actual TU1.5 source world absent");
    check(match.netcode_mode()==mode,"Match did not retain the host netcode mode");
    check((mode==NetcodeMode::original)==(match.lag_compensation()==nullptr),"Original installs no rewind owner; others do");
    observe_rewinds();
    if(!warm)return;
    for(unsigned tick=0;tick<90;++tick)step();
    shooter=match.player(0).source_current_object;target=match.player(1).source_current_object;
    check(shooter!=null_datum && target!=null_datum,"Shooter or target was not born");
    // The original selector chooses among the declared spawns itself; place
    // both bodies explicitly through the original B95D0 position owner.
    place(shooter,0,geometry.shooter.position);place(target,1,geometry.a);
    for(unsigned tick=0;tick<15;++tick)step();
    check(distance(match.state().players[0].position,geometry.shooter.position)<.3f,"Shooter did not stay at its placed spawn");
    {const auto s=match.state();yaw=s.players[0].yaw;pitch=s.players[0].pitch;target_yaw=s.players[1].yaw;}
    aiming=true;aim=up(geometry.a,target_height*.55f);
    for(unsigned tick=0;tick<10;++tick)step();
    check(match.state().netcode_mode==mode,"Snapshot does not carry the host netcode mode");
  }
  static MultiplayerOptions with(MultiplayerOptions o,NetcodeMode mode,const std::filesystem::path &files,const Geometry &g) {
    o.netcode_mode=mode;o.source_object_savegame_directory=files;
    auto start=g.shooter;start.facing=std::atan2(g.forward.y,g.forward.x);
    Spawn target=start;target.position=g.a;target.facing=start.facing+3.14159265f;
    o.native_spawn_inputs={start,target};
    o.defer_initial_player_spawn=true;
    o.player_birth_owner=[](MultiplayerPlayer &p,unsigned){p.source_flags=8;};
    o.pre_player_update_owner=[](MultiplayerMatch &m) {
      for(unsigned slot=0;slot<2;++slot) {
        const auto &p=m.player(slot);
        if(p.connected && !(p.source_flags&1u))m.apply_source_player_activation(p.source_datum,p.source_flags,p.source_flags|1u);
      }
    };
    return o;
  }
  void place(std::uint32_t unit,unsigned slot,Vec3 floor) {
    world.source_objects->tu15_world().set_object_position(unit,floor);
    Spawn at;at.position=floor;at.bsp=0;test::place_player(match,slot,at);
  }
  void place(Vec3 floor){place(target,1,floor);}
  // The current rewind owner reports to records (an imported cut can replace it).
  void observe_rewinds() {
    if(auto *lag=match.lag_compensation())lag->observe([this](const Record &r){records.push_back(r);});
  }
  void step(bool fire=false) {
    last_view=NetcodeViewTime{std::uint16_t((match.simulation().ticks()-latency)&65535u),0};
    match.set_remote_view(0,last_view);
    std::array<Controls,multiplayer_capacity> input{};
    // Look input is delta-based: steer the raw accumulator by the error
    // between the desired aim and the published facing.
    const auto state=match.state();
    if(aiming && state.players[0].connected && !state.players[0].dead) {
      const auto eye=up(match.unit(0).body.position,match.unit(0).definition.camera_height>0?match.unit(0).definition.camera_height:camera_fallback);
      const Vec3 d{aim.x-eye.x,aim.y-eye.y,aim.z-eye.z};
      yaw+=std::remainder(std::atan2(d.y,d.x)-state.players[0].yaw,6.28318531f);
      pitch+=std::atan2(d.z,std::hypot(d.x,d.y))-state.players[0].pitch;
    }
    input[0].yaw=yaw;input[0].pitch=pitch;input[0].fire=fire;
    input[1].yaw=target_yaw;input[1].pitch=0;
    last_input=input;
    match.step(input);
  }
  std::uint32_t next()const{return std::uint32_t(match.simulation().ticks())+1u;}
  void recharge() {
    for(unsigned n=0;n<450;++n) {
      const auto s=match.state().players[1];
      if(!s.dead && s.shield>=.999f && s.health>=.999f && n>=20)return;
      step();
    }
    check(false,"Target shields did not recharge between scenarios");
  }
  // schedule: tick offsets from the fire tick F (a step producing F) and the
  // floor point the target is placed at before that step.
  Shot shoot(unsigned display_latency,Vec3 aim_at,std::vector<std::pair<int,Vec3>> schedule,
      std::function<void(int)> around={}) {
    latency=display_latency;recharge();
    place(schedule.front().second);for(unsigned n=0;n<4;++n)step();
    aim=up(aim_at,target_height*.55f);
    const auto first=std::ranges::min(schedule,{},[](const auto &e){return e.first;}).first;
    Shot shot;shot.fire_tick=next()+unsigned(8-first);
    records.clear();
    {const auto s=match.state().players[1];shot.shield_before=shot.shield_after=s.shield;shot.health_before=shot.health_after=s.health;}
    for(;next()<shot.fire_tick+24;) {
      const int offset=int(next())-int(shot.fire_tick);
      for(const auto &[at,where]:schedule)if(at==offset && at!=schedule.front().first)place(where);
      if(around)around(offset);
      if(offset==0)shot.target_at=match.state().players[1].position;
      step(offset==0);
      if(offset>=0) {
        const auto s=match.state().players[1];
        shot.shield_after=std::min(shot.shield_after,s.shield);shot.health_after=std::min(shot.health_after,s.health);
      }
    }
    shot.damaged=shot.shield_after<shot.shield_before-1e-4f || shot.health_after<shot.health_before-1e-4f;
    shot.records=records;
    if(!records.empty())shot.first_present_offset=int(records.front().present)-int(shot.fire_tick-1);
    return shot;
  }
  const NativeLagCompensation::Stats *stats()const{return match.lag_compensation()?&match.lag_compensation()->stats():nullptr;}
  Files files;
  const Geometry &geometry;
  MultiplayerOptions options;
  MultiplayerPreparedWorld world;
  MultiplayerMatch &match;
  std::uint32_t shooter=null_datum,target=null_datum;
  unsigned latency{};
  Vec3 aim{};
  bool aiming{};
  float yaw{},pitch{},target_yaw{};
  std::vector<Record> records;
  // The last step's inputs and slot 0's displayed view, as the step consumed them.
  std::array<Controls,multiplayer_capacity> last_input{};
  NetcodeViewTime last_view{};
};
const Record *first_rewound(const Shot &s) {
  for(const auto &r:s.records)if(r.rewound)return &r;
  return nullptr;
}

struct WorldReport {
  std::map<std::string,Shot> shots;
  std::map<std::string,Pages> pages; // ready, before, after
  int lead{};
  std::vector<std::uint64_t> stats;
  std::vector<std::pair<std::uint32_t,std::uint32_t>> target_state; // Rewound target byte ranges.
};
struct Fixture {
  MultiplayerOptions base;std::optional<Cache> placement;LevelContent content;
  std::optional<BspSurfaceLighting> structure;std::optional<Geometry> geometry;
};
// Shared deterministic setup: cache, battle-rifle loadout and geometry.
void prepare(Fixture &f,const char *map,const char *shared,const char *xbe,bool verbose) {
  auto &base=f.base;base.map=map;base.shared_maps=shared;base.gameplay_xbe=xbe;
  base.local_players=2;base.mute=true;base.source_object_world=true;base.session_random_seed=0x12345678u;
  try{f.placement.emplace(base.map,base.shared_maps,OriginalData::load_xbe(base.gameplay_xbe));}
  catch(const std::exception &error) {
    if(verbose)std::cout<<"TU1.5 XBE not admitted with this MCC cache ("<<error.what()<<"); using compiled TU1.5 operands\n";
    base.gameplay_xbe.clear();f.placement.emplace(base.map,base.shared_maps);
  }
  const auto &placement=*f.placement;check(placement.is_mcc(),"Lag compensation fixture requires prepared MCC content");
  f.content=placement.load_content(0,true);
  std::vector<LagCompensatedWeaponRow> rows;const auto projectiles=lag_compensated_projectiles(f.content,&rows);
  if(verbose)for(const auto &row:rows)std::cout<<"instantaneous weapon "<<row.name<<" trigger="<<row.trigger_behavior
      <<" zoom="<<row.zoom_levels<<" rps="<<row.rounds_per_second<<" -> "<<(row.admitted?"rewound":"host time")<<'\n';
  std::uint32_t rifle=null_datum;
  for(unsigned option=3;option<=20 && !base.starting_weapon;++option)try {
    const auto weapon=placement.slayer_starting_weapon(option);const auto found=f.content.weapons.find(weapon);
    if(found!=f.content.weapons.end() && found->second.name.ends_with("battle_rifle")){base.starting_weapon=option;rifle=weapon;}
  }catch(const FormatError &){}
  check(rifle!=null_datum,"No starting-weapon option selects the authored battle rifle");
  check(projectiles.contains(f.content.weapons.at(rifle).projectile),"Battle rifle projectile is not classified as precision hitscan");
  for(const auto &row:rows)if(row.name.ends_with("\\smg"))check(!row.admitted,"Automatic SMG classified as precision hitscan");
  f.structure=placement.load_bsp_surface_lighting(0);
  f.geometry=find_geometry(placement,f.content,*f.structure);
  if(verbose)std::cout<<"Geometry: shooter "<<f.geometry->shooter.position.x<<','<<f.geometry->shooter.position.y<<','
      <<f.geometry->shooter.position.z<<" A "<<f.geometry->a.x<<','<<f.geometry->a.y<<','<<f.geometry->a.z
      <<" hidden candidates "<<f.geometry->hidden.size()<<'\n';
  check(!f.geometry->hidden.empty(),"No hidden target position behind structure near a visible corridor");
}
// One world in this process; every scenario, page hashes at three cuts.
WorldReport run_world(const Fixture &f,NetcodeMode mode,bool exact_only) {
  const auto &geometry=*f.geometry;const auto &A=geometry.a,&B=geometry.b,&C=geometry.c;
  const auto sky=geometry.eye+Vec3{-geometry.right.x*3.f,-geometry.right.y*3.f,2.5f}+geometry.forward*2.f;
  WorldReport report;auto &shots=report.shots;
  Driver d(f.base,mode,geometry);
  report.pages["ready"]=page_hashes(d.world.source_objects->memory());
  {
    // Ranges a rewind replaces for the target: node bank, spheres, position.
    const auto &bank=d.world.source_objects->memory();
    const auto body=d.world.source_objects->source_object_body(d.target);
    const auto i16=[&](std::uint32_t at){std::int16_t v;std::memcpy(&v,bank.view(at,2).data(),2);return v;};
    report.target_state={{body+std::uint32_t(i16(body+0x116u)),std::uint32_t(i16(body+0x114u))},{body+0x30u,0x20u},{body+0x64u,12u}};
  }
  std::cout<<"World "<<netcode_mode_name(mode)<<" ready at tick "<<d.match.simulation().ticks();
  if(auto *lag=d.match.lag_compensation())std::cout<<" cap="<<lag->cap_ticks()<<" veto="<<lag->veto_ticks()<<" projectiles="<<lag->projectiles().size();
  std::cout<<'\n';
  // (d) A rewound shot that hits nothing: compare the whole bank.
  shots["exact"]=d.shoot(3,sky,{{-30,A},{-2,B}},[&](int offset) {
    if(offset==0)report.pages["before"]=page_hashes(d.world.source_objects->memory());
  });
  report.pages["after"]=page_hashes(d.world.source_objects->memory());
  if(!exact_only) {
    // Sanity: a stationary target, displayed at present, is hit in every mode.
    shots["present"]=d.shoot(0,A,{{-30,A}});
    // (a) Displayed 100 ms ago, at the cap.
    shots["a"]=d.shoot(3,A,{{-30,A},{-2,B}});
    report.lead=shots["a"].first_present_offset>-1000?shots["a"].first_present_offset:0;
    // (b) Displayed 200 ms ago: Balanced stops at the 100 ms cap.
    shots["b-old"]=d.shoot(6,A,{{-30,A},{-4,C},{-2,B}});
    shots["b-cap"]=d.shoot(6,C,{{-30,A},{-4,C},{-2,B}});
    // (c) Hidden since the tick after the displayed one (3 ticks > veto window);
    // the control hides one tick later, so it was exposed inside the window.
    for(std::size_t i=0;i<geometry.hidden.size();++i) {
      const auto key="c"+std::to_string(i);
      shots[key]=d.shoot(3,A,{{-30,A},{-3+report.lead,geometry.hidden[i]}});
      shots[key+"-exposed"]=d.shoot(3,A,{{-30,A},{-2+report.lead,geometry.hidden[i]}});
    }
  }
  if(const auto *s=d.stats()) {
    report.stats={s->queries,s->considered,s->rewound,s->vetoed,s->capped,s->no_history,
        s->membership_mismatches,s->restore_conflicts,s->restore_mismatches};
    std::cout<<"  stats queries="<<s->queries<<" considered="<<s->considered<<" rewound="<<s->rewound
        <<" vetoed="<<s->vetoed<<" capped="<<s->capped<<" no_history="<<s->no_history<<" membership_mismatches="<<s->membership_mismatches
        <<" restore_conflicts="<<s->restore_conflicts<<" restore_mismatches="<<s->restore_mismatches<<'\n';
  }
  for(const auto &[name,shot]:shots)std::cout<<"  "<<netcode_mode_name(mode)<<' '<<name<<": "<<describe(shot)<<'\n';
  return report;
}
void write_report(const WorldReport &r,const std::filesystem::path &path) {
  std::ofstream out(path);
  out<<"lead "<<r.lead<<"\nstats";for(auto v:r.stats)out<<' '<<v;out<<'\n';
  for(const auto &[name,shot]:r.shots) {
    out<<"shot "<<name<<' '<<shot.damaged<<' '<<shot.first_present_offset<<'\n';
    for(const auto &x:shot.records)out<<"rec "<<name<<' '<<x.present<<' '<<x.viewed<<' '<<x.target_tick<<' '<<x.rewind_ticks<<' '
        <<x.capped<<' '<<x.rewound<<' '<<x.rewound_hit<<' '<<x.vetoed<<' '<<x.hit<<' '<<x.targets<<' '<<x.result_type<<'\n';
  }
  for(const auto &[at,n]:r.target_state)out<<"target "<<at<<' '<<n<<'\n';
  for(const auto &[cut,pages]:r.pages)for(const auto &[at,h]:pages)out<<"page "<<cut<<' '<<at<<' '<<h<<'\n';
  check(bool(out),"Cannot write world report");
}
WorldReport read_report(const std::filesystem::path &path) {
  WorldReport r;std::ifstream in(path);std::string kind;
  check(bool(in),"World process wrote no report");
  while(in>>kind) {
    if(kind=="lead")in>>r.lead;
    else if(kind=="stats"){std::string line;std::getline(in,line);std::istringstream values(line);std::uint64_t v;while(values>>v)r.stats.push_back(v);}
    else if(kind=="shot"){std::string name;bool damaged;int offset;in>>name>>damaged>>offset;r.shots[name].damaged=damaged;r.shots[name].first_present_offset=offset;}
    else if(kind=="rec"){std::string name;Record x;in>>name>>x.present>>x.viewed>>x.target_tick>>x.rewind_ticks>>x.capped>>x.rewound
        >>x.rewound_hit>>x.vetoed>>x.hit>>x.targets>>x.result_type;r.shots[name].records.push_back(x);}
    else if(kind=="target"){std::uint32_t at,n;in>>at>>n;r.target_state.push_back({at,n});}
    else if(kind=="page"){std::string cut;std::uint32_t at;std::uint64_t h;in>>cut>>at>>h;r.pages[cut][at]=h;}
    else throw FormatError("Unknown world report entry "+kind);
  }
  return r;
}

// ---- Replay-mode migration (--replay) ----
using Sha=std::array<unsigned char,32>;
constexpr SourceMatchAuthority::AuthorityDigest replay_binding{std::byte{0x6c},std::byte{0xa9},std::byte{0x02}};
std::string hex(const Sha &d) {
  std::ostringstream out;out<<std::hex;
  for(unsigned i=0;i<8;++i)out<<unsigned(d[i]>>4)<<unsigned(d[i]&15);
  return out.str();
}
// SHA-256 of a complete authority checkpoint: every bank region byte, then
// the archive encoding of everything else (Match, rewind history, world
// owners). with_history=false drops only the rewind history, which isolates
// the simulated world from the retained frames.
Sha state_digest(SourceMatchAuthority::Checkpoint c,bool with_history=true) {
  std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(),EVP_MD_CTX_free);
  check(ctx && EVP_DigestInit_ex(ctx.get(),EVP_sha256(),nullptr)==1,"Cannot hash the authority checkpoint");
  const auto update=[&](const void *data,std::size_t n){check(EVP_DigestUpdate(ctx.get(),data,n)==1,"Cannot hash the authority checkpoint");};
  for(const auto &region:c.world.world.bank.regions) {
    const std::uint64_t header[4]{region.address,std::uint64_t(region.access),region.page_bytes,region.bytes.size()};
    update(header,sizeof(header));update(region.bytes.data(),region.bytes.size());
    update(region.committed.data(),region.committed.size());update(region.owner.data(),region.owner.size());
  }
  c.world.world.bank.regions.clear();
  if(!with_history)c.match.lag_compensation.reset();
  const auto rest=encode_source_authority(c);update(rest.data(),rest.size());
  Sha out{};unsigned n{};check(EVP_DigestFinal_ex(ctx.get(),out.data(),&n)==1 && n==out.size(),"Cannot hash the authority checkpoint");
  return out;
}
// Names the checkpoint fields that differ between a and b, by substituting
// one field of b into a copy of a at a time (diagnostics on a mismatch).
using Checkpoint=SourceMatchAuthority::Checkpoint;
template<class T,class Access,std::size_t... I>
void differing(const Checkpoint &a,const Checkpoint &b,Access access,std::string_view prefix,
    std::vector<std::string> &out,std::index_sequence<I...>) {
  constexpr auto names=boost::pfr::names_as_array<T>();
  const auto base=state_digest(a);
  ([&] {
    auto x=a;boost::pfr::get<I>(access(x))=boost::pfr::get<I>(access(const_cast<Checkpoint &>(b)));
    if(state_digest(std::move(x))!=base)out.push_back(std::string(prefix)+std::string(names[I]));
  }(),...);
}
template<class T,class Access>
void differing(const Checkpoint &a,const Checkpoint &b,Access access,std::string_view prefix,std::vector<std::string> &out) {
  differing<T>(a,b,access,prefix,out,std::make_index_sequence<boost::pfr::tuple_size_v<T>>{});
}
std::string explain(const Checkpoint &a,const Checkpoint &b) {
  std::vector<std::string> out;
  differing<MultiplayerMatch::AuthorityCheckpoint>(a,b,[](Checkpoint &c)->auto &{return c.match;},"match.",out);
  differing<PhysicsLiveWorldTu15::AuthorityCheckpoint>(a,b,[](Checkpoint &c)->auto &{return c.world.world;},"world.world.",out);
  if(a.world.death_reports.size()!=b.world.death_reports.size())out.push_back("world.death_reports");
  std::string text;for(const auto &name:out)text+=(text.empty()?"":",")+name;
  return text.empty()?std::string("no single field"):text;
}
bool same_record(const Record &a,const Record &b) {
  return a.projectile==b.projectile && a.definition==b.definition && a.shooter==b.shooter && a.site==b.site &&
      a.present==b.present && a.viewed==b.viewed && a.target_tick==b.target_tick && a.rewind_ticks==b.rewind_ticks &&
      a.hit==b.hit && a.result_type==b.result_type && a.targets==b.targets && a.capped==b.capped && a.rewound==b.rewound &&
      a.rewound_hit==b.rewound_hit && a.vetoed==b.vetoed &&
      std::bit_cast<std::array<std::uint32_t,3>>(a.point)==std::bit_cast<std::array<std::uint32_t,3>>(b.point);
}
int run_replay(const Fixture &f) {
  const auto &g=*f.geometry;const auto A=g.a,B=g.b;
  Driver host(f.base,NetcodeMode::balanced,g,true,0);
  // The replay world starts as an Original world: no rewind owner of its own.
  Driver replica(f.base,NetcodeMode::original,g,false,0);
  check(!replica.match.lag_compensation() && replica.match.netcode_mode()==NetcodeMode::original,
      "Original replay world installed a rewind owner");
  SourceMatchAuthority ha(host.match,*host.world.source_objects),ra(replica.match,*replica.world.source_objects);
  // Scenario (a) up to its last placement: the target stands at A, the
  // shooter displays the host 3 ticks behind and aims at A, and the target is
  // moved to B before the step producing F-2; the shot fires at F.
  host.latency=3;host.recharge();host.place(A);
  for(unsigned n=0;n<4;++n)host.step();
  host.aim=up(A,target_height*.55f);
  const auto fire=host.next()+38u;
  while(int(host.next())-int(fire)<-2)host.step();
  host.place(B);host.step();
  // The cut, one completed step after the last placement: the target's poses
  // at A now exist only in the rewind history.
  const auto cut=ha.checkpoint(replay_binding);
  const auto cut_tick=ha.execution_tick();
  check(cut.match.netcode_mode==NetcodeMode::balanced && cut.match.lag_compensation &&
      cut.match.lag_compensation->mode==NetcodeMode::balanced && !cut.match.lag_compensation->frames.empty(),
      "Balanced cut omitted its netcode mode or rewind history");
  const auto &frames=cut.match.lag_compensation->frames;
  std::cout<<"Replay cut at tick "<<cut_tick<<": rewind history "<<frames.size()<<" frames (ticks "<<frames.front().tick
      <<".."<<frames.back().tick<<", "<<frames.back().bodies.size()<<" bodies), encoded cut "<<encode_source_authority(cut).size()<<" bytes";
  {auto bare=cut;bare.match.lag_compensation.reset();std::cout<<" ("<<encode_source_authority(bare).size()<<" without the history)\n";}
  const auto cut_state=state_digest(cut),cut_world=state_digest(cut,false);
  // Record every tick from the cut through the shot's result as replay phases.
  std::vector<SourceReplayStep> steps;std::vector<Sha> expected,expected_world;
  std::vector<std::array<Controls,multiplayer_capacity>> inputs;std::vector<NetcodeViewTime> views;
  std::size_t shot_phase=SIZE_MAX;host.records.clear();
  std::vector<Checkpoint> host_cuts; // Diagnostics: which field a mismatching replay changed.
  const auto before_shot=host.match.state().players[1];
  float shield=before_shot.shield,health=before_shot.health;
  for(int offset=-1;offset<24;++offset) {
    const auto before=ha.execution_tick();const auto records=host.records.size();
    auto phase=ha.capture_execution_phase([&] {
      host.step(offset==0);
      const auto s=host.match.state().players[1];shield=std::min(shield,s.shield);health=std::min(health,s.health);
    });
    if(shot_phase==SIZE_MAX && std::any_of(host.records.begin()+std::ptrdiff_t(records),host.records.end(),
        [](const Record &r){return r.rewound;}))shot_phase=steps.size();
    steps.push_back({before,ha.execution_tick(),std::move(phase)});
    auto now=ha.checkpoint(replay_binding);
    expected_world.push_back(state_digest(now,false));expected.push_back(state_digest(now));
    host_cuts.push_back(std::move(now));
    inputs.push_back(host.last_input);views.push_back(host.last_view);
  }
  const Record *shot=nullptr;for(const auto &r:host.records)if(r.rewound){shot=&r;break;}
  const bool damaged=shield<before_shot.shield-1e-4f || health<before_shot.health-1e-4f;
  check(shot && shot_phase!=SIZE_MAX && shot->rewound_hit && !shot->vetoed && shot->rewind_ticks==3 && damaged &&
      shot->target_tick<=frames.back().tick,"Balanced host did not land a rewound hit from the cut's history");
  std::cout<<"Host shot: present="<<shot->present<<" viewed="<<shot->viewed<<" target="<<shot->target_tick
      <<" rewind="<<shot->rewind_ticks<<" rewound_hit="<<shot->rewound_hit<<" hit="<<std::hex<<shot->hit<<std::dec
      <<" shield "<<before_shot.shield<<"->"<<shield<<" health "<<before_shot.health<<"->"<<health<<'\n';
  // Every tick, the rewound one included, records the shooter's displayed view
  // with the step, in the documented argument encoding.
  unsigned view_steps=0;
  for(std::size_t i=0;i<steps.size();++i)for(const auto &call:steps[i].phase.calls) {
    check(call.kind!=SourceExecutionCallKind::MatchStep,"A tick with a remote view recorded the view-less MatchStep");
    if(call.kind!=SourceExecutionCallKind::MatchStepViews)continue;
    std::array<std::optional<NetcodeViewTime>,multiplayer_capacity> v{};v[0]=views[i];
    check(call.arguments==source_execution_arguments(inputs[i],NetcodeMode::balanced,v),
        "Recorded step views differ from the shooter's displayed view");
    ++view_steps;
  }
  check(view_steps==steps.size(),"Not every recorded tick carries its displayed views");
  // The network path of the replay publisher: Rust capability policy and the
  // replay batch codec, as one batch from the cut's seal.
  AuthorityTransferContext context;context.session[0]=std::byte{1};context.epoch=1;context.membership_revision=1;
  context.peers=3;context.host=0;
  const AuthorityCutSeal seal{1,cut_tick,1,{}};
  const SourceReplayBinding bound{context,seal,seal,1};
  const auto wire=encode_source_replay_batch({bound,steps});
  const auto received=decode_source_replay_batch(wire,bound);
  check(received.steps.size()==steps.size(),"Replay batch lost a recorded tick");
  std::cout<<"Recorded "<<steps.size()<<" ticks as replay phases ("<<view_steps<<" with displayed views; the rewound shot in tick "
      <<steps[shot_phase].after_tick<<"); replay batch "<<wire.size()<<" bytes; no tick unrecorded, so no full capture is forced\n";

  const auto import=[&](const SourceMatchAuthority::Checkpoint &c) {
    auto prepared=ra.prepare(c,replay_binding);check(ra.commit(std::move(prepared)),"Replay world did not commit the cut");
  };
  // First divergence from the host when replaying `tape`, by world digest
  // (with_history=false) or by the complete checkpoint digest.
  std::string divergence;
  const auto replay=[&](const std::vector<SourceReplayStep> &tape,bool with_history)->std::optional<std::size_t> {
    for(std::size_t i=0;i<tape.size();++i) {
      try{ra.replay_execution_phase(tape[i].phase);}
      catch(const std::exception &error){divergence=std::string("replay rejected: ")+error.what();return i;}
      auto now=ra.checkpoint(replay_binding);
      if(state_digest(now,with_history)!=(with_history?expected:expected_world)[i]) {
        divergence="checkpoint digest differs";
        if(with_history)divergence+=" in "+explain(now,host_cuts.at(i));
        return i;
      }
    }
    divergence.clear();return std::nullopt;
  };
  // An Original cut removes an adopted owner again: the replay world mirrors
  // the producer's netcode, and Original steps keep the view-less MatchStep.
  import(cut);
  check(replica.match.lag_compensation() && replica.match.netcode_mode()==NetcodeMode::balanced,"Balanced cut did not install a rewind owner");
  auto original_cut=cut;original_cut.match.netcode_mode=NetcodeMode::original;original_cut.match.lag_compensation.reset();
  import(original_cut);
  check(!replica.match.lag_compensation() && replica.match.netcode_mode()==NetcodeMode::original,"Original cut kept a rewind owner");
  {
    const auto phase=ra.capture_execution_phase([&]{replica.step();});
    check(std::ranges::any_of(phase.calls,[](const auto &c){return c.kind==SourceExecutionCallKind::MatchStep;}) &&
        std::ranges::none_of(phase.calls,[](const auto &c){return c.kind==SourceExecutionCallKind::MatchStepViews;}),
        "An Original world recorded displayed views with its step");
  }
  // Controls: the rewind history and the recorded views are both load-bearing.
  auto stripped=cut;stripped.match.lag_compensation->frames.clear();
  import(stripped);
  const auto without_history=replay(received.steps,false);
  const auto history_reason=divergence;
  check(without_history && *without_history>=shot_phase,"Replay without the cut's rewind history still matched the host");
  auto present_view=received.steps;
  for(auto &call:present_view[shot_phase].phase.calls)if(call.kind==SourceExecutionCallKind::MatchStepViews) {
    std::array<std::optional<NetcodeViewTime>,multiplayer_capacity> v{};
    v[0]=NetcodeViewTime{std::uint16_t(present_view[shot_phase].before_tick&65535u),0};
    call.arguments=source_execution_arguments(inputs[shot_phase],NetcodeMode::balanced,v);
  }
  import(cut);
  const auto without_view=replay(present_view,false);
  check(without_view && *without_view>=shot_phase,"Replay with an undelayed view still matched the host");
  std::cout<<"Controls: without the cut's history the replay diverges at tick "<<received.steps[*without_history].after_tick
      <<" ("<<history_reason<<"); with the shot's view at present at tick "<<received.steps[*without_view].after_tick
      <<" ("<<divergence<<")\n";

  // The replay itself: byte for byte after every tick, the same rewound query.
  import(cut);
  {
    const auto imported=ra.checkpoint(replay_binding);
    check(state_digest(imported)==cut_state && state_digest(imported,false)==cut_world,
        "Imported cut differs from the host's in "+explain(imported,cut));
  }
  replica.records.clear();replica.observe_rewinds();
  const auto diverged=replay(received.steps,true);
  check(!diverged,"Replay world differs from the host after tick "+std::to_string(diverged?received.steps[*diverged].after_tick:0)+": "+divergence);
  check(replica.records.size()==host.records.size() &&
      std::ranges::equal(replica.records,host.records,same_record),"Replay world resolved a different rewound query");
  const auto *ls=replica.match.lag_compensation();
  std::cout<<"Replay world matched the host after all "<<received.steps.size()<<" ticks (final "<<hex(expected.back())
      <<"); rewound queries "<<ls->stats().rewound<<" (host "<<host.stats()->rewound<<"), records "<<replica.records.size()<<'\n';

  // Promotion: the replay world continues as the host, in the host's mode,
  // rewinding with the history it replayed and keeps recording.
  replica.shooter=host.shooter;replica.target=host.target;replica.yaw=host.yaw;replica.pitch=host.pitch;
  replica.target_yaw=host.target_yaw;replica.aim=host.aim;replica.aiming=true;
  check(replica.match.state().netcode_mode==NetcodeMode::balanced,"Promoted host does not publish the Balanced mode");
  const auto promoted=replica.shoot(3,A,{{-30,A},{-2,B}});
  const auto *hit=first_rewound(promoted);
  check(hit && hit->rewind_ticks==3 && hit->rewound_hit && !hit->vetoed && promoted.damaged,
      "Promoted host lost Balanced rewind: "+describe(promoted));
  std::cout<<"Promoted host: "<<describe(promoted)<<'\n';
  std::cout<<"PASS lag-compensated replay: a Balanced host's rewound hit is recorded with its displayed views, replayed "
      "byte for byte from the cut's rewind history, and the promoted Original-built world keeps Balanced rewind\n";
  return 0;
}
}

int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;
  // Child: one world, the first one built in this process.
  if(argc==8 && std::string_view(argv[1])=="--world") {
    const auto mode=parse_netcode_mode(argv[2]);check(mode.has_value(),"Unknown world mode");
    Fixture f;prepare(f,argv[4],argv[5],argv[6],false);
    write_report(run_world(f,*mode,std::string_view(argv[3])=="exact"),argv[7]);
    return 0;
  }
  const bool replay=argc==5 && std::string_view(argv[4])=="--replay";
  if((argc!=4 && !replay) || !std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_directory(argv[2]))return 77;
  if(replay) {
    ::unsetenv("H2_NET_MODE");
    Fixture fixture;prepare(fixture,argv[1],argv[2],argv[3],true);
    return run_replay(fixture);
  }
  // Mode selection: H2_NET_MODE overrides only with a valid name.
  ::unsetenv("H2_NET_MODE");
  check(resolve_netcode_mode(NetcodeMode::balanced)==NetcodeMode::balanced,"Unset H2_NET_MODE changed the selection");
  ::setenv("H2_NET_MODE","full",1);check(resolve_netcode_mode(NetcodeMode::original)==NetcodeMode::full,"H2_NET_MODE=full ignored");
  ::setenv("H2_NET_MODE","bogus",1);check(resolve_netcode_mode(NetcodeMode::balanced)==NetcodeMode::balanced,"Invalid H2_NET_MODE admitted");
  ::unsetenv("H2_NET_MODE");
  check(default_netcode_mode==NetcodeMode::balanced && MultiplayerOptions{}.netcode_mode==NetcodeMode::balanced,"Default mode is not Balanced");
  // Displayed-time quantization and full-tick recovery.
  check(netcode_view_time(41,42,.5f)==NetcodeViewTime{41,128} && netcode_view_time(41,42,1.f)==NetcodeViewTime{42,0} &&
      netcode_view_time(0x1fffe,0x1ffff,.999f)==NetcodeViewTime{0xffff,0},"Displayed-time quantization differs");
  check(netcode_view_tick({0xfffe,0},0x20001)==0x1fffeu && netcode_view_tick({5,0},0x10005)==0x10005u &&
      !netcode_view_tick({9,0},5),"Displayed-tick recovery differs");
  Fixture fixture;prepare(fixture,argv[1],argv[2],argv[3],true);
  const auto &geometry=*fixture.geometry;const auto &A=geometry.a;

  // Each world runs in its own process, so it is the first world built there:
  // two Original worlds are then byte-identical and the comparison is exact.
  Files reports;
  const auto world=[&](const char *mode,bool exact,const std::string &name) {
    const auto path=reports.path/(name+".report");
    std::vector<std::string> args{argv[0],"--world",mode,exact?"exact":"all",argv[1],argv[2],argv[3],path.string()};
    std::vector<char*> raw;for(auto &a:args)raw.push_back(a.data());raw.push_back(nullptr);
    pid_t pid{};check(::posix_spawn(&pid,argv[0],nullptr,nullptr,raw.data(),environ)==0,"Cannot start world process");
    int status{};check(::waitpid(pid,&status,0)==pid,"World process lost");
    check(WIFEXITED(status) && WEXITSTATUS(status)==0,std::string("World process failed: ")+name);
    return read_report(path);
  };
  auto original=world("original",false,"original");
  auto baseline=world("original",true,"baseline");
  auto balanced=world("balanced",false,"balanced");
  auto full=world("full",false,"full");
  auto &original_shots=original.shots,&balanced_shots=balanced.shots,&full_shots=full.shots;
  const auto lead=balanced.lead;

  // (d) Whole-bank identity around a rewound shot that hits nothing.
  const auto *exact=first_rewound(balanced_shots["exact"]);
  check(exact && exact->targets && !exact->rewound_hit,"Exact-restoration shot did not apply a rewind: "+describe(balanced_shots["exact"]));
  // Every committed page, with only the characterized nondeterministic bytes
  // masked, must be identical between two Original worlds and the Balanced one.
  for(const auto &[at,n]:original.target_state)for(const auto &[mask,size]:nondeterministic_bytes)
    check(std::uint64_t(at)+n<=mask || std::uint64_t(mask)+size<=at,"Rewound target state overlaps a masked byte");
  for(const auto *cut:{"ready","before","after"}) {
    const auto noise=differing(original.pages[cut],baseline.pages[cut]),diff=differing(original.pages[cut],balanced.pages[cut]);
    std::cout<<"(d) "<<cut<<": "<<original.pages[cut].size()<<" bank pages ("<<nondeterministic_bytes.size()
        <<" masked runs); Original vs Original differing "<<noise.size()<<", Original vs Balanced differing "<<diff.size()<<'\n';
    for(const auto *pages:{&noise,&diff})for(std::size_t i=0;i<std::min<std::size_t>(pages->size(),8);++i)
      std::cout<<"    "<<(pages==&noise?"Original":"Balanced")<<" differing page "<<std::hex<<(*pages)[i]<<std::dec<<'\n';
    check(noise.empty(),std::string("Two Original worlds differ outside the characterized nondeterministic bytes at ")+cut);
    check(diff.empty(),std::string("Rewound shot left world state different from the Original world at ")+cut);
  }
  check(!original_shots["exact"].damaged && !balanced_shots["exact"].damaged,"Exact-restoration shot damaged the target");
  check(balanced.stats.size()==9 && full.stats.size()==9,"Lag compensation statistics missing");
  check(balanced.stats[7]==0 && full.stats[7]==0,"A rewound query wrote into replaced target state");
  check(balanced.stats[8]==0 && full.stats[8]==0,"Rewound target state was not restored exactly");

  for(auto *shots:{&original_shots,&balanced_shots,&full_shots})
    check((*shots)["present"].damaged,"A stationary target at the aim point was not hit: "+describe((*shots)["present"]));
  // (a)
  check(!original_shots["a"].damaged,"Original hit a target at its 100 ms old position: "+describe(original_shots["a"]));
  const auto *a=first_rewound(balanced_shots["a"]);
  check(a && a->rewind_ticks==3 && !a->capped && a->rewound_hit && balanced_shots["a"].damaged,
      "Balanced missed the target at its displayed 100 ms position: "+describe(balanced_shots["a"]));
  check(full_shots["a"].damaged,"Full missed the target at its displayed 100 ms position");
  // (b)
  const auto *old=first_rewound(balanced_shots["b-old"]);
  check(old && old->capped && old->rewind_ticks==3 && !old->rewound_hit && !balanced_shots["b-old"].damaged,
      "Balanced rewound beyond its cap: "+describe(balanced_shots["b-old"]));
  const auto *cap=first_rewound(balanced_shots["b-cap"]);
  check(cap && cap->capped && cap->rewind_ticks==3 && cap->rewound_hit && balanced_shots["b-cap"].damaged,
      "Balanced did not rewind exactly to its cap: "+describe(balanced_shots["b-cap"]));
  check(!original_shots["b-old"].damaged && !original_shots["b-cap"].damaged,"Original hit a past target position");
  const auto *full_old=first_rewound(full_shots["b-old"]);
  check(full_old && !full_old->capped && full_old->rewind_ticks==6 && full_old->rewound_hit && full_shots["b-old"].damaged,
      "Full's higher cap did not reach the 200 ms position: "+describe(full_shots["b-old"]));
  // (c) The first hidden candidate whose rewound query found the target.
  bool veto_checked{};
  for(std::size_t i=0;i<geometry.hidden.size() && !veto_checked;++i) {
    const auto key="c"+std::to_string(i);
    const auto *full=first_rewound(full_shots[key]),*rewound=first_rewound(balanced_shots[key]);
    if(!full || !full->rewound_hit || !rewound || !rewound->rewound_hit)continue;
    check(full_shots[key].damaged,"Full did not keep its rewound hit: "+describe(full_shots[key]));
    check(rewound->vetoed && !balanced_shots[key].damaged,"Balanced cover veto did not reject a long-hidden target: "+describe(balanced_shots[key]));
    check(!original_shots[key].damaged,"Original hit a target behind structure");
    const auto *exposed=first_rewound(balanced_shots[key+"-exposed"]);
    check(exposed && exposed->rewound_hit && !exposed->vetoed && balanced_shots[key+"-exposed"].damaged,
        "Balanced vetoed a target exposed inside the window: "+describe(balanced_shots[key+"-exposed"]));
    std::cout<<"(c) hidden candidate "<<i<<" at "<<distance(geometry.hidden[i],A)<<" wu from A\n";
    veto_checked=true;
  }
  check(veto_checked,"No hidden candidate produced a rewound hit to veto");
  std::cout<<"PASS lag-compensated hitscan: Balanced rewinds a remote battle rifle within 100 ms, caps a 200 ms view at "
      "100 ms, vetoes a target hidden beyond 70 ms, keeps one exposed inside it, and leaves the bank byte-identical "
      "to an Original world (outside 131 bytes that vary between Original worlds too); "
      "Original is unchanged and Full reaches 200 ms without a veto (fire lead "<<lead<<" ticks)\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
