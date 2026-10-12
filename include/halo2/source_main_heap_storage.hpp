#pragma once
#include "halo2/physics_contact_record.hpp"
#include "halo2/source_static_allocation.hpp"

namespace halo2 {
// Complete TU1.1 1482A0 memory/EAX owner over the real33785C resource boundary.
// Retains raw storage with the provider, writes alignment metadata and480D90.
// Success returns the raw allocation; failure returns actual GetLastError EAX.
// Does not construct heap records, alter pressure or represent caller GP/stack
// state. The later148190 owner consumes these retained bytes with its full ABI.
std::uint32_t source_initialize_main_heap_storage_1_1(
    SourcePhysicsContactMemory &, const SourceStaticAllocationServices &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
