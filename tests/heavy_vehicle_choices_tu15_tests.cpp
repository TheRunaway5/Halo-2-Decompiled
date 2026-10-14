#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <bit>
using namespace halo2;
namespace {void check(bool b,const char* why){if(!b)throw FormatError(why);}}
int main(int argc,char**argv)try{
  check(argc==2,"Supply original TU1.5 heavy-choice fixture");std::ifstream f(argv[1]);if(!f)return 77;
  nlohmann::json fixture;f>>fixture;check(fixture.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong heavy-choice visual authority");
  const auto original=OriginalData::load_xbe(fixture.at("xbe_path").get<std::string>());
  check(original->image==OriginalData::Image::TitleUpdate15,"Heavy choices require the original TU1.5 executable");
  unsigned count=0,children=0;
  for(const auto& map:fixture.at("maps")){
    Cache cache(map.at("cache_path").get<std::string>(),fixture.at("shared_maps").get<std::string>(),original);
    for(int mode:{0,1,2,4}){
      auto variant=source_game_variant_default(0);source_game_variant_set(variant,78,mode);
      const auto content=cache.load_content(0,true,variant);
      for(const auto& c:map.at("cases"))if(c.at("mode")==mode){
        const auto row=c.at("row").get<unsigned>(),definition=c.at("definition").get<std::uint32_t>();
        const auto i=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto&o){return o.netgame_row==int(row) && !o.model_attachment;});
        if(definition==null_datum)check(i==content.objects.end(),"Original NONE choice created a native vehicle template");
        else {
          check(i!=content.objects.end() && i->definition==definition && i->variant==c.at("variant"),"Native heavy definition/variant differs from original 15CC70");
          const auto words=[](Vec3 v){return std::array{std::bit_cast<std::uint32_t>(v.x),std::bit_cast<std::uint32_t>(v.y),std::bit_cast<std::uint32_t>(v.z)};};
          check(words(i->position)==c.at("position_words").get<std::array<std::uint32_t,3>>() &&
              words(i->rotation)==c.at("rotation_words").get<std::array<std::uint32_t,3>>() && i->flags==1,"Heavy choice changed original position/rotation or cold admission");
          const auto placement=std::size_t(i-content.objects.begin());
          for(const auto&o:content.objects)if(o.model_attachment && o.model_attachment->parent==placement)++children;
        }
        ++count;
      }
    }
    auto random=source_game_variant_default(0);source_game_variant_set(random,78,3);bool rejected=false;
    try{cache.load_content(0,true,random);}catch(const FormatError&){rejected=true;}check(rejected,"Unresolved random heavy selection was admitted");
  }
  check(count>0 && children>0,"Heavy-choice regression omitted models or their authored children");
  std::cout<<"PASS "<<count<<" original heavy choices, "<<children<<" authored child templates; random remains explicit\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
