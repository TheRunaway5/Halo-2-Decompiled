#pragma once
#include <array>
#include <cstdint>

namespace halo2 {
// Inline player view +0xF4, passed by 2B790 to render-context +0x290.
// Words 0..5 are exposure ranges; +0x18 is the literal source resource
// token, +0x1C the signed view index and +0x20 the update counter.
struct SourceBloomScreenContext {
    std::array<std::uint32_t,9> words{};
};
// Complete state after the normal-player 13870 -> 245C0 call. 13870
// explicitly supplies mode zero, which resets these values every call.
// The source resource token is diagnostic data, never a native pointer.
SourceBloomScreenContext source_player_bloom_screen_context(std::int16_t view_index);
// 13870's optional screen context; null retains the unmodified .9 threshold.
// The following two subtractions are performed by bsp_bloom_frame.
float source_bloom_screen_threshold_factor(const SourceBloomScreenContext *);
} // namespace halo2
