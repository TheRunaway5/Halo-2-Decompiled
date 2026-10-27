#pragma once
#include "halo2/source_view_projection.hpp"
#include <array>
#include <cstddef>
#include <span>

namespace halo2 {
// Literal3BCF0 CPU setup before12F80/3EDD0. The camera remains camera2;
// only its raster rectangle changes. Reflection changes the copied render
// bank directly, preserving the original projection, near/far and W owner.
// This value is not a completed sky render, target allocation or GPU return.
struct SourceWaterReflectionTu15 {
    std::array<std::byte,0x74> submitting_camera;
    SourceViewProjectionBank rendering;
    std::array<ShaderVector,12> constants;
};
SourceWaterReflectionTu15 source_water_reflection_tu15(
    std::span<const std::byte,0x74> actual_submitting_camera,
    const SourceViewProjectionBank &actual_submitting_rendering,
    float actual_water_height);
}
