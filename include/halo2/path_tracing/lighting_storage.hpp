#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>

namespace halo2::path_tracing {
// Every size is a multiple of four words: the denoiser and resolve read
// records, histories and packed cells as 16-byte lanes.
// Per grid cell: the current record (surface guides and lighting), two
// persistent histories (world/foreground) and two eight-word packed banks
// the spatial (a-trous) denoiser ping-pongs between.
inline constexpr unsigned lighting_record_words=20,lighting_history_words=24,lighting_filter_words=16;
static_assert(lighting_record_words%4==0 && lighting_history_words%4==0 && lighting_filter_words%4==0);
// Previous cameras/viewports, the union of changed geometry and light
// bounds, then up to lighting_change_boxes local changed-geometry boxes.
inline constexpr unsigned lighting_change_boxes=32;
inline constexpr unsigned lighting_previous_header_bytes=224+lighting_change_boxes*2*16;
struct LightingStoragePlan {
    unsigned scale{};
    std::uint64_t cells{},current_bytes{},previous_bytes{};
};
constexpr LightingStoragePlan lighting_storage_plan(unsigned width,unsigned height,std::uint64_t limit) {
    const auto pixels=std::uint64_t(width)*height;
    if(!width || !height || pixels>std::numeric_limits<std::uint32_t>::max())return {};
    // The depth prefix is padded to whole 16-byte lanes (see Render).
    const auto depth=32+(pixels+3)/4*16;
    if(depth+(lighting_record_words+2*lighting_history_words+lighting_filter_words)*4>limit ||
       lighting_previous_header_bytes+2*lighting_history_words*4>limit)return {};
    // Choose the least undersampling that fits. Powers-of-two rounding made
    // modest record growth jump from4x4 to8x8 on a128MiB-range device at4K.
    const auto plan=[&](unsigned scale) {
        const auto cells=((std::uint64_t(width)+scale-1)/scale)*((std::uint64_t(height)+scale-1)/scale);
        const auto current=depth+cells*(lighting_record_words+2*lighting_history_words+lighting_filter_words)*4;
        const auto previous=lighting_previous_header_bytes+cells*2*lighting_history_words*4;
        return LightingStoragePlan{scale,cells,current,previous};
    };
    // Ray cost scales with cells, while indirect lighting is low-frequency and
    // direct sun light is resolved per pixel. Keep about a 1440p half-resolution
    // grid at any output size: 2x2 up to 1440p, 3x3 at 4K. Cell count is
    // monotonic in scale, so the search below still finds the least scale.
    constexpr std::uint64_t cell_budget=1300000;
    const auto fits=[&](const LightingStoragePlan& value) {
        return value.current_bytes<=limit && value.previous_bytes<=limit && value.cells<=cell_budget;
    };
    if(const auto native=plan(2);fits(native))return native;
    // Cell count is monotonic in scale. Bound admission to32 steps even for
    // an adversarial public helper input such as UINT32_MAX by1 pixels.
    unsigned first=2,last=std::max({width,height,2u});
    while(first<last) {
        const auto middle=first+(last-first)/2;
        if(fits(plan(middle)))last=middle;
        else first=middle+1;
    }
    return plan(first);
}
}
