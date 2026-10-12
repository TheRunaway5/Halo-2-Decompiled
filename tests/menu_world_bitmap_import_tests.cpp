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

#include "menu_world_bitmap_import_support.hpp"
#include "halo2/frontend_material.hpp"
// Actual Cache/material import versus independently exported original1335
// whole-resource texels. BC1/BC3 source expansion is a CPU diagnostic only:
// the live renderer still uploads those imported blocks compressed.
using namespace halo2;
namespace {
using namespace menu_world_bitmap_import;
// Only the retained formats are admitted. Actual production Cache::load_bitmap
// and selected Material preparation own decoding/layout. The existing source
// BC helpers diagnose the contents of the compressed imported mip, without
// silently substituting expanded storage in the renderer.
Data decoded_mip(const Texture &texture,const TextureMip &mip) {
  const Bytes source(texture.pixels);const auto bytes=source.slice(mip.offset,mip.size);
  if(texture.format==11) {
    require(mip.size==std::size_t(mip.width)*mip.height*4,"native RGBA8 mip size differs");return {bytes.begin(),bytes.end()};
  }
  if(texture.format==16) {
    const auto decoded=prepare_ui_bitmap(Bytes(bytes),mip.width,mip.height,16,0);
    require(decoded.format==11 && decoded.layers==1 && decoded.mips.size()==1,"source BC3 diagnostic decode failed");return decoded.pixels;
  }
  require(texture.format==14,"unadmitted native bitmap format");
  const Texture face{mip.width,mip.height,14,{bytes.begin(),bytes.end()},{{mip.width,mip.height,0,mip.size}},1};
  return prepare_xemu_bc1_texture(face).pixels;
}
void storage_controls(const Imported &native,const Fixture &expected) {
  Material material;material.diffuse=native[0].texture;material.detail=native[1].texture;
  material.bump=native[2].texture;material.auxiliary=native[3].texture;
  for(unsigned stage=0;stage<4;++stage)material.animated_textures[stage].push_back(native[stage].texture);
  auto shared=std::make_shared<Material>();shared->diffuse=native[1].texture;
  material.environment_pass=shared;material.planar_fog_pass=shared;
  material.world_overlays_before.push_back(shared);material.world_overlays_after.push_back(shared);
  material.world_bloom_passes.push_back(shared);material.world_planar_passes[0]=shared;
  prepare_frontend_material_storage(material);
  const Texture *stages[]{&material.diffuse,&material.detail,&material.bump,&material.auxiliary};
  for(unsigned stage=0;stage<4;++stage) {
    Data original;for(const auto &m:expected.stages[stage].expected)original.insert(original.end(),m.sampled.begin(),m.sampled.end());
    for(const auto *t:std::array<const Texture*,2>{stages[stage],&material.animated_textures[stage][0]}) {
      require(t->format==11 && t->layers==native[stage].texture.layers && !t->depth &&
          t->mips.size()==native[stage].texture.mips.size(),"frontend source storage lost native mip/face topology");
      require(t->pixels==original,"production frontend source storage differs from independently retained original texels");
      for(unsigned i=0;i<t->mips.size();++i)require(t->mips[i].width==expected.stages[stage].expected[i].width &&
          t->mips[i].height==expected.stages[stage].expected[i].height && t->mips[i].size==expected.stages[stage].expected[i].sampled.size(),
          "frontend expanded mip logical extent changed");
    }
  }
  for(const auto &child:{material.environment_pass,material.planar_fog_pass,
      material.world_overlays_before[0],material.world_overlays_after[0],material.world_bloom_passes[0],material.world_planar_passes[0]})
    require(child!=shared && child->diffuse.format==11 && child->diffuse.layers==6,
        "frontend child storage conversion escaped ownership or omitted a pass");
  require(shared->diffuse.format==14 && shared->diffuse.pixels==native[1].texture.pixels,
      "frontend storage mutated a separately retained child owner");
  Material unsupported;unsupported.detail=native[0].texture;unsupported.detail.depth=1;
  const auto before_volume=unsupported.detail.pixels;
  prepare_frontend_material_storage(unsupported);
  require(unsupported.detail.depth==1 && unsupported.detail.pixels==before_volume && unsupported.rendering_gaps.size()==1,
      "unproved volume storage was converted or its boundary hidden");
  unsigned rejected{};
  const auto rejects=[&](Texture malformed) {
    try{(void)prepare_xemu_bc_texture(malformed);}catch(const FormatError &){++rejected;return;}
    throw FormatError("source BC preparer admitted unsupported/malformed storage");
  };
  auto bad=native[0].texture;bad.format=17;rejects(bad);
  bad=native[0].texture;bad.depth=1;rejects(bad);
  bad=native[0].texture;bad.layers=2;rejects(bad);
  bad=native[0].texture;bad.mips.clear();rejects(bad);
  bad=native[1].texture;bad.mips.pop_back();rejects(bad);
  bad=native[1].texture;--bad.mips[bad.mips.size()/6].width;rejects(bad);
  bad=native[0].texture;bad.mips[0].offset=bad.pixels.size();rejects(bad);
  bad=native[0].texture;--bad.mips[0].size;rejects(bad);
  bad=native[0].texture;bad.mips[0].depth=2;rejects(bad);
  bad=native[0].texture;bad.width=0;rejects(bad);
  bad=native[0].texture;bad.pixels.clear();rejects(bad);
  require(rejected==11,"source BC rejection coverage changed");
  auto cyclic=std::make_shared<Material>();cyclic->environment_pass=cyclic;
  bool cycle=false;try{prepare_frontend_material_storage(*cyclic);}catch(const FormatError &){cycle=true;}
  cyclic->environment_pass.reset();require(cycle,"frontend storage recursion admitted a cycle");
  std::cout<<"Frontend production storage: all4 original banks+animated frames,6 child ownership paths,11 malformed/unsupported controls and cycle rejection\n";
}
struct Measurement {unsigned stage{},source_format{},native_format{},mips{};std::size_t bytes{},resource_diff{},sampled_diff{};};
} // namespace

int main(int argc,char **argv) {try {
  require(argc==4,"bitmap import test needs mainmenu.map,1335fixture,fresh-output");
  if(!fs::exists(argv[1]) || !fs::exists(fs::path(argv[2])/"manifest.tsv"))return 77;
  const fs::path output=argv[3];require(!fs::exists(output),"bitmap import output must be fresh");fs::create_directories(output);
  const auto expected=fixture(argv[2]);const Cache cache(argv[1]);
  const auto imported_textures=imports(cache,expected);
  storage_controls(imported_textures,expected);
  std::array<Measurement,4> measurements{};std::size_t all_bytes{},all_mips{},all_diff{};
  for(unsigned stage=0;stage<4;++stage) {
    const auto &e=expected.stages[stage];const auto &native=imported_textures[stage];
    const auto id=native.bitmap;const auto &t=native.texture;
    Measurement measure{stage,e.source_format,t.format,unsigned(t.mips.size())};Data imported;
    for(unsigned i=0;i<t.mips.size();++i) {
      const auto &m=t.mips[i];const auto &original=e.expected[i];
      require(m.width==original.width && m.height==original.height,"native mip dimensions differ from original view");
      const auto decoded=decoded_mip(t,m);
      measure.resource_diff+=differences(decoded,original.original);
      measure.sampled_diff+=differences(decoded,original.sampled);
      measure.bytes+=decoded.size();imported.insert(imported.end(),decoded.begin(),decoded.end());
    }
    save(output/("stage-"+std::to_string(stage)+"-native-source-decoded-rgba.bin"),imported);
    measurements[stage]=measure;all_bytes+=measure.bytes;all_mips+=measure.mips;all_diff+=measure.sampled_diff;
    std::cout<<"stage "<<stage<<" bitmap "<<std::hex<<id.datum<<std::dec<<" source format "<<e.source_format
        <<", native storage "<<t.format<<", "<<measure.mips<<" mip/faces, "<<measure.sampled_diff
        <<" differing sampled RGBA bytes (resource bytes "<<measure.resource_diff<<", alphaONE "<<e.alpha_one<<")\n";
  }
  require(all_bytes==3429580 && all_mips==91,"native original1335 texture coverage differs");
  std::ofstream report(output/"report.json");report<<"{\n  \"status\":\""<<(all_diff?"failed":"passed")<<"\",\n"
      <<"  \"event\":1335,\n  \"mip_faces\":"<<all_mips<<",\n  \"sampled_RGBA_bytes\":"<<all_bytes
      <<",\n  \"sampled_RGBA_differing_bytes\":"<<all_diff<<",\n  \"measurements\":[\n";
  for(unsigned i=0;i<measurements.size();++i) {const auto &m=measurements[i];report<<"    {\"stage\":"<<m.stage
      <<",\"source_format\":"<<m.source_format<<",\"native_storage_format\":"<<m.native_format
      <<",\"mip_faces\":"<<m.mips<<",\"bytes\":"<<m.bytes<<",\"resource_RGBA_differing_bytes\":"<<m.resource_diff
      <<",\"sampled_RGBA_differing_bytes\":"<<m.sampled_diff<<"}"<<(i==3?"\n":",\n");}
  report<<"  ],\n  \"scope\":\"Actual source-identified Cache/material import; original whole-resource texels and alphaONE view semantics. Compressed BC1/BC3 source expansions are CPU diagnostics using existing source decoders; native GPU compressed decoding/filtering, texture residency/LOD and live renderer admission remain unproved.\"\n}\n";
  report.flush();require(bool(report),"cannot retain bitmap import report");
  require(!all_diff,"native original1335 bitmap import differs from independently retained sampled texels");
  std::cout<<"4 source-bound bitmap imports / 91 mip/faces / 3429580 sampled RGBA bytes exact; no GPU/runtime admission\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
