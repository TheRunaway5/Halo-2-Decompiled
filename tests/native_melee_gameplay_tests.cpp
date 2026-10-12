#include "halo2/multiplayer.hpp"
#include "halo2/original_data.hpp"
#include <iostream>
using namespace halo2;
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
int main(int argc,char **argv){try{
 check(argc==4,"Supply MAP SHARED XBE");
 const auto original=OriginalData::load_xbe(argv[3]);
 auto cache=std::make_shared<Cache>(argv[1],argv[2],original);
 auto content=cache->load_content(0,true);content.collision.clear();
 SlayerRules rules;rules.weapon=cache->slayer_starting_weapon(5);rules.score_limit=0;rules.time_ticks=0;
 Spawn spawn;spawn.player_type=0;std::array<Spawn,1> spawns{spawn};
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
 std::cout<<"PASS actual melee admission, authored impact frame and target damage\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
