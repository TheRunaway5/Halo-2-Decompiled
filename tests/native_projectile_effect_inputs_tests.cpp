#include "halo2/simulation.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <bit>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try{
  if(argc!=4)throw FormatError("Supply MAP SHARED XBE");
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  const auto content=cache.load_content(0,true);
  const auto found=std::find_if(content.weapons.begin(),content.weapons.end(),[](const auto &p){return p.second.name.ends_with("\\shotgun");});
  if(found==content.weapons.end())throw FormatError("Actual Shotgun definition missing");
  const auto &weapon=found->second;
  const auto tag=cache.load_source_tag_metadata(weapon.projectile);
  const Bytes root(tag.bytes_at(tag.source_address(),0x1a4));
  // FBFD0 / FC2E0 stores the dry air-damage-range upper bound at body170.
  // Needler original-constructor evidence independently retains42700000
  // (60), whereas its velocity is40C00000 (6). These are distinct owners.
  const float damage_range=root.f32(0x16c);
  if(!(damage_range>0) || damage_range==weapon.projectile_speed || !(weapon.projectile_flags&16))
    throw FormatError("Missing authored falloff/velocity distinction");
  Simulation simulation(content,0,cache.spawns().front());simulation.equip(weapon.datum);
  Controls input{};input.yaw=simulation.player().yaw;input.pitch=-1.2f;
  unsigned checked{};std::uint64_t serial{};
  for(unsigned t=0;t<90 && !checked;++t){
    input.fire=t>=3;simulation.step(input);
    const auto &unit=simulation.player();
    if(unit.weapon.last_shot!=simulation.ticks())continue;
    const auto eye=ordinary_unit_eye(unit.body.position,unit.definition.camera_height,
        unit.definition.crouch_camera_height,unit.crouch);
    for(const auto &hit:simulation.bullet_impacts()){
      if(hit.serial<=serial || hit.tick!=simulation.ticks() || !hit.terrain)continue;
      serial=hit.serial;
      if(!hit.presentation)throw FormatError("Actual Shotgun terrain contact lost effect inputs");
      const auto row=source_projectile_material_response(weapon.projectile_material_responses,content.material_parents,hit.material);
      if(!row)throw FormatError("Actual Shotgun material row unavailable");
      const auto expected=source_projectile_contact_damage_scale(weapon.projectile_flags,
          eye,hit.position,0,damage_range,1);
      const auto effect=source_projectile_effect_scale(std::span<const std::byte>(weapon.projectile_material_responses).subspan(*row*0x58,0x58),0,expected);
      if(std::bit_cast<std::uint32_t>(hit.presentation->scale_a)!=std::bit_cast<std::uint32_t>(effect)){
        std::cerr<<"Actual scale="<<hit.presentation->scale_a<<" original range="<<damage_range<<" expected scale="<<effect<<" velocity="<<weapon.projectile_speed<<'\n';
        throw FormatError("Actual Shotgun contact uses velocity instead of original damage range");
      }
      ++checked;
    }
  }
  if(!checked)throw FormatError("Actual Shotgun firing did not reach terrain on its birth tick");
  std::cout<<"PASS actual Shotgun birth-tick contacts="<<checked<<" authored damage range="<<damage_range<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
