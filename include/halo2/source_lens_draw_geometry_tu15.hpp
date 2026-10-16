#pragma once
#include "halo2/cache.hpp"
namespace halo2 {
struct SourceLensVertex {
    ShaderVector position{};
    Vec2 uv{};
    std::uint32_t rgba{};
    float secondary_alpha{};
};
static_assert(sizeof(SourceLensVertex)==32);
struct SourceLensDraw {
    unsigned material{};
    bool depth_test{};
    std::array<SourceLensVertex,4> vertices{};
};
// Only the exact complete136-byte48090 packet is admitted. Resource, shader
// and depth state come from the caller's original SDK owner, not geometry.
SourceLensDraw source_lens_draw_geometry_tu15(std::span<const std::byte>,unsigned material,
    float secondary_alpha,bool depth_test);
}
