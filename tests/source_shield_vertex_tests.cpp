#include "halo2/cache.hpp"
#include "halo2/model.hpp"
#include "halo2/content.hpp"
#include "halo2/source_cpu_skin_inputs.hpp"
#include "halo2/source_vertex_inputs.hpp"
#include "halo2/source_model_palette.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/world_material.hpp"
#include "gpu_compute.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace halo2;
namespace {
void check(bool v,const char *why){if(!v)throw FormatError(why);}
template<class F>void rejects(F &&f){bool caught=false;try{f();}catch(const FormatError&){caught=true;}check(caught,"Unproved offset input variant admitted");}
ShaderVector xyz(Vec3 v){return {v.x,v.y,v.z,1};}
struct Fixture {
  std::array<VertexInstruction,136> instructions{};std::array<ShaderVector,16> attributes{};
  std::array<std::uint32_t,4> flags{};std::array<ShaderVector,192> constants{};
  std::array<ShaderVector,5> streams{};ShaderVector local_position{};
  PackedSourceVertexDeclaration declaration{};
};
static_assert(sizeof(Fixture)==5632);
std::array<ShaderVector,192> constants(){
  std::array<ShaderVector,192> c{};
  c[0]={1,0,0,0};c[1]={0,1,0,0};c[2]={0,0,1,0};c[3]={1,0,0,4};
  c[4]={1,0,0,1};c[5]={0,1,0,.5f};c[6]={-1,0,0,2};c[7]={3,1,2,767.8125f};
  c[10]={320,-240,1,1};c[11]={.5f,.5f,0,1};
  c[12]={.1f,.2f,.3f,.1f};c[13]={.3f,.2f,.1f,.1f};c[14]={.2f,.1f,.3f,.1f};c[15]={.2f,0,1,1};
  for(unsigned i=18;i<24;++i)c[i]={float(i)/32,.25f,.5f,.75f};
  c[26]={0,0,0,.05f};c[170]={1,1,1,0};return c;
}
std::array<Transform,4> nodes(){
  std::array<Transform,4> n{};
  n[0].basis={Vec3{0,1,0},Vec3{-1,0,0},Vec3{0,0,1}};n[0].position={.25f,-.5f,.125f};
  n[1].basis={Vec3{1,0,0},Vec3{0,0,1},Vec3{0,-1,0}};n[1].position={-.25f,.5f,.75f};
  n[2].basis={Vec3{0,0,1},Vec3{0,1,0},Vec3{-1,0,0}};n[2].position={.5f,.25f,-.5f};
  n[3].basis={Vec3{1,0,0},Vec3{0,0,1},Vec3{0,-1,0}};n[3].position={-.5f,-.25f,.5f};return n;
}
std::array<ShaderVector,16> attributes(){std::array<ShaderVector,16> a{};for(auto &v:a)v={.125f,.25f,.5f,.75f};return a;}
bool exact(const std::array<ShaderVector,16> &a,const std::array<ShaderVector,16> &b){return std::bit_cast<std::array<std::uint32_t,64>>(a)==std::bit_cast<std::array<std::uint32_t,64>>(b);}
void assets(const Cache &cache) {
  const auto content=cache.load_content(0,true);unsigned loaded=0;std::size_t vertices=0;
  for(unsigned representation:{2u,3u}) {
    const auto &r=content.player_representations.at(representation);
    for(auto datum:{r.biped.model,r.hands}) {
      const auto model=cache.load_model(datum,false);const auto pose=pose_model(model);
      check(pose.size()==model.mesh.vertices.size(),"Shield pose changed source vertex count");
      for(unsigned i=0;i<pose.size();++i) {
        const auto original=xyz(model.mesh.vertices[i].position);
        check(std::bit_cast<std::array<std::uint32_t,4>>(xyz(model.mesh.vertices[i].source_local_position))==
              std::bit_cast<std::array<std::uint32_t,4>>(original),"Model loader lost decompressed local position");
        check(std::bit_cast<std::array<std::uint32_t,4>>(xyz(pose[i].source_local_position))==
              std::bit_cast<std::array<std::uint32_t,4>>(original),"Current pose changed shield noise position");
        ++vertices;
      }
      ++loaded;
    }
  }
  check(loaded==4&&vertices>1000,"Incomplete actual body/FP local-position coverage");
  std::cout<<loaded<<" actual Spartan/Elite body/arms models retained "<<vertices<<" local positions through posing\n";
}
}
int main(int argc,char **argv) {
  try {
    const auto path=std::filesystem::path(HALO2_SOURCE_DIR)/"assets/disc/maps/lockout.map";
    if(!std::filesystem::exists(path))return 77;Cache cache(path);assets(cache);
    const auto world_bytes=cache.load_vertex_shader_code(0xe95307ce,0);
    const auto world=decode_vertex_program(world_bytes);
    const auto declaration=cache.load_source_vertex_declaration(0xe95307ce,0);
    const auto current_nodes=nodes();std::array<Transform,4> inverse{};
    const auto palette=source_model_skin_palette(current_nodes,inverse);
    constexpr std::array<std::array<std::uint8_t,4>,3> weights{{{128,128,0,0},{0,128,128,0},{1,250,200,100}}};
    std::vector<Fixture> fixtures;std::vector<std::array<ShaderVector,16>> expected;
    unsigned exact_words=0,texture_controls=0,normal_controls=0;
    for(unsigned classification=1;classification<=6;++classification)for(unsigned variant=0;variant<3;++variant) {
      const auto source_bytes=cache.load_vertex_shader_code(0xe95307ce,classification);
      const auto transform=source_cpu_skin_input_transform(world_bytes,source_bytes,classification);
      check(transform==SourceWorldVertexInputTransform::OffsetLocalPosition,"Shield source class lost local-position transform");
      auto material=cache.load_material(0xe9f70872);check(prepare_shield_overlay_material(cache,material),"Shield vertex material missing");
      check(prepare_cpu_skin_material_inputs(cache,material,classification)&&material.world_vertex_input_transform==transform,
            "Shield compiled identity did not select its input adapter");
      const unsigned count=classification<4?1:classification-2;
      RenderModel model;model.bones.resize(4);model.mesh.vertices.resize(1);model.skin.resize(1);
      for(auto &bone:model.bones)bone.inverse={1,0,0,0,1,0,0,0,1,0,0,0};
      auto &v=model.mesh.vertices.front();v.position={.375f+float(variant)/16,-.625f,1.25f};v.source_local_position=v.position;
      v.normal={-.75f,.25f,.5f};v.tangent={.5f,.75f,-.25f};v.binormal={.25f,-.5f,.75f};
      model.skin.front().bones={0,1,2,3};model.skin.front().weights=source_model_skin_weights(weights[variant],count);
      const auto posed=skin_model_world_nodes(model,current_nodes).front();
      VertexProgramState original;original.attributes=attributes();original.constants=constants();
      for(unsigned n=0;n<palette.size();++n)for(unsigned row=0;row<3;++row)for(unsigned axis=0;axis<4;++axis)
        original.constants[50+n*3+row][axis]=palette[n][row*4+axis];
      const auto local_declaration=cache.load_source_vertex_declaration(0xe95307ce,classification);
      for(unsigned i=0;i<local_declaration.semantics.size();++i)switch(local_declaration.semantics[i]) {
      case 0:original.attributes[i]=xyz(v.position);break;
      case 1:original.attributes[i]={0,1/255.f,2/255.f,3/255.f};break;
      case 2:for(unsigned a=0;a<4;++a)original.attributes[i][a]=float(weights[variant][a])/255;break;
      case 4:original.attributes[i]=xyz(v.normal);break;case 5:original.attributes[i]=xyz(v.tangent);break;
      case 6:original.attributes[i]=xyz(v.binormal);break;default:throw FormatError("Unproved shield source semantic");
      }
      execute_vertex_program(decode_vertex_program(source_bytes),original);
      const auto add=[&](Vertex vertex,SourceWorldVertexInputTransform policy) {
        Fixture f;f.instructions=world.instructions;f.attributes=attributes();f.flags[3]=world.count;
        f.constants=original.constants;f.streams={xyz(vertex.position),xyz(vertex.normal),xyz(vertex.tangent),xyz(vertex.binormal),ShaderVector{}};
        f.local_position=xyz(vertex.source_local_position);f.declaration=pack_source_vertex_declaration(declaration,policy);
        VertexProgramState reference;reference.constants=f.constants;
        reference.attributes=source_world_vertex_inputs(declaration,vertex,f.attributes,policy);
        execute_vertex_program(world,reference);
        apply_cpu_skin_texture_coordinates(policy,vertex.source_local_position,reference.constants,reference.outputs);
        fixtures.push_back(f);expected.push_back(reference.outputs);return reference.outputs;
      };
      const auto actual=add(posed,transform);check(exact(actual,original.outputs),"Shield CPU lowering differs from actual local/skinned program");exact_words+=64;
      const auto wrong_texture=add(posed,SourceWorldVertexInputTransform::Preserve);
      check(!exact(wrong_texture,original.outputs),"Missing local noise coordinates control did not respond");++texture_controls;
      auto wrong_normal=posed;const auto n=posed.normal;const float magnitude=std::sqrt(n.x*n.x+n.y*n.y+n.z*n.z);
      wrong_normal.normal={n.x/magnitude,n.y/magnitude,n.z/magnitude};
      check(!exact(add(wrong_normal,transform),original.outputs),"Wrong shield normal normalization control did not respond");++normal_controls;
    }
    auto bad=cache.load_vertex_shader_code(0xe95307ce,6);bad.back()^=std::byte{2};
    rejects([&]{source_cpu_skin_input_transform(world_bytes,bad,6);});
    rejects([&]{source_cpu_skin_input_transform(world_bytes,bad,7);});
    rejects([&]{pack_source_vertex_declaration(SourceVertexDeclaration{0,{0,3}},SourceWorldVertexInputTransform::OffsetLocalPosition);});
    check(source_cpu_skin_input_transform(world_bytes,world_bytes,0)==SourceWorldVertexInputTransform::Preserve,
          "Original world class unexpectedly consumed local position");
    if(argc>1&&std::string_view(argv[1])=="--gpu") {
      const auto layers=std::getenv("VK_INSTANCE_LAYERS"),sync=std::getenv("VK_LAYER_VALIDATE_SYNC");
      check(layers&&std::string_view(layers).find("VK_LAYER_KHRONOS_validation")!=std::string_view::npos&&sync&&std::string_view(sync)=="1",
            "Shield GPU check requires Vulkan and synchronization validation");
      halo2_test::Compute<Fixture,16> gpu(HALO2_SHIELD_VERTEX_TEST_SHADER);const auto output=gpu.run(fixtures);
      check(output.size()==fixtures.size()*16,"Shield GPU output truncated");unsigned gpu_words=0;
      for(unsigned i=0;i<fixtures.size();++i)for(unsigned reg=0;reg<16;++reg)for(unsigned axis=0;axis<4;++axis) {
        const auto a=output[i*16+reg][axis],b=expected[i][reg][axis];
        check(std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<=2e-5f*std::max({1.f,std::abs(a),std::abs(b)}),
              "Production shield GPU lowering differs from original compiled-program output");++gpu_words;
      }
      std::cout<<gpu_words<<" production GPU output words within bounded arithmetic tolerance; ";
    }
    std::cout<<exact_words<<" CPU output words exact against all six original local/skinned classes, "
             <<texture_controls<<" local-position and "<<normal_controls<<" raw-normal controls\n";
    return 0;
  } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
