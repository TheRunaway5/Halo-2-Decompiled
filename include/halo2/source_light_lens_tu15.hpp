#pragma once
#include "halo2/effect_light_view.hpp"
#include "halo2/source_lens_frame_world_tu15.hpp"

namespace halo2 {
struct SourceLightLensRequestTu15 {
    std::uint32_t identity,lens;
    Vec3 position,forward,color;
    float brightness,opacity;
};
// Original2C320/166BB0's actual visible order, then31500/C5370's closed
// standalone branch. Attached/first-person pose and priority remain separate.
std::vector<SourceLightLensRequestTu15> source_standalone_light_lenses_tu15(
    const SourceEffectLightView &);
// Literal C54FB..C5545 request ABI, submitted to this frame's actual bank.
void source_request_light_lens_tu15(SourceLensFrameWorldTu15,
    const SourceLightLensRequestTu15 &);
}
