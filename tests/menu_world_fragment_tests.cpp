#include "halo2/menu_world_fragment.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
std::vector<std::byte> read(const std::filesystem::path &path,std::size_t size) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);
  require(bool(file) && file.tellg()==std::streamoff(size),"fragment admission original input extent differs");
  std::vector<std::byte> result(size);file.seekg(0);
  require(bool(file.read(reinterpret_cast<char *>(result.data()),size)),"fragment admission original input is truncated");
  return result;
}
unsigned number(const std::string &value) {
  std::size_t end{};const auto result=std::stoul(value,&end);
  require(end==value.size() && result<=std::numeric_limits<unsigned>::max(),"invalid fragment admission fixture number");
  return unsigned(result);
}
std::vector<std::byte> hexadecimal(const std::string &value) {
  require(value.size()%2==0,"invalid fragment admission hex");std::vector<std::byte> bytes;
  for(std::size_t i=0;i<value.size();i+=2) {
    std::size_t end{};const auto byte=std::stoul(value.substr(i,2),&end,16);
    require(end==2,"invalid fragment admission hex digit");bytes.push_back(std::byte(byte));
  }
  return bytes;
}
template<class Function>void rejects(Function function,unsigned &count) {
  bool rejected=false;try{function();}catch(const FormatError &){rejected=true;}
  require(rejected,"unsupported raw fragment input was admitted");++count;
}
}
int main(int argc,char **argv){try {
  require(argc==3,"fragment admission tests need original TSV and title-camera-matrix-audit directory");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
  std::ifstream fixture(argv[1]);std::string line;
  require(bool(std::getline(fixture,line)) && line=="H2_MENU_WORLD_FRAGMENT_V1","invalid fragment admission fixture header");
  unsigned rows{},rejections{},aniso_rows{},pixel_rows{},enabled_alpha_rows{},packet_words{};
  std::set<std::string> families;std::set<unsigned> events;
  while(std::getline(fixture,line)) {
    if(line.empty())continue;
    std::istringstream input_line(line);std::vector<std::string> fields;std::string field;
    while(std::getline(input_line,field,'\t'))fields.push_back(field);
    require(fields.size()==10,"invalid fragment admission fixture columns");
    const auto program_bytes=read(std::filesystem::path(argv[2])/fields[2],240);
    PixelProgram literal;std::memcpy(literal.words.data(),program_bytes.data(),240);
    const auto completed=read(std::filesystem::path(argv[2])/"world-fragment-constant-expansion"/
        ("event-"+fields[0]+"-program.bin"),240);
    PixelProgram program;std::memcpy(program.words.data(),completed.data(),240);
    // Only independently reconstructed actual constant slots may differ.
    auto structural=program;
    for(unsigned i=0;i<18;++i) {
      const auto slot=i<16?((i&1)?18+i/2:10+i/2):43+i-16;
      structural.words[slot]=literal.words[slot];
    }
    require(structural.words==literal.words,"completed diagnostic program changed source structure");
    const auto uniform=read(std::filesystem::path(argv[2])/fields[3],944);
    MenuWorldFragmentUniforms packet;
    std::memcpy(packet.clip_range.data(),uniform.data()+272,16);
    std::memcpy(packet.regions.data(),uniform.data()+288,128);
    std::memcpy(packet.depth_factor_offset.data(),uniform.data()+832,8);
    std::memcpy(packet.surface_scale.data(),uniform.data()+864,8);
    const auto expected=hexadecimal(fields[8]);
    require(expected.size()==160 && std::memcmp(&packet,expected.data(),160)==0,
        "compact fragment packet changed independently captured original words");packet_words+=40;
    const auto missing_constants=packet;
    bind_menu_world_fragment_completed_program(packet,program);
    require(std::memcmp(packet.combiner_constants.data(),uniform.data()+544,288)==0,
        "CPU completed constant expansion differs from actual original float words");packet_words+=72;
    const auto state_bytes=hexadecimal(fields[4]);require(state_bytes.size()==96,"fragment fixture needs four texture records");
    std::array<ShaderTextureState,4> states;
    for(unsigned i=0;i<4;++i)states[i]=decode_texture_state(std::span(state_bytes).subspan(i*24,24));
    ForwardRasterState raster;raster.alpha_test=number(fields[5]);raster.alpha_function=number(fields[6]);
    raster.alpha_reference=number(fields[7]);
    // Controlled image/capability facts test admission, not original sampling.
    // Actual texture views/samplers/UBO and output bytes have a separate GPU gate.
    MenuWorldFragmentSamplerLimits limits{16.f,16.f,true};
    std::array<Texture,4> images;
    MenuWorldFragmentInput input;
    input.program=&program;input.raster=&raster;input.texture_states=&states;input.sampler_limits=&limits;
    input.uniforms=&packet;input.depth=MenuWorldFragmentDepth::clipped_w_d24;
    input.window=MenuWorldFragmentWindow::inclusive;input.colors=MenuWorldFragmentColors::front;
    input.primitive=MenuWorldFragmentPrimitive::ordinary_triangles;input.demote_to_helper_enabled=true;
    bool anisotropic=false;const auto pixel_mask=number(fields[9]);
    for(unsigned stage=0;stage<4;++stage) {
      const auto mode=(program.words[54]>>(stage*5))&31u;
      if(mode==0)continue;
      require(mode==1 || mode==3,"fixture widened the original texture corridor");
      images[stage].width=images[stage].height=2;images[stage].layers=mode==3?6:1;
      images[stage].pixels.resize(4*4*images[stage].layers);
      input.textures[stage]={&images[stage],(pixel_mask>>stage)&1u?
          MenuWorldTextureCoordinates::pixel:MenuWorldTextureCoordinates::normalized,
          MenuWorldTextureSemantics::ordinary_rgba};
      anisotropic|=states[stage].min_filter==3 || states[stage].mag_filter==3;
    }
    validate_menu_world_fragment(input);families.insert(fields[1]);events.insert(number(fields[0]));++rows;
    {
      auto fog_program=program;fog_program.words[8]=(fog_program.words[8]&~0x00ff0000u)|0x00330000u;
      auto fog_input=input;fog_input.program=&fog_program;
      rejects([&]{validate_menu_world_fragment(fog_input);},rejections);
      fog_input.fog_alpha_bound=true;validate_menu_world_fragment(fog_input);
      fog_program.words[8]=(fog_program.words[8]&~0x00ff0000u)|0x00230000u;
      rejects([&]{validate_menu_world_fragment(fog_input);},rejections);
    }
    auto unbound=input;unbound.uniforms=&missing_constants;
    rejects([&]{validate_menu_world_fragment(unbound);},rejections);
    const auto valid_constants=packet.combiner_constants;
    for(unsigned i=0;i<18;++i)for(unsigned c=0;c<4;++c) {
      packet.combiner_constants[i][c]=std::bit_cast<float>(
          std::bit_cast<std::uint32_t>(valid_constants[i][c])^1u);
      rejects([&]{validate_menu_world_fragment(input);},rejections);
      packet.combiner_constants=valid_constants;
    }
    program.words[43]^=1u;
    rejects([&]{validate_menu_world_fragment(input);},rejections);program.words[43]^=1u;
    aniso_rows+=anisotropic;pixel_rows+=pixel_mask!=0;enabled_alpha_rows+=raster.alpha_test!=0;
    auto altered=input;
    altered.program=nullptr;rejects([&]{validate_menu_world_fragment(altered);},rejections);
    altered=input;altered.raster=nullptr;rejects([&]{validate_menu_world_fragment(altered);},rejections);
    altered=input;altered.texture_states=nullptr;rejects([&]{validate_menu_world_fragment(altered);},rejections);
    altered=input;altered.sampler_limits=nullptr;rejects([&]{validate_menu_world_fragment(altered);},rejections);
    altered=input;altered.uniforms=nullptr;rejects([&]{validate_menu_world_fragment(altered);},rejections);
    altered=input;altered.depth=MenuWorldFragmentDepth::unspecified;rejects([&]{validate_menu_world_fragment(altered);},rejections);
    altered=input;altered.window=MenuWorldFragmentWindow::exclusive;rejects([&]{validate_menu_world_fragment(altered);},rejections);
    altered=input;altered.colors=MenuWorldFragmentColors::back_or_two_sided;rejects([&]{validate_menu_world_fragment(altered);},rejections);
    altered=input;altered.primitive=MenuWorldFragmentPrimitive::other;rejects([&]{validate_menu_world_fragment(altered);},rejections);
    altered=input;altered.demote_to_helper_enabled=false;rejects([&]{validate_menu_world_fragment(altered);},rejections);
    const auto saved_packet=packet;packet.surface_scale[0]=0;
    rejects([&]{validate_menu_world_fragment(input);},rejections);packet=saved_packet;
    packet.depth_factor_offset[0]=std::numeric_limits<float>::infinity();
    rejects([&]{validate_menu_world_fragment(input);},rejections);packet=saved_packet;
    packet.clip_range[2]=std::numeric_limits<float>::quiet_NaN();
    rejects([&]{validate_menu_world_fragment(input);},rejections);packet=saved_packet;
    const auto saved_program=program;
    program.words[34]=(program.words[34]&0xffffff00u)|3u;
    rejects([&]{validate_menu_world_fragment(input);},rejections);program=saved_program;
    program.words[53]=(program.words[53]&0xffffff00u)|9u;
    rejects([&]{validate_menu_world_fragment(input);},rejections);program=saved_program;
    program.words[54]=(program.words[54]&~31u)|4u;
    rejects([&]{validate_menu_world_fragment(input);},rejections);program=saved_program;
    for(const auto source:{6u,7u,14u,15u}) {
      program.words[34]=(program.words[34]&0xffffff00u)|source;
      rejects([&]{validate_menu_world_fragment(input);},rejections);program=saved_program;
    }
    for(const auto target:{1u,2u,3u,6u,7u,14u,15u}) {
      program.words[45]=(program.words[45]&0xfffffff0u)|target;
      rejects([&]{validate_menu_world_fragment(input);},rejections);program=saved_program;
    }
    for(const auto flag:{0x1000u,0x2000u,0x40000u,0x80000u}) {
      program.words[26]|=flag;
      rejects([&]{validate_menu_world_fragment(input);},rejections);program=saved_program;
    }
    for(const auto mapping:{5u,7u}) {
      program.words[45]=(program.words[45]&~0x38000u)|(mapping<<15);
      rejects([&]{validate_menu_world_fragment(input);},rejections);program=saved_program;
    }
    program.words[45]|=0x100000u;
    rejects([&]{validate_menu_world_fragment(input);},rejections);program=saved_program;
    program.words[53]|=0x200u;
    rejects([&]{validate_menu_world_fragment(input);},rejections);program=saved_program;
    program.words[8]=program.words[9]=0;
    rejects([&]{validate_menu_world_fragment(input);},rejections);program=saved_program;
    program.words[9]|=1u;
    rejects([&]{validate_menu_world_fragment(input);},rejections);program=saved_program;
    for(const auto mapping:{2u,3u,4u,5u,6u,7u}) {
      program.words[8]=(program.words[8]&~0xe0u)|(mapping<<5);
      rejects([&]{validate_menu_world_fragment(input);},rejections);program=saved_program;
    }
    for(const auto shift:{16u,24u}) {
      program.words[9]=(program.words[9]&~(15u<<shift))|(15u<<shift);
      rejects([&]{validate_menu_world_fragment(input);},rejections);program=saved_program;
    }
    const auto saved_raster=raster;raster.alpha_function=0x208;
    rejects([&]{validate_menu_world_fragment(input);},rejections);raster=saved_raster;
    raster.alpha_reference=256;rejects([&]{validate_menu_world_fragment(input);},rejections);raster=saved_raster;
    altered=input;altered.textures[0].image=nullptr;rejects([&]{validate_menu_world_fragment(altered);},rejections);
    altered=input;altered.textures[0].semantics=MenuWorldTextureSemantics::unspecified;
    rejects([&]{validate_menu_world_fragment(altered);},rejections);
    altered=input;altered.textures[0].coordinates=MenuWorldTextureCoordinates::unspecified;
    rejects([&]{validate_menu_world_fragment(altered);},rejections);
    const auto saved_state=states[0];states[0].color_key_op=1;
    rejects([&]{validate_menu_world_fragment(input);},rejections);states[0]=saved_state;
    states[0].color_sign=0x10000000u;
    rejects([&]{validate_menu_world_fragment(input);},rejections);states[0]=saved_state;
    states[0].address_u=5;rejects([&]{validate_menu_world_fragment(input);},rejections);states[0]=saved_state;
    for(const auto color:{0u,0xff000000u,0xffffffffu}) {
      states[0].address_u=states[0].address_v=states[0].address_w=4;
      states[0].border_color=color;validate_menu_world_fragment(input);
    }
    states[0].border_color=0x12345678u;
    rejects([&]{validate_menu_world_fragment(input);},rejections);
    limits.custom_border_colors_enabled=true;validate_menu_world_fragment(input);
    limits.custom_border_colors_enabled=false;states[0]=saved_state;
    images[0].depth=2;altered=input;
    altered.textures[0].coordinates=MenuWorldTextureCoordinates::normalized;
    validate_menu_world_fragment(altered);images[0].depth=0;
    // PROJECT3D admits the actual normalized volume binding, including its
    // W projection. Pixel-coordinate remapping remains a distinct boundary.
    program.words[54]=(program.words[54]&~31u)|2u;images[0].depth=2;
    altered=input;altered.textures[0].coordinates=MenuWorldTextureCoordinates::normalized;
    validate_menu_world_fragment(altered);
    altered.textures[0].coordinates=MenuWorldTextureCoordinates::pixel;
    rejects([&]{validate_menu_world_fragment(altered);},rejections);
    images[0].depth=0;program=saved_program;
    images[0].layers=6;rejects([&]{validate_menu_world_fragment(input);},rejections);images[0].layers=1;
    altered=input;altered.w_clip_mode=MenuWorldWClipMode::clamp;
    validate_menu_world_fragment(altered);
    altered.w_clip_mode=static_cast<MenuWorldWClipMode>(1);
    rejects([&]{validate_menu_world_fragment(altered);},rejections);
    if(anisotropic) {
      limits.anisotropy_enabled=false;rejects([&]{validate_menu_world_fragment(input);},rejections);
      limits.anisotropy_enabled=true;limits.maximum_anisotropy=1;
      rejects([&]{validate_menu_world_fragment(input);},rejections);limits.maximum_anisotropy=16;
    }
    // Empty windows/inverted clip intervals are legitimate all-reject states.
    // The validator must preserve them, including non-default polygon offsets.
    packet.regions.fill({7,8,3,4});packet.clip_range[2]=3;packet.clip_range[3]=2;
    packet.depth_factor_offset={-0.0f,2.5f};const auto unmodified=packet;
    validate_menu_world_fragment(input);
    require(std::memcmp(&packet,&unmodified,sizeof(packet))==0,"fragment admission repaired caller packet");
    altered=input;altered.w_clip_mode=MenuWorldWClipMode::clamp;
    rejects([&]{validate_menu_world_fragment(altered);},rejections);
  }
  require(rows==8 && families.size()==5 && events==std::set<unsigned>{345,528,1335,1448,1727,1735,1880,2409} &&
      pixel_rows==2 && enabled_alpha_rows==2 && aniso_rows==4 && packet_words==896,
      "fragment admission did not cover all original families/packet states");
  std::cout<<rows<<" original banks / "<<families.size()<<" literal families / "<<packet_words
      <<" compact packet words / "<<rejections<<" rejected controls; controlled image/capability facts; no GPU/live admission\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
