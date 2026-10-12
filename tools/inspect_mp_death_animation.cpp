// Read-only graph request inspection. This does not admit a death action,
// choose a clip by its name, advance a clock, or construct a ragdoll body.
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include "halo2/original_data.hpp"
#include <iostream>
using namespace halo2;
int main(int argc,char**argv){try{
  if(argc!=4)return 77;
  Cache c(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  if(const auto original=c.original_data();original && original->unit_action_tables){
    const auto& table=*original->unit_action_tables;
    const auto target=original->image==OriginalData::Image::TitleUpdate15?0xf0e70u:0xeff50u;
    for(unsigned action=0;action<table.descriptors.size();++action){
      const Bytes bytes(table.descriptors[action]);
      for(unsigned field=0;field<16;field+=4)if(bytes.u32(field)==target)
        std::cout<<"actual_action="<<action<<" descriptor="<<std::hex<<table.descriptor_addresses[action]
            <<" callback_offset="<<field<<" callback="<<target<<std::dec<<'\n';
    }
  }
  for(const auto id:{0x0c000043u,0x07000001u,0x0700005du,0x07000101u,0x06000086u,0x0400000cu})
    std::cout<<std::hex<<id<<std::dec<<" string="<<c.string_id(id)<<'\n';
  // Exact existing equipped fixture's mode/class/weapon, supplied to the pure
  // graph reader. E6050's actual request uses current identifiers via NONE.
  const auto identifiers=c.load_weapon_animation_identifiers(0xe57b0406u);
  for(const auto [mode,activity]:{std::pair{0x06000086u,0x0c000043u},
      std::pair{0x07000001u,0x0700005du}}){
    std::cout<<"query mode="<<std::hex<<mode<<" activity="<<activity<<" class="<<identifiers[0]
        <<" weapon="<<identifiers[1]<<std::dec;
    try{const auto clip=c.load_unit_weapon_action(0xe9180793u,mode,identifiers[0],identifiers[1],activity);
      std::cout<<" graph="<<std::hex<<clip.source_graph<<std::dec<<" index="<<clip.source_index
          <<" frames="<<clip.frame_count<<" name="<<clip.name<<'\n';
    }catch(const std::exception&e){std::cout<<" unavailable="<<e.what()<<'\n';}
  }
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
