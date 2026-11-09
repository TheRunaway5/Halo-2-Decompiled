#pragma once
#include "halo2/native_tu15_object_scan_kernel.hpp"
namespace halo2 {
struct NativeTu15BspObjectAssociationScratch {
    NativeTu15ObjectScanState reset_scan,association_scan;
};
struct NativeTu15BspObjectAssociationStats {
    std::uint64_t attempts{},completed{},refusals{},iterator_calls{},
        reset_objects{},association_records{},matched_objects{},excluded_objects{};
};
// Sole native58-site BCEE0 DATA iterator. No GP image or executor is accepted.
std::uint32_t native_tu15_next_scanned_object(SourcePhysicsContactMemory&,
    NativeTu15ObjectScanState,const SourcePhysicsContactRecordObserver& = {});
// Actual219150 resets per-object BSP association fields,2191D0 restores
// authored associations,2190F0 excludes flagged scenery and2190C0 composes all.
void native_tu15_reset_bsp_object_associations(SourcePhysicsContactMemory&,
    NativeTu15ObjectScanState,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15BspObjectAssociationStats* = nullptr);
void native_tu15_apply_bsp_object_associations(SourcePhysicsContactMemory&,
    NativeTu15BspObjectAssociationScratch,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15BspObjectAssociationStats* = nullptr);
void native_tu15_exclude_noncollidable_scenery(SourcePhysicsContactMemory&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15BspObjectAssociationStats* = nullptr);
void native_tu15_initialize_bsp_object_associations(SourcePhysicsContactMemory&,
    NativeTu15BspObjectAssociationScratch,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15BspObjectAssociationStats* = nullptr);
} // namespace halo2
