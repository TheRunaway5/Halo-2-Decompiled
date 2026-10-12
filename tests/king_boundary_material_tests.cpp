#include "halo2/cache.hpp"
#include "halo2/world_material.hpp"
#include "halo2/source_vertex_inputs.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/texture_program.hpp"
#include "halo2/shader_animation.hpp"
#include "gpu_compute.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool pass,const std::string& why){if(!pass)throw FormatError(why);}
std::vector<std::byte> bytes(const std::string& hex) {
  std::vector<std::byte> out;for(unsigned i=0;i<hex.size();i+=2)out.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));return out;
}
std::uint32_t datum(const Json& text){return std::uint32_t(std::stoul(text.get<std::string>(),nullptr,16));}
ShaderVector vector(const Json& words) {
  ShaderVector out{};for(unsigned i=0;i<words.size();++i)out.at(i)=std::bit_cast<float>(words[i].get<std::uint32_t>());return out;
}
bool exact(const ShaderVector& a,const ShaderVector& b){return std::bit_cast<std::array<std::uint32_t,4>>(a)==std::bit_cast<std::array<std::uint32_t,4>>(b);}
struct Fixture {
  std::array<VertexInstruction,136> instructions{};
  std::array<ShaderVector,16> attributes{};
  std::array<std::uint32_t,4> flags{};
  std::array<ShaderVector,192> constants{};
  std::array<ShaderVector,10> streams{};
  PackedSourceVertexDeclaration declaration=legacy_source_vertex_declaration;
};
static_assert(sizeof(Fixture)==5696);
bool same_texture(const Texture& a,const Texture& b) {
  if(a.width!=b.width || a.height!=b.height || a.format!=b.format || a.layers!=b.layers || a.depth!=b.depth || a.pixels!=b.pixels || a.mips.size()!=b.mips.size())return false;
  for(unsigned i=0;i<a.mips.size();++i){const auto& x=a.mips[i];const auto& y=b.mips[i];if(x.width!=y.width || x.height!=y.height || x.depth!=y.depth || x.offset!=y.offset || x.size!=y.size)return false;}
  return true;
}
}
int main(int argc,char** argv)try {
  const std::filesystem::path root=HALO2_SOURCE_DIR;
  std::ifstream visual_file(root/"analysis/multiplayer/game-variants/king-visual-consumers-source-20261012/complete/report.json");Json visual;visual_file>>visual;
  std::ifstream emission_file(root/"analysis/multiplayer/game-variants/king-hill-emission-source-20261012/complete/report.json");Json emission;emission_file>>emission;
  unsigned maps{};Material lockout;
  for(const auto& row:visual.at("cache_inventory")) {
    const auto path=root/row.at("path").get<std::string>();Cache cache(path);
    const auto shader=cache.load_multiplayer_hill_shader();check(shader==datum(row.at("hill_shader").at("datum")),"Native hill selector differs from actual settings+C4");
    const auto original=cache.load_material(shader);auto material=original;
    check(prepare_simple_world_material(cache,material),"Actual settings-selected simple-world hill material remains unbound");
    check(material.source_shader_datum==shader && material.selected_shader_lod==0 && material.forward_vertex==12 && material.world_pass && material.forward_pass,
      "Hill material lost selected source identity or immediate RGBA11 input");
    check(material.forward_pass_index<original.passes.size(),"Hill material selected absent pass");
    const auto& pass=original.passes[material.forward_pass_index];
    check(pass.implementation==0 && material.forward_program.words==pass.program.words &&
      material.vertex_program==pass.vertex_code && material.program_constants==pass.constants &&
      material.vertex_declaration==pass.vertex_declaration,"Hill binder changed authored programs/declaration/constants");
    check(material.vertex_program.size()==132 && texture_program_supported(material.forward_program),"Actual hill programs are not executable");
    check(material.vertex_declaration && material.vertex_declaration->semantics==std::vector<std::uint16_t>{0,3},"Actual hill lost position/UV register ordinals");
    check(material.forward_blend==3 && material.forward_raster.color_write==0x10101 && material.forward_raster.depth_write==0 &&
      material.forward_raster.alpha_test==1 && material.forward_raster.cull==0,
      "Authored hill pass state order differs: blend="+std::to_string(material.forward_blend)+
      " color="+std::to_string(material.forward_raster.color_write)+" depth="+std::to_string(material.forward_raster.depth_write)+
      " alpha="+std::to_string(material.forward_raster.alpha_test)+" cull="+std::to_string(material.forward_raster.cull));
    const auto& bitmap_row=row.at("hill_shader_decoded").at("postprocess")[0].at("bitmaps")[0];
    const auto bitmap=datum(bitmap_row.at("bitmap").at("datum"));
    check(same_texture(material.diffuse,cache.load_bitmap(bitmap,bitmap_row.at("index").get<std::uint32_t>())),"Hill pass lost actual bitmap bytes, dimensionality or authored mip chain");
    const auto before=material.program_constants;const auto before_program=material.forward_program.words;ShaderInput input;
    const bool animated=animate_forward_material(material,.5,input);
    check(animated && (before!=material.program_constants || before_program!=material.forward_program.words),"Authored animated hill overlays became a static tint");
    if(path.filename()=="lockout.map")lockout=original;
    ++maps;
  }
  check(maps==12 && !lockout.passes.empty(),"Incomplete disc-map hill material coverage");
  Cache cache(root/"assets/disc/maps/lockout.map");check(prepare_simple_world_material(cache,lockout),"Lockout immediate-color material unavailable");
  const auto program=decode_vertex_program(lockout.vertex_program);
  std::vector<Fixture> fixtures;std::vector<std::array<ShaderVector,16>> inputs,outputs;unsigned source_vertices{},legacy_controls{};
  for(const auto& row:emission.at("cases"))for(const auto& vertex:row.at("vertices")) {
    const auto p=vector(vertex.at("position0_bits")),uv=vector(vertex.at("uv1_bits")),rgba=vector(vertex.at("rgba11_bits"));
    Vertex native;native.position={p[0],p[1],p[2]};native.uv={uv[0],uv[1]};native.color={rgba[0],rgba[1],rgba[2]};native.source_color_alpha=rgba[3];
    Fixture f;f.instructions=program.instructions;f.attributes=lockout.program_constants[0];f.flags={0,1,12,program.count};
    std::copy(lockout.program_constants[1].begin(),lockout.program_constants[1].end(),f.constants.begin()+18);
    // Declared CPU/GPU comparison view; this fixture does not claim the
    // original draw's camera/program-bank setup or final projected pixels.
    f.constants[0]={1,0,0,0};f.constants[1]={0,1,0,0};f.constants[2]={0,0,1,0};f.constants[3]={0,0,0,1};
    f.constants[10]={320,-240,16777215,0};f.constants[11]={320.5f,240.5f,0,0};
    f.declaration=pack_source_vertex_declaration(lockout.vertex_declaration);
    f.streams[0]=p;f.streams[1]=uv;f.streams[7]=rgba;
    auto attributes=source_immediate_color11_vertex_inputs(*lockout.vertex_declaration,native,f.attributes);
    check(exact(attributes[0],p) && exact(attributes[1],ShaderVector{uv[0],uv[1],0,1}) && exact(attributes[11],rgba),"Native immediate register mapping lost emitted source data");
    VertexProgramState state;state.attributes=attributes;state.constants=f.constants;execute_vertex_program(program,state);
    check(exact(state.outputs[3],rgba),"Actual hill program lost source RGBA11 color output");
    fixtures.push_back(f);inputs.push_back(attributes);outputs.push_back(state.outputs);++source_vertices;
    // The mode remains explicit: ordinary models preserve their uniform v11.
    f.flags[2]=1;attributes=source_world_vertex_inputs(*lockout.vertex_declaration,native,f.attributes);
    state.attributes=attributes;execute_vertex_program(program,state);
    fixtures.push_back(f);inputs.push_back(attributes);outputs.push_back(state.outputs);++legacy_controls;
  }
  check(source_vertices==1452 && legacy_controls==1452,"Incomplete immediate-color vertex/control corpus");
  if(argc>1 && std::string_view(argv[1])=="--gpu") {
    const auto layer=std::getenv("VK_INSTANCE_LAYERS"),sync=std::getenv("VK_LAYER_VALIDATE_SYNC");
    check(layer && std::string_view(layer).find("VK_LAYER_KHRONOS_validation")!=std::string_view::npos && sync && std::string_view(sync)=="1","King GPU check requires Vulkan and synchronization validation");
    halo2_test::Compute<Fixture,32> gpu(HALO2_VERTEX_DECLARATION_TEST_SHADER);const auto result=gpu.run(fixtures);
    check(result.size()==fixtures.size()*32,"GPU immediate-color result count differs");unsigned exact_inputs{},exact_outputs{},max_ulp{};
    for(unsigned n=0;n<fixtures.size();++n)for(unsigned reg=0;reg<16;++reg) {
      check(exact(result[n*32+reg],inputs[n][reg]),"Production GLSL immediate RGBA11/declaration inputs differ");exact_inputs+=4;
      for(unsigned axis=0;axis<4;++axis) {
        const auto a=result[n*32+16+reg][axis],b=outputs[n][reg][axis];const auto aw=std::bit_cast<std::uint32_t>(a),bw=std::bit_cast<std::uint32_t>(b);
        if(aw==bw){++exact_outputs;continue;}
        const auto ulp=(aw>>31)==(bw>>31)?(aw>bw?aw-bw:bw-aw):0xffffffffu;
        check(std::isfinite(a) && std::isfinite(b) && (ulp<=8 || std::abs(a-b)<=2e-5f),"Production authored hill vertex output differs");
        if(ulp!=0xffffffffu)max_ulp=std::max(max_ulp,ulp);
      }
    }
    std::cout<<exact_inputs<<" production GLSL input words exact; "<<exact_outputs<<" authored output words exact, maxULP "<<max_ulp<<'\n';
  }
  std::cout<<maps<<" actual settings-selected hill materials retain original program/texture/animation data; "<<source_vertices<<" emitted vertices and "<<legacy_controls<<" uniform-color controls pass\n";
  std::cout<<"LIMIT authored pass binding and host shader comparison; source preparation overrides, world draw, pixels and normal King admission remain separate\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
