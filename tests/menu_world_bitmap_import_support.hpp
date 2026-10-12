#pragma once
#include "halo2/menu_world_source_geometry.hpp"
#include "halo2/menu_world_source_pass.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/world_material.hpp"
#include "halo2/frontend.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

// Test-only shared source identity/import support. Independent fixture bytes
// are expectations; candidate texture storage always comes from Cache.
namespace menu_world_bitmap_import {
using namespace halo2;
namespace fs=std::filesystem;
using Data=std::vector<std::byte>;
inline void require(bool value,const std::string &message) {if(!value)throw FormatError(message);}
inline unsigned number(const std::string &text,unsigned radix=10) {
  std::size_t used{};const auto result=std::stoull(text,&used,radix);
  require(used==text.size() && result<=0xffffffffu,"invalid bitmap import fixture integer");return unsigned(result);
}
inline std::vector<std::string> fields(const std::string &line) {
  std::vector<std::string> result;std::istringstream input(line);std::string value;
  while(std::getline(input,value,'\t'))result.push_back(value);return result;
}
inline Data read(const fs::path &path,std::size_t size) {
  std::ifstream input(path,std::ios::binary|std::ios::ate);require(bool(input),"original bitmap expectation missing: "+path.string());
  require(input.tellg()==std::streamoff(size),"original bitmap expectation extent changed");Data result(size);input.seekg(0);
  require(bool(input.read(reinterpret_cast<char *>(result.data()),size)),"original bitmap expectation truncated");return result;
}
inline void save(const fs::path &path,const Data &data) {
  std::ofstream output(path,std::ios::binary);
  require(bool(output.write(reinterpret_cast<const char *>(data.data()),data.size())),"cannot retain imported bitmap texels");
}
inline std::size_t differences(std::span<const std::byte> a,std::span<const std::byte> b) {
  require(a.size()==b.size(),"imported bitmap texel extent differs");std::size_t result{};
  for(std::size_t i=0;i<a.size();++i)result+=a[i]!=b[i];return result;
}
struct Mip {unsigned face{},level{},width{},height{};Data original,sampled;};
struct Stage {
  unsigned parameter{},external{},datum{},index{},source_format{},width{},height{},layers{},mips{};
  bool alpha_one{};std::vector<Mip> expected;
};
struct Fixture {std::array<Stage,4> stages;};
inline Fixture fixture(const fs::path &path) {
  std::ifstream input(path/"manifest.tsv");std::string line;
  require(bool(std::getline(input,line)) && line=="H2_MENU_WORLD_BITMAP_IMPORT_1335_V1","bitmap import manifest version differs");
  require(bool(std::getline(input,line)) && line=="1335\t0\t58\t52\t2\tE777054D\t0\t3\t0\t3","bitmap import scope changed");
  Fixture result;std::array<bool,4> seen{};unsigned mip_count{};
  while(std::getline(input,line)) {
    const auto f=fields(line);require(!f.empty(),"empty bitmap import fixture row");
    if(f[0]=="S") {
      require(f.size()==12,"invalid bitmap import stage row");const auto stage=number(f[1]);
      require(stage<4 && !seen[stage],"duplicate bitmap import stage");seen[stage]=true;auto &s=result.stages[stage];
      s.parameter=number(f[2]);s.external=number(f[3]);s.datum=number(f[4],16);s.index=number(f[5]);s.source_format=number(f[6]);
      s.width=number(f[7]);s.height=number(f[8]);s.layers=number(f[9]);s.mips=number(f[10]);s.alpha_one=number(f[11])!=0;
      require(s.width && s.height && (s.layers==1 || s.layers==6) && s.mips,"invalid bitmap import shape");
    } else {
      require(f[0]=="M" && f.size()==8,"invalid bitmap import mip row");const auto stage=number(f[1]);
      require(stage<4 && seen[stage],"bitmap mip precedes stage");auto &s=result.stages[stage];
      Mip m{number(f[2]),number(f[3]),number(f[4]),number(f[5]),{}, {}};
      require(m.face<s.layers && m.level<s.mips && m.width==std::max(1u,s.width>>m.level) &&
          m.height==std::max(1u,s.height>>m.level),"bitmap import mip shape differs");
      for(const auto &name:{f[6],f[7]})require(!fs::path(name).has_parent_path(),"bitmap fixture path escapes directory");
      const auto size=std::size_t(m.width)*m.height*4;m.original=read(path/f[6],size);m.sampled=read(path/f[7],size);
      s.expected.push_back(std::move(m));++mip_count;
    }
  }
  require(std::all_of(seen.begin(),seen.end(),[](bool value){return value;}) && mip_count==91,"bitmap import coverage differs");
  for(auto &s:result.stages) {
    require(s.expected.size()==std::size_t(s.layers)*s.mips,"bitmap mip/face coverage differs");
    std::sort(s.expected.begin(),s.expected.end(),[](const auto &a,const auto &b){return std::pair{a.face,a.level}<std::pair{b.face,b.level};});
    for(unsigned face=0;face<s.layers;++face)for(unsigned level=0;level<s.mips;++level) {
      const auto &m=s.expected[face*s.mips+level];require(m.face==face && m.level==level,"duplicate/missing bitmap mip");
    }
  }
  return result;
}
inline std::uint32_t normalization_bitmap(const Cache &cache) {
  const Tag *globals=nullptr;for(const auto &tag:cache.tags())if(tag.group=="matg") {
    require(!globals,"multiple matg tags in bitmap fixture");globals=&tag;
  }
  require(globals,"matg bitmap fixture missing");const auto metadata=cache.load_source_tag_metadata(globals->datum);
  const Bytes root(metadata.bytes_at(metadata.source_address(),metadata.size()));
  require(root.u32(0x108)==1,"normalization rasterizer block count changed");
  const Bytes rasterizer(metadata.bytes_at(root.u32(0x10c),0x68));
  require(rasterizer.u32(8)==0x6269746du,"normalization reference is not a bitmap");return rasterizer.u32(12);
}
inline ShaderBitmap identity(const Cache &cache,const Material &material,const MaterialPass &pass,unsigned stage) {
  const ShaderTextureBinding *binding=nullptr;for(const auto &b:pass.textures)if(b.stage==stage) {
    require(!binding,"duplicate source bitmap stage");binding=&b;
  }
  require(binding && !binding->flags,"source bitmap stage missing/unsupported");
  if(binding->external) {
    require(binding->external==1 && binding->parameter==255,"unproved bitmap extern");return {normalization_bitmap(cache),0,0};
  }
  const ShaderRemap *remap=nullptr;for(const auto &r:pass.bitmap_remaps)if((r.destination&255)==binding->parameter) {
    require(!remap,"ambiguous source bitmap remap");remap=&r;
  }
  require(remap && remap->source<material.bitmaps.size(),"source bitmap remap missing");return material.bitmaps[remap->source];
}
// Both diagnostics enter through the same actual Cache/material preparation.
// This owns unchanged native payloads; it performs no CPU BC expansion.
struct ImportedStage {Texture texture;ShaderBitmap bitmap;unsigned source_format{};};
using Imported=std::array<ImportedStage,4>;
inline Imported imports(const Cache &cache,const Fixture &expected) {
  const auto owner=load_menu_world_source_bsp_owner(cache,0,58);require(owner.section==52,"original1335 owner section changed");
  const auto section=load_menu_world_source_bsp_section(cache,0,MenuWorldSourceSectionKind::bsp_instance_definition,owner.section);
  require(section.parts().size()>2 && section.parts()[2].shader==0xe777054du,"original1335 actual source part shader differs");
  const auto chain=load_menu_world_source_pass(cache,section.parts()[2].shader,{0,3,0,3});
  require(chain.shader_pass()==0xe1c90054u && chain.vertex_shader()==0xe1cc0057u,"original1335 explicit pass chain changed");
  auto material=cache.load_material(chain.shader());material.selected_shader_lod=0;
  require(prepare_authored_lighting_material(cache,material),"native original1335 authored lighting material unavailable");
  require(material.forward_pass && material.forward_pass_index<material.passes.size(),"native original1335 pass not selected");
  const auto &pass=material.passes[material.forward_pass_index];
  require(pass.template_pass==chain.template_pass() && pass.implementation==3,"native selected bitmap pass differs from explicit source chain");
  const Texture *textures[]{&material.diffuse,&material.detail,&material.bump,&material.auxiliary};
  Imported result;
  for(unsigned stage=0;stage<4;++stage) {
    const auto &e=expected.stages[stage];const auto id=identity(cache,material,pass,stage);
    const auto binding=std::find_if(pass.textures.begin(),pass.textures.end(),[&](const auto &b){return b.stage==stage;});
    require(binding!=pass.textures.end() && binding->parameter==e.parameter && binding->external==e.external,
        "native bitmap parameter/extern binding differs from original implementation");
    require(id.datum==e.datum && id.index==e.index,"native source bitmap identity differs from literal original binding");
    const auto source=cache.load_bitmap_resource(id.datum,id.index);
    require(source.format==e.source_format,"native literal bitmap source format changed");
    const auto &t=*textures[stage];require(t.width==e.width && t.height==e.height && t.layers==e.layers &&
        !t.depth && t.mips.size()==std::size_t(e.layers)*e.mips,"native full mip/face import differs from original view");
    const auto direct=cache.load_bitmap(id.datum,id.index);
    require(direct.format==t.format && direct.pixels==t.pixels && direct.mips.size()==t.mips.size(),
        "native selected material changed imported bitmap payload");
    for(unsigned i=0;i<t.mips.size();++i) {
      require(t.mips[i].width==e.expected[i].width && t.mips[i].height==e.expected[i].height,
          "native mip dimensions differ from original view");
    }
    result[stage]={t,id,e.source_format};
  }
  return result;
}
} // namespace menu_world_bitmap_import
