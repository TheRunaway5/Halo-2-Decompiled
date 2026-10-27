#pragma once
#include "halo2/path_tracing/scene.hpp"
namespace halo2::path_tracing {
// Evaluate the bounded secondary surface atlas at raw authored mesh UV.
// A missing value is an actual used-stage texture alpha-kill rejection;
// source raster alpha comparison remains the caller's independent operation.
std::optional<ShaderVector> material_surface(const Scene&,std::uint32_t material,Vec2);
}
