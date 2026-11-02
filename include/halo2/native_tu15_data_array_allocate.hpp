#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
struct SourcePhysicsContactRuntimeAbi;
// Whole16E6A0: select an actual free slot, publish occupancy/cursors/count,
// clear its complete record and issue the source salt. Allocation refusal is
// FFFFFFFF. Header/record aliases and all memory failure boundaries stay real.
std::uint32_t native_tu15_data_array_allocate(SourcePhysicsContactMemory &,
    std::uint32_t pool);
// Same whole16E6A0 with all eight GPRs and source frame/DF transport for
// compiled CPU callers such as the page allocator. Arithmetic flags are scratch.
void native_tu15_data_array_allocate_abi(SourcePhysicsContactMemory &,
    SourcePhysicsContactRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});
}
