#pragma once
#include "halo2/render_program.hpp"

namespace halo2 {
// The four-tap 352E0 mode. Resource selection/readiness, target allocation and
// viewport publication remain the caller's responsibility.
struct SourceSceneCopyContext {
    // Signed source camera shorts, in original top,left,bottom,right order.
    std::array<std::int16_t,4> camera_rectangle;
    std::int32_t destination_width, destination_height;
    float near_plane, far_plane;
};
struct SourceSceneCopyVertex {
    ShaderVector position;
    // Original attributes1..4, in source pixel coordinates, including Z0/W1.
    std::array<ShaderVector,4> texture_coordinates;
    ShaderVector corner; // Attribute5: corner X/Y, Z0/W1.
};
struct SourceSceneCopyFrame {
    PixelProgram program;
    // Original quad order: (0,0),(0,1),(1,1),(1,0).
    std::array<SourceSceneCopyVertex,4> vertices;
};
PixelProgram source_scene_copy_program();
// Whole34A90 full-target/camera-source flags and34100/350E0 finite arithmetic.
// Nearest-even, masked, gradual binary32 is required. Coordinates describe CPU
// emission; they do not establish GPU raster centers or framebuffer results.
SourceSceneCopyFrame source_scene_copy_frame(const SourceSceneCopyContext &);

// Native fragment delivery for the same original four-tap program. Coordinate
// extent is the actual source texture's coordinate space, before any explicit
// host resolution scaling. It is not inferred from the destination or window.
struct SourceSceneCopyShaderUniform {
    std::array<std::uint32_t,60> program;
    ShaderVector source_rectangle,taps,destination;
};
static_assert(sizeof(SourceSceneCopyShaderUniform)==288);
SourceSceneCopyShaderUniform source_scene_copy_shader_uniform(
    const SourceSceneCopyContext &,std::array<std::int32_t,2> coordinate_extent);
} // namespace halo2
