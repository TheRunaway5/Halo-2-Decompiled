#pragma once
#include "halo2/native_tu15_island_partition_metadata.hpp"

namespace halo2 {
// Actual325E90, table41F478 virtual+28, RET4(info16B). Every packet byte is
// initialized before return. Current collector+94 is read after the initial
// four stores; no source GP/control metadata or pointer lifetime is exposed.
void native_tu15_get_contact_collector_info(SourcePhysicsContactMemory&,
    std::uint32_t collector,NativeTu15IslandConstraintInfo,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
