#pragma once
#include "halo2/cache.hpp"
#include "halo2/render_view.hpp"

namespace halo2 {
struct ScopeCopyFrame {
  PixelProgram program;
  // Normalized in the complete active scene image, including split views.
  ShaderVector source_rectangle{}; // left, top, right, bottom
  ShaderTextureState texture_state;
  static constexpr unsigned target_extent = 64;
};
// Original 2B000 copy combiner. Its 14850/158F0 target is 64x64 at every
// resolution; 2B220 scales the sampled source region with the player view.
ScopeCopyFrame scope_copy_frame(ShaderVector normalized_source_rectangle);
ShaderVector scope_source_rectangle(RenderViewport, Vec2 authored_scope_size,
                                  Vec2 source_center_pixels, Vec2 scene_extent);
// 1396C7 mode4 rounds the integer viewport center down after adding .5.
// For an active player, its successfully projected aiming point changes
// only Y. That projection is supplied by the original observer/biped query.
Vec2 scope_sample_center(RenderViewport,
                        std::optional<float> projected_player_aim_y = {});
// Original 2B220 destination position/depth, useful for source fixtures.
ShaderVector scope_copy_vertex(Vec2 uv, float eye_distance,
                              float near_plane, float far_plane);
} // namespace halo2
