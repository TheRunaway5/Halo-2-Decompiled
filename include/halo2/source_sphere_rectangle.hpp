#pragma once
#include "halo2/source_view_projection.hpp"

namespace halo2 {
// Complete30290: actual rendering bank4B9E14, camera position4B9DA0 and
// signed-short outer rectangle at camera+30. Output is x0,x1,y0,y1 in
// framebuffer coordinates, with the original maximum-float empty/full bounds.
// No viewport reconstruction, world bounds fitting or integer scissor mapping.
std::array<float,4> source_sphere_rectangle(const SourceViewProjectionBank&,
    Vec3 actual_camera_position,std::array<std::int16_t,4> viewport,
    Vec3 center,float radius);
}
