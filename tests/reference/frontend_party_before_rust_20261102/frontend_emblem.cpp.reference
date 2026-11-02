#include "halo2/frontend_emblem.hpp"
#include "halo2/shader_animation.hpp"
#include "halo2/source_object_emblem.hpp"
#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

namespace halo2 {
Material frontend_emblem_material(const Cache &cache, std::uint32_t shader) {
  auto m = cache.load_material(shader, true);
  if (m.passes.size() != 1 || m.passes[0].implementation != 0 ||
      m.passes[0].name != "shaders\\shader_passes\\hud\\waypoint_emblem")
    throw FormatError("unsupported original UI emblem pass");
  const auto &pass = m.passes[0];
  unsigned source = 1, destination = 0;
  bool blend = false;
  for (const auto &state : pass.states) {
    if (state.index == 59) blend = state.value != 0;
    if (state.index == 62) source = state.value;
    if (state.index == 63) destination = state.value;
  }
  if (!blend || source != 0x302 || destination != 0x303)
    throw FormatError("unsupported original UI emblem blend");
  m.forward_pass = m.hud_pass = m.ui_emblem_pass = true;
  m.static_lightmap_pass = false;
  m.forward_pass_index = 0;
  m.forward_vertex = 11; // Bounded screen_hud quad branch in hud.frag.
  m.forward_blend = 1;
  m.forward_program = pass.program;
  m.program_constants = pass.constants;
  m.vertex_program.clear();
  if (pass.textures.size() != 2 || pass.texture_states.size() != 2)
    throw FormatError("UI emblem must retain its two authored texture stages");
  Texture *stages[]{&m.diffuse, &m.detail};
  for (const auto &binding : pass.textures) {
    if (binding.external || binding.stage > 1 || binding.parameter > 1)
      throw FormatError("unsupported original UI emblem texture binding");
    const auto remap = std::find_if(pass.bitmap_remaps.begin(),
        pass.bitmap_remaps.end(), [&](const auto &r) {
          return (r.destination & 255) == binding.parameter;
        });
    if (remap == pass.bitmap_remaps.end() || remap->source >= m.bitmaps.size())
      throw FormatError("UI emblem bitmap remap is outside its source bank");
    const auto &bitmap = m.bitmaps[remap->source];
    *stages[binding.stage] = cache.load_bitmap(bitmap.datum, bitmap.index);
  }
  std::copy(pass.texture_states.begin(), pass.texture_states.end(),
             m.texture_states.begin());
  prepare_bitmap_animation(cache, m);
  if(cache.is_mcc()) {
    // The admitted PC tag bank has its own string IDs. Bind those exact
    // authored names to22ACE7's native inputs while retaining every MCC
    // texture page, shader program, constant and UV definition.
    constexpr std::array<std::pair<std::string_view,std::uint32_t>,7> inputs{{
      {"emblem_foreground_index",0x170005fa},{"emblem_background_index",0x170005fb},
      {"emblem_toggle_foreground",0x180005fc},{"emblem_foreground_map_scale_x",0x1d0005fd},
      {"emblem_background_map_scale_x",0x1d0005fe},{"emblem_active",0x0d0005ff},
      {"hud_wp_alpha",0x0c0005cb},
    }};
    const auto native_input=[&](std::uint32_t id) {
      if(!id || id==0xffffffffu)return id;
      const auto name=cache.string_id(id);
      const auto found=std::find_if(inputs.begin(),inputs.end(),[&](const auto& input){return input.first==name;});
      return found==inputs.end()?id:found->second;
    };
    for(auto &overlay:m.animation.overlays) {
      overlay.input=native_input(overlay.input);overlay.range=native_input(overlay.range);
    }
  }
  return m;
}
float frontend_emblem_input(const FrontendAppearance &a, std::uint32_t name,
                            float opacity) {
  switch (name) {
  case 0x0d0005ff: return 1;
  case 0x170005fa: return float(a.foreground) * .015625f;
  case 0x170005fb: return float(a.background) * .03125f;
  case 0x180005fc: return (a.flags & 1) ? 0.f : 1.f;
  case 0x1d0005fd: return (a.flags & 2) ? 1.f : -1.f;
  case 0x1d0005fe: return (a.flags & 4) ? 1.f : -1.f;
  case 0x0c0005cb: return opacity;
  default: return 1; // 22ACE7's default, including unused input zero.
  }
}
std::optional<float> frontend_model_emblem_input(const FrontendAppearance &a,
                                               std::uint32_t name) {
  if(const auto value=source_object_emblem_input(
      {a.foreground,a.background,a.flags,a.reserved},2,name))return value->value;
  return {};
}
void update_frontend_emblem(Material &m, const FrontendAppearance &a,
                            std::span<const Vec3> palette, float opacity) {
  if (!m.ui_emblem_pass)
    throw FormatError("frontend emblem update needs its authored pass");
  apply_object_colors(m, frontend_appearance_colors(a, palette));
  const auto input = [&](std::uint32_t name) {
    return frontend_emblem_input(a, name, opacity);
  };
  animate_forward_material(m, 0, input);
  animate_material_bitmaps(m, 0, input);
}
} // namespace halo2
