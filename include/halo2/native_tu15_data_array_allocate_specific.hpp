#pragma once
#include "halo2/physics_contact_runtime.hpp"

namespace halo2 {
// Whole16E770 requested-datum allocation and its16E860 record initializer.
// These retain all eight general registers, private frame bytes and
// actual DF-directed clears. A missing/occupied slot returns FFFFFFFF;
// allocation is not resource I/O completion or sound-cache readiness.
void native_tu15_data_array_allocate_specific_abi(SourcePhysicsContactMemory &,
    SourcePhysicsContactRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});
void native_tu15_data_array_initialize_record_abi(SourcePhysicsContactMemory &,
    SourcePhysicsContactRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});
}
