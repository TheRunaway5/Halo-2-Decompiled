#include "halo2/shield_overlay.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value,const std::string &message) {if(!value)throw FormatError(message);}
std::uint32_t word(std::istringstream &row) {
  std::string text;check(bool(row>>text),"Missing original shield selector field");
  return std::stoul(text,nullptr,16);
}
std::uint32_t bits(float value) {return std::bit_cast<std::uint32_t>(value);}
}
int main(int argc,char **argv) {
  try {
    if(argc!=3)throw FormatError("Supply original Lockout and shield selector fixtures");
    for(int i=1;i<3;++i)if(!std::filesystem::exists(argv[i]))return 77;
    Cache cache(argv[1]);const auto content=cache.load_content(0,true);
    unsigned count=0,damage=0,overshield=0,both=0;
    std::ifstream file(argv[2]);
    for(std::string line;std::getline(file,line);) {
      if(line.empty()||line[0]=='#')continue;
      std::istringstream row(line);const auto id=word(row);
      std::array<std::uint32_t,20> input;for(auto &v:input)v=word(row);
      std::array<std::uint32_t,8> expected;for(auto &v:expected)v=word(row);
      std::string extra;check(!(row>>extra),"Trailing original shield selector field");
      const auto found=std::find_if(content.player_representations.begin(),content.player_representations.end(),
          [&](const auto &r){return r.biped.datum==input[0];});
      check(found!=content.player_representations.end(),"Original shield representation missing");
      auto definition=found->biped.vitality;
      check(definition.damage_info_count==1,"Original signed damage-info count was not imported");
      const bool elite=input[0]==0xea1b0896;
      check(definition.overshield_first_person_shader==(elite?0xea8d0908u:0xe96307deu)&&
            definition.overshield_shader==(elite?0xea8e0909u:0xe96607e1u),
            "Original overshield references differ");
      definition.damage_info_count=std::bit_cast<std::int32_t>(input[16]);
      std::uint32_t *shaders[]={&definition.shield_damaged_first_person_shader,
          &definition.shield_damaged_shader,&definition.overshield_first_person_shader,&definition.overshield_shader};
      for(unsigned i=0;i<4;++i)if(input[17]&(1u<<i))*shaders[i]=null_datum;
      const bool visible=!input[6]&&!input[7]&&(!input[8]||(!input[9]&&!input[10]));
      const bool first_person=visible&&input[4]&&!input[5];
      check(first_person==bool(expected[0]),"Original first-person/parent visibility gate differs");
      const auto models=first_person?std::int16_t(input[13]):std::int16_t(!input[12]);
      // The oracle's unchanged 4C450 owns the resolved geometry LOD. This
      // selector takes that explicit caller result, rather than reproducing it.
      const auto lod=std::uint8_t(expected[1]);
      ShieldOverlayGeometry geometry{visible,!bool(input[11]),first_person,models,lod,input[3]};
      const int accepted=visible&&!input[11]&&models>0&&lod!=0xff?models:0;
      const auto original_count=std::bit_cast<std::int32_t>(expected[2]);
      check(accepted==std::max(0,original_count),"Original model-count gate differs");
      const auto actual=select_shield_overlays(definition,std::bit_cast<float>(input[1]),
          std::bit_cast<float>(input[2]),geometry);
      const auto context="Original shield selector case "+std::to_string(id);
      check(actual.flags==expected[3],context+" flags");
      check(actual.count==unsigned(bool(expected[3]&1))+unsigned(bool(expected[3]&0x20)),context+" count");
      for(const auto &selected:actual.selected()) {
        const bool is_damage=selected.kind==ShieldOverlayKind::Damaged;
        check(selected.shader==expected[is_damage?4:6]&&bits(selected.vitality)==expected[is_damage?5:7],
            context+" shader/vitality");
        if(is_damage)++damage;else ++overshield;
      }
      if(actual.count==2) {
        check(actual.overlays[0].kind==ShieldOverlayKind::Damaged&&actual.overlays[1].kind==ShieldOverlayKind::Overshield,
            context+" collector order");++both;
      }
      ++count;
    }
    check(count==1800&&damage==316&&overshield==137&&both==72,"Incomplete original shield selector corpus");
    std::cout<<count<<" original shield selector cases, "<<damage<<" damaged and "<<overshield
             <<" overshield selections matched bit for bit\n";
    return 0;
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
