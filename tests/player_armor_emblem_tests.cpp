#include "halo2/player_armor_emblem.hpp"
#include "halo2/frontend_emblem.hpp"
#include "halo2/full_scoreboard_assets.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_render_policy.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
std::uint32_t word(float value){return std::bit_cast<std::uint32_t>(value);}
void inputs(const std::filesystem::path &fixture,
    std::span<const std::array<std::uint32_t,3>> palette) {
  std::ifstream file(fixture);check(bool(file),"Original armor emblem fixture missing");
  unsigned inputs=0,copies=0,colors=0;
  for(std::string line;std::getline(file,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);char kind;row>>kind;
    if(kind=='I') {
      int scenario;unsigned foreground,background,flags,name,expected,supported,enabled;
      row>>scenario>>foreground>>background>>flags>>std::hex>>name>>expected>>std::dec>>supported>>enabled;
      const auto value=source_object_emblem_input(
          {std::uint8_t(foreground),std::uint8_t(background),std::uint8_t(flags),0xa5},
          scenario==-1?std::nullopt:std::optional<std::int16_t>(scenario),name);
      check(word(value?value->value:0.f)==expected,"Armor function differs from original BCC20/BC970");
      check(unsigned(bool(value))==supported && unsigned(value && value->enabled)==enabled,
          "Armor emblem supported/active boundary differs from original");
      ++inputs;
    } else if(kind=='C') {
      std::string supplied,expected;row>>supplied>>expected;
      SourcePlayerConfiguration profile{};
      for(unsigned i=0;i<4;++i)profile[0x45+i]=std::uint8_t(std::stoul(supplied.substr(i*2,2),nullptr,16));
      const auto emblem=source_player_object_emblem(profile);
      for(unsigned i=0;i<4;++i)check(emblem[i]==std::stoul(expected.substr(i*2,2),nullptr,16),
          "Live player profile emblem dword differs from original14BFC0 copy");
      ++copies;
    } else if(kind=='P') {
      unsigned primary,secondary;row>>primary>>secondary;
      SourcePlayerConfiguration profile{};profile[0x42]=primary;profile[0x43]=secondary;
      const auto actual=source_player_emblem_colors(profile,palette);
      for(const auto &color:actual)for(float channel:{color.x,color.y,color.z}) {
        unsigned expected;row>>std::hex>>expected;
        check(word(channel)==expected,"Armor emblem palette differs from original7F790/1A06A0");
      }
      ++colors;
    } else check(false,"Unknown original armor fixture row");
    check(bool(row),"Truncated original armor emblem fixture");
  }
  check(inputs==24576 && copies==768 && colors==256,"Original armor fixture coverage changed");
  std::cout<<inputs<<" original input cases, "<<copies<<" profile copies, "<<colors<<" palette cases exact\n";
}
unsigned materials(const std::filesystem::path &maps,const char *map) {
  Cache cache(maps/(std::string(map)+".map"));
  const auto palette=cache.load_full_scoreboard_assets().palette;
  unsigned banks=0;
  for(const auto *species:{"mc_emblem","elite_emblem"}) {
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto &tag){
      return tag.group=="shad" && tag.name.ends_with(std::string("\\")+species);
    });
    check(tag!=cache.tags().end(),"Original armor emblem asset missing");
    const auto authored=cache.load_material(tag->datum);
    for(unsigned lod=0;lod<authored.lods.size();++lod) {
      auto material=authored;material.selected_shader_lod=lod;
      check(prepare_dynamic_material(cache,material,{},true),"Original armor pass unavailable");
      check(source_material_layer(material)==2,
          "Armor emblem lost its authored layer2 submission");
      check(material.animated_textures[0].size()==64 && material.vertex_program.size()>0,
          "Armor lost original foreground images or vertex placement program");
      SourcePlayerConfiguration profile{};
      profile[0x42]=9;profile[0x43]=1;profile[0x45]=26;profile[0x46]=9;
      const ObjectColors original_colors{{std::bit_cast<Vec3>(palette[8]),std::bit_cast<Vec3>(palette[12]),
          std::bit_cast<Vec3>(palette[9]),std::bit_cast<Vec3>(palette[1])}};
      apply_object_colors(material,original_colors);
      const auto original_primary=(*material.object_render_colors)[0];
      const auto vertices=material.vertex_program;
      const auto declarations=material.vertex_declaration;
      const auto sampler=material.texture_states;
      const auto blend=material.forward_blend;
      const auto update=[&](Material &m,const SourcePlayerConfiguration &p,double time=0.) {
        animate_player_armor_emblem(m,time,p,cache.load_source_geometry_scenario_type(),palette,
            [](std::uint32_t){return 0.f;});
      };
      update(material,profile);
      check(material.texture_frames[0]==26,"Gameplay armor ignores selected profile foreground: expected page26");
      if(std::string(map)=="mainmenu") {
        const auto species_name=std::string(species)=="mc_emblem"?"spartan":"elite";
        std::ifstream program(maps.parent_path().parent_path().parent_path()/"analysis/frontend/customization"/
            (std::string("frontend-model-emblem-")+species_name+"-live-program.bin"),std::ios::binary);
        std::array<std::uint32_t,60> expected{};program.read(reinterpret_cast<char*>(expected.data()),sizeof expected);
        check(program.gcount()==sizeof expected && material.forward_program.words==expected,
            "Armor program differs from retained original model draw");
      }
      auto peer=material;
      for(unsigned page=0;page<64;++page) {
        profile[0x45]=page;update(material,profile,12.375);
        check(material.texture_frames[0]==page,"Armor foreground frame overlay selects wrong page");
        check(peer.texture_frames[0]==26,"One player's emblem overwrote a second player's page");
      }
      const auto visible=material.forward_program.words;
      profile[0x47]=1;update(material,profile);
      check(material.forward_program.words!=visible,"Armor foreground toggle did not reach authored pixel program");
      const auto hidden=material.forward_program.words;
      profile[0x46]=31;profile[0x47]=7;update(material,profile,99.5);
      check(material.forward_program.words==hidden,"Armor gained a background, flip or clock input absent from authored pass");
      profile[0x47]=0;profile[0x42]=3;profile[0x43]=4;update(material,profile);
      check(material.forward_program.words!=visible,"Actual selected emblem colors did not reach authored pass");
      check(word(material.object_render_colors->at(0).x)==word(original_primary.x) &&
          word(material.object_render_colors->at(0).y)==word(original_primary.y) &&
          word(material.object_render_colors->at(0).z)==word(original_primary.z),
          "Emblem binding changed separately owned primary armor color");
      check(material.vertex_program==vertices && material.vertex_declaration==declarations &&
          material.forward_blend==blend,"Emblem binding changed authored placement or blending");
      check(material.texture_states==sampler,"Emblem binding changed authored sampler");
      Material root;root.world_overlays_after.push_back(std::make_shared<Material>(peer));
      auto other=root;const auto shared=other.world_overlays_after.front();
      update(root,profile);
      check(root.world_overlays_after.front()!=shared && shared->texture_frames[0]==26 &&
          root.world_overlays_after.front()->texture_frames[0]==63,
          "Per-player animation mutated a shared resident child pass");
      ++banks;
    }
  }
  return banks;
}
}
int main(int argc,char **argv)try {
  check(argc==3,"Supply original map directory and original fixture");
  const std::filesystem::path maps=argv[1];Cache cache(maps/"lockout.map");
  inputs(argv[2],cache.load_full_scoreboard_assets().palette);
  const auto banks=materials(maps,"lockout")+materials(maps,"mainmenu");
  std::cout<<banks<<" authored Spartan/Elite LOD banks, all64 pages, colors, toggle and independent child banks pass\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
