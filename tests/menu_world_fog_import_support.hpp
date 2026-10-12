#pragma once
#include "halo2/atmosphere.hpp"
#include "halo2/menu_world_source_geometry.hpp"
#include "halo2/menu_world_source_pass.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <span>
extern "C" {
#include <libavutil/mem.h>
#include <libavutil/sha.h>
}

// Test-only original1880 image identities. Expected images come exclusively
// from the unchanged original export; candidate images come exclusively from
// the actual Cache material and prepare_menu_bsp_fog. No live admission policy.
namespace menu_world_fog_import {
using namespace halo2;
namespace fs=std::filesystem;
using Data=std::vector<std::byte>;
inline void require(bool value,const std::string &message) {
  if(!value)throw FormatError(message);
}
inline std::string digest(std::span<const std::byte> bytes) {
  std::unique_ptr<AVSHA,decltype(&av_free)> context(av_sha_alloc(),av_free);
  require(context && av_sha_init(context.get(),256)==0,"fog import SHA256 context unavailable");
  av_sha_update(context.get(),reinterpret_cast<const std::uint8_t *>(bytes.data()),bytes.size());
  std::array<std::uint8_t,32> raw{};av_sha_final(context.get(),raw.data());
  const char *hex="0123456789abcdef";std::string result;
  for(const auto value:raw) {result.push_back(hex[value>>4]);result.push_back(hex[value&15]);}
  return result;
}
inline Data read(const fs::path &path,std::size_t size) {
  std::ifstream input(path,std::ios::binary|std::ios::ate);
  require(bool(input) && input.tellg()==std::streamoff(size),"fog import fixture missing or extent changed: "+path.string());
  Data result(size);input.seekg(0);
  require(bool(input.read(reinterpret_cast<char *>(result.data()),result.size())),"fog import fixture truncated");
  return result;
}
inline std::size_t differences(std::span<const std::byte> a,std::span<const std::byte> b) {
  require(a.size()==b.size(),"fog import comparison extent differs");std::size_t count{};
  for(std::size_t i=0;i<a.size();++i)count+=a[i]!=b[i];return count;
}
inline std::uint32_t gradient_bitmap(const Cache &cache) {
  const Tag *globals=nullptr;
  for(const auto &tag:cache.tags())if(tag.group=="matg") {
    require(!globals,"fog import has multiple globals tags");globals=&tag;
  }
  require(globals && globals->datum==0xe1740000u,"fog import globals identity differs");
  const auto metadata=cache.load_source_tag_metadata(globals->datum);
  const Bytes root(metadata.bytes_at(metadata.source_address(),metadata.size()));
  require(root.u32(0x108)==1,"fog import rasterizer block count differs");
  const Bytes rasterizer(metadata.bytes_at(root.u32(0x10c),0x1c8));
  require(rasterizer.u32(0x10)==0x6269746du && rasterizer.u32(0x14)==0xe4570237u,
      "fog import rasterizer gradients reference differs");
  return rasterizer.u32(0x14);
}
struct StageIdentity {
  // Resource/view/sampler numbers identify the independent original capture,
  // never new Vulkan handles or production Cache resource allocations.
  unsigned stage{},external{},datum{},index{},resource{},view{},sampler{};
  unsigned width=16,height=16,layers=1,mips=1,source_format=11;
  bool alpha_one=false;
};
inline constexpr std::array<StageIdentity,2> identities{{
  {0,14,0xe4570237u,0,13842,13843,13844},
  {1,15,0xe4570237u,1,13846,13847,13848}
}};
struct ExpectedStage {StageIdentity identity;Data original_bgra,rgba;std::string original_sha256;};
using Fixture=std::array<ExpectedStage,2>;
inline Fixture fixture(const fs::path &original_export) {
  const auto manifest=read(original_export/"original-images.json",240281);
  require(digest(manifest)=="62cbb846ca1bf90409874299c402f0bfd14cb6520fac3a9ffa1f7a713ae060c3",
      "original1880 image export identity changed");
  constexpr std::array<const char *,2> hashes{
    "b4980da59a29dbe1498a9fbc802d7f5fdff66c262b4c55ec3a22c41260a08c85",
    "c06636e96bcf1ddcf7b25202dc01945f8e92488723b91831b20a9e3c82523ce2"};
  Fixture result;
  for(unsigned stage=0;stage<2;++stage) {
    auto &image=result[stage];image.identity=identities[stage];image.original_sha256=hashes[stage];
    image.original_bgra=read(original_export/("image-"+image.original_sha256+".bin"),1024);
    require(digest(image.original_bgra)==image.original_sha256,"original1880 gradient payload identity changed");
    // Actual original resource/view is BGRA8 with identity components. Cache
    // format11 owns RGBA bytes; convert expectations explicitly even though
    // these particular gradient pages have identical R/G/B values.
    image.rgba=image.original_bgra;
    for(std::size_t i=0;i<image.rgba.size();i+=4)std::swap(image.rgba[i],image.rgba[i+2]);
  }
  require(differences(result[0].rgba,result[1].rgba)>0,"original gradient pages no longer distinguish their stage binding");
  return result;
}
inline void topology(const Texture &texture) {
  require(texture.width==16 && texture.height==16 && texture.layers==1 && !texture.depth &&
      texture.format==11 && texture.pixels.size()==1024 && texture.mips.size()==1,
      "prepared fog gradient format/shape/topology differs from original1880");
  const auto &m=texture.mips.front();
  require(m.width==16 && m.height==16 && m.depth==1 && m.offset==0 && m.size==1024,
      "prepared fog gradient mip extent differs from original1880");
}
struct ImportedStage {StageIdentity identity;Texture texture;ShaderTextureState authored_sampler;};
using Imported=std::array<ImportedStage,2>;
inline Imported imports(const Cache &cache) {
  const auto section=load_menu_world_source_model_section(cache,0xe1d70062u,2);
  require(!section.parts().empty() && section.parts()[0].material==0 && section.parts()[0].shader==0xe1d80063u,
      "original1880 render-model section/part material identity differs");
  const auto chain=load_menu_world_source_pass(cache,section.parts()[0].shader,{0,16,0,0});
  require(chain.shader_pass()==0xe1b90044u && chain.vertex_shader()==0xe1ba0045u,
      "original1880 actual planar pass identity differs");
  Mesh mesh;mesh.materials.push_back(cache.load_material(section.parts()[0].shader));
  require(mesh.materials[0].source_shader_datum==0xe1d80063u,"actual prepared fog material owner differs");
  prepare_menu_bsp_fog(cache,mesh);
  require(mesh.materials[0].planar_fog_pass!=nullptr,"actual original1880 material did not prepare its planar pass");
  const auto &fog=*mesh.materials[0].planar_fog_pass;
  require(fog.forward_pass && fog.forward_pass_index==0 && fog.passes.size()==1,
      "actual original1880 material did not preserve its selected fog pass");
  const auto &pass=fog.passes[0];
  require(pass.template_pass==chain.template_pass() && pass.implementation==0 && pass.fragments.empty() &&
      pass.name=="shaders\\shader_passes\\fog\\fog_planar" &&
      pass.vertex_shader=="rasterizer\\vertex_shaders_xbox\\fog_planar" && pass.textures.size()==2 &&
      pass.texture_states.size()==2,"actual original1880 prepared pass binding differs");
  const auto datum=gradient_bitmap(cache);
  const auto source_metadata=cache.load_source_tag_metadata(datum);
  const Bytes bitmap(source_metadata.bytes_at(source_metadata.source_address(),source_metadata.size()));
  require(bitmap.u32(68)==5,"original gradient tag page count differs");
  const auto source_address=bitmap.u32(72);
  const Texture *textures[]{&fog.diffuse,&fog.detail};
  const ShaderTextureState expected_sampler{3,3,3,2,2,0,0,0,0,0,0,0,0,0};
  Imported result;
  for(unsigned stage=0;stage<2;++stage) {
    const auto &binding=pass.textures[stage];
    require(binding.parameter==255 && binding.external==identities[stage].external &&
        binding.stage==stage && binding.flags==0,"actual fog gradient extern14/15 stage binding differs");
    const Bytes record(source_metadata.bytes_at(source_address+stage*116,116));
    require(record.u16(4)==16 && record.u16(6)==16 && record.u16(8)==1 && record.u16(10)==0 &&
        record.u16(12)==11 && record.u16(14)==4617 && record.u16(20)==0 && record.u32(52)==1024,
        "actual source gradient page metadata differs from original1880");
    topology(*textures[stage]);
    const auto direct=cache.load_bitmap(datum,stage);topology(direct);
    require(textures[stage]->pixels==direct.pixels,"actual fog material changed the Cache imported gradient");
    require(pass.texture_states[stage]==expected_sampler && fog.texture_states[stage]==expected_sampler,
        "actual fog material changed original authored sampler state");
    result[stage]={identities[stage],*textures[stage],fog.texture_states[stage]};
  }
  return result;
}
} // namespace menu_world_fog_import
