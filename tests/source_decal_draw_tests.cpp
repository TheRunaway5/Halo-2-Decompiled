#include "halo2/source_decal_draw.hpp"
#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <bit>
#include <iostream>
using namespace halo2;
int main(int argc,char**argv)try {
 if(argc!=6)throw FormatError("Supply owned XBE, Zanzibar cache, completed cache fixture, original layer fixture and original traversal fixture");
 for(unsigned i=1;i<6;++i)if(!std::filesystem::is_regular_file(argv[i]))return 77;
 nlohmann::json original;std::ifstream(argv[4])>>original;
 unsigned layers{};
 for(const auto &row:original.at("passes")) {
  const auto state=source_decal_layer_state_tu15(row.at("layer"));
  if(state.pixel.words!=row.at("program").get<std::array<std::uint32_t,60>>() ||
     state.cull!=row.at("cull") || state.depth_function!=row.at("depth_function") ||
     state.color_mask!=row.at("color_write"))throw FormatError("Cached decal original layer program/raster state changed");
  for(const auto &command:row.at("states")) {
   const unsigned index=command[0],value=command[1];
   if((index==62 && state.source_blend!=value) || (index==63 && state.destination_blend!=value) ||
      (index==74 && state.blend_equation!=value) ||
      (index==78 && std::bit_cast<std::uint32_t>(state.depth_bias)!=value))
    throw FormatError("Cached decal original blend/offset state changed");
  }
  ++layers;
 }
 if(layers!=6)throw FormatError("Missing original decal layer cases");
 if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]) || !std::filesystem::exists(argv[3]))return 77;
 Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
 SourceDecalScene scene(cache);if(!scene.active())throw FormatError("Original cached decal scene did not bind");
 nlohmann::json fixture;std::ifstream(argv[3])>>fixture;SourceRuntimeMemory memory;
 for(const auto&r:fixture.at("regions")) {
  std::vector<std::byte>b;
  if(r.contains("file")){std::ifstream f(std::filesystem::path(argv[3]).parent_path()/r.at("file").get<std::string>(),std::ios::binary);if(!f)return 77;for(char v;f.get(v);)b.push_back(std::byte(static_cast<unsigned char>(v)));}
  else{auto v=r.at("value").get<std::uint32_t>();for(unsigned i=0;i<4;++i)b.push_back(std::byte(v>>(i*8)));}
  memory.mount(r.at("address"),b,SourceRuntimeMemory::Access::ReadWrite,"Completed original cache fixture");
 }
 const std::array<std::byte,4> zero{};memory.mount(0x4c6858u,zero,SourceRuntimeMemory::Access::ReadWrite,"Declared original view serial");
 // Declared complete head-bank traversal, independent of camera/PVS acceptance.
 SourceBspAdmission view;for(unsigned i=0;i<512;++i)view.clusters.push_back({i,0,0xffff,0});
 const auto before=source_decal_vertex_cache_tu15(memory);
 std::array<ShaderVector,192> constants{};const auto draws=scene.draw(memory,view,constants,{0,0,640,480},{0,16777215,16,1048576});
 std::size_t quads{};for(const auto&d:draws){if(d.layer!=1 || !d.texture || d.color!=ShaderVector{1,1,1,1} || d.state.destination_blend!=0x303)throw FormatError("Source decal material/color binding differs");quads+=d.quads.size();}
 nlohmann::json traversal;std::ifstream(argv[5])>>traversal;
 if(draws.size()!=traversal.at("draws").size())throw FormatError("Source head traversal omitted or duplicated retained geometry");
 std::size_t original_quads{};
 for(unsigned i=0;i<draws.size();++i) {
  const auto &expected=traversal.at("draws").at(i);const auto &actual=draws[i];
  original_quads+=expected.at("vertices").get<unsigned>()/4;
  if(actual.datum!=expected.at("datum") || actual.primary!=expected.at("primary") ||
     actual.layer!=expected.at("layer") || actual.quads.size()*4!=expected.at("vertices"))
   throw FormatError("Original complete cache callback draw sequence changed");
 }
 if(quads!=original_quads)throw FormatError("Original callback cache quad count changed");
 const auto after=source_decal_vertex_cache_tu15(memory);if(after.size()!=before.size() || after.front().quads.front()[0].packed_projection!=before.front().quads.front()[0].packed_projection)throw FormatError("Draw planning changed retained geometry");
 const auto repeat=scene.draw(memory,view,constants,{0,0,640,480},{0,16777215,16,1048576});if(repeat.size()!=draws.size())throw FormatError("Next original view serial lost decals");
 std::cout<<"PASS "<<draws.size()<<" retained decal draws / "<<quads<<" quads, authored bitmap/color/program and view serial\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
