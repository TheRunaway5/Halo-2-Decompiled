#pragma once
#include "halo2/light_volume.hpp"
#include "halo2/source_transparent_sort.hpp"
#include "halo2/source_view_projection.hpp"

namespace halo2 {
// Normalized callback identity used by the native transparent adapter. Original
// TU1.1/TU1.5 pointers are4F000; the retained retail token is4F010.
inline constexpr std::uint32_t source_light_volume_callback=0x4f010;
struct SourceLightVolumeMaterial {std::uint32_t bitmap{};unsigned material{};};
struct SourceLightVolumeRange {std::uint32_t first{},count{};unsigned material{};};
struct SourceLightVolumeGeometry {
  std::vector<LightVolumeVertex> vertices;
  std::vector<SourceLightVolumeRange> ranges;
};
struct SourceLightVolumeDraw {
  std::uint32_t object=0xffffffffu,definition=0xffffffffu;
  unsigned material{}; // Resident native slot for the original glow program.
  std::vector<SourceLightVolumeMaterial> bitmap_materials;
  SourceTransparentCommand command;
  std::shared_ptr<const LightVolumeDefinition> volume;
  std::shared_ptr<const ShaderFunctionTables> tables;
  LightVolumeState state;
  SourceViewProjectionBank camera;
  RenderViewport viewport;
  float actual_w_distance{};
  std::vector<LightVolumeVertex> vertices;
  std::vector<SourceLightVolumeRange> ranges;
};
//41980's completed marker, first-volume flag20, BE6D0 opacity and BAD50 color.
// The caller owns those inputs; no marker, function or color is inferred here.
LightVolumeState source_light_volume_attachment_state(const Transform &marker,
    bool use_marker_scale,float opacity,Vec3 color);
//42850's successful allocation stores. Untouched plane bytes survive. The
// caller owns the real shared1024-record allocation/overflow state.
void source_light_volume_command(SourceTransparentCommand &,std::uint32_t object,
    std::uint32_t definition,const LightVolumeState &,Vec3 eye,Vec3 forward);
// Use the completed2FD90 bank, including its actual projection and viewport
// metrics. This does not construct a title lens or choose a gameplay camera.
SourceLightVolumeGeometry source_light_volume_vertices(
    const LightVolumeDefinition &,const LightVolumeState &,
    const SourceViewProjectionBank &,RenderViewport,float actual_w_distance,
    const ShaderFunctionTables &,std::span<const SourceLightVolumeMaterial>);
}
