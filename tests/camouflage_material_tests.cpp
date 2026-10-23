#include "halo2/world_material.hpp"
#include "halo2/shader_animation.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
Material decode(const nlohmann::json &row) {
  Material m;m.forward_pass=true;m.forward_pass_index=0;m.passes.emplace_back();
  auto &pass=m.passes[0];pass.program.words=row.at("initial").get<std::array<std::uint32_t,60>>();
  m.shader_switches[0]=row.at("selected");
  if(row.at("fragmented").get<bool>()) {
    pass.fragments.push_back({0,{std::uint16_t(8<<9)}});
    pass.program.fragment_offsets[0]=row.at("offset");
  }
  const auto remaps=[](const nlohmann::json &input) {
    std::vector<ShaderRemap> result;
    for(const auto &v:input){const auto word=v.get<std::uint32_t>();
      result.push_back({std::uint16_t(word),std::uint8_t(word>>16),std::uint8_t(word>>24)});}
    return result;
  };
  pass.pixel_externs=remaps(row.at("externs"));
  auto &a=m.animation;a.mappings=remaps(row.at("animated"));
  a.lods.push_back({1,{std::uint16_t(1<<9)}});a.layers.push_back({std::uint16_t(1<<9)});
  a.passes.push_back({std::uint16_t(1<<9)});
  a.implementations.push_back({{},{std::uint16_t(a.mappings.size()<<9)},{}});
  const auto function=row.at("function").get<std::array<std::uint32_t,6>>();
  ShaderOverlay overlay;overlay.period=std::bit_cast<float>(function[0]);
  for(unsigned i=1;i<function.size();++i)for(unsigned b=0;b<4;++b)overlay.curve.push_back(std::byte(function[i]>>(b*8)));
  a.parameters.push_back({std::uint16_t(1<<9)});a.references.push_back({0,0});a.overlays.push_back(overlay);
  a.tables=std::make_shared<ShaderFunctionTables>();m.forward_program=pass.program;
  return m;
}
}
int main(int argc,char **argv)try {
  check(argc==2,"Original material animation fixture required");
  std::ifstream file(argv[1]);check(bool(file),"Original material animation fixture missing");
  nlohmann::json fixture;file>>fixture;unsigned cases=0;bool late_detected=false,missing_detected=false;
  for(const auto &row:fixture.at("cases")) {
    auto material=decode(row);const auto original=material;
    const float camouflage=std::bit_cast<float>(row.at("camouflage").get<std::uint32_t>());
    const float seconds=std::bit_cast<float>(row.at("seconds").get<std::uint32_t>());
    auto alias_program=original.forward_program;auto alias_constants=original.program_constants;
    auto alias_blend=original.forward_blend_color;
    bind_object_camouflage_constants(alias_program,alias_constants,alias_blend,
        original.passes[0],original.shader_switches,camouflage);
    bind_object_camouflage(material,camouflage);
    const auto before=row.at("before_animation").get<std::array<std::uint32_t,60>>();
    check(material.forward_program.words==before,"Original19CA0 camouflage bank differs");
    check(material.passes[0].program.words==before,"Retained camouflage base differs");
    check(alias_program.words==before && alias_constants==material.program_constants &&
        alias_blend==material.forward_blend_color,"Independent draw camouflage differs from original material binding");
    check(animate_forward_material(material,seconds,{}),"Camouflage animation not admitted");
    const auto after=row.at("after_animation").get<std::array<std::uint32_t,60>>();
    if(material.forward_program.words!=after)throw FormatError("Original camouflage/animation mismatch case "+std::to_string(cases));
    material.forward_program.words.fill(0xfeedface);
    animate_forward_material(material,seconds,{});
    check(material.forward_program.words==after,"Previous animated frame leaked into camouflage base");
    auto missing=original;animate_forward_material(missing,seconds,{});
    missing_detected|=missing.forward_program.words!=after;
    auto late=original;animate_forward_material(late,seconds,{});bind_object_camouflage(late,camouflage);
    late_detected|=late.forward_program.words!=after;
    // Several owners begin with the same child allocation, as actual model
    // copies do. The binding must detach it before writing either bank.
    Material parent;parent.world_overlays_after.push_back(std::make_shared<Material>(original));
    auto other=parent;bind_object_camouflage(parent,camouflage);
    check(parent.world_overlays_after[0]!=other.world_overlays_after[0],"Camouflage owners retained a shared mutable child");
    check(other.world_overlays_after[0]->forward_program.words==original.forward_program.words,"Camouflage changed another owner");
    check(parent.world_overlays_after[0]->forward_program.words==before,"Child camouflage input differs from original");
    ++cases;
  }
  check(cases!=0 && late_detected && missing_detected,"Missing/reversed camouflage binding controls did not differ");
  std::cout<<cases<<" unchanged original camouflage/animation banks matched; stale, shared-owner and wrong-order controls passed\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
