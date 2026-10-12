#include "halo2/particle_movement.hpp"
#include "halo2/muzzle_effects.hpp"
#include "halo2/content.hpp"
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char*message){if(!value)throw FormatError(message);}
void equal(const ParticleMovementDefinition&a,const ParticleMovementDefinition&b){
  check(a.flags==b.flags&&a.controllers.size()==b.controllers.size(),"Alias changed authored movement definition");
  for(unsigned i=0;i<a.controllers.size();++i){const auto&x=a.controllers[i];const auto&y=b.controllers[i];
    check(x.type==y.type&&x.constant_parameters==y.constant_parameters&&x.required_values==y.required_values&&
        x.parameters.size()==y.parameters.size(),"Alias changed authored controller");
    for(unsigned j=0;j<x.parameters.size();++j){const auto&p=x.parameters[j];const auto&q=y.parameters[j];
      check(p.input==q.input&&p.range==q.range&&p.modifier_input==q.modifier_input&&p.modifier==q.modifier&&p.curve==q.curve,
          "Alias changed authored movement curve");
    }
  }
}
}
int main(int argc,char**argv){try{
  if(argc!=4)throw FormatError("Supply MAP SHARED_MAPS UPDATE_XBE");
  if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[3]))return 77;
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));unsigned aliases{};
  for(const auto&tag:cache.tags())if(tag.group=="pmov"&&tag.address&&!tag.size){
    const Tag*backing=nullptr;
    for(const auto&candidate:cache.tags())if(candidate.group==tag.group&&candidate.address==tag.address&&candidate.size){
      check(!backing||backing->size==candidate.size,"Actual alias has inconsistent owned backing");backing=&candidate;
    }
    check(bool(backing),"Actual movement alias has no same-address owned packet");
    const auto a=load_particle_movement_definition(cache,tag.datum),b=load_particle_movement_definition(cache,backing->datum);
    check(a.datum==tag.datum&&b.datum==backing->datum,"Alias identity was replaced by backing identity");equal(a,b);++aliases;
  }
  const auto content=cache.load_content(0,true);const auto&weapon=content.weapons.at(cache.slayer_starting_weapon(5));
  check(!weapon.firing_effects.empty(),"Actual SMG has no firing effect bank");
  const auto muzzle=cache.load_muzzle_effect(weapon.firing_effects.front().effects[0]);
  check(!muzzle.systems.empty(),"Actual SMG effect has no admitted source first-person systems");
  std::cout<<"PASS "<<cache.name()<<": "<<aliases<<" same-address pmov aliases retain identity and all controller data; actual SMG muzzle import succeeds\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
