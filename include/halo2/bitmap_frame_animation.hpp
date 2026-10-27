#pragma once
#include "halo2/shader_animation.hpp"

namespace halo2 {
// Prepared once from the selected pass's template bitmap slots. The frame
// evaluator retains only authored functions/counts, never image payloads.
struct BitmapFrameAnimation {
    unsigned stage{},image_count{};
    ShaderOverlay function;
    std::shared_ptr<const ShaderFunctionTables> tables;
};
std::vector<BitmapFrameAnimation> bitmap_frame_animations(const Material &);
std::array<std::uint32_t,4> animate_bitmap_frames(
    std::span<const BitmapFrameAnimation>,double seconds,const ShaderInput &,
    std::array<std::uint32_t,4> initial);
}
