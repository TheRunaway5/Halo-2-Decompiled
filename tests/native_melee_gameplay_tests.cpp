#include "halo2/multiplayer.hpp"
#include "halo2/original_data.hpp"
#include "halo2/native_player_animation.hpp"
#include "halo2/native_player_profile.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <set>
#include <iostream>
using namespace halo2;
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
int main(int argc,char **argv){try{
 check(argc==4,"Supply MAP SHARED XBE");
 const auto original=OriginalData::load_xbe(argv[3]);
 auto cache=std::make_shared<Cache>(argv[1],argv[2],original);
 auto content=cache->load_content(0,true);content.collision.clear();
 SlayerRules rules;rules.weapon=cache->slayer_starting_weapon(5);rules.score_limit=0;rules.time_ticks=0;
 Spawn spawn;spawn.player_type=0;spawn.position={0,0,10};std::array<Spawn,1> spawns{spawn};
 MultiplayerMatch match(content,0,spawns,rules);
 match.configure_melee_animation(cache,*original->melee_constants);
 const auto a=match.join("Attacker"),b=match.join("Target");
 const_cast<Unit&>(match.unit(a)).body.position={0,0,10};const_cast<Unit&>(match.unit(b)).body.position={.65f,0,10};
 const float before=match.unit(b).vitality.body+match.unit(b).vitality.shield;
 std::array<Controls,16> controls{};controls[a].melee=true;match.step(controls);controls[a].melee=false;
 check(match.unit(a).melee_action.active(),"Melee request was not admitted");
 const auto impact=match.unit(a).melee_action.impact;
 check(impact>=0,"Authored ordinary melee has no impact event");
 for(int tick=1;tick<impact;++tick)match.step(controls);
 check(match.unit(b).vitality.body+match.unit(b).vitality.shield==before,"Melee applied damage before its event");
 match.step(controls);
 check(match.unit(b).vitality.body+match.unit(b).vitality.shield<before,"Melee impact event never damages an in-range target");
 const auto sword=std::find_if(content.weapons.begin(),content.weapons.end(),[](const auto &entry) {
   return entry.second.inventory_flags&(1u<<24);
 });
 check(sword!=content.weapons.end(),"Actual cache has no authored sword");
 for(unsigned representation:{2u,3u})for(bool airborne:{false,true})for(bool crouch:{false,true}) {
  auto sword_content=content;
  sword_content.collision={{{-100,-100,10},{100,-100,10},{100,100,10},0,0},
                           {{-100,-100,10},{100,100,10},{-100,100,10},0,0}};
  MultiplayerMatch special(sword_content,0,spawns,rules);
  special.configure_weapon_pickups();
  special.configure_melee_animation(cache,*original->melee_constants);
  const std::array<std::uint8_t,9> appearance{10,0,0,10,std::uint8_t(representation-2),0,0,0,0};
  const auto attacker=special.join("Sword",-1,native_selected_player_configuration("Sword",appearance,-1));
  const auto target=special.join("Target");
  std::array<Controls,16> input{};
  for(unsigned tick=0;tick<90;++tick)special.step(input);
  const auto pickup=std::find_if(special.weapon_pickups().begin(),special.weapon_pickups().end(),
      [&](const auto &item){return item.definition==sword->first;});
  check(pickup!=special.weapon_pickups().end(),"Lockout has no actual sword pickup");
  const_cast<Unit&>(special.unit(attacker)).body.position=pickup->position-Vec3{.2f,0,0};
  input[attacker].pickup=true;input[attacker].pickup_milliseconds=1000;
  special.step(input);input[attacker].pickup=false;
  check(special.unit(attacker).weapon.definition==sword->first,"Sword pickup never equipped its real retained item");
  const_cast<Unit&>(special.unit(attacker)).body.position={0,0,10};
  const_cast<Unit&>(special.unit(target)).body.position={.65f,0,10};
  input[attacker].crouch=crouch;
  for(unsigned tick=0;tick<30;++tick)special.step(input);
  if(airborne) {
   input[attacker].jump=true;special.step(input);input[attacker].jump=false;
   check(!special.unit(attacker).body.grounded,"Sword airborne stimulus never jumped");
  }
  const auto model=cache->load_model(special.unit(attacker).definition.datum,false);
  NativePlayerAnimation presenter(*cache,model.animation,sword->first);
  // Idle is a primary row, loaded explicitly while action lookup stays under test.
  const auto identifiers=cache->load_weapon_animation_identifiers(sword->first);
  const auto base=cache->load_unit_weapon_action(model.animation,crouch?0x06000087u:0x06000086u,identifiers[0],identifiers[1],0x0400000cu);
  input[attacker].fire=true;special.step(input);input[attacker].fire=false;
  const auto &action=special.unit(attacker).melee_action;
  check(action.active(),"Actual sword fire never admitted a special melee action");
  check(action.activity==(airborne?0x140005b3u:0x0b0005b2u),"Sword special did not select its authored ground/air strike");
  const auto *clip=presenter.action(action.activity,crouch,false);
  if(!clip)std::cerr<<"Missing special body representation="<<representation<<" crouch="<<crouch<<" airborne="<<airborne<<'\n';
  check(clip,"Admitted sword special melee has no third-person animation");
  std::set<std::uint64_t> poses;const auto started=action.started;
  for(unsigned tick=0;tick<60 && special.unit(attacker).melee_action.active();++tick) {
   const float frame=float(special.state().tick-started);
   const auto locals=clip->animation_type==0?presenter.pose(model,*clip,frame,0,crouch,false,false):
       presenter.pose(model,base,0,0,crouch,false,false,clip,frame);
   const auto vertices=pose_model_locals_world(model,{},locals);
   std::uint64_t hash=0;
   for(const auto &v:vertices)for(float component:{v.position.x,v.position.y,v.position.z}) {
    check(std::isfinite(component)&&std::abs(component)<5,"Special melee generated an exploded body vertex");
    hash=hash*1099511628211ull+std::bit_cast<std::uint32_t>(component);
   }
   poses.insert(hash);special.step(input);
  }
  check(poses.size()>3,"Sword special animation stayed frozen on the third-person model");
  check(!special.unit(attacker).melee_action.active(),"Sword special did not finish");
 }
 std::cout<<"PASS actual melee admission, authored impact frame and target damage\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
