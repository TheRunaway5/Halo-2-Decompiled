#include "halo2/source_bloom_context.hpp"
#include <bit>

namespace halo2 {
SourceBloomScreenContext source_player_bloom_screen_context(std::int16_t view_index) {
    // 13870 writes the signed player index and EAX=51ECF0, EBX=0;
    // 245C0 writes original 45DC2C/45DD30, then increments the reset counter.
    // Its 24EE0 LUT submission does not modify these nine context words.
    return {{{0,0x3f400000u,0x3ecccccdu,0,0x3f400000u,0x3ecccccdu,
              0x0051ecf0u,std::uint32_t(std::int32_t(view_index)),1}}};
}
float source_bloom_screen_threshold_factor(const SourceBloomScreenContext *context) {
    return context?std::bit_cast<float>(context->words[4]):1.f;
}
} // namespace halo2
