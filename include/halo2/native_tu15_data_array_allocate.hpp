#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Whole16E6A0: select an actual free slot, publish occupancy/cursors/count,
// clear its complete record and issue the source salt. Allocation refusal is
// FFFFFFFF. Header/record aliases and all memory failure boundaries stay real.
std::uint32_t native_tu15_data_array_allocate(SourcePhysicsContactMemory &,
    std::uint32_t pool);
}
