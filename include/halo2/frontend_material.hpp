#pragma once
#include "halo2/appearance.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <unordered_set>
namespace halo2 {
// Explicit frontend storage policy, after pass/bitmap selection. Only the
// source-proved BC1/BC2/BC3 2D/cube formats change. Other formats remain unchanged;
// the owning material records unhandled volume boundaries separately.
inline bool prepare_frontend_texture_storage(Texture &texture) {
  if(texture.depth || texture.format<14 || texture.format>16)return false;
  texture=prepare_xemu_bc_texture(texture);return true;
}
namespace frontend_material_detail {
inline void storage(Material &material,std::unordered_set<const Material*> &active) {
  const auto texture=[&](Texture &source) {
    if(source.depth) {
      const std::string gap="Frontend volume texture storage remains unverified";
      if(std::find(material.rendering_gaps.begin(),material.rendering_gaps.end(),gap)==material.rendering_gaps.end())
        material.rendering_gaps.push_back(gap);
    }
    prepare_frontend_texture_storage(source);
  };
  for(auto *source:{&material.diffuse,&material.detail,&material.bump,&material.auxiliary})texture(*source);
  for(auto &stage:material.animated_textures)for(auto &frame:stage)texture(frame);
  const auto child=[&](std::shared_ptr<Material> &pass) {
    if(!pass)return;
    const auto *original=pass.get();
    if(!active.insert(original).second)throw FormatError("cyclic frontend material storage tree");
    // A selected child can also be referenced by another owner/material bank.
    // Its frontend texture conversion must not mutate that other owner's data.
    if(pass.use_count()!=1)pass=std::make_shared<Material>(*pass);
    storage(*pass,active);active.erase(original);
  };
  child(material.environment_pass);child(material.planar_fog_pass);
  for(auto &pass:material.world_overlays_before)child(pass);
  for(auto &pass:material.world_overlays_after)child(pass);
  for(auto &pass:material.world_bloom_passes)child(pass);
  for(auto &pass:material.world_planar_passes)child(pass);
}
} // namespace frontend_material_detail
inline void prepare_frontend_material_storage(Material &material) {
  std::unordered_set<const Material*> active{&material};
  frontend_material_detail::storage(material,active);
}
// Bind the original menu transparency and emissive pass data separately from
// the interactive UI, then prepare the selected textures before GPU filtering.
// Generic Cache/gameplay texture uploads retain their existing behavior.
inline void prepare_frontend_materials(const Cache &cache, Mesh &mesh) {
  prepare_forward_materials(cache,mesh);
  for(auto &material:mesh.materials)prepare_frontend_material_storage(material);
}
}
