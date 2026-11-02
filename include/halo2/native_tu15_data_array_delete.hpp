#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
struct SourcePhysicsContactRuntimeAbi;
// Original16E8A0, generic real data-array free. The caller supplies its actual
// pool and datum with the engine ABI's forward string direction (DF=0).
// Reads and stores remain lazy, including aliasing metadata.
void native_tu15_data_array_delete(SourcePhysicsContactMemory &,
    std::uint32_t pool,std::uint32_t datum);
// Complete16E8A0 integer/frame ABI, including the caller's actual string
// direction. The semantic owner above keeps its existing forward contract.
void native_tu15_data_array_delete_abi(SourcePhysicsContactMemory &,
    SourcePhysicsContactRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});
}
