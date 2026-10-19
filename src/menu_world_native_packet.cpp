#include "halo2/menu_world_native_packet.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
MenuWorldNativeDrawInput menu_world_native_packet(const MenuWorldNativePacketInput &input) {
  if(!input.material_set)
    throw FormatError("raw world packet has no actual material descriptor");
  if(input.forward_blend!=0 && input.forward_blend!=1 && input.forward_blend!=3 && input.forward_blend!=5 && input.forward_blend!=8 && input.forward_blend!=9 && input.forward_blend!=10)
    throw FormatError("raw world packet has an unsupported source blend mapping");
  if(input.source.draw().topology!=MenuWorldSourceTopology::triangle_list &&
     input.source.draw().topology!=MenuWorldSourceTopology::triangle_strip)
    throw FormatError("raw world packet requires an original triangle topology");
  const auto &vertex_shader=menu_world_vertex_catalog_lookup(input.catalog,
      input.source.program_bytes(),input.source.declaration());
  if(!input.source.external_attribute_registers().empty()) {
    if(vertex_shader.abi!=menu_world_vertex_attribute_catalog_abi)
      throw FormatError("raw world packet constant attributes require ABI3");
    const auto reads=menu_world_vertex_input_components(input.source.program(),input.source.declaration().class_index>=2);
    for(const auto reg:input.source.external_attribute_registers())
      if((input.vertex_attribute_components[reg]&reads[reg])!=reads[reg])
        throw FormatError("raw world packet consumes unbound external vertex attribute components");
  }
  if(!std::ranges::equal(vertex_shader.consumed_inputs,input.source.consumed_registers()) ||
      !std::ranges::equal(vertex_shader.consumed_constants,input.source.consumed_constants()))
    throw FormatError("raw world packet catalog read set differs from its source binding");

  for(const auto value:input.surface_size)if(!std::isfinite(value) || value<=0)
    throw FormatError("raw world packet surface extent is not positive and finite");
  for(const auto value:input.fog_parameters)if(!std::isfinite(value))
    throw FormatError("raw world packet hardware fog state is nonfinite");

  const auto &material=input.material;
  if(material.alpha_test[0]!=input.forward.alpha_test ||
      material.alpha_test[1]!=input.forward.alpha_function ||
      material.alpha_test[2]!=input.forward.alpha_reference)
    throw FormatError("raw world packet material alpha fields differ from the selected raster state");
  const PixelProgram program{material.program,input.fragment_offsets};
  MenuWorldVertexUniforms vertex;
  vertex.constants=input.source.constants();
  vertex.clip_range=input.clip_range;
  vertex.fog_parameters=input.fog_parameters;
  vertex.surface_size=input.surface_size;
  MenuWorldFragmentUniforms fragment;
  fragment.clip_range=input.clip_range;
  fragment.regions=input.raster.regions;
  fragment.depth_factor_offset=input.raster.depth_factor_offset;
  fragment.surface_scale=input.raster.surface_scale;
  bind_menu_world_fragment_completed_program(fragment,program);

  MenuWorldFragmentInput validation;
  validation.program=&program;
  validation.raster=&input.forward;
  validation.texture_states=&input.texture_states;
  validation.sampler_limits=&input.sampler_limits;
  validation.uniforms=&fragment;
  validation.depth=MenuWorldFragmentDepth::clipped_w_d24;
  validation.w_clip_mode=input.w_clip_mode;
  validation.window=input.raster.window_exclusive?
      MenuWorldFragmentWindow::exclusive:MenuWorldFragmentWindow::inclusive;
  validation.colors=MenuWorldFragmentColors::front;
  validation.primitive=MenuWorldFragmentPrimitive::ordinary_triangles;
  validation.demote_to_helper_enabled=input.demote_to_helper_enabled;
  validation.fog_alpha_bound=input.fog_alpha_bound;
  for(unsigned stage=0;stage<4;++stage) {
    const auto mode=(program.words[54]>>(5*stage))&31u;
    // NONE/CLIPPLANE do not sample an image; their unused descriptor slots
    // and effects are not evidence for any hardware image or sampler state.
    if(mode!=1 && mode!=2 && mode!=3)continue;
    const auto &effects=material.texture_effects[stage];
    const auto volume=input.images[stage] && input.images[stage]->depth;
    const auto coordinates=input.texture_coordinates[stage];
    const auto pixel=coordinates==MenuWorldTextureCoordinates::pixel;
    if(effects[0]!=input.texture_states[stage].alpha_kill || effects[1]!=unsigned(pixel) || effects[2]!=unsigned(volume))
      throw FormatError("raw world packet texture effects differ from explicit sampling coordinates");
    validation.textures[stage]={input.images[stage],coordinates,
        MenuWorldTextureSemantics::ordinary_rgba};
  }
  validate_menu_world_fragment(validation);
  MenuWorldNativeDrawInput result{input.source,&vertex_shader,vertex,fragment,input.raster,input.forward,
      input.forward_blend,input.material_set,input.material_offset,input.w_clip_mode};
  result.vertex_attributes=input.material.vertex_attributes;
  result.vertex_attribute_components=input.vertex_attribute_components;
  result.fog_alpha_bound=input.fog_alpha_bound;
  return result;
}
} // namespace halo2
