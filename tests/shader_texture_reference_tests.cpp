#include "halo2/cache.hpp"
#include "halo2/source_shader_texture.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool condition,const char *message){if(!condition)throw FormatError(message);}
nlohmann::json json(const std::filesystem::path &path){std::ifstream file(path);check(bool(file),"Missing shader texture source report");return nlohmann::json::parse(file);}
void word(std::vector<std::byte> &bytes,std::size_t offset,std::uint32_t value) {
  check(offset<=bytes.size() && bytes.size()-offset>=4,"Texture test patch outside declared metadata");
  for(unsigned i=0;i<4;++i)bytes[offset+i]=std::byte((value>>(i*8))&255u);
}
}
int main(int argc,char **argv)try {
  check(argc==5,"Disc maps, map-pack maps, original pass report and asset census required");
  const auto report=json(argv[3]),assets=json(argv[4]);
  Cache lockout(std::filesystem::path(argv[1])/"lockout.map");
  unsigned prefixes=0;
  for(const auto &row:report.at("texture_resolution_prefixes")) {
    const auto reference=source_shader_texture_reference(lockout,row.at("shader"),row.at("external"));
    check(reference.bitmap==row.at("input").at("datum") && reference.frame==row.at("input").at("index"),
        "Shader texture reference differs from unchanged original19530");
    ++prefixes;
  }
  check(prefixes==24,"Original texture prefix coverage changed");
  unsigned maps=0;
  for(const auto &row:assets.at("maps")) {
    const auto name=row.at("map").get<std::string>()+".map";
    auto path=std::filesystem::path(argv[1])/name;
    if(!std::filesystem::exists(path))path=std::filesystem::path(argv[2])/name;
    Cache cache(path);
    check(source_camouflage_shader(cache)==row.at("rasterizer_rows").at(0).at("global_shader"),
        "Camouflage reference differs from actual owned-map census");
    const auto normal=source_shader_texture_reference(cache,0xffffffffu,1);
    check(normal.frame==0 && cache.load_bitmap(normal.bitmap,normal.frame).layers==6,
        "Authored normalization reference does not load its cube resource");
    const auto bump=source_shader_texture_reference(cache,source_camouflage_shader(cache),26);
    const auto texture=cache.load_bitmap(bump.bitmap,bump.frame);
    check(texture.width && texture.height && !texture.pixels.empty(),"Authored global bump resource did not load");
    ++maps;
  }
  check(maps==21,"Owned-map texture reference coverage changed");
  // Declared edited-tag inputs demonstrate field routing. These are separate
  // from the unchanged original-execution comparisons above.
  const auto tag=std::find_if(lockout.tags().begin(),lockout.tags().end(),[](const auto&t){return t.group=="matg";});
  check(tag!=lockout.tags().end(),"Original matg missing");
  const auto globals=lockout.load_source_tag_metadata(tag->datum);
  const auto raw=globals.bytes_at(globals.source_address(),globals.size());
  const auto rasterizer=Bytes(raw).u32(0x10c);
  const auto backing=std::find_if(lockout.tags().begin(),lockout.tags().end(),[&](const auto &t){
    return rasterizer>=t.address && std::uint64_t(rasterizer)+0xd4<=std::uint64_t(t.address)+t.size;
  });
  check(backing!=lockout.tags().end(),"Declared rasterizer row has no actual resident owner");
  const auto resident=lockout.load_source_tag_metadata(backing->datum);
  const auto original_row=resident.bytes_at(rasterizer,0xd4);
  std::vector<std::byte> edited(original_row.begin(),original_row.end());
  const auto alternate=source_shader_texture_reference(lockout,source_camouflage_shader(lockout),26);
  word(edited,0xc,alternate.bitmap);
  const SourceTagMetadataProviders changed{
    [&](auto datum)->const SourceTagMetadata &{check(datum==globals.datum(),"Unexpected declared tag owner");return globals;},
    [&](auto datum){check(datum==globals.datum(),"Unexpected declared tag address");return globals.source_address();},
    [&](auto address,auto extent)->std::span<const std::byte>{
      check(address>=rasterizer && std::uint64_t(address)-rasterizer<=edited.size() &&
          extent<=edited.size()-(std::uint64_t(address)-rasterizer),"Declared edited row read escapes backing");
      return std::span<const std::byte>(edited).subspan(address-rasterizer,extent);
    }};
  check(source_shader_texture_reference(changed,globals.datum(),0xffffffffu,1)==SourceShaderTextureReference{alternate.bitmap,0},
      "Edited normalization reference was ignored in favor of a bitmap name");
  bool rejected=false;try{(void)source_shader_texture_reference(changed,globals.datum(),0xffffffffu,17);}catch(const FormatError&){rejected=true;}
  check(rejected,"Framebuffer extern was substituted without its target owner");
  std::cout<<prefixes<<" unchanged original texture prefixes, "<<maps
      <<" owned-map reference/resources and edited-tag routing passed\n";
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
