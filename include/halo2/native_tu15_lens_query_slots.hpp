#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
inline constexpr std::uint32_t native_tu15_lens_query_slot_count = 511;

// Actual TU1.5 20E30. Retire one of the 511 query records and clear its bit
// in all three visibility banks. Returns the stored disabled-query flags.
// The bank header, count, neighboring records and other mask bits survive.
// Stores retain source order; observers run after each committed store.
std::uint32_t native_tu15_retire_lens_query_slot(SourcePhysicsContactMemory&,
    std::uint32_t slot, const SourcePhysicsContactRecordObserver& = {});
}
