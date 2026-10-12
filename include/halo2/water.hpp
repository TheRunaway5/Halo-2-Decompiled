#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
struct WaterDefinition {
  std::uint32_t shader = 0xffffffffu;
  // The original 0xAC water record, indexed exactly as 3D000's real array.
  // Metadata occupies entries 0..12; authored reals occupy 13..42.
  std::array<float,43> parameters{};
  Mesh geometry; // Authored position-only quad list expanded into triangles.
};
struct BspWater { std::vector<WaterDefinition> definitions; };
struct WaterFrame {
  std::array<ShaderVector,5> constants{}; // Original c21..25, 3D000.
  std::array<ShaderVector,4> bump_matrices{}; // MAT00,01,10,11, 3C650.
  std::array<ShaderVector,3> local_to_world{}; // Original c50..52, 3CA90/151C0.
  std::array<Vec3,4> corners{}; // Visibility/stencil quad, 3BF20.
  Vec3 horizon_color;
  std::uint32_t horizon_argb{}; // 131ED0 packed RGB; alpha remains zero.
  bool infinite{};
};
// 3C650, 3D000 and 3CA90. viewport_aspect is width / height of the source
// outer camera rectangle; vertical_fov is the raw source camera+0x28 angle,
// before 2F970 safe-frustum expansion. The original wide-view flag
// applies its authored 1.5 aspect factor when the viewport is anamorphic.
WaterFrame water_frame(const WaterDefinition &, Vec3 eye, Vec3 forward,
                       float vertical_fov, float viewport_aspect,
                       bool anamorphic = false);
Vec3 water_world_position(const WaterFrame &, Vec3 local);
// Retail352E0 mode1 samples the active color target four times. 34100 offsets
// by a quarter destination pixel mapped through the source/destination ratio.
// Water target22 is512x256 (1D770). The supplied spans are normalized in the
// complete source image, including the active player's camera rectangle.
PixelProgram water_copy_program();
std::array<ShaderVector,4> water_copy_taps(float normalized_source_width,
                                        float normalized_source_height);
// Highest authored LOD only. Infinite records use layer 3; pools use 15.
bool prepare_water_material(const Cache &, Material &, bool infinite = false);
}
