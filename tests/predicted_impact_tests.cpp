// Predicted impact sparks (NETCODE_ENHANCED_GOAL item 6), asset-free part:
// the local press gate (only presses that certainly fire are predicted, the
// first round on the firing input) and the host-copy ledger (shooter, tick
// window, position tolerance and the same effect tag; player hits and other
// shooters' contacts never match). The TU1.5 world check is
// predicted_impact_tu15_tests.cpp.
#include "halo2/predicted_impacts.hpp"
#include "halo2/native_grenade.hpp"
#include <iostream>
#include <string>

using namespace halo2;
namespace {
void check(bool yes,const std::string &message){if(!yes)throw std::runtime_error(message);}
constexpr std::uint32_t player=0x10001,unit=0x20002,weapon_object=0x30003,rifle=0xe0010040u,bullet=0xe0020050u;
constexpr std::uint32_t spark=0xe0030060u,sweetener=0xe0030061u,blood=0xe0030070u,other_unit=0x20009;

void classification() {
  WeaponDefinition w;w.datum=rifle;w.projectile=bullet;w.instantaneous=true;w.capacity=36;
  w.triggers.push_back({});w.triggers.back().behavior=1;w.rounds_per_second=15;w.burst=3;w.fire_recovery=.2f;
  w.ready_seconds=.5f;w.rounds_per_shot=1;w.range=200;
  const auto shot=predicted_shot_weapon(w);
  check(shot && shot->rounds_per_press==3 && std::abs(shot->round_seconds-1.0/15)<1e-9 &&
      std::abs(shot->cycle_seconds-(3.0/15+.2))<1e-6 && shot->range==200,"Precision weapon timing");
  auto automatic=w;automatic.triggers.back().behavior=0;
  check(!predicted_shot_weapon(automatic),"Spew (automatic) trigger predicted");
  auto travelling=w;travelling.instantaneous=false;
  check(!predicted_shot_weapon(travelling),"Travelling projectile predicted");
  auto energy=w;energy.energy.emplace();
  check(!predicted_shot_weapon(energy),"Energy weapon (unreplicated heat) predicted");
  auto no_magazine=w;no_magazine.capacity=0;
  check(!predicted_shot_weapon(no_magazine),"Weapon without magazine rounds predicted");
}

PredictedShotWeapon battle_rifle() {
  PredictedShotWeapon w;w.weapon=rifle;w.projectile=bullet;w.rounds_per_press=3;w.round_seconds=2.0/30;
  w.cycle_seconds=3*2.0/30+.2;w.ready_seconds=.5;w.rounds_per_shot=1;w.range=200;
  return w;
}
PredictedShotGate::Player armed(std::uint32_t tick,int loaded=36) {
  PredictedShotGate::Player p;p.armed=true;p.player=player;p.unit=unit;p.weapon_object=weapon_object;p.weapon=rifle;
  p.loaded=loaded;p.tick=tick;p.shot_tick=0;
  return p;
}
void gate() {
  const auto w=battle_rifle();
  PredictedShotGate gate;
  const PredictedShotGate::Controls idle{},fire{true};
  double t=0;std::uint32_t tick=100;
  // One input tick; the player state is the newest replicated one.
  const auto step=[&](const PredictedShotGate::Controls &c,PredictedShotGate::Player p){
    t+=1.0/30;++tick;p.tick=tick;return gate.input(t,c,p,&w);
  };
  const auto rest=[&](unsigned inputs){for(unsigned n=0;n<inputs;++n)step(idle,armed(tick));};
  // The weapon becomes current at the first input: not ready yet.
  check(step(idle,armed(tick))==0,"Idle input scheduled a round");
  check(step(fire,armed(tick))==0,"Press before the weapon's ready time was predicted");
  rest(21);
  // A press of a ready weapon: the whole burst, its first round due at once.
  check(step(fire,armed(tick))==3,"Ready press did not schedule the burst");
  const double press=t;const auto press_tick=tick;
  auto due=gate.take_due(press);
  check(due.size()==1 && due[0].index==0 && due[0].tick_floor==press_tick && due[0].unit==unit &&
      due[0].projectile==bullet && due[0].seconds==press,"First round is not due on the firing input");
  check(step(fire,armed(tick))==0,"A held trigger is a new press");
  due=gate.take_due(press+2*w.round_seconds+1e-9);
  check(due.size()==2 && due[0].index==1 && due[1].index==2,"Later burst rounds are not due at the barrel rate");
  // Within the weapon's cycle (plus margin) no press is predicted, and a
  // refused press restarts the spacing (it may have fired on the host).
  rest(1);
  check(step(fire,armed(tick))==0,"Press inside the weapon cycle was predicted");
  rest(8);
  check(step(fire,armed(tick))==0,"Press too soon after a refused press was predicted");
  rest(20);
  // The host's own last shot is recent: not predicted.
  auto host_fired=armed(tick);host_fired.shot_tick=tick;
  check(step(fire,host_fired)==0,"Press right after a host shot was predicted");
  rest(20);
  // Rounds predicted within the ammunition window are not yet in `loaded`.
  check(step(fire,armed(tick))==3,"Ready press not predicted");
  gate.take_due(t+1);
  rest(16);
  check(step(fire,armed(tick,4))==1,"Burst not truncated to the unconfirmed remaining rounds");
  gate.take_due(t+1);
  rest(20);
  check(step(fire,armed(tick,0))==0,"Empty magazine predicted");
  rest(20);
  // Other weapon actions suspend prediction for their hold.
  step({false,true},armed(tick));
  rest(12);
  check(step(fire,armed(tick))==0,"Press during a reload hold was predicted");
  rest(30);
  check(step(fire,armed(tick))==3,"Press after the hold was not predicted");
  gate.take_due(t+1);
  rest(20);
  auto seated=armed(tick);seated.armed=false;
  check(step(fire,seated)==0,"Unarmed (seated, dead or busy) player predicted");
  rest(20);
  auto other=armed(tick);other.weapon=0xe0010041u;
  check(step(fire,other)==0,"Press of a different weapon definition predicted");
  // A new weapon object restarts its ready time and drops pending rounds.
  rest(20);
  check(step(fire,armed(tick))==3,"Ready press not predicted");
  auto swapped=armed(tick);swapped.weapon_object=0x30004;
  step(idle,swapped);
  check(gate.take_due(t+1).empty(),"Pending rounds survived a weapon change");
  // 0.57 s later: past the press spacing, short of ready time plus margin.
  for(unsigned n=0;n<16;++n)step(idle,swapped);
  check(step(fire,swapped)==0,"New weapon object predicted before its ready time");
  for(unsigned n=0;n<20;++n)step(idle,swapped);
  check(step(fire,swapped)==3,"Ready new weapon object not predicted");
  const auto &stats=gate.statistics();
  std::cout<<"gate: presses "<<stats.presses<<", admitted "<<stats.admitted<<", rounds "<<stats.rounds
      <<", refused "<<stats.refused<<'\n';
}

PredictedImpactLedger::Prediction prediction(double seconds,Vec3 at) {
  PredictedImpactLedger::Prediction p;p.player=player;p.unit=unit;p.weapon=rifle;p.projectile=bullet;
  p.tick_floor=100;p.seconds=seconds;p.position=at;p.distance=10;p.material=7;
  p.effects={spark,sweetener,null_datum,null_datum};
  return p;
}
NativeGrenadeDetonation contact(std::uint32_t shooter_unit,std::uint32_t effect,std::uint64_t tick,Vec3 at) {
  NativeGrenadeDetonation e;e.serial=tick;e.tick=tick;e.projectile=0x40004;e.definition=bullet;e.effect=effect;
  e.player=shooter_unit==unit?player:0x10009;e.unit=shooter_unit;e.position=at;
  SourceEffectLocation location;location.frame.position=at;e.source_locations={location};
  return e;
}
void ledger() {
  PredictedImpactLedger ledger;
  const Vec3 wall{10,0,1};
  ledger.predict(prediction(1.0,wall));
  check(!ledger.suppress(predicted_host_impact(contact(other_unit,spark,104,wall)),1.1),"Another shooter's impact suppressed");
  check(!ledger.suppress(predicted_host_impact(contact(unit,blood,104,wall+Vec3{0,.1f,0})),1.1),
      "A player hit (blood effect) suppressed by a predicted world spark");
  check(!ledger.suppress(predicted_host_impact(contact(unit,spark,140,wall)),1.1),"Impact outside the tick window suppressed");
  check(!ledger.suppress(predicted_host_impact(contact(unit,spark,98,wall)),1.1),"Impact before the press suppressed");
  check(!ledger.suppress(predicted_host_impact(contact(unit,spark,104,wall+Vec3{0,1,0})),1.1),"Impact outside the position tolerance suppressed");
  check(ledger.suppress(predicted_host_impact(contact(unit,sweetener,104,wall+Vec3{0,.3f,0})),1.1),"Matching host impact not suppressed");
  check(!ledger.suppress(predicted_host_impact(contact(unit,spark,105,wall)),1.1),"One prediction suppressed two host impacts");
  // Unconfirmed predictions expire; a later host copy then spawns.
  ledger.predict(prediction(2.0,wall));
  check(!ledger.suppress(predicted_host_impact(contact(unit,spark,104,wall)),3.2),"Expired prediction suppressed an impact");
  check(ledger.pending()==0,"Expired prediction retained");
  // Legacy native contacts: weapon, terrain flag and global material.
  ledger.predict(prediction(4.0,wall));
  BulletImpact legacy;legacy.tick=103;legacy.weapon=rifle;legacy.position=wall;legacy.material=7;legacy.terrain=false;
  check(!ledger.suppress(predicted_host_impact(legacy,bullet),4.1),"Legacy object contact suppressed");
  legacy.terrain=true;legacy.material=8;
  check(!ledger.suppress(predicted_host_impact(legacy,bullet),4.1),"Legacy contact on another material suppressed");
  legacy.material=7;
  check(ledger.suppress(predicted_host_impact(legacy,bullet),4.1),"Matching legacy terrain contact not suppressed");
  // Burst order: each host round consumes the earliest matching prediction.
  for(unsigned n=0;n<3;++n)ledger.predict(prediction(5.0+n*.066,wall+Vec3{0,float(n)*.05f,0}));
  for(unsigned n=0;n<3;++n)
    check(ledger.suppress(predicted_host_impact(contact(unit,spark,104+2*n,wall)),5.3),"Burst round not suppressed");
  const auto &stats=ledger.statistics();
  check(stats.suppressed==5 && ledger.pending()==0,"Ledger accounting");
  std::cout<<"ledger: predicted "<<stats.predicted<<", suppressed "<<stats.suppressed<<", expired "<<stats.expired<<'\n';
}
}

int run_predicted_impact_tests() {
  try {
    check(!predicted_impacts_enabled(NetcodeMode::original) && predicted_impacts_enabled(NetcodeMode::balanced) &&
        predicted_impacts_enabled(NetcodeMode::full),"Predicted impacts mode gate");
    classification();
    gate();
    ledger();
    std::cout<<"Predicted impact gate and host-copy ledger passed\n";
    return 0;
  } catch(const std::exception &error) {
    std::cerr<<"predicted impacts: "<<error.what()<<'\n';
    return 1;
  }
}
