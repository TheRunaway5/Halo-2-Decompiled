#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/world_material.hpp"
#include <bit>
#include <algorithm>
#include <iostream>
using namespace halo2;
void require(bool value,const char *message){if(!value)throw FormatError(message);}
int main(int argc,char **argv){
 require(argc==3,"Supply the actual campaign map and English TU1.5 image");
 const auto original=OriginalData::load_xbe(argv[2]);
 require(original->image==OriginalData::Image::TitleUpdate15,"Cinematic material requires TU1.5");
 Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path(),original);
 const auto tag=cache.query_tag({.group=TagGroup::from_string("shad"),
     .name="objects\\cinematics\\human\\weapons\\battle_rifle\\shaders\\smg_cinematic"});
 require(tag!=nullptr,"Actual cinematic shader missing");
 const auto authored=cache.load_material(tag->datum);
 require(authored.lods.size()==1 && authored.lods[0].available_layers==(1u<<15) && authored.passes.size()==1,
     "Cinematic shader's authored LOD/layer/pass changed");
 const auto &pass=authored.passes[0];
 require(pass.vertex_externs.empty() && pass.pixel_externs.empty() && pass.state_externs.empty(),
     "Cinematic case has unresolved source externs");
 unsigned source{},destination{},operation{},color{},write{};
 for(const auto &state:pass.states)switch(state.index){
  case 62:source=state.value;break;case 63:destination=state.value;break;
  case 74:operation=state.value;break;case 75:color=state.value;break;case 67:write=state.value;break;
 }
 require(source==0x8001 && destination==0x303 && operation==0x8006 && color==0x00ffffff && write==0x1010101,
     "Cinematic source blend no longer has constant white RGB and zero alpha");
 auto prepared=authored;
 require(prepare_dynamic_material(cache,prepared,{},true),"Actual admitted cinematic material failed source preparation");
 require(prepared.forward_blend==5 && prepared.forward_blend_color==color,
     "Cinematic RGBA factors were replaced by ordinary premultiplied alpha");
 require(prepared.forward_pass_index==0 && prepared.forward_raster.color_write==write && !prepared.forward_raster.depth_write,
     "Cinematic selected pass/raster state differs");
 require(prepared.forward_program.words==pass.program.words && prepared.vertex_program==pass.vertex_code &&
     std::bit_cast<std::array<std::uint32_t,128>>(prepared.program_constants)==
     std::bit_cast<std::array<std::uint32_t,128>>(pass.constants) && std::ranges::equal(prepared.texture_states,pass.texture_states),
     "Cinematic original program/constants/samplers were changed");
 std::cout<<"PASS actual TU1.5 cinematic material: original layer15/program/constant RGB and zero-alpha blend retained\n";
}
