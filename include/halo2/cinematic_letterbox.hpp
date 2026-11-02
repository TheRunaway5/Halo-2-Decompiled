#pragma once
#include <array>
#include <optional>

namespace halo2 {
// Exact overlay.frag push constants. The source progress remains independent
// of the PC surface's physical bar height.
inline std::array<float,8> cinematic_letterbox_program(float progress,
    unsigned width,unsigned height,std::optional<float> authored_aspect={}) {
    float bar=progress*.125f;
    const float surface_aspect=float(width)/float(height);
    if(authored_aspect && surface_aspect>*authored_aspect) {
        const float framed_aspect=*authored_aspect/(1.f-2.f*bar);
        bar=surface_aspect>=framed_aspect?0.f:(1.f-surface_aspect/framed_aspect)*.5f;
    }
    return {0,0,0,0,float(width),float(height),bar,progress};
}
}
