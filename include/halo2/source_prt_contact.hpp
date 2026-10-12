#pragma once
#include "halo2/source_prt.hpp"
#include <optional>
#include <span>

namespace halo2 {
// Literal current 104-byte record at retail4B89B0. This is the current object
// cache, not an authored render-model PRT definition or a GPU uniform bank.
struct SourcePrtContactCacheRecord { std::array<std::uint32_t,26> words{}; };
static_assert(sizeof(SourcePrtContactCacheRecord)==104);
struct SourcePrtContactPoint { float cluster{}; std::array<float,4> weights{}; };
struct SourcePrtContactSample { Vec3 incident{},color{}; };
// D1E10's CPU declaration57 conversion. Its midpoint quantizer differs from
// the NV2A SHORT vertex-input conversion used to draw the same raw resource.
SourcePrtContactPoint source_prt_contact_short_point(const std::array<std::int16_t,5>&);
// Complete retail241C0. The ordered cache prefix and arena starting at487B1C
// must be supplied by the actual current cache producer. Full owner keys are
// compared, including salt. NONE means no matching current record, with no
// sample written. Cache slot is D1E10 contact+1C (render region), independently
// of its geometry section+14. Palette is the separate original argument.
std::optional<SourcePrtContactSample> source_prt_contact_sample(
    std::span<const SourcePrtContactCacheRecord>,std::int32_t active_count,
    std::span<const std::byte> arena,std::uint32_t owner,std::uint32_t cache_slot,
    std::int32_t palette,const std::array<SourcePrtContactPoint,3>&,
    float fraction0,float fraction1);
}
