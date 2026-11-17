#pragma once
#include <array>
#include <cstdint>
namespace halo2 {
struct NativeTu15BoxShapeSupportPoint {
    std::array<std::uint32_t,4> point{},masked_direction{};
    std::uint32_t feature{};
};
// Actual306790 uses bitwise sign selection, including the fourth payload lane.
// No floating-point comparison or arithmetic participates in this policy.
inline constexpr NativeTu15BoxShapeSupportPoint native_tu15_box_shape_support_point(
    const std::array<std::uint32_t,4>&extent,const std::array<std::uint32_t,4>&sign,
    const std::array<std::uint32_t,4>&direction)noexcept{
    NativeTu15BoxShapeSupportPoint result;
    for(unsigned n=0;n<4;++n){result.masked_direction[n]=direction[n]&sign[n];result.point[n]=extent[n]^result.masked_direction[n];}
    for(unsigned n=0;n<3;++n)result.feature|=(result.point[n]>>31u)<<n;
    return result;
}
} // namespace halo2
