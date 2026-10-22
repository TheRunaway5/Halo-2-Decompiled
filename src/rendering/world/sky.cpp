#include "halo2/sky.hpp"
#include "halo2/math.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/appearance.hpp"
#include <iterator>
#include <set>
namespace halo2 {
std::optional<SkyDefinition> Cache::load_visible_sky(std::size_t index) const {
  if (index >= bsps_.size())
    throw FormatError("sky BSP index outside scenario");
  const auto &bsp = bsps_[index];
  const Bytes metadata(Bytes(cache_bytes()).slice(bsp.file_offset, bsp.size));
  
  const auto [cluster_count, clusters] =
      TagAddressSpace(metadata, bsp.memory_address).block(16 + 156, 176);
  std::set<int> visible;
  for (std::size_t i = 0; i < cluster_count; ++i) {
    // 14B360 reads the visible sky at +0x6e, not the lighting sky at +0x6c.
    const auto sky = std::int8_t(std::to_integer<unsigned>(
        metadata.slice(clusters + i * 176 + 0x6e, 1)[0]));
    if (sky >= 0)
      visible.insert(sky);
  }
  const auto scenario =
      query_tag({.group=tag_groups::scenario, .name=scenario_});
  if (!scenario)
    return {};
  const Bytes data(cache_bytes());
  const auto root = metadata_offset(scenario->address, scenario->size);
  const auto [sky_count, skies] = TagAddressSpace(data, metadata_magic_).block(root + 8, 8);
  if (visible.empty() && (data.u16(root + 0x12) & 2) && sky_count)
    visible.insert(0);
  if (visible.empty())
    return {};
  if (visible.size() != 1)
    throw FormatError(
        "multiple visible skies require retail per-cluster selection");
  const auto selected = std::size_t(*visible.begin());
  if (selected >= sky_count)
    throw FormatError("visible sky outside scenario references");
  return load_sky(selected);
}
std::optional<SkyDefinition> Cache::load_sky(std::size_t selected) const {
  const auto scenario =
      query_tag({.group=tag_groups::scenario, .name=scenario_});
  if (!scenario)return {};
  const Bytes data(cache_bytes());
  const auto root = metadata_offset(scenario->address, scenario->size);
  const auto sky_count=data.u32(root+8);
  if(selected>=sky_count)throw FormatError("sky outside scenario references");
  const auto skies=TagAddressSpace(data,metadata_magic_).offset(data.u32(root+12),std::size_t(sky_count)*8);
  
  const auto datum = data.u32(skies + selected * 8 + 4);
  if (datum == 0xffffffffu)
    return {};
  const auto tag = query_tag({.handle=TagHandle{datum}, .group=tag_groups::sky});
  if (!tag || tag->size < 0xac)
    throw FormatError("missing/truncated retail sky");
  const auto base = metadata_offset(tag->address, tag->size);
  SkyDefinition result{tag->name,
                       data.u32(base + 4),
                       data.f32(base + 0x14),
                       data.f32(base + 0x18),
                       data.f32(base + 0x80),
                       {},{}};
  if (result.model == 0xffffffffu)
    return {};
  if (data.u32(base + 12) != 0xffffffffu)
    throw FormatError("animated sky graph requires retail sky sampling");
  for (float v : {result.scale, result.movement, result.yaw})
    if (!std::isfinite(v))
      throw FormatError("nonfinite sky transform");
  if (data.u32(base + 0x10) & 32)
    result.clear = Vec3{data.f32(base + 0xa0), data.f32(base + 0xa4),
                        data.f32(base + 0xa8)};
  const auto [fog_count,fog] = TagAddressSpace(data, metadata_magic_).block(base+0x48, 24);
  if(fog_count) {
    AtmosphericFog atmosphere{{data.f32(fog),data.f32(fog+4),data.f32(fog+8)},
        data.f32(fog+12),data.f32(fog+16),data.f32(fog+20)};
    for(float value:{atmosphere.color.x,atmosphere.color.y,atmosphere.color.z,
                    atmosphere.maximum_density,atmosphere.start,atmosphere.opaque})
      if(!std::isfinite(value))throw FormatError("nonfinite sky atmospheric fog");
    // 001301C0 clamps density and enforces the retail minimum range.
    atmosphere.maximum_density=atmosphere.maximum_density<=.0001f?0.f:
        atmosphere.maximum_density>.9999f?1.f:atmosphere.maximum_density;
    atmosphere.opaque=std::max(atmosphere.opaque,atmosphere.start+.0001f);
    result.atmospheric_fog=atmosphere;
  }
  return result;
}
std::vector<Vertex> pose_sky(const RenderModel &model, const SkyDefinition &sky,
                             Vec3 eye) {
  auto vertices = pose_model(model);
  const auto forward = direction(sky.yaw),
             left = Vec3{-std::sin(sky.yaw), std::cos(sky.yaw), 0};
  const auto rotate = [&](Vec3 v) {
    return forward * v.x + left * v.y + Vec3{0, 0, v.z};
  };
  for (auto &v : vertices) {
    v.position = rotate(v.position) * sky.scale + eye * sky.movement;
    v.normal = rotate(v.normal);
    v.tangent = rotate(v.tangent);
    v.binormal = rotate(v.binormal);
  }
  return vertices;
}
BspSkyModels::BspSkyModels(const Cache &cache,const BspAtmosphere &atmosphere,
                           Mesh &destination)
    :models_(atmosphere.skies.size()),referenced_(atmosphere.skies.size()) {
  for(const auto &cluster:atmosphere.clusters)if(cluster.visible_sky>=0) {
    const auto index=unsigned(cluster.visible_sky);
    if(index>=referenced_.size())throw FormatError("visible sky outside scenario references");
    referenced_[index]=true;
  }
  // Preserve TU1.5 14DE90's existing scenario flag fallback, including its ordinal.
  if((atmosphere.scenario_flags&2u) && !referenced_.empty())referenced_[0]=true;
  std::vector<Material> prepared;
  for(unsigned index=0;index<referenced_.size();++index)if(referenced_[index]) {
    auto definition=cache.load_sky(index);
    if(!definition)continue; // Authored NONE/model-less sky, with no replacement.
    auto model=cache.load_model(definition->model);
    prepare_forward_materials(cache,model.mesh);
    const auto base=std::uint64_t(destination.materials.size())+prepared.size();
    if(base+model.mesh.materials.size()>0xffffffffull)
      throw FormatError("BSP sky material namespace exhausted");
    for(auto &material:model.mesh.materials)prepared.push_back(std::move(material));
    model.mesh.materials.clear();
    models_[index]=Model{std::move(*definition),std::move(model),std::uint32_t(base)};
  }
  // Admission is transactional, and the destination alone owns texture pixels.
  destination.materials.insert(destination.materials.end(),
      std::make_move_iterator(prepared.begin()),std::make_move_iterator(prepared.end()));
}
std::vector<Vertex> BspSkyModels::pose(const BspFogSample &view,Vec3 eye)const {
  if(view.visible_sky<0)return {};
  const auto index=unsigned(view.visible_sky);
  if(index>=models_.size() || !referenced_[index])
    throw FormatError("current view selected an unprepared BSP sky ordinal");
  if(!models_[index])return {};
  const auto &selected=*models_[index];
  auto vertices=pose_sky(selected.geometry,selected.definition,eye);
  for(auto &vertex:vertices)vertex.material+=selected.material_base;
  return vertices;
}
} // namespace halo2
