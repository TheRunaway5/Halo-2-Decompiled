#include "halo2/menu_world_fragment.hpp"
#include "halo2/source_texture_filtering.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
std::array<ShaderVector,18> completed_constants(const PixelProgram &program) {
  std::array<ShaderVector,18> result;
  for(unsigned i=0;i<18;++i) {
    const unsigned slot=i<16?((i&1)?18+i/2:10+i/2):43+i-16;
    const auto color=program.words[slot];
    // Original pgraph.h byte/255.0f boundary, not rounded-reciprocal GPU
    // arithmetic. Compile this source with strict floating-point semantics.
    for(unsigned c=0;c<4;++c) {
      constexpr unsigned shifts[]{16,8,0,24};
      result[i][c]=float((color>>shifts[c])&255u)/255.0f;
    }
  }
  return result;
}
void operand(unsigned value,bool final,bool product_input=false) {
  const auto source=value&15u;
  if(source==3)throw FormatError("raw menu fragment consumes an unbound fog register");
  if(source==6 || source==7)throw FormatError("raw menu fragment consumes an unsupported combiner register");
  if(!final && source>=14)
    throw FormatError("raw menu fragment consumes a final-only register in a general stage");
  if(final && ((value>>5)&7u)>1)
    throw FormatError("raw menu fragment final input mapping is unsupported");
  if(product_input && source==15)
    throw FormatError("raw menu fragment final product recursively reads itself");
}
void inputs(std::uint32_t word,bool final=false) {
  for(unsigned shift=0;shift<32;shift+=8)operand(word>>shift,final);
}
void output(std::uint32_t word,bool alpha) {
  if(word&~0xfffffu)
    throw FormatError("raw menu fragment has reserved combiner output flags");
  if(alpha && (word&0xc3000u))
    throw FormatError("raw menu fragment alpha output uses RGB-only flags");
  const auto mapping=(word>>15)&7u;
  if(mapping==5 || mapping==7)
    throw FormatError("raw menu fragment has an unsupported combiner output mapping");
  for(unsigned shift:{0u,4u,8u}) {
    const auto target=(word>>shift)&15u;
    if(target!=0 && target!=4 && target!=5 && (target<8 || target>13))
      throw FormatError("raw menu fragment writes an unsupported combiner register");
  }
}
void sampler(const ShaderTextureState &state,const Texture &image,
    const MenuWorldFragmentSamplerLimits &limits) {
  // Literal address4 is CLAMP_TO_BORDER in the retained NV2A Vulkan mapping
  // and native_sampler(). Mirror-clamp remains a separate source boundary.
  bool border=false;
  for(const auto address:{state.address_u,state.address_v,state.address_w})
    if(address<1 || address>4)throw FormatError("raw menu fragment has an unsupported address/border state");
    else border|=address==4;
  if(border && state.border_color!=0 && state.border_color!=0xff000000u &&
     state.border_color!=0xffffffffu && !limits.custom_border_colors_enabled)
    throw FormatError("raw menu fragment requires enabled custom sampler border colors");
  if(state.min_filter<1 || state.min_filter>3 || state.mag_filter<1 || state.mag_filter>3 || state.mip_filter>2)
    throw FormatError("raw menu fragment has an unsupported texture filter");
  const auto bias=std::bit_cast<float>(state.lod_bias);
  if(!std::isfinite(bias) || std::abs(bias)>limits.maximum_lod_bias)
    throw FormatError("raw menu fragment sampler bias exceeds actual limits");
  const auto filtering=source_texture_filtering(state);
  if(filtering.anisotropy>1 &&
      (!limits.anisotropy_enabled || float(filtering.anisotropy)>limits.maximum_anisotropy))
    throw FormatError("raw menu fragment requires enabled, sufficient sampler anisotropy");
  if(state.color_key_op)throw FormatError("raw menu fragment color-key state is unsupported");
  if(state.color_sign)throw FormatError("raw menu fragment signed image state is not admitted");
  if(state.alpha_kill!=0 && state.alpha_kill!=4)
    throw FormatError("raw menu fragment has an unsupported alpha-kill nibble");
  if(!image.width || !image.height || (image.layers!=1 && image.layers!=6) || (image.depth && image.layers!=1))
    throw FormatError("raw menu fragment requires an actual ordinary 2D/cube/volume image");
}
}
void bind_menu_world_fragment_completed_program(MenuWorldFragmentUniforms &packet,
    const PixelProgram &completed_selected_program) {
  packet.combiner_constants=completed_constants(completed_selected_program);
}
void validate_menu_world_fragment(const MenuWorldFragmentInput &input) {
  if(!input.program || !input.raster || !input.texture_states || !input.sampler_limits || !input.uniforms)
    throw FormatError("raw menu fragment is missing its explicit program/state/packet");
  if(!std::isfinite(input.sampler_limits->maximum_lod_bias) || input.sampler_limits->maximum_lod_bias<0 ||
      !std::isfinite(input.sampler_limits->maximum_anisotropy) || input.sampler_limits->maximum_anisotropy<1)
    throw FormatError("raw menu fragment sampler capabilities are invalid");
  if(input.depth!=MenuWorldFragmentDepth::clipped_w_d24)
    throw FormatError("raw menu fragment requires the explicit clipped W/D24 policy");
  if(input.window!=MenuWorldFragmentWindow::inclusive)
    throw FormatError("raw menu fragment requires an explicit inclusive window policy");
  if(input.colors!=MenuWorldFragmentColors::front)
    throw FormatError("raw menu fragment back/two-sided color policy is unsupported");
  if(input.primitive!=MenuWorldFragmentPrimitive::ordinary_triangles)
    throw FormatError("raw menu fragment requires ordinary source triangles");
  if(!input.demote_to_helper_enabled)
    throw FormatError("raw menu fragment requires enabled demote-to-helper support");
  const auto &packet=*input.uniforms;
  for(const auto value:packet.clip_range)if(!std::isfinite(value))
    throw FormatError("raw menu fragment clip packet is nonfinite");
  for(const auto value:packet.depth_factor_offset)if(!std::isfinite(value))
    throw FormatError("raw menu fragment polygon offset packet is nonfinite");
  if(packet.surface_scale[0]<=0 || packet.surface_scale[1]<=0)
    throw FormatError("raw menu fragment surface scale must be positive");
  const auto constants=completed_constants(*input.program);
  for(unsigned i=0;i<18;++i)for(unsigned c=0;c<4;++c)
    if(std::bit_cast<std::uint32_t>(packet.combiner_constants[i][c])!=
        std::bit_cast<std::uint32_t>(constants[i][c]))
      throw FormatError("raw menu fragment float constants are missing or stale for the completed program");
  // Inverted/empty rectangles and clip intervals are valid rejection inputs;
  // do not repair them or manufacture a full-target rectangle.
  const auto &raster=*input.raster;
  if(raster.alpha_test>1 || raster.alpha_reference>255 ||
      raster.alpha_function<0x200 || raster.alpha_function>0x207)
    throw FormatError("raw menu fragment alpha state is unsupported");
  const auto &words=input.program->words;
  const auto count=words[53]&255u;
  if(count>8)throw FormatError("raw menu fragment exceeds eight combiner stages");
  if(words[53]&~0x111ffu)
    throw FormatError("raw menu fragment has reserved combiner count flags");
  for(unsigned i=0;i<count;++i) {
    inputs(words[i]);inputs(words[34+i]);output(words[26+i],true);output(words[45+i],false);
  }
  if(!words[8] && !words[9])
    throw FormatError("raw menu fragment requires an explicit final combiner");
  if(words[9]&0x1fu)
    throw FormatError("raw menu fragment has reserved final combiner flags");
  inputs(words[8],true);
  // E/F are product inputs; G selects final alpha. The low byte is flags.
  operand(words[9]>>24,true,true);operand(words[9]>>16,true,true);
  operand(words[9]>>8,true);
  for(unsigned stage=0;stage<4;++stage) {
    const auto mode=(words[54]>>(5*stage))&31u;
    if(mode!=0 && mode!=1 && mode!=2 && mode!=3 && mode!=5)
      throw FormatError("raw menu fragment texture mode is outside the bounded bridge");
    if(mode==0 || mode==5)continue;
    const auto &binding=input.textures[stage];
    if(!binding.image || binding.semantics!=MenuWorldTextureSemantics::ordinary_rgba ||
        binding.coordinates==MenuWorldTextureCoordinates::unspecified)
      throw FormatError("raw menu fragment is missing proved texture binding/coordinate facts");
    sampler((*input.texture_states)[stage],*binding.image,*input.sampler_limits);
    if((mode==3 && (binding.image->layers!=6 || binding.image->depth)) ||
        ((mode==1 || mode==2) && binding.image->layers!=1))
      throw FormatError("raw menu fragment texture dimensionality disagrees with its mode");
    if(binding.coordinates==MenuWorldTextureCoordinates::pixel && (stage!=0 || mode!=1 || binding.image->depth))
      throw FormatError("raw menu fragment admits pixel coordinates only for projected stage0");
    if(binding.coordinates!=MenuWorldTextureCoordinates::normalized &&
        binding.coordinates!=MenuWorldTextureCoordinates::pixel)
      throw FormatError("raw menu fragment coordinate policy is unsupported");
  }
}
} // namespace halo2
