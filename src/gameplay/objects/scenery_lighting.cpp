#include "halo2/object_initialization.hpp"
#include "halo2/appearance.hpp"
#include "halo2/bsp_lighting.hpp"
#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include "halo2/scenery.hpp"
#include "halo2/world_material.hpp"
#include <algorithm>
#include <set>

namespace halo2 {
namespace {
const BspLightingScenery* scenery_record(const BspLighting& lighting,
    const RenderModel& model,const ScenarioObject& object,int policy) {
  if(policy==2)return nullptr; // Original 10A820 skips baked lookup.
  const auto found=std::find_if(lighting.scenery.begin(),lighting.scenery.end(),
      [&](const auto& info) {
        return info.unique_id==object.unique_id && info.type==object.type &&
            info.source==object.source && info.checksum==model.checksum &&
            (info.source!=0 || info.origin_bsp==object.origin_bsp);
      });
  return found==lighting.scenery.end()?nullptr:&*found;
}
} // namespace
std::optional<SceneryDefinition> Cache::load_scenery_definition(std::uint32_t datum) const {
  const auto tag=query_tag({.handle=TagHandle{datum}, .require_address=true});
  if(!tag || tag->group!="scen")return std::nullopt;
  if(tag->size<0xc2)throw FormatError("truncated scenery definition");
  const Bytes data(cache_bytes());
  const auto root=metadata_offset(tag->address,tag->size);
  return SceneryDefinition{datum,data.u32(root+0x38),data.u16(root+2),
      static_cast<std::int16_t>(data.u16(root+0xc0))};
}

std::vector<SceneryLightprobe> Cache::load_scenery_lightprobes(std::size_t index) const {
  if(index>=bsps_.size())throw FormatError("BSP index outside scenario");
  const auto& bsp=bsps_[index];
  const Bytes data(Bytes(cache_bytes()).slice(bsp.file_offset,bsp.size));
  const auto count=data.u32(16+0x22c);
  if(count>0x7fff)throw FormatError("excessive source object lightprobe count");
  if(!count)return {};
  const auto pointer=TagAddressSpace(data,bsp.memory_address).offset(data.u32(16+0x230),std::size_t(count)*0x5c);
  const auto vector=[&](std::size_t at) {return Vec3{data.f32(at),data.f32(at+4),data.f32(at+8)};};
  std::vector<SceneryLightprobe> result;
  result.reserve(count);
  for(std::size_t i=0;i<count;++i) {
    const auto at=std::size_t(pointer)+i*0x5c;
    SceneryLightprobe value;
    value.unique_id=data.u32(at);value.origin_bsp=std::int16_t(data.u16(at+4));
    value.type=std::int8_t(std::to_integer<unsigned>(data.slice(at+6,1)[0]));
    value.source=std::int8_t(std::to_integer<unsigned>(data.slice(at+7,1)[0]));
    value.lighting={vector(at+8),vector(at+0x14),data.f32(at+0x20),data.f32(at+0x24),
        vector(at+0x28),vector(at+0x34),vector(at+0x40),vector(at+0x4c)};
    value.sh_index=std::int16_t(data.u16(at+0x58));value.unknown=std::int16_t(data.u16(at+0x5a));
    result.push_back(value);
  }
  return result;
}

const SceneryLightprobe* scenery_lightprobe(std::span<const SceneryLightprobe> probes,
    const ScenarioObject& object,const SceneryDefinition& definition) {
  if(!(definition.object_flags&0x100u))return nullptr;
  const auto found=std::find_if(probes.begin(),probes.end(),[&](const auto& value) {
    return value.unique_id==object.unique_id && value.type==object.type &&
        value.source==object.source && (value.source!=0 || value.origin_bsp==object.origin_bsp);
  });
  return found==probes.end()?nullptr:&*found;
}

bool scenery_multiplayer_game_allowed(const ScenarioObject& object,int engine) {
  return source_scenario_factory_game_allowed(object,2,engine);
}

bool scenery_automatic_creation_allowed(const ScenarioObject& object,int engine) {
  return (object.flags&1u)==0 && object.definition!=null_datum &&
      scenery_multiplayer_game_allowed(object,engine);
}

SceneryLightingImplementation scenery_lighting_implementation(const BspLighting& lighting,
    const RenderModel& model,const ScenarioObject& object,const SceneryDefinition& definition) {
  if(!lighting.structure_matches || lighting.bitmap_datum==null_datum)
    return SceneryLightingImplementation::unsupported;
  const auto* record=scenery_record(lighting,model,object,definition.lightmapping_policy);
  if(!record)return SceneryLightingImplementation::dynamic; // Instance0x7FF.
  const auto index=record->reference.bucket;
  if(index<0 || std::size_t(index)>=lighting.buckets.size())
    throw FormatError("scenery references missing baked lighting bucket");
  const auto& bucket=lighting.buckets[std::size_t(index)];
  // 2D000 requires residency. Native resources are loaded eagerly; retain
  // missing color/incident streams as unresolved instead of selecting dynamic.
  return !bsp_incident_count(bucket) || bucket.radiance.empty()?
      SceneryLightingImplementation::unsupported:SceneryLightingImplementation::per_vertex;
}

namespace {
void apply_vertex_lighting(RenderModel& model,const BspLighting& lighting,
                           const BspLightingReference& ref) {
  if(ref.bucket<0 || std::size_t(ref.bucket)>=lighting.buckets.size())
    throw FormatError("scenery references missing baked lighting bucket");
  if(model.vertex_origins.size()!=model.mesh.vertices.size())
    throw FormatError("scenery model lost source vertex ordinals");
  const auto& bucket=lighting.buckets[std::size_t(ref.bucket)];
  for(std::size_t i=0;i<model.mesh.vertices.size();++i) {
    const auto& origin=model.vertex_origins[i];
    if(origin.section>=ref.section_offsets.size())
      throw FormatError("scenery lighting reference missing render section");
    const auto offset=ref.section_offsets[origin.section];
    if(offset==0xffff)throw FormatError("selected scenery render section has no baked lighting");
    const auto source=std::size_t(offset)+origin.vertex;
    if(source>=bsp_incident_count(bucket) || source>=bucket.radiance.size())
      throw FormatError("scenery lightmap source vertex outside bucket");
    auto& vertex=model.mesh.vertices[i];
    // Original static scenery uses implementation 1. 4D300 binds NORMPACKED3
    // incident at section_offset*4; 4DC60 binds RGB8 at section_offset*3.
    // Incident stays in world space as the authored lightmap_pervertex VP
    // transforms the geometry basis, not v5. Posing preserves these streams.
    vertex.incident=bsp_incident(bucket,source);
    vertex.color=decode_bsp_radiance(bucket.radiance[source]);
    vertex.lightmap=0;
  }
}
}
void apply_scenery_vertex_lighting(RenderModel& model,const BspLighting& lighting,unsigned instance) {
  if(!lighting.structure_matches || lighting.bitmap_datum==null_datum ||
      instance==0x7ffu || instance>=lighting.scenery.size())
    throw FormatError("campaign scenery unit lost its admitted baked reference");
  const auto& record=lighting.scenery[instance];
  if(record.checksum!=model.checksum)
    throw FormatError("campaign scenery baked reference changed its model checksum");
  apply_vertex_lighting(model,lighting,record.reference);
}

bool Cache::apply_scenery_lighting(RenderModel& model,const BspLighting& lighting,
                                   const ScenarioObject& object) const {
  if(!lighting.structure_matches)return false;
  const auto definition=load_scenery_definition(object.definition);
  // Original 10A8B0 uses the complete object identifier plus render-model
  // checksum. Only structure-origin objects compare the origin BSP ordinal.
  const auto* record=scenery_record(lighting,model,object,
      definition?definition->lightmapping_policy:0);
  if(!record)return false;
  apply_vertex_lighting(model,lighting,record->reference);
  std::set<std::uint32_t> materials;
  for(const auto& vertex:model.mesh.vertices)materials.insert(vertex.material);
  const auto colors=load_object_colors(object.definition,object.variant);
  for(const auto slot:materials) {
    auto& material=model.mesh.materials.at(slot);
    if(!prepare_world_material(*this,material,true))
      model.mesh.material_warnings.push_back(material.name+": unsupported static scenery lighting pass");
    apply_object_colors(material,colors);
  }
  return true;
}
} // namespace halo2
