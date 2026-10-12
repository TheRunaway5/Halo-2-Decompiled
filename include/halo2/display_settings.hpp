#pragma once
#include <cstdint>
#include "halo2/source_geometry_lod.hpp"
namespace halo2 {
// Native presentation preferences shared across menu/game renderer lifetimes.
struct DisplaySettings {
    bool crt_filter{};
    unsigned internal_width{}, internal_height{}; // Zero follows the drawable.
    unsigned anti_aliasing{}; // Native SSAA: 0=off, 1=2x, 2=4x per axis.
    unsigned brightness=3, gamma=0, detail=0;
    bool safe_area{};
    unsigned volume=10, subtitles=0, speakers=4;
    bool operator==(const DisplaySettings &) const = default;
};
// PC590B26: High/Medium share the object policy. Low caps ordinary geometry
// at3 and writes coverage .8, secondary .5, shader metric .3. Retain stricter
// Xbox split-screen policy where applicable; postprocess detail is separate.
inline SourceGeometryLodPolicy native_detail_policy(unsigned detail,unsigned players=1) {
    auto policy=source_geometry_lod_policy(players);
    if(detail==2) {
        if(policy.ordinary_maximum>3)policy.ordinary_maximum=3;
        if(policy.coverage_multiplier>.8f)policy.coverage_multiplier=.8f;
        if(policy.secondary_scale>.5f)policy.secondary_scale=.5f;
        if(policy.effect_scale>.3f)policy.effect_scale=.3f;
    }
    return policy;
}
} // namespace halo2
