#include "halo2/frontend_material.hpp"
#include <algorithm>
#include <utility>
namespace halo2 {
bool prepare_forward_material(const Cache &cache, Material &m) {
  // A caller that already selected the original LOD/layer implementation
  // has a complete pass binding. Do not replace it with a different LOD's
  // first transparent or emissive record during the menu's final sweep.
  if(m.forward_pass)return true;
  const Texture zero{1, 1, 11, std::vector<std::byte>(4), {}};
    for (unsigned pass_index=0;pass_index<m.passes.size();++pass_index) {
      const auto &p=std::as_const(m).passes[pass_index];
      if (p.implementation != 0)
        continue;
      const bool transparent =
          p.name.starts_with("shaders\\shader_passes\\transparent\\") ||
          p.name == "shaders\\shader_passes\\plasma\\plasma_alpha";
      const bool emissive =
          p.name.ends_with("\\default_self_illumination_single_pass");
      if (!transparent && !emissive)
        continue;
      m.forward_pass = true;
      m.forward_pass_index = pass_index;
      m.forward_vertex = transparent ? 1 : 2;
      m.forward_program = p.program;
      m.vertex_program = p.vertex_code;
      m.vertex_declaration = p.vertex_declaration;
      m.program_constants = p.constants;
      std::copy(p.texture_states.begin(),p.texture_states.end(),m.texture_states.begin());
      m.forward_diffuse = p.constants[0][10];
      m.forward_specular = p.constants[0][11];
      m.diffuse = m.detail = m.bump = m.auxiliary = zero;
      for (unsigned i = 0; i < 8; ++i)
        m.forward_transforms[i] = p.constants[1][i];
      for (const auto &b : p.textures) {
        if (b.stage > 3)
          throw FormatError("frontend texture stage exceeds NV2A slots");
        if (b.parameter == 255 && b.external == 1) {
          // Retail 18A90 -> 19530 case 1 binds rasterizer globals + 0x0c:
          // the authored vector-normalization cube, not a white/zero image.
          const auto tag = cache.query_tag({.group=tag_groups::bitmap, .name="rasterizer\\vector_normalization"});
          if (!tag)
            throw FormatError("missing retail vector-normalization cube");
          auto texture = cache.load_bitmap(tag->datum, 0);
          if (texture.layers != 6)
            throw FormatError("retail normalization texture is not a cube");
          Texture *stages[]{&m.diffuse, &m.detail, &m.bump, &m.auxiliary};
          *stages[b.stage] = std::move(texture);
          continue;
        }
        if (b.external || b.parameter == 255) {
          if (((p.program.words[54] >> (5 * b.stage)) & 31) != 0)
            m.rendering_gaps.push_back(
                "forward pass requires an external texture: " + p.name);
          continue;
        }
        const auto remap = std::find_if(
            p.bitmap_remaps.begin(), p.bitmap_remaps.end(), [&](const auto &r) {
              return (r.destination & 255) == b.parameter;
            });
        if (remap == p.bitmap_remaps.end() || remap->source >= m.bitmaps.size())
          throw FormatError("missing frontend shader bitmap remap");
        const auto &image = m.bitmaps[remap->source];
        try {
          auto texture = cache.load_bitmap(image.datum, image.index);
          Texture *stages[]{&m.diffuse, &m.detail, &m.bump, &m.auxiliary};
          *stages[b.stage] = std::move(texture);
        } catch (const FormatError &e) {
          m.rendering_gaps.push_back("forward texture: " +
                                     std::string(e.what()));
        }
      }
      unsigned source = 1, destination = 0;
      m.forward_raster = {};
      for (const auto &state : p.states) {
        if (state.index == 62)
          source = state.value;
        if (state.index == 63)
          destination = state.value;
        switch (state.index) {
        case 57: m.forward_raster.depth_function = state.value; break;
        case 58: m.forward_raster.alpha_function = state.value; break;
        case 60: m.forward_raster.alpha_test = state.value; break;
        case 61: m.forward_raster.alpha_reference = state.value; break;
        case 64: m.forward_raster.depth_write = state.value; break;
        case 67: m.forward_raster.color_write = state.value; break;
        case 143: m.forward_raster.depth_enable = state.value; break;
        case 147: m.forward_raster.cull = state.value; break;
        default: break;
        }
      }
      m.forward_blend = source == 1 && destination == 1           ? 3
                        : source == 1 && destination == 0x303     ? 2
                        : source == 0x302 && destination == 0x303 ? 1
                                                                  : 0;
      m.rendering_gaps.push_back(
          "fog and complete pass scheduling "
          "need complete retail execution");
      return true;
    }
  return false;
}
void prepare_forward_materials(const Cache &cache, Mesh &mesh) {
  for (auto &m : mesh.materials) prepare_forward_material(cache,m);
}
} // namespace halo2
