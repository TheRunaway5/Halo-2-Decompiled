#pragma once
#include <cstdint>
#include <span>

namespace halo2 {
// Explicit pattern phase at this target's pixel(0,0). The retained Xbox
// 640x480 hardware profile is {0,31}; a cropped target must preserve its
// actual global origin rather than restart the pattern at the view edge.
// No framebuffer-height or viewport-origin producer is inferred here.
struct SourceStippleRasterPhase {
    std::uint32_t column_origin{},row_origin{};
};
// Coordinates are integer framebuffer pixels in top-left order, not normalized
// UVs or sample-center floats. Uploaded rows are literal NV097 0x1480 DWORDs.
// The observed hardware reads MSB-first within each byte, retaining byte order.
bool source_stipple_pixel(std::span<const std::uint32_t,32> uploaded_rows,
    std::uint32_t target_x,std::uint32_t target_y,SourceStippleRasterPhase);
}
