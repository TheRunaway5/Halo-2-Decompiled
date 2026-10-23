#include "halo2/cache.hpp"
#include "halo2/damage_feedback.hpp"
#include "halo2/shader_animation.hpp"
#include "halo2/world_material.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
std::vector<std::uint32_t> words(const Material &m) {
  std::vector<std::uint32_t> result(m.forward_program.words.begin(),m.forward_program.words.end());
  for(const auto &bank:m.program_constants)for(const auto &v:bank)for(float c:v)
    result.push_back(std::bit_cast<std::uint32_t>(c));
  return result;
}
struct Result {FunctionValue input;std::vector<std::uint32_t> baseline,animated;};
Result evaluate(const Cache &cache,std::span<const std::uint32_t> row,float camouflage=0.f) {
  check(row.size()==8,"Shield material fixture fields");
  auto material=cache.load_material(row[0]);
  check(!material.forward_pass,"Unresolved shield pass was admitted by ordinary material binder");
  check(prepare_shield_overlay_material(cache,material),"Authored shield pass binding failed");
  bind_object_camouflage(material,camouflage);
  const bool overshield=material.passes.at(material.forward_pass_index).name==
      "shaders\\shader_passes\\plasma\\overshield";
  check(material.source_shader_datum==row[0]&&material.selected_shader_lod==0&&
        material.forward_pass_index==0&&material.forward_vertex==1&&material.forward_blend==3,
        "Shield authored pass selection changed");
  check(material.forward_raster.color_write==0x00010101u&&!material.forward_raster.depth_write,
        "Shield RGB/additive/no-depth-write raster states changed");
  check(material.diffuse.depth&&material.detail.depth&&material.forward_program.words[54]==0x42u&&
        !(material.forward_program.words[59]&0x100u),"Shield authored PROJECT3D volume bindings changed");
  check(material.vertex_declaration&&material.vertex_declaration->class_index==0&&
        material.vertex_declaration->semantics==std::vector<std::uint16_t>({0,4,5,6}),
        "Shield authored class0 input declaration changed");
  const auto functions=cache.load_object_functions(row[1]);
  DamageFeedbackState state;state.shield.visible=std::bit_cast<float>(row[2]);
  const float body=std::bit_cast<float>(row[3]),shield=std::bit_cast<float>(row[4]);
  const bool dead=row[5];
  const auto input=[&](std::uint32_t name) {
    return evaluate_object_function(functions,*material.animation.tables,name,0,{},
        [&](std::uint32_t requested)->std::optional<FunctionValue> {
          if(const auto value=damage_feedback_input(state,body,shield,dead,requested))
            return FunctionValue{value->value,value->active};
          return {};
        });
  };
  Result result;result.input=input(overshield?0x1800055au:
      row[1]==0xea1b0896u?0x12001a0bu:0x0a0012b4u);
  result.baseline=words(material);
  const double seconds=std::bit_cast<double>(std::uint64_t(row[6])|(std::uint64_t(row[7])<<32));
  check(animate_forward_material(material,seconds,[&](auto name){return input(name).value;}),
        "Shield authored animated constants did not bind");
  result.animated=words(material);
  material.forward_program.words.fill(0xfeedface);
  for(auto &bank:material.program_constants)for(auto &v:bank)v.fill(-23.f);
  animate_forward_material(material,seconds,[&](auto name){return input(name).value;});
  check(words(material)==result.animated,"Shield animation retained stale per-object constants");
  return result;
}
}
int main(int argc,char **argv) {
  try {
    check(argc==3 || (argc==4 && std::string(argv[3])=="--camouflage"),
        "Supply Lockout and shield material fixture/--probe, optionally --camouflage");
    if(!std::filesystem::exists(argv[1]))return 77;
    Cache cache(argv[1]);const bool camouflaged=std::string(argv[2])=="--probe-camouflaged" || argc==4;
    const bool probe=std::string(argv[2])=="--probe" || std::string(argv[2])=="--probe-camouflaged";
    std::ifstream file;if(!probe){if(!std::filesystem::exists(argv[2]))return 77;file.open(argv[2]);}
    auto &stream=probe?std::cin:file;unsigned count=0;bool responds=false;
    for(std::string line;std::getline(stream,line);) {
      if(line.empty()||line[0]=='#')continue;
      std::istringstream row(line);row>>std::hex;std::array<std::uint32_t,8> fields{};
      for(auto &value:fields)check(bool(row>>value),"Truncated shield material input");
      const auto result=evaluate(cache,fields,camouflaged?1.f:0.f);
      std::vector<std::uint32_t> actual{std::bit_cast<std::uint32_t>(result.input.value),result.input.enabled};
      if(probe)actual.insert(actual.end(),result.baseline.begin(),result.baseline.end());
      actual.insert(actual.end(),result.animated.begin(),result.animated.end());
      if(probe) {for(auto value:actual)std::cout<<std::hex<<value<<' ';std::cout<<'\n';}
      else {
        for(auto value:actual){std::uint32_t expected;check(bool(row>>expected),"Truncated original shield constant bank");
          check(value==expected,"Original shield input/animated constant bank differs");}
        std::string extra;check(!(row>>extra),"Trailing shield material fixture field");
        responds|=result.animated!=result.baseline;
      }
      ++count;
    }
    if(!probe){check(count==240&&responds,"Incomplete original shield material corpus");
      std::cout<<count<<" actual Spartan/Elite FP/world shield input and animation cases matched bit for bit\n";}
    return 0;
  } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
