#pragma once
#include <cstdint>

namespace halo2 {
class NativeTu15DamageRegionEligibilityStorage {
public:
    virtual ~NativeTu15DamageRegionEligibilityStorage()=default;
    // Query the actual constructed object's regions before reading/testing
    // the authored runtime index, including a missing/out-of-range region.
    virtual void query_regions()=0;
    virtual std::int16_t runtime_region()=0;
    virtual std::int32_t region_count()=0;
    virtual std::int8_t current_state(std::int32_t region)=0;
    virtual std::int16_t requested_state()=0;
};
enum class NativeTu15DamageRegionEligibility {
    missing_region,below_requested,already_at_or_above
};
constexpr bool native_tu15_damage_region_allowed(NativeTu15DamageRegionEligibility value) {
    return value!=NativeTu15DamageRegionEligibility::already_at_or_above;
}
// Actual DC4C0 predicate. Signed byte/word comparisons and default-allow are
// source behavior; no region-name search, enum clamp or object fallback exists.
inline NativeTu15DamageRegionEligibility native_tu15_damage_region_eligibility(
    NativeTu15DamageRegionEligibilityStorage& storage) {
    storage.query_regions();
    const auto region=storage.runtime_region();
    if(region<0||region>=storage.region_count())return NativeTu15DamageRegionEligibility::missing_region;
    const auto current=storage.current_state(region);
    return current<storage.requested_state()?NativeTu15DamageRegionEligibility::below_requested:
        NativeTu15DamageRegionEligibility::already_at_or_above;
}
}
