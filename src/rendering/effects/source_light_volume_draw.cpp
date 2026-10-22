#include "halo2/source_light_volume_draw.hpp"
#include <cstring>
#include <algorithm>
namespace halo2 {
LightVolumeState source_light_volume_attachment_state(const Transform &marker,
    bool scale,float opacity,Vec3 color) {
  auto result=LightVolumeState{marker,opacity,color};
  if(!scale)result.marker.scale=1.f;
  return result;
}
void source_light_volume_command(SourceTransparentCommand &out,std::uint32_t object,
    std::uint32_t definition,const LightVolumeState &s,Vec3 eye,Vec3 forward) {
  out.priority=3;out.secondary=0;out.plane_present&=0xffffff00u;
  const auto delta=s.marker.position-eye;
  const float z=forward.z*delta.z,y=forward.y*delta.y,x=forward.x*delta.x;
  out.depth=0.f-((z+y)+x);out.callback=source_light_volume_callback;out.center=s.marker.position;
  const std::array<std::uint32_t,2> identities{object,definition};
  std::memcpy(out.payload.data(),identities.data(),8);
  const std::array<float,14> values{s.marker.position.x,s.marker.position.y,s.marker.position.z,
      s.marker.basis[0].x,s.marker.basis[0].y,s.marker.basis[0].z,
      s.marker.basis[2].x,s.marker.basis[2].y,s.marker.basis[2].z,
      s.marker.scale,s.opacity,s.color.x,s.color.y,s.color.z};
  std::memcpy(out.payload.data()+8,values.data(),56);
}
SourceLightVolumeGeometry source_light_volume_vertices(const LightVolumeDefinition &definition,
    const LightVolumeState &state,const SourceViewProjectionBank &bank,RenderViewport viewport,
    float distance,const ShaderFunctionTables &tables,std::span<const SourceLightVolumeMaterial> materials) {
  LightVolumeProjection projection;projection.world_to_view=bank.inverse_view;
  projection.viewport=viewport;projection.far_distance=distance;
  std::memcpy(projection.projection.data(),bank.projection.data(),64);
  const LightVolumeView view{bank.camera_to_world.position,bank.inverse_view,bank.viewport_half_metrics};
  SourceLightVolumeGeometry result;
  for(const auto &batch:light_volume_sprites(definition,state,view,tables)) {
    if(batch.fuzzy)throw FormatError("Light-volume draw requires its original fuzzy GPU owner");
    const auto binding=std::find_if(materials.begin(),materials.end(),[&](const auto &m){return m.bitmap==batch.bitmap;});
    if(binding==materials.end())throw FormatError("Light-volume draw lost its actual bitmap material");
    const auto first=std::uint32_t(result.vertices.size());
    for(const auto &sprite:batch.sprites)if(const auto quad=light_volume_quad(sprite,batch.cosine,batch.sine,projection))
      for(const auto index:{0u,1u,2u,0u,2u,3u})result.vertices.push_back((*quad)[index]);
    if(result.vertices.size()!=first)result.ranges.push_back({first,std::uint32_t(result.vertices.size())-first,binding->material});
  }
  return result;
}
}
