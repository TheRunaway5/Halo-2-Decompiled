#pragma once
#include "halo2/native_tu15_array_growth.hpp"

namespace halo2 {
// Source31C450 writes its removed value (or zero) to the caller's ordinary4B
// DATA. It shifts remaining8B entries in order and rereads count/backing.
void native_tu15_remove_body_property(SourcePhysicsContactMemory&,std::uint32_t body,
    std::uint32_t property,std::uint32_t result,
    const SourcePhysicsContactRecordObserver& = {});
// Source31C5E0 leaves an already present key unchanged; only a missing key is
// appended, using native2E3250 growth with the caller's typed native allocator.
void native_tu15_add_body_property(SourcePhysicsContactMemory&,std::uint32_t body,
    std::uint32_t property,std::uint32_t value,NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Source1D7660 takes its body as semantic ESI and RET4(value). The dead removed
// value uses independently owned caller DATA; no saved incoming ECX is copied.
void native_tu15_replace_part_contact_property(SourcePhysicsContactMemory&,
    std::uint32_t body,std::uint32_t value,std::uint32_t removed_result,
    NativeTu15PhysicsFrameFp&,const NativeTu15ArrayGrowthServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
