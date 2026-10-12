#include "menu_world_fog_import_support.hpp"
#include "halo2/frontend_material.hpp"
#include <iostream>

namespace {
namespace fog=menu_world_fog_import;
using namespace halo2;
void save(const fog::fs::path &path,std::span<const std::byte> bytes) {
  std::ofstream output(path,std::ios::binary);
  fog::require(bool(output.write(reinterpret_cast<const char *>(bytes.data()),bytes.size())),
      "cannot retain actual imported fog image");
}
}
int main(int argc,char **argv) {
  fog::fs::path output;bool owns_output=false,complete=false;std::size_t compared{},difference_count{},controls{};
  try {
    fog::require(argc==4,"fog import test needs mainmenu.map,original-image-export-directory,fresh-output-directory");
    output=argv[3];fog::require(!fog::fs::exists(output),"fog import output must be a fresh directory");
    fog::fs::create_directories(output);owns_output=true;
    const auto map=fog::read(argv[1],59670016);
    fog::require(fog::digest(map)=="1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b",
        "original mainmenu cache identity differs");
    const auto expected=fog::fixture(argv[2]);const Cache cache(argv[1]);auto actual=fog::imports(cache);
    std::ofstream metadata(output/"native-fog-imports.tsv");
    metadata<<"stage\texternal\tbitmap_datum\tpage\toriginal_resource\toriginal_format\tnative_format\twidth\theight\tlayers\tmips\tRGBA_differences\tRGBA_sha256\n";
    for(unsigned stage=0;stage<2;++stage) {
      auto &image=actual[stage];const auto &reference=expected[stage];
      const auto different=fog::differences(image.texture.pixels,reference.rgba);
      compared+=reference.rgba.size();difference_count+=different;
      save(output/("stage-"+std::to_string(stage)+"-native-rgba.bin"),image.texture.pixels);
      metadata<<stage<<'\t'<<image.identity.external<<'\t'<<image.identity.datum<<'\t'<<image.identity.index<<'\t'
          <<reference.identity.resource<<"\tBGRA8\tRGBA8\t16\t16\t1\t1\t"<<different<<'\t'<<fog::digest(image.texture.pixels)<<'\n';
      fog::require(different==0,"actual Cache/prepared fog gradient differs from independent original1880 image");
      // Format11 is uncompressed. Actual frontend storage preparation must not
      // reinterpret or alter it when the ordinary material pipeline runs.
      auto prepared=image.texture;
      fog::require(!prepare_frontend_texture_storage(prepared) && prepared.pixels==image.texture.pixels,
          "frontend storage changed an original uncompressed fog gradient");fog::topology(prepared);
      // Independent pages make a mistaken extern14/15 swap detectable, and a
      // modified texel makes the exact byte comparison demonstrably sensitive.
      fog::require(fog::differences(image.texture.pixels,expected[1-stage].rgba)>0,
          "wrong-gradient-page negative control is insensitive");++controls;
      auto changed=image.texture.pixels;changed[stage*4]^=std::byte{1};
      fog::require(fog::differences(changed,reference.rgba)==1,
          "single-component negative control is insensitive");++controls;
    }
    fog::require(bool(metadata),"fog import metadata write failed");complete=true;
  }catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';
    // Retain failure measurements without inventing an accepted comparison.
    if(owns_output && fog::fs::is_directory(output)) {
      std::ofstream report(output/"report.json");
      report<<"{\"status\":\"failed\",\"complete\":false,\"compared_RGBA_bytes\":"<<compared
          <<",\"differing_RGBA_bytes\":"<<difference_count<<"}\n";
    }
    return 1;
  }
  std::ofstream report(output/"report.json");
  report<<"{\"status\":\"passed\",\"complete\":"<<(complete?"true":"false")
      <<",\"event\":1880,\"sampled_stages\":2,\"compared_RGBA_bytes\":"<<compared
      <<",\"differing_RGBA_bytes\":"<<difference_count<<",\"negative_controls\":"<<controls
      <<",\"actual_Cache_material_import_verified\":true,\"GPU_executed\":false,\"live_admission_verified\":false}\n";
  if(!report){std::cerr<<"fog import report write failed\n";return 1;}
  std::cout<<"Actual original1880 Cache/material gradients: "<<compared<<" RGBA bytes, "<<difference_count
      <<" differences, extern14/15+single mip/stage+authored sampler exact; "<<controls<<" sensitive controls\n";
  return 0;
}
