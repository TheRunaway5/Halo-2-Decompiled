#pragma once
#include "halo2/cache.hpp"
#include "halo2/render_view.hpp"

namespace halo2 {
struct SourceViewGrid {
  std::array<std::int16_t,2> grid{1,1}, span{1,1}, position{};
};
// Original 2F600/2F640 layout, including the spanning first view for three
// players. Widescreen selects original layout mode2, standard mode1.
SourceViewGrid source_view_grid(unsigned players,unsigned view,bool widescreen);
struct SourceCameraAngles {
  float projection_angle{}; // Render camera+0x28; vertical angle in 2FB70.
  float lighting_distance_scale{}; // Render camera+0x2C, global4B9DCC.
};
// 171D90 creates the observer's vertical angle and FOV/baseline ratio.
// 2F800 applies the original split-grid factors to both of those values.
// Grid/span entries are {columns,rows}, as 2F600/2F640 produce them.
SourceCameraAngles source_camera_angles(float observer_horizontal_angle,
    float observer_aspect = 1.3333333730697632f,
    std::array<std::int16_t,2> grid = {1,1},
    std::array<std::int16_t,2> span = {1,1},
    float baseline_horizontal_angle = 1.2217304706573486f);
struct SourceObserverAim {
  Vec3 forward{}, up{};
  float displacement{}; // Original observer+0x48, consumed by 1396C7.
};
// Original 171D90 screen-offset rotations, before 2F800 changes the angle
// for the current player cell. Inputs are the unrotated aiming direction,
// perpendicular up and authored matg+0xF0 control record+0x18/+0x1C.
SourceObserverAim source_observer_aim(Vec3 forward, Vec3 up,
    float observer_horizontal_angle, Vec2 screen_offset,
    float observer_aspect = 1.3333333730697632f);
struct SourceHudAimCamera {
  Vec3 position{}, forward{}, up{};
  float projection_angle{};
  bool widescreen{};
};
// Original 2F970 subrectangle passed into 2FB70. The safe rectangle expands
// the rendered field beyond the raw observer angle, including its center.
ShaderVector source_camera_frustum(RenderViewport outer, RenderViewport safe);
struct SourceCameraProjection {
  float horizontal_scale{},vertical_scale{},horizontal_offset{},vertical_offset{};
  float rendered_vertical_angle{};
  Vec2 clip_scale{1,1},clip_offset{}; // Unflipped clip XY += clip_offset*W.
};
SourceCameraProjection source_camera_projection(float camera_projection_angle,
    RenderViewport outer, RenderViewport safe, bool widescreen);
// Original 1396C7 mode4: inactive midpoint uses the safe camera rectangle;
// active 30710 projection uses the outer rectangle, with the 2F970 frustum. Only
// anchor Y is replaced on successful projection; both coordinates round
// with the source floor(value+.5) conversion.
Vec2 source_hud_aim_anchor(const SourceHudAimCamera &,
    RenderViewport outer, RenderViewport safe, float observer_displacement,
    bool active_first_person = true);
} // namespace halo2
