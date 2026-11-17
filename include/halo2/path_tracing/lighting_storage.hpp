#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>

namespace halo2::path_tracing {
inline constexpr unsigned lighting_record_words=12,lighting_history_words=22;
inline constexpr unsigned lighting_previous_header_bytes=224;
struct LightingStoragePlan {
    unsigned scale{};
    std::uint64_t cells{},current_bytes{},previous_bytes{};
};
constexpr LightingStoragePlan lighting_storage_plan(unsigned width,unsigned height,std::uint64_t limit) {
    const auto pixels=std::uint64_t(width)*height;
    if(!width || !height || pixels>std::numeric_limits<std::uint32_t>::max())return {};
    const auto depth=32+pixels*4;
    if(depth+(lighting_record_words+2*lighting_history_words)*4>limit ||
       lighting_previous_header_bytes+2*lighting_history_words*4>limit)return {};
    // Choose the least undersampling that fits. Powers-of-two rounding made
    // modest record growth jump from4x4 to8x8 on a128MiB-range device at4K.
    const auto plan=[&](unsigned scale) {
        const auto cells=((std::uint64_t(width)+scale-1)/scale)*((std::uint64_t(height)+scale-1)/scale);
        const auto current=depth+cells*(lighting_record_words+2*lighting_history_words)*4;
        const auto previous=lighting_previous_header_bytes+cells*2*lighting_history_words*4;
        return LightingStoragePlan{scale,cells,current,previous};
    };
    const auto fits=[&](const LightingStoragePlan& value){return value.current_bytes<=limit && value.previous_bytes<=limit;};
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
