#include "halo2/appearance.hpp"
#include "halo2/object_lighting.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
int color_slot(unsigned index) {
  // Retail g_43E909 maps named externs 4/5/48/49 to color-bank indices
  // 3/4/21/22. Postprocess indices differ from the named enum values.
  switch (index) {
  case 3:
    return 0;
  case 4:
    return 1;
  case 21:
    return 2;
  case 22:
    return 3;
  default:
    return -1;
  }
}
void apply_render_colors(Material &material, const ObjectColors &colors,bool reset_program=true) {
  material.object_render_colors=colors;
  const auto child=[&](std::shared_ptr<Material>& pass) {
    if(!pass)return;
    if(pass.use_count()!=1)pass=std::make_shared<Material>(*pass);
    apply_render_colors(*pass,colors);
  };
  if(reset_program) {
    child(material.environment_pass);
    child(material.planar_fog_pass);
    for(auto& pass:material.world_overlays_before)child(pass);
    for(auto& pass:material.world_overlays_after)child(pass);
    for(auto& pass:material.world_bloom_passes)child(pass);
    for(auto& pass:material.world_planar_passes)child(pass);
  }
  if (!material.forward_pass ||
      material.forward_pass_index >= material.passes.size())
    return;
  const auto &pass = material.passes[material.forward_pass_index];
  // Retail 19BE0 destination 0x1C8A writes only v10.xyz from color-bank
  // source 3 (ObjectChangePrimary). The authored fourth component survives.
  // Animated remapping starts from this object-context binding as well.
  for(const auto& r:pass.vertex_externs)
    if(r.destination==0x1c8a && r.source==3) {
      auto& attribute=material.program_constants[0][10];
      attribute[0]=colors[0].x;attribute[1]=colors[0].y;attribute[2]=colors[0].z;
      auto& authored=material.passes[material.forward_pass_index].constants[0][10];
      std::copy_n(attribute.begin(),3,authored.begin());
      material.forward_diffuse=attribute;
    }
  if(reset_program)material.forward_program = pass.program;
  std::array<ShaderVector, 256> values{};
  std::vector<ShaderRemap> mappings;
  for (const auto &r : pass.pixel_externs) {
    if ((r.component & 7) != 4)
      continue;
    const auto slot = color_slot(r.source);
    if (slot < 0)
      continue;
    values[r.source] = {colors[slot].x,colors[slot].y,colors[slot].z,0};
    mappings.push_back(r);
  }
  remap_pixel_externs(material.forward_program,{},values,mappings,
                     pass.fragments,material.shader_switches);
}
} // namespace
void apply_object_render_color_bindings(PixelProgram &program,ShaderConstants &constants,
    const MaterialPass &pass,const ObjectColors &colors,std::span<const std::int32_t> switches) {
  for(const auto &r:pass.vertex_externs)if(r.destination==0x1c8a && r.source==3) {
    constants[0][10][0]=colors[0].x;constants[0][10][1]=colors[0].y;constants[0][10][2]=colors[0].z;
  }
  std::array<ShaderVector,256> values{};std::vector<ShaderRemap> mappings;
  for(const auto &r:pass.pixel_externs) {
    if((r.component&7)!=4)continue;
    const auto slot=color_slot(r.source);if(slot<0)continue;
    values[r.source]={colors[slot].x,colors[slot].y,colors[slot].z,0};mappings.push_back(r);
  }
  remap_pixel_externs(program,{},values,mappings,pass.fragments,switches);
}
ObjectColors source_object_render_colors(const ObjectColors &colors) {
  ObjectColors result;
  constexpr float byte_scale=std::bit_cast<float>(0x3b808081u); //Retail45DBB4.
  for(unsigned i=0;i<colors.size();++i) {
    const auto packed=pack_object_lighting_rgb(colors[i]);
    result[i]={float((packed>>16)&255u)*byte_scale,
               float((packed>>8)&255u)*byte_scale,
               float(packed&255u)*byte_scale};
  }
  return result;
}
void apply_object_colors(Material &material, const ObjectColors &colors) {
  apply_object_render_colors(material,source_object_render_colors(colors));
}
void apply_object_render_colors(Material &material, const ObjectColors &colors) {
  apply_render_colors(material,colors);
}
void reapply_object_colors(Material &material) {
  if(material.object_render_colors)
    apply_render_colors(material,*material.object_render_colors,false);
}
void prepare_surface_material(const Cache &cache, Material &material) {
  if (material.static_lightmap_pass)
    return;
  const auto selected = std::find_if(
      material.passes.begin(), material.passes.end(), [](const auto &p) {
        return p.implementation == 0 &&
               (p.vertex_shader.ends_with("\\stencil_texture") ||
                p.vertex_shader.ends_with("\\stencil_emissive")) &&
               (p.name.find("\\texture_mapping\\") != std::string::npos ||
                p.name.find("\\self_illumination\\") != std::string::npos);
      });
  if (selected == material.passes.end())
    return;
  const auto &pass = *selected;
  material.forward_pass = true;
  material.forward_pass_index = unsigned(selected - material.passes.begin());
  material.forward_vertex =
      pass.vertex_shader.ends_with("\\stencil_texture") ? 3 : 2;
  material.forward_program = pass.program;
  const Texture zero{1, 1, 11, std::vector<std::byte>(4), {}};
  material.vertex_program = pass.vertex_code;
  material.vertex_declaration = pass.vertex_declaration;
  material.program_constants = pass.constants;
  std::copy(pass.texture_states.begin(),pass.texture_states.end(),
            material.texture_states.begin());
  material.diffuse = material.detail = material.bump = material.auxiliary =
      zero;
  for (unsigned i = 0; i < 8; ++i)
    material.forward_transforms[i] = pass.constants[1][i];
  material.forward_diffuse = pass.constants[0][10];
  material.forward_specular = pass.constants[0][11];
  for (const auto &binding : pass.textures) {
    if (binding.stage > 3)
      throw FormatError("surface texture stage outside NV2A slots");
    if (binding.external || binding.parameter == 255) {
      if (((pass.program.words[54] >> (binding.stage * 5)) & 31) != 0)
        material.rendering_gaps.push_back(
            "surface pass has unresolved external texture: " + pass.name);
      continue;
    }
    const auto remap =
        std::find_if(pass.bitmap_remaps.begin(), pass.bitmap_remaps.end(),
                     [&](const auto &r) {
                       return (r.destination & 255) == binding.parameter;
                     });
    if (remap == pass.bitmap_remaps.end() ||
        remap->source >= material.bitmaps.size())
      throw FormatError("missing surface bitmap remap");
    const auto &image = material.bitmaps[remap->source];
    Texture *stages[]{&material.diffuse, &material.detail, &material.bump,
                      &material.auxiliary};
    *stages[binding.stage] = cache.load_bitmap(image.datum, image.index);
  }
  apply_object_colors(material, {{{1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}}});
  material.rendering_gaps.push_back(
      "surface program bound; lighting/environment layers and animated "
      "constants pending");
}
} // namespace halo2
