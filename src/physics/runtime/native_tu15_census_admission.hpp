#pragma once
#include <cstdint>
#include <cstdlib>

namespace halo2::native_census_detail {
// HALO2_CENSUS_NATIVE_BRIDGES=1 is a diagnostic for instruction censuses
// (HALO2_INSTRUCTION_CENSUS with HALO2_REFERENCE_INSTRUCTION_CALLBACKS=1).
// Those callbacks carry no literal instruction cursor, so every cursor-gated
// native bridge would refuse and its translated fallback would be counted.
// With this switch a bridge whose closure holds none of the three live
// service PCs runs natively under such callbacks: only its own per-PC census
// counts and the last-PC diagnostic disappear. Stores, observers, children
// and results are unchanged; the census then counts only translated code.
inline bool enabled() noexcept {
    static const bool value=[]{const auto*p=std::getenv("HALO2_CENSUS_NATIVE_BRIDGES");return p&&*p;}();
    return value;
}
inline constexpr std::uint32_t service_sites[]{0x38846eu,0x21f590u,0x33b595u};
// True for full callbacks without a cursor when the switch is set and no
// service PC lies in [first,last].
template<class State>bool uncursored(const State&s,std::uint32_t first,std::uint32_t last) noexcept {
    if constexpr(requires{s.instruction_cursor;s.services.instruction;}) {
        if(!enabled()||!s.services.instruction||s.instruction_cursor)return false;
        for(const auto site:service_sites)if(site>=first&&site<=last)return false;
        return true;
    } else return false;
}
// HALO2_CENSUS_COUNT_NATIVE_REPORTS=1 keeps the original-PC reports of the
// complete natives below in the census (to measure what they contribute).
inline bool count_native_reports() noexcept {
    static const bool value=[]{const auto*p=std::getenv("HALO2_CENSUS_COUNT_NATIVE_REPORTS");return p&&*p;}();
    return value;
}
// Services-level form for complete natives that report each original PC
// through services.instruction (the weapon marker owners, D80A0, the BCB70
// scalar frame). True only under the census switch with full callbacks and
// no cursor, and never for a live service PC: the report is then skipped, so
// the census counts translated instructions only. Production (cursor
// present) and every other diagnostic keep the report.
template<class Services>bool skip_report(const Services&services,std::uint32_t pc) noexcept {
    if constexpr(requires{services.instruction;services.instruction_cursor.position;}) {
        if(!enabled()||count_native_reports()||!services.instruction||services.instruction_cursor.position)return false;
        for(const auto site:service_sites)if(site==pc)return false;
        return true;
    } else return false;
}
} // namespace halo2::native_census_detail
