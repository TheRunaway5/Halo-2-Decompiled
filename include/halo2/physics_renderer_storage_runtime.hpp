#pragma once
#include "halo2/physics_pool_host_storage.hpp"
#include <stdexcept>
namespace halo2 {
// Complete raw renderer producer 133760, descriptor constructor 1360F0 and
// child constructor 165740. TU1.5 selection is independently paired. These
// are source bodies; actual process-heap allocation remains a required owner.
void source_run_physics_renderer_storage_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry,SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsContactImage,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
class SourcePhysicsRendererAllocatorUnavailable final : public std::runtime_error {
public:
    explicit SourcePhysicsRendererAllocatorUnavailable(SourcePhysicsContactRuntimeCall c)
        :std::runtime_error("Required original renderer process-heap allocation owner unavailable"),call(c){}
    SourcePhysicsContactRuntimeCall call;
};
// Synchronous fresh native pages for the original raw wrapper only. No child
// descriptor or ordinary allocation is fabricated. The caller exclusively
// owns this bank; all supplied lower owners must retain the full source ABI.
void source_run_physics_renderer_storage_with_native_pages(SourceRuntimeMemory &,
    std::uint32_t canonical_entry,SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices &,
    SourcePhysicsPoolHostStorage &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
