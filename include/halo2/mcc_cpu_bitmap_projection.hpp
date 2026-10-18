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
    void lookup(SourcePhysicsWorldRuntimeAbi&, SourcePhysicsFactoryFloatingPoint,
                const SourcePhysicsWorldRuntimeServices&,
                const SourcePhysicsContactRecordObserver&);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
