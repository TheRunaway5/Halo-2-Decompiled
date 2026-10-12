#include "halo2/native_unit_interaction.hpp"
#include "halo2/simulation.hpp"
#include <iostream>
#include <algorithm>
using namespace halo2;
int main(int argc,char **argv)try{
 if(argc!=2)throw FormatError("Supply campaign cache");Cache cache(argv[1]);auto c=cache.load_content(0);
 for(auto &s:c.squads)for(auto &p:s.starts)p.flags|=0; // Keep original squad/loadout data.
 std::cout<<"player team="<<c.player.team<<" squads="<<c.squads.size()<<"\n";
 for(const auto &s:c.squads){
  if(s.team!=c.player.team && !(s.team==2 && c.player.team==1) && !(s.team==1 && c.player.team==2))continue;
  Simulation sim(c,0,cache.spawns().front());if(!sim.place_squad(s.name))continue;
  if(sim.player().weapon.definition==null_datum)for(const auto &item:sim.weapon_pickups())if(item.weapon){sim.equip(item.definition);break;}
  std::cout<<"squad "<<s.name<<" team="<<s.team<<" actors="<<sim.actors().size()<<"\n";
  for(const auto &ally:sim.actors()){
   std::cout<<" ally team="<<ally.team<<" exists="<<ally.exists<<" dead="<<ally.vitality.dead<<" visible="<<ally.visible<<" hidden="<<ally.bsp_hidden<<" weapon="<<(c.weapons.contains(ally.weapon.definition)?c.weapons.at(ally.weapon.definition).name:"NONE")<<"\n";
   if(ally.weapon.definition==null_datum || ally.weapon.definition==sim.player().weapon.definition)continue;
   for(float distance:{.4f,.6f,.8f}){
    const auto direction=halo2::direction(ally.yaw);Spawn p;p.position=ally.body.position+direction*distance;p.facing=ally.yaw+3.141592654f;p.bsp=0;
    sim.debug_teleport(0,p);
    if(!native_can_trade_weapon(sim.player(),ally,c,sim.allied(sim.player().team,ally.team)))continue;
    const auto beforep=sim.player().weapon,beforea=ally.weapon;const auto actor=ally.source_datum;
    const auto prompt=sim.action_prompt();if(prompt.target.action!=2 || prompt.target.object!=actor)throw FormatError("Trade was hidden by a different action");
    Controls input;input.yaw=p.facing;input.pickup=true;input.pickup_milliseconds=1000;sim.step(input);
    const auto found=std::find_if(sim.actors().begin(),sim.actors().end(),[&](const auto &u){return u.source_datum==actor;});
    if(sim.player().weapon.source_datum!=beforea.source_datum || found->weapon.source_datum!=beforep.source_datum)throw FormatError("Actual weapon generations did not trade");
    if(sim.player().weapon.loaded!=beforea.loaded || sim.player().weapon.reserve!=beforea.reserve)throw FormatError("Trade changed existing ammo");
    if(!(sim.player().source_player_actions.flags&0x10))throw FormatError("Hold action was not consumed");
    std::cout<<"PASS actual squad "<<s.name<<" ally="<<actor<<" trade "<<c.weapons.at(beforep.definition).name<<" <-> "<<c.weapons.at(beforea.definition).name<<'\n';return 0;
   }
  }
 }
 throw FormatError("No actual friendly armed squad admitted trade in this map");
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
