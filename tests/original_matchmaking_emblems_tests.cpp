#include "halo2/frontend.hpp"
#include "halo2/frontend_emblem.hpp"
#include "halo2/original_matchmaking_emblems.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char*why){if(!value)throw FormatError(why);}
Texture rgba(const Texture&t){return t.format>=14&&t.format<=16?prepare_xemu_bc_texture(t):t;}
}
int main(int argc,char**argv)try {
  if(argc!=3 || !std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
  const auto original=Cache(argv[1]).load_frontend(),mcc=Cache(argv[2]).load_frontend();
  const auto find=[](const auto&content)->const Material& {
    const auto i=std::find_if(content.emblem_shaders.begin(),content.emblem_shaders.end(),
        [](const auto&v){return v.second.name==original_matchmaking_emblem_source_name;});
    check(i!=content.emblem_shaders.end(),"Offline reference emblem owner absent");return i->second;
  };
  const auto &reference=find(original),&sampled=find(mcc);const auto prepared=make_original_matchmaking_emblem(sampled);
  for(unsigned stage=0;stage<2;++stage) {
    check(prepared.animated_textures[stage].size()==reference.animated_textures[stage].size(),"Offline original page count differs");
    for(unsigned page=0;page<reference.animated_textures[stage].size();++page) {
      const auto a=rgba(reference.animated_textures[stage][page]),b=rgba(prepared.animated_textures[stage][page]);
      check(a.width==b.width && a.height==b.height && a.pixels==b.pixels,"Offline original decoded emblem page differs");
    }
  }
  for(unsigned page=0;page<64;++page)for(unsigned flags:{0u,1u,2u,4u,7u})for(float opacity:{1.f,.5f}) {
    FrontendAppearance appearance;appearance.colors={8,12,9,1};appearance.foreground=page;appearance.background=page%32;appearance.flags=flags;
    auto selected=prepared,actual=reference;
    update_frontend_emblem(selected,appearance,mcc.appearance_palette,opacity);
    update_frontend_emblem(actual,appearance,mcc.appearance_palette,opacity);
    check(selected.texture_frames==actual.texture_frames && selected.forward_program.words==actual.forward_program.words &&
          selected.forward_transforms==actual.forward_transforms,"Offline original emblem input result differs");
  }
  auto wrong=sampled;wrong.name="unrelated";bool refused=false;
  try{make_original_matchmaking_emblem(std::move(wrong));}catch(const FormatError&){refused=true;}
  check(refused,"Offline provider accepted another owner");
  std::cout<<"PASS96 offline DVD reference pages and640 native appearance controls; production uses admitted MCC assets\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
