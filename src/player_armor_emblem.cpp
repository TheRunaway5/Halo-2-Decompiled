#include "halo2/player_armor_emblem.hpp"

namespace halo2 {
namespace {
void animate(Material &material,double seconds,const ShaderInput &input) {
  if(material.forward_pass) {
    animate_forward_material(material,seconds,input);
    animate_material_bitmaps(material,seconds,input);
  }
  const auto child=[&](std::shared_ptr<Material> &pass) {
    if(!pass)return;
    if(!pass.unique())pass=std::make_shared<Material>(*pass);
    animate(*pass,seconds,input);
  };
  child(material.environment_pass);
  for(auto &pass:material.world_overlays_before)child(pass);
  for(auto &pass:material.world_overlays_after)child(pass);
  for(auto &pass:material.world_bloom_passes)child(pass);
}
}
void animate_player_armor_emblem(Material &material, double seconds,
    const std::optional<SourcePlayerConfiguration> &profile, std::int16_t scenario_type,
    std::span<const std::array<std::uint32_t,3>> palette, const ShaderInput &fallback) {
  if(profile) {
    const auto colors=source_player_emblem_colors(*profile,palette);
    const auto packed=source_object_render_colors({{{1,1,1},{1,1,1},colors[0],colors[1]}});
    auto bank=material.object_render_colors.value_or(ObjectColors{{{1,1,1},{1,1,1},{1,1,1},{1,1,1}}});
    bank[2]=packed[2];bank[3]=packed[3];
    // Preserve the body's separately owned primary/secondary color producer.
    // This also detaches shared child passes before any per-player writes.
    apply_object_render_colors(material,bank);
  }
  const auto input=[&](std::uint32_t name) {
    if(profile)if(const auto value=source_object_emblem_input(
        source_player_object_emblem(*profile),scenario_type,name))return value->value;
    return fallback?fallback(name):0.f;
  };
  animate(material,seconds,input);
}
}
