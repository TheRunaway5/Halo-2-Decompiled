#pragma once
#include "halo2/transform.hpp"
#include "halo2/render_view.hpp"

namespace halo2 {
struct LightVolumeAspect {
  std::array<std::vector<std::byte>, 2> curves;
  float parallel_scale{}, threshold{}, exponent{};
};
struct LightVolume {
  std::uint32_t flags{}, bitmap = 0xffffffffu, sprite_count{}, runtime_flags{};
  // Original +10 offset, +18 radius, +20 brightness, +28 color, +30 facing.
  std::array<std::vector<std::byte>, 5> curves;
  std::optional<LightVolumeAspect> aspect;
  std::vector<Vec2> phases;
};
struct LightVolumeDefinition {
  float falloff{}, cutoff{};
  std::vector<LightVolume> volumes;
};
struct LightVolumeView {
  Vec3 eye;
  Transform world_to_view;
  Vec2 projection_scale; // Original 2FD90 pixel scales: P00*width/2, P11*height/2.
};
struct LightVolumeState {
  Transform marker;
  float opacity = 1;
  Vec3 color{1,1,1};
};
struct LightVolumeAttachment {
  std::uint32_t datum{};
  std::vector<Transform> markers; // Posed model coordinates, before placement.
};
struct LightVolumeInstance {
  std::shared_ptr<const LightVolumeDefinition> definition;
  LightVolumeState state;
  unsigned material{};
};
struct LightVolumeSprite {
  Vec3 center;
  Vec2 radius;
  std::uint32_t argb{};
};
struct LightVolumeBatch {
  std::uint32_t bitmap = 0xffffffffu;
  bool fuzzy{};
  float cosine = 1, sine{};
  std::vector<LightVolumeSprite> sprites;
};
struct LightVolumeProjection {
  Transform world_to_view;
  std::array<ShaderVector,4> projection; // Original column-major matrix.
  RenderViewport viewport;
  float far_distance = 1024;
};
struct LightVolumeVertex {
  ShaderVector screen; // Original XY in pixels; Z/W in 24-bit depth units.
  ShaderVector color;
  Vec2 uv;
};
// Original 2FD90 perspective branch, with the title's positive near plane.
// Projection axes come from original 2FB70; no oblique near-plane substitute.
LightVolumeProjection light_volume_title_projection(Vec3 eye, Vec3 forward, Vec3 up,
    Vec2 projection_axes, RenderViewport);
// Original 48B00/480A0, before NV2A's 1/16-pixel vertex quantization.
std::optional<std::array<LightVolumeVertex,4>> light_volume_quad(
    const LightVolumeSprite &, float cosine, float sine, const LightVolumeProjection &);
// Original4F010/4FF50: cached phase, linear offset and geometric radius
// sampling. Fuzzy rendering retains its separate GPU owner.
std::vector<LightVolumeBatch> light_volume_sprites(const LightVolumeDefinition &,
    const LightVolumeState &, const LightVolumeView &, const ShaderFunctionTables &);
PixelProgram light_volume_program(); // Original 4FE20 non-fuzzy pixel program.
} // namespace halo2
