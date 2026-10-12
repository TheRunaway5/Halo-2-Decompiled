#pragma once
#include "halo2/source_bitmap_manager_transition.hpp"

namespace halo2 {
struct SourceBitmapReservationInput {
    std::int32_t bytes{},age{};
};
struct SourceBitmapReservationResult {
    SourceBitmapManagerBoundary boundary{SourceBitmapManagerBoundary::Complete};
    std::uint32_t handle{0xffffffffu};
    std::uint32_t pending_delete{0xffffffffu};
};
// Complete13D370 for the proved texture-manager ranking2/shift12 and ages2/5.
// Caller owns coherent paired raw banks and the actual consumed busy inputs.
// Complete+NONE is a source failure, preserving its fence/eviction/limit writes.
// A real callback wait reports a boundary and preserves preceding writes; it
// supplies neither a wait result nor a retry-safe continuation.
// Success reserves only a physical slot. The source caller's same-salt image
// binding must occur before another paired-bank reservation or manager call.
SourceBitmapReservationResult source_bitmap_reserve(SourceBitmapManagerBanks &,
    const SourceBitmapReservationInput &,const SourceBitmapManagerContext &);
} // namespace halo2
