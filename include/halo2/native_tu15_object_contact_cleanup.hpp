#pragma once
#include "halo2/native_tu15_array_growth.hpp"

namespace halo2 {
// Object-factory pair deletion uses the same native contact cleanup services
// as the world step. The caller owns at least 0x1a0 bytes of private DATA
// below scratch_end. Unknown children refuse without any CPU transport.
void native_tu15_cleanup_object_factory_contact(SourcePhysicsContactMemory&,
    std::uint32_t receiver,std::uint32_t owner,std::uint32_t scratch_end,NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&,const SourcePhysicsContactRecordObserver& = {});
}
