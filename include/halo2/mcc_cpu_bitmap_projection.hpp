#pragma once
#include "halo2/mcc_runtime_delivery.hpp"
#include "halo2/physics_world_runtime.hpp"
#include <memory>
namespace halo2::private_cpu_bitmap {
// World-owned CPU bitmap projection. World must enforce its completed,
// exclusive executor/no-active-call/no-borrow gate before lifecycle methods.
// Strict export and ownership-only prior-checkpoint recovery remain separate.
class WorldProjection final {
public:
    WorldProjection(const Cache&, SourceRuntimeMemory&, const MccRuntimeCacheDelivery&);
    ~WorldProjection();
    bool has_resources() const noexcept;
    // Copy-only export of exact authenticated derived storage. No live eviction.
    SourceRuntimeMemory::Checkpoint checkpoint_without_resources() const;
    bool matches_read_only_checkpoint_without_resources(
        const SourceRuntimeMemory::Checkpoint&) const noexcept;
    bool matches_read_only_checkpoint_for_restore(
        const SourceRuntimeMemory::Checkpoint&) const noexcept;
    unsigned first_row_for_canonical_root(TagHandle requested, unsigned canonical_root);
    unsigned row_for_canonical_frame(TagHandle requested,unsigned canonical_root,unsigned frame);
    void lookup(SourcePhysicsWorldRuntimeAbi&, SourcePhysicsFactoryFloatingPoint,
                const SourcePhysicsWorldRuntimeServices&,
                const SourcePhysicsContactRecordObserver&);
    // Original118480 consumes only the selected row's width at11860A.
    // Preserve that metadata lookup without loading/qualifying pixel resources.
    void lookup_dimensions(SourcePhysicsWorldRuntimeAbi&, SourcePhysicsFactoryFloatingPoint,
                           const SourcePhysicsWorldRuntimeServices&,
                           const SourcePhysicsContactRecordObserver&);
private:
    unsigned stage_frame(TagHandle,unsigned canonical_root,unsigned frame,bool resource_only);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
