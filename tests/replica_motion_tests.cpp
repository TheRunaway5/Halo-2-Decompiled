// Non-host dead reckoning (NETCODE_ENHANCED_GOAL item 3b) and the replicated
// physics impulse journal (item 4a): vector publication, journal retention
// and validation, first-frame reaction to an impulse, ballistic agreement
// with the next authoritative snapshot and continuous corrections.
#include "halo2/replica_motion.hpp"
#include "halo2/physics_impulse_journal.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/kernels.hpp"
#include "halo2/math.hpp"
#include <bit>
#include <cmath>
#include <functional>
#include <limits>
#include <iostream>
#include <string>

using namespace halo2;
namespace {
void check(bool yes,const std::string &message){if(!yes)throw std::runtime_error(message);}
void rejects(const std::function<void()> &f,const std::string &message) {
  bool rejected=false;
  try{f();}catch(const FormatError &){rejected=true;}
  check(rejected,message);
}
using Kind=ReplicaMotion::Kind;
constexpr double frame=1.0/60.0,tick=1.0/30.0;
float distance(Vec3 a,Vec3 b){return length(a-b);}

void vector_quantization() {
  const auto value=[](Vec3 v){return physics_impulse_quantize(v);};
  const auto same=[](Vec3 a,Vec3 b){return a.x==b.x && a.y==b.y && a.z==b.z;};
  check(same(value({2,.5f,1.5f}),{2,.5f,1.5f}) && same(value({40,-3,12.5f}),{40,-3,12.5f}),"Exact published vectors changed");
  check(same(value({6,.3f,3}),{6,.3125f,3}) && same(value({1.0001f,.5f,1.5f}),{1,.5f,1.5f}),
      "Published vectors round to 1/127 of their largest component");
  const auto bits=physics_impulse_vector_bits({127,-127,0});
  check(bits[0]==0 && std::int8_t(bits[1])==127 && std::int8_t(bits[2])==-127 && bits[3]==0,"Vector mantissa layout");
  check(physics_impulse_vector_bits({127.6f,0,0})[0]==1,"Rounding past 127 takes the next exponent");
  check(same(value({}),{}) && physics_impulse_vector_bits({})[0]==std::uint8_t(std::int8_t(-24)),"Zero vector form");
  check(std::bit_cast<std::uint32_t>(value({-0.f,1,0}).x)==0,"Negative zero publishes as zero");
  check(same(value({1e30f,0,0}),{127*16777216.f,0,0}),"Vector saturates at the largest exponent");
  // Every published vector is a fixed point of publication.
  std::uint32_t seed=7;unsigned checked=0;
  for(unsigned n=0;n<20000;++n) {
    const auto next=[&]{seed=seed*1664525u+1013904223u;return std::ldexp(float(std::int32_t(seed))/2147483648.f,int(seed%40)-20);};
    const auto once=value({next(),next(),next()});
    check(same(value(once),once),"Published vector is not a fixed point");++checked;
  }
  check(checked==20000,"Vector census");
  rejects([&]{physics_impulse_vector_bits({std::numeric_limits<float>::infinity(),0,0});},"Infinite impulse value accepted");
  rejects([&]{physics_impulse_vector_bits({0,std::numeric_limits<float>::quiet_NaN(),0});},"NaN impulse value accepted");
  rejects([&]{physics_impulse_vector_value({25,0,0,0});},"Out-of-range impulse exponent decoded");
  rejects([&]{physics_impulse_vector_value({0,0x80,0,0});},"Out-of-range impulse mantissa decoded");
}
PhysicsImpulseEvent event(std::uint32_t serial,std::uint32_t tick_value,std::uint32_t object) {
  PhysicsImpulseEvent e;e.serial=serial;e.tick=tick_value;e.object=object;
  e.linear_velocity={2,.5f,1.5f};e.angular_velocity={0,0,4};
  return e;
}
void journal_validation() {
  const std::uint32_t at=100;
  std::vector<PhysicsImpulseEvent> valid{event(7,95,0x00120004u),event(8,100,0x00120004u),event(9,100,0x00130005u)};
  validate_physics_impulse_journal(valid,at);
  validate_physics_impulse_journal({},at);
  const auto mutated=[&](const std::function<void(std::vector<PhysicsImpulseEvent>&)> &change,const char *what) {
    auto rows=valid;change(rows);
    rejects([&]{validate_physics_impulse_journal(rows,at);},what);
  };
  mutated([](auto &r){r[0].serial=0;},"Zero impulse serial accepted");
  mutated([](auto &r){r[1].serial=7;},"Repeated impulse serial accepted");
  mutated([](auto &r){r[2].serial=6;},"Decreasing impulse serial accepted");
  mutated([](auto &r){r[2].serial=11;},"Non-contiguous impulse serial accepted");
  mutated([](auto &r){r[2].tick=101;},"Impulse after its snapshot accepted");
  mutated([](auto &r){r[0].tick=at-301;},"Impulse older than retention accepted");
  mutated([](auto &r){r[1].tick=94;},"Impulse ticks out of serial order accepted");
  mutated([](auto &r){r[0].object=null_datum;},"NONE impulse object accepted");
  mutated([](auto &r){r[0].object=0x00000004u;},"Unsalted impulse object accepted");
  mutated([](auto &r){r[0].object=0x00120800u;},"Impulse object outside the pool accepted");
  mutated([](auto &r){r[2].object=r[1].object;},"Repeated object within one tick accepted");
  mutated([](auto &r){r[1].linear_velocity.x=1.0001f;},"Unpublished impulse velocity accepted");
  mutated([](auto &r){r[1].angular_velocity.z=std::numeric_limits<float>::infinity();},"Non-finite impulse spin accepted");
  mutated([](auto &r){for(unsigned n=0;n<30;++n)r.push_back(event(10+n,100,0x00140000u+n));},
      "Impulse journal beyond capacity accepted");
}
void retention() {
  std::vector<PhysicsImpulseEvent> journal;std::uint32_t serial=0;
  auto fresh=event(0,0,0x00210001u);fresh.linear_velocity.x=1.0001f; // Quantized on retention.
  retain_physics_impulses(journal,serial,std::vector{fresh,event(0,0,0x00210002u),event(0,0,0x00210003u)},10);
  check(journal.size()==3 && journal[0].serial==1 && journal[2].serial==3 && journal[2].tick==10 &&
      journal[0].linear_velocity.x==1.f,"Retention assigns serials, ticks and binary16 values");
  validate_physics_impulse_journal(journal,10);
  retain_physics_impulses(journal,serial,{},310);
  check(journal.size()==3,"Events within 300 ticks are retained");
  retain_physics_impulses(journal,serial,{},311);
  check(journal.empty(),"Events older than 300 ticks retire");
  std::vector<PhysicsImpulseEvent> burst;
  for(unsigned n=0;n<40;++n)burst.push_back(event(0,0,0x00220000u+n));
  retain_physics_impulses(journal,serial,burst,320);
  check(journal.size()==physics_impulse_journal_capacity && journal.front().serial==12 && journal.back().serial==43 &&
      journal.front().object==0x00220008u,"Capacity retains the newest 32 events");
  validate_physics_impulse_journal(journal,320);
}
WorldWeaponPresentation loose_weapon(std::uint32_t handle,Vec3 position) {
  WorldWeaponPresentation w;w.handle=handle;w.definition=3;w.position=position;w.forward={1,0,0};w.up={0,0,1};
  return w;
}
// A loose weapon at rest is shot on tick 202. The non-host replica starts
// moving on its first presented frame after that snapshot; an identical
// replica without the event stays put.
void impulse_reaction() {
  const std::uint32_t handle=0x12340005u;const Vec3 rest{1,2,3};
  ReplicaMotion shot,quiet;
  MultiplayerState s;s.world_weapons={loose_weapon(handle,rest)};
  double clock=10;
  for(std::uint32_t t=200;t<202;++t,clock+=tick){s.tick=t;shot.receive(s,clock);quiet.receive(s,clock);}
  s.tick=202;
  quiet.receive(s,clock);
  s.physics_impulses={event(1,202,handle)};
  shot.receive(s,clock);
  const auto at_receipt=shot.root(Kind::world_weapon,handle,clock);
  check(at_receipt && distance(at_receipt->position,rest)<1e-5f,"Impulse snapshot receipt is continuous");
  const auto first=shot.root(Kind::world_weapon,handle,clock+frame);
  const auto still=quiet.root(Kind::world_weapon,handle,clock+frame);
  check(still && distance(still->position,rest)==0 && still->basis[0].x==1,"Resting replica without an impulse moved");
  // Launched upward: event velocity plus the Havok gravity step.
  const auto e=float(frame);const auto v=event(1,202,handle).linear_velocity;
  const Vec3 expected=rest+v*e+Vec3{0,0,0.f-retail_gravity}*(.5f*e*(e+float(tick)));
  check(first && distance(first->position,expected)<1e-5f && distance(first->position,rest)>.03f,
      "Shot replica did not start moving on the first presented frame");
  const float yaw=std::atan2(first->basis[0].y,first->basis[0].x);
  check(std::abs(yaw-4*e)<1e-4f && std::abs(first->basis[2].z-1)<1e-5f,"Shot replica did not start turning");
  check(shot.statistics().impulses==1 && quiet.statistics().impulses==0,"Impulse application count");
  // Presentation copies and posed nodes follow the same motion.
  auto shown=s;shot.present(shown,clock+frame);
  check(distance(shown.world_weapons[0].position,first->position)==0,"Presented root differs from the replica");
  const auto motions=shot.node_motions(clock+frame);
  check(motions.size()==1 && motions[0].first==handle,"Shot object lacks a node motion");
  SourceWorldInterpolation nodes;
  Transform node0=source_object_transform(rest,{1,0,0},{0,0,1},1,false),node1=node0;node1.position=rest+Vec3{0,0,.5f};
  nodes.reset(202,std::vector{SourceWorldPose{handle,{node0,node1},{}}});
  nodes.present(motions);
  const auto posed=nodes.sample(handle,.25f);
  check(distance(posed[0].position,first->position)<1e-5f && distance(posed[1].position-posed[0].position,
      first->basis[2]*.5f)<1e-5f,"WORLD nodes did not move rigidly with the replica");
  nodes.present({});
  check(distance(nodes.sample(handle,.25f)[0].position,rest)==0,"Cleared node motion persisted");
  // A retained copy of the same event is not applied twice.
  s.tick=203;s.world_weapons[0].position=first->position;shot.receive(s,clock+tick);
  check(shot.statistics().impulses==1,"Retained impulse applied twice");
  // Every loose physics kind reacts; occupied vehicles and players never do.
  for(const auto kind:{Kind::vehicle,Kind::dynamic_object,Kind::scene_root}) {
    ReplicaMotion replica;MultiplayerState state;const std::uint32_t object=0x00450007u;
    Transform root=source_object_transform(rest,{1,0,0},{0,0,1},1,false);
    if(kind==Kind::vehicle){WorldVehiclePresentation v;v.handle=object;v.transform=root;state.world_vehicles={v};}
    if(kind==Kind::dynamic_object){SourceDynamicObject o;o.object=object;o.transform=root;state.source_dynamic_objects={o};}
    if(kind==Kind::scene_root){SourceSceneRoot r;r.object=object;r.transform=root;r.visible=true;state.source_scene_roots={r};}
    state.tick=50;replica.receive(state,0);
    state.tick=51;state.physics_impulses={event(3,51,object)};replica.receive(state,tick);
    const auto moved=replica.root(kind,object,tick+frame);
    check(moved && distance(moved->position,rest)>.03f,"Loose physics kind did not react to its impulse");
  }
  {
    ReplicaMotion replica;MultiplayerState state;const std::uint32_t vehicle=0x00460008u;
    WorldVehiclePresentation v;v.handle=vehicle;v.transform=source_object_transform(rest,{1,0,0},{0,0,1},1,false);
    v.lighting_owner=vehicle;v.lighting_velocity={5,0,0};state.world_vehicles={v};
    state.players[0].connected=true;state.players[0].source_parent=vehicle;state.players[0].position=rest;
    state.players[0].velocity={5,0,0};
    state.tick=60;replica.receive(state,0);
    check(!replica.root(Kind::vehicle,vehicle,frame) && replica.tracked()==0,"Occupied vehicle was dead-reckoned");
    auto shown=state;replica.present(shown,frame);
    check(distance(shown.players[0].position,rest)==0 && distance(shown.world_vehicles[0].transform.position,rest)==0,
        "Player or occupied vehicle presentation changed");
  }
}
// The original F8EB0 projectile flight against the replica's prediction:
// each dead-reckoned position just before the next snapshot must match it.
// Once warmed up (velocity/spin/gravity known and any first correction
// blended out), every snapshot lands within rounding of the prediction, so
// no further correction occurs.
void ballistic_projectile() {
  const std::uint32_t datum=0x00610003u,definition=0xe1230456u;const float gravity=.5f*retail_gravity;
  for(const bool authored:{true,false}) {
    ReplicaMotion replica({},authored?ReplicaMotion::ProjectileGravity{[&](std::uint32_t d)->std::optional<float>{
      return d==definition?std::optional{gravity}:std::nullopt;}}:ReplicaMotion::ProjectileGravity{});
    Vec3 position{-10,4,2},velocity{25,-3,9};float traveled{};
    const std::uint32_t warm=authored?0:7;float worst{},first{};std::uint64_t settled{};
    for(std::uint32_t k=0;k<30;++k) {
      MultiplayerState s;s.tick=1000+k;
      s.weapon_projectiles={{datum,definition,position,velocity,normalized(velocity),{0,0,1},traveled}};
      const double at=50+k*tick;
      replica.receive(s,at);
      if(k==warm)settled=replica.statistics().corrections;
      const auto flight=projectile_flight(position,velocity,gravity,tick_seconds,traveled,0);
      const auto predicted=replica.root(Kind::weapon_projectile,datum,at+tick);
      check(bool(predicted),"Projectile was not tracked");
      const float error=distance(predicted->position,flight.position);
      if(k==0)first=error;
      if(k>=warm)worst=std::max(worst,error);
      position=flight.position;velocity=flight.velocity;traveled=flight.traveled;
    }
    std::cout<<"Projectile dead reckoning ("<<(authored?"authored":"measured")<<" gravity): first tick "<<first
        <<" wu, from tick "<<warm<<" worst "<<worst<<" wu per tick, corrections "<<replica.statistics().corrections<<"\n";
    check(worst<2e-4f && replica.statistics().corrections==settled,
        "Dead-reckoned projectile diverged from its next authoritative snapshot");
    check(authored?first<2e-4f:first<.5f*gravity*float(tick*tick)+2e-4f,"First projectile prediction error");
  }
  // A Havok prop in free flight: replicated (or snapshot-derived) linear
  // velocity, spin measured from consecutive orientations, gravity detected
  // from the authority's own velocity change.
  for(const bool replicated:{true,false}) {
    ReplicaMotion replica;const std::uint32_t object=0x00620009u;
    Vec3 position{3,3,6},velocity{1.5f,-2,4};const Vec3 spin{.7f,-.4f,2.5f};
    Transform root=source_object_transform(position,{1,0,0},{0,0,1},1,false);
    const std::uint32_t warm=18;float worst_position{},worst_angle{};std::uint64_t settled{};
    for(std::uint32_t k=0;k<32;++k) {
      MultiplayerState s;s.tick=300+k;
      if(replicated){SourceDynamicObject o;o.object=object;o.transform=root;o.velocity=velocity;s.source_dynamic_objects={o};}
      else s.world_weapons={WorldWeaponPresentation{object,3,root.position,root.basis[0],root.basis[2]}};
      const double at=7+k*tick;replica.receive(s,at);
      if(k==warm)settled=replica.statistics().corrections;
      // Havok: gravity enters the velocity, then the pose integrates it.
      velocity.z-=retail_gravity*tick_seconds;position=position+velocity*tick_seconds;
      const float angle=length(spin)*tick_seconds;const auto axis=normalized(spin);
      const auto turn=[&](Vec3 v){return v*std::cos(angle)+cross(axis,v)*std::sin(angle)+axis*(dot(axis,v)*(1-std::cos(angle)));};
      Transform next=root;next.position=position;
      for(auto &b:next.basis)b=normalized(turn(b));
      next.basis[1]=cross(next.basis[2],next.basis[0]);
      const auto predicted=replica.root(replicated?Kind::dynamic_object:Kind::world_weapon,object,at+tick);
      check(bool(predicted),"Prop was not tracked");
      if(k>=warm) {
        worst_position=std::max(worst_position,distance(predicted->position,next.position));
        for(unsigned c=0;c<3;++c)worst_angle=std::max(worst_angle,distance(predicted->basis[c],next.basis[c]));
      }
      root=next;
    }
    std::cout<<"Havok prop dead reckoning ("<<(replicated?"replicated":"derived")<<" velocity): from tick "<<warm
        <<" worst "<<worst_position<<" wu, basis "<<worst_angle<<" per tick, corrections "<<replica.statistics().corrections<<"\n";
    check(worst_position<2e-4f && worst_angle<2e-4f && replica.statistics().corrections==settled,
        "Dead-reckoned prop diverged from its next authoritative snapshot");
  }
}
// A newer snapshot that disagrees with the prediction blends in without a
// discontinuity; a large error snaps.
void correction_blend() {
  const std::uint32_t object=0x00710002u;
  ReplicaMotion replica;
  const auto state=[&](std::uint32_t t,Vec3 position,Vec3 velocity,Vec3 forward) {
    MultiplayerState s;s.tick=t;SourceDynamicObject o;o.object=object;o.velocity=velocity;
    o.transform=source_object_transform(position,normalized(forward),{0,0,1},1,false);
    s.source_dynamic_objects={o};return s;
  };
  replica.receive(state(10,{0,0,0},{10,0,0},{1,0,0}),0);
  replica.receive(state(11,{10*float(tick),0,0},{10,0,0},{1,0,0}),tick);
  // The host's body hit something: bounced and turned.
  const double at=2*tick+.004;
  const auto before=replica.root(Kind::dynamic_object,object,at);
  const Vec3 bounced{.6f,.08f,0},rebound{-5,1,0};
  replica.receive(state(12,bounced,rebound,{std::cos(.3f),std::sin(.3f),0}),at);
  const auto after=replica.root(Kind::dynamic_object,object,at);
  check(before && after && distance(before->position,after->position)<1e-5f &&
      distance(before->basis[0],after->basis[0])<1e-5f,"Correction introduced a discontinuity");
  check(replica.statistics().corrections==1 && replica.statistics().snaps==0,"Small error was not corrected smoothly");
  // Frame-to-frame motion stays bounded by the blended speeds.
  const double step=1.0/240;
  // The blend's steepest pull is ln(1/0.55)*30 = 17.9 times the error per second.
  const float error=distance(before->position,bounced),speed=length(rebound)+18*error+1;
  Vec3 last=after->position;float worst{};
  for(double t=at+step;t<at+.2;t+=step) {
    const auto now=replica.root(Kind::dynamic_object,object,t);
    worst=std::max(worst,distance(now->position,last));last=now->position;
  }
  check(worst<=speed*float(step),"Correction jumped between presented frames");
  // After the correction window, presentation is the new authority's motion.
  // Past the 0.25 s extrapolation limit the authority's path holds still.
  const auto final_pose=replica.root(Kind::dynamic_object,object,at+.4);
  check(distance(final_pose->position,bounced+rebound*.25f)<2e-4f,"Correction did not converge to the authority");
  const double five=5*tick;
  const auto blended=replica.root(Kind::dynamic_object,object,at+five);
  const float remaining=distance(blended->position,bounced+rebound*float(five));
  check(remaining<.06f*error,"Correction kept more than 5% of the error after five ticks");
  std::cout<<"Correction: initial error "<<error<<" wu, "<<remaining<<" wu left after 5 ticks, largest 240 Hz step "
      <<worst<<" wu\n";
  // A teleport-sized error snaps to the authority at once.
  const Vec3 teleport{40,-12,3};
  replica.receive(state(13,teleport,{},{1,0,0}),at+tick);
  const auto snapped=replica.root(Kind::dynamic_object,object,at+tick);
  check(distance(snapped->position,teleport)<1e-4f && replica.statistics().snaps==1,"Large error did not snap");
}
}
// Run by multiplayer_tests --replica-motion.
int run_replica_motion_tests() {
  try {
    vector_quantization();
    journal_validation();
    retention();
    impulse_reaction();
    ballistic_projectile();
    correction_blend();
    std::cout<<"Replica dead reckoning and physics impulse journal passed\n";
    return 0;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
