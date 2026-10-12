#include "halo2/source_weapon_carry.hpp"
#include <iostream>
using namespace halo2;
int main(){int actor,compatible;unsigned flags;while(std::cin>>actor>>compatible>>flags){
 SourceWeaponCarryQuery q;q.actor=actor;q.compatible=compatible;
 SourceWeaponCarryItem t;t.datum=0x10006;t.definition=3;t.alternate=4;t.flags=flags;
 int finite,energy;std::cin>>finite>>energy>>t.reserve>>t.loaded>>t.spent;t.finite_magazine=finite;t.energy=energy;
 int a,b;std::cin>>a>>b;q.hands={std::int8_t(a),std::int8_t(b)};
 for(unsigned s=0;s<4;++s){auto &w=q.slots[s];int exists,busy;std::cin>>exists>>w.definition>>w.flags>>w.reserve>>w.loaded>>w.spent>>busy;w.datum=exists?0x10002+s:0xffffffffu;w.busy=busy;w.finite_magazine=true;}
 const auto p=source_weapon_carry_permissions(q,t);std::cout<<int(p[0])<<int(p[1])<<int(p[2])<<int(p[3])<<'\n';
}return 0;}
