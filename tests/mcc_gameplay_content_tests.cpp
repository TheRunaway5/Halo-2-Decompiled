#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/tags/system.hpp"
#include <iostream>

namespace {
void check(bool value,const char* message) {
  if(!value)throw std::runtime_error(message);
}
}
int main(int argc,char** argv)try {
  if(argc<2)throw std::runtime_error("Supply imported MCC gameplay maps");
  for(int arg=1;arg<argc;++arg) {
    const auto owner=std::make_shared<const halo2::Cache>(argv[arg]);
    const auto& cache=*owner;const halo2::TagSystem tags(owner);
    check(cache.is_mcc(),"MCC content regression requires an MCC map");
    const auto* tag=cache.query_tag({.group=halo2::tag_groups::weapon,
        .name="objects\\vehicles\\falcon\\weapon\\rpg",.require_address=true});
    check(tag,"MCC shared Falcon RPG tag was lost");
    const auto metadata=cache.load_source_tag_metadata(tag->datum);
    const halo2::Bytes source(metadata.bytes_at(metadata.source_address(),metadata.size()));
    check(source.u32(0x2c8)==1&&source.u32(0x2d0)==1,"Unexpected Falcon trigger/barrel source shape");
    const halo2::Bytes trigger(tags.read_source(source.u32(0x2cc),0x40));
    check(trigger.u16(8)==0xffff&&trigger.u16(10)==0xffff,"Falcon source is not the authored NONE/NONE case");
    for(bool multiplayer:{false,true}) {
      const auto content=cache.load_content(0,multiplayer);
      for(const auto& limitation:content.limitations) {
        std::cerr<<limitation<<'\n';
        check(limitation.find(tag->name)==std::string::npos,"Valid NONE-trigger weapon left unresolved");
      }
      const auto found=content.weapons.find(tag->datum);
      check(found!=content.weapons.end(),"Valid NONE-trigger weapon was dropped");
      const auto& weapon=found->second;
      check(weapon.name==tag->name&&!weapon.source_runtime_only,"Inactive authored weapon became an unsupported fallback");
      check(weapon.triggers.size()==1&&weapon.triggers[0].primary_barrel==-1&&
          weapon.triggers[0].secondary_barrel==-1,"NONE trigger indices were replaced");
      check(weapon.barrel_ammunition.size()==1,"Actual inactive barrel metadata was dropped");
      const halo2::Bytes barrel(tags.read_source(source.u32(0x2d4),0xec));
      check(weapon.barrel_ammunition[0].flags==barrel.u32(0)&&
          std::uint16_t(weapon.barrel_ammunition[0].magazine)==barrel.u16(0x28),
          "Inactive barrel ammunition metadata changed");
      check(!weapon.barrel_timing&&weapon.projectile==halo2::null_datum,
          "NONE trigger fabricated a selected projectile barrel");
      std::cout<<cache.name()<<' '<<(multiplayer?"multiplayer":"campaign")<<": "
          <<content.weapons.size()<<" weapons, Falcon NONE/NONE retained\n";
    }
  }
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
