#include "xemu_bc2_texture_support.hpp"
#include "halo2/frontend.hpp"
#include "halo2/frontend_frame_texture.hpp"
#include "halo2/frontend_material.hpp"
#include <array>
#include <iostream>

using namespace halo2;
namespace {
using namespace xemu_bc2_test;
bool same(const Texture &a,const Texture &b) {
  if(a.width!=b.width || a.height!=b.height || a.format!=b.format || a.layers!=b.layers ||
      a.depth!=b.depth || a.pixels!=b.pixels || a.mips.size()!=b.mips.size())return false;
  for(unsigned i=0;i<a.mips.size();++i) {
    const auto &x=a.mips[i],&y=b.mips[i];
    if(x.width!=y.width || x.height!=y.height || x.offset!=y.offset || x.size!=y.size || x.depth!=y.depth)return false;
  }
  return true;
}
template<class Function> void rejects(Function function,const std::string &label,unsigned &count) {
  try{function();}catch(const FormatError &){++count;return;}
  throw FormatError("BC2 malformed/unsupported input admitted: "+label);
}
const Image &image(const std::vector<Image> &images,const std::string &label) {
  const auto found=std::find_if(images.begin(),images.end(),[&](const auto &item){return item.label==label;});
  require(found!=images.end(),"missing named original BC2 image: "+label);return *found;
}
void synthetic_cases(const fs::path &directory) {
  const auto cases=load_synthetic(directory);
  for(const auto &test:cases) {
    const auto before=test.source;
    exact(prepare_xemu_bc2_texture(test.source),test.source,test.expected,test.label+" direct");
    exact(prepare_xemu_bc_texture(test.source),test.source,test.expected,test.label+" generic");
    auto frontend=test.source;
    require(prepare_frontend_texture_storage(frontend),test.label+": frontend omitted BC2 conversion");
    exact(frontend,test.source,test.expected,test.label+" frontend");
    require(same(test.source,before),test.label+": decoder mutated compressed input");
  }
  // The frozen original control deliberately has c0<c1. DXT1's three-color
  // palette changes24 RGB bytes; a misplaced transparent-RGB branch cannot
  // pass merely because its alpha happens to be decoded independently.
  const auto found=std::find_if(cases.begin(),cases.end(),[](const auto &item){return item.label=="colors0-rotation0-4x4";});
  require(found!=cases.end(),"original DXT1/DXT3 format control is missing");
  const auto &test=*found;Data rgb_block(test.source.pixels.begin()+8,test.source.pixels.end());
  const auto bc1=prepare_xemu_bc1_texture(Texture{4,4,14,rgb_block,{{4,4,0,8}}});
  std::size_t rgb_differences{};
  for(std::size_t i=0;i<test.expected.size();++i)if(i%4!=3)rgb_differences+=bc1.pixels[i]!=test.expected[i];
  require(rgb_differences==24,"original BC2 four-color negative control lost sensitivity");
  auto alpha_mutation=test.source;alpha_mutation.pixels[0]^=std::byte{1};
  const auto altered=prepare_xemu_bc2_texture(alpha_mutation);unsigned alpha_differences{};
  for(unsigned i=0;i<altered.pixels.size();++i)if(altered.pixels[i]!=test.expected[i]) {
    require(i%4==3,"BC2 alpha mutation changed RGB");++alpha_differences;
  }
  require(alpha_differences==1,"BC2 independent-alpha negative control lost sensitivity");
  std::cout<<"768 unchanged-original cases exact through direct/generic/frontend APIs; format/alpha controls sensitive\n";
}
void image_cases(const std::vector<Image> &images) {
  std::size_t mip_faces{},bytes{};
  for(const auto &test:images) {
    const auto before=test.source;
    const auto decoded=prepare_xemu_bc_texture(test.source);
    exact(decoded,test.source,test.expected,test.label+" generic");
    if(test.source.layers==1)exact(prepare_xemu_bc2_texture(test.source),test.source,test.expected,test.label+" direct");
    auto frontend=test.source;require(prepare_frontend_texture_storage(frontend),test.label+": frontend BC2 was not converted");
    exact(frontend,test.source,test.expected,test.label+" frontend");
    const auto expanded=frontend;
    require(!prepare_frontend_texture_storage(frontend) && same(frontend,expanded),
            test.label+": repeated frontend storage conversion changed expanded texels");
    require(same(test.source,before),test.label+": image decode mutated compressed input");
    // The actual compressed import API remains a separate ownership stage.
    // Cube resource faces include128-byte source padding, discarded by the
    // importer; the original oracle itself covers logical face-major mips.
    Texture imported;
    if(test.source.layers==1) {
      imported=prepare_texture(Bytes(test.source.pixels),test.source.width,test.source.height,15,false,test.levels-1);
    }else {
      const auto face_size=test.source.pixels.size()/6;
      const auto stride=(face_size+127)&~std::size_t(127);Data padded(stride*6,std::byte{0xd7});
      for(unsigned face=0;face<6;++face)
        std::copy_n(test.source.pixels.begin()+face*face_size,face_size,padded.begin()+face*stride);
      imported=prepare_cubemap(Bytes(padded),test.source.width,15,false,test.levels-1);
    }
    require(same(imported,test.source),test.label+": compressed importer lost source face/mip bytes");
    exact(prepare_xemu_bc_texture(imported),test.source,test.expected,test.label+" imported");
    mip_faces+=test.source.mips.size();bytes+=test.expected.size();
  }
  require(mip_faces==59 && bytes==9611464,"original BC2 image totals changed");
  // UI content and mod-export ownership retain the original compressed frame.
  // Expansion operates on a material copy, leaving the DDS stock page exact.
  const auto &frame=image(images,"shell-framing");
  const auto ui=prepare_ui_bitmap(Bytes(frame.source.pixels),frame.source.width,frame.source.height,15,0);
  require(same(ui,frame.source),"BC2 storage preparation changed compressed UI asset identity");
  const auto dds=frontend_frame_dds(ui);const auto reloaded=read_frontend_frame_dds(Bytes(dds),ui);
  require(same(reloaded,ui),"BC2 decoder changed the independent compressed DDS mod seam");
  auto prepared=reloaded;require(prepare_frontend_texture_storage(prepared),"reloaded BC2 edge texture omitted frontend expansion");
  exact(prepared,frame.source,frame.expected,"independent frame DDS");
  std::cout<<"11 original images /59 face-mips /9611464 RGBA bytes exact, including6 authored pages and65536 endpoint blocks\n";
}
void material_ownership(const std::vector<Image> &images) {
  const auto &test=image(images,"cube-three-mips");
  Material material;material.diffuse=test.source;material.detail=test.source;
  material.bump=test.source;material.auxiliary=test.source;
  for(auto &stage:material.animated_textures)stage.push_back(test.source);
  auto retained=std::make_shared<Material>();retained->diffuse=test.source;
  material.environment_pass=retained;material.planar_fog_pass=retained;
  material.world_overlays_before.push_back(retained);material.world_overlays_after.push_back(retained);
  material.world_bloom_passes.push_back(retained);material.world_planar_passes[0]=retained;
  prepare_frontend_material_storage(material);
  for(const auto *texture:{&material.diffuse,&material.detail,&material.bump,&material.auxiliary})
    exact(*texture,test.source,test.expected,"BC2 material stage");
  for(const auto &stage:material.animated_textures)exact(stage.at(0),test.source,test.expected,"BC2 animated frame");
  for(const auto &child:{material.environment_pass,material.planar_fog_pass,material.world_overlays_before.at(0),
      material.world_overlays_after.at(0),material.world_bloom_passes.at(0),material.world_planar_passes.at(0)}) {
    require(child!=retained,"BC2 child material conversion retained shared writable ownership");
    exact(child->diffuse,test.source,test.expected,"BC2 child material");
  }
  require(same(retained->diffuse,test.source),"BC2 conversion changed another retained material owner");
  require(material.rendering_gaps.empty(),"source-proved BC2 storage still records an unsupported-format gap");
  auto volume=test.source;volume.layers=1;volume.depth=1;const auto before=volume;
  require(!prepare_frontend_texture_storage(volume) && same(volume,before),"BC2 volume was silently treated as2D");
  Material unproved;unproved.diffuse=volume;prepare_frontend_material_storage(unproved);
  require(same(unproved.diffuse,before) && std::find(unproved.rendering_gaps.begin(),unproved.rendering_gaps.end(),
      "Frontend volume texture storage remains unverified")!=unproved.rendering_gaps.end(),
      "BC2 volume's unproved storage boundary is missing");
  std::cout<<"BC2 material stages/animated frames/6 child ownership paths exact; volumes remain unchanged and explicit\n";
}
void malformed_inputs(const std::vector<Image> &images) {
  const auto &valid=image(images,"clipped-2d-chain").source;unsigned count{};
  const auto reject=[&](Texture bad,const std::string &label) {
    rejects([&]{(void)prepare_xemu_bc_texture(bad);},label,count);
  };
  auto bad=valid;bad.format=17;reject(bad,"other format");
  bad=valid;bad.depth=1;reject(bad,"authored1x1x1 volume");
  bad=valid;bad.layers=2;reject(bad,"two layers");
  bad=valid;bad.layers=6;reject(bad,"nonsquare cube");
  bad=valid;bad.mips.clear();reject(bad,"empty chain");
  bad=valid;bad.width=0;reject(bad,"zero width");
  bad=valid;bad.height=0;reject(bad,"zero height");
  bad=valid;bad.width=4097;reject(bad,"width bound");
  bad=valid;bad.height=4097;reject(bad,"height bound");
  bad=valid;bad.mips[0].width=0;reject(bad,"zero mip width");
  bad=valid;bad.mips[1].height=2;reject(bad,"mip extent");
  bad=valid;bad.mips[0].depth=2;reject(bad,"mip depth");
  bad=valid;--bad.mips[0].size;reject(bad,"short block size");
  bad=valid;++bad.mips[0].size;reject(bad,"extra block size");
  bad=valid;bad.pixels.pop_back();reject(bad,"truncated tail block");
  bad=valid;bad.mips[0].offset=bad.pixels.size();reject(bad,"out-of-bounds offset");
  bad=valid;bad.mips[0].offset=std::numeric_limits<std::size_t>::max();reject(bad,"overflow offset");
  bad=valid;bad.mips.push_back(bad.mips.back());reject(bad,"excessive mip count");
  bad=image(images,"cube-three-mips").source;bad.mips.pop_back();reject(bad,"missing cube face level");
  bad=image(images,"cube-three-mips").source;--bad.mips[3].width;reject(bad,"second face base extent");
  bad=valid;bad.format=14;rejects([&]{(void)prepare_xemu_bc2_texture(bad);},"direct BC1",count);
  bad=valid;bad.format=16;rejects([&]{(void)prepare_xemu_bc2_texture(bad);},"direct BC3",count);
  bad=image(images,"cube-three-mips").source;rejects([&]{(void)prepare_xemu_bc2_texture(bad);},"direct cube",count);
  rejects([&]{(void)prepare_texture(Bytes(valid.pixels),9,7,15,true);},"swizzled BC2",count);
  rejects([&]{(void)prepare_texture(Bytes(valid.pixels),0,7,15,false);},"import zero extent",count);
  rejects([&]{(void)prepare_texture(Bytes(valid.pixels),9,7,15,false,4);},"import excessive mips",count);
  rejects([&]{(void)prepare_texture(Bytes(std::span<const std::byte>(valid.pixels).first(95)),9,7,15,false);},"import truncated block",count);
  rejects([&]{(void)prepare_ui_bitmap(Bytes(valid.pixels),9,7,15,16);},"linear-pitch BC2",count);
  rejects([&]{(void)prepare_volume_texture(Bytes(valid.pixels),4,4,1,15,false);},"compressed volume importer",count);
  require(count==29,"BC2 malformed input coverage changed");
  std::cout<<count<<" explicit malformed/unsupported BC2 controls rejected\n";
}
void actual_cache(const fs::path &path,const std::vector<Image> &images) {
  struct Actual {const char *label;std::uint32_t datum;unsigned index;};
  const std::array bindings{
      Actual{"title-logo",0xE52C030C,0},Actual{"shell-framing",0xE5350315,0},
      Actual{"full-dialog-panel",0xE59D037D,0},Actual{"full-dialog-secondary-page",0xE59D037D,1},
      Actual{"appearance-child-panel",0xE5590339,0},Actual{"create-profile-placeholder",0xE5EE03CE,0}};
  const Cache cache(path);
  for(const auto &binding:bindings) {
    const auto &test=image(images,binding.label);auto imported=cache.load_bitmap(binding.datum,binding.index);
    require(same(imported,test.source),test.label+": actual Cache bitmap differs from frozen original source bytes");
    require(prepare_frontend_texture_storage(imported),test.label+": actual Cache frontend conversion omitted BC2");
    exact(imported,test.source,test.expected,test.label+" actual Cache");
  }
  std::cout<<"6 original datum/page Cache imports exact; no GPU/filtering/visibility claim\n";
}
} // namespace
int main(int argc,char **argv) {try {
  require(argc==2 || argc==3,"BC2 test needs fixture-directory and optional mainmenu.map");
  const fs::path directory=argv[1];if(!fs::exists(directory/"manifest.tsv"))return 77;
  synthetic_cases(directory);const auto images=load_images(directory);image_cases(images);
  material_ownership(images);malformed_inputs(images);
  if(argc==3){if(!fs::exists(argv[2]))return 77;actual_cache(argv[2],images);}
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
