#include "halo2/native_unit_interaction.hpp"
#include "halo2/simulation.hpp"
#include "halo2/math.hpp"
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try{
 if(argc!=2)throw FormatError("Supply cache");Cache cache(argv[1]);auto c=cache.load_content(0,cache.name()=="zanzibar" || cache.name()=="coagulation" || cache.name()=="headlong");
 const auto &a=*c.unit_interactions;
 std::cout<<cache.name()<<" unit definitions="<<a.units.size()<<" flip threshold="<<a.flipped_up_threshold<<'\n';
 for(const auto &[id,d]:a.units){std::cout<<cache.query_tag({.handle=TagHandle{id}})->name<<" seats="<<d.seats.size()<<" animation modes="<<d.seat_animations.size()<<'\n';
  for(const auto &s:d.seats)std::cout<<" seat mode="<<cache.string_id(s.mode)<<" radius="<<s.radius<<" marker="<<cache.string_id(s.marker)<<" entry="<<cache.string_id(s.entry_marker)<<" flags="<<std::hex<<s.flags<<std::dec<<'\n';}
 auto spawns=cache.spawns();if(spawns.empty())throw FormatError("No spawn");
 Simulation sim(c,0,spawns.front());for(unsigned i=0;i<30;++i)sim.step({});unsigned found=0;
 for(const auto &o:sim.scene().objects()) {
  if(o.handle==null_datum || c.objects[o.placement].type!=1 || !o.visible)continue;
  for(float r:{.2f,.5f,.8f,1.1f,1.5f,2.f,2.5f})for(unsigned angle=0;angle<32;++angle) {
   const float theta=float(angle)*6.2831853f/32;Spawn at;at.position=o.position+direction(theta)*r;at.position.z=o.position.z;at.facing=theta+3.14159265f;at.bsp=0;
   const auto floor=sim.collision().raycast(at.position+Vec3{0,0,2},{0,0,-1},4);if(!floor)continue;at.position.z=floor->position.z+.01f;
   sim.debug_teleport(0,at);
   const auto p=sim.unit_action_prompt(sim.player());if(p.target.object!=o.handle || p.target.action<4 || p.target.action>6)continue;
   std::cout<<"ADMITTED vehicle="<<cache.query_tag({.handle=TagHandle{p.definition}})->name<<" action="<<p.target.action<<" seat="<<p.target.index<<" position="<<at.position.x<<','<<at.position.y<<','<<at.position.z<<" yaw="<<at.facing<<'\n';
   if(!sim.take_unit_action(const_cast<Unit&>(sim.player()),p))throw FormatError("Displayed vehicle prompt not executable");
   if(sim.player().source_parent!=o.handle || sim.player().source_seat!=p.target.index)throw FormatError("Vehicle seat not retained");
   for(unsigned i=0;i<120;++i){Controls control;control.yaw=at.facing;sim.step(control);}
   if(sim.player().seat_action.phase!=Unit::SeatAction::Seated)throw FormatError("Entry animation failed to finish");
   std::cout<<"PASS actual vehicle seat attachment and authored animation completion\n";return 0;
  }
  ++found;
 }
 throw FormatError("No executable reachable seat among "+std::to_string(found)+" actual vehicles");
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
