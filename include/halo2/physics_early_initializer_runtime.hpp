#pragma once
#include "halo2/physics_pool_host_storage.hpp"
#include <stdexcept>
namespace halo2 {
// Complete TU1.5 early initializer and its guarded arena client. Correspondence
// IDs 1C5F00 and 1F15B0 select independently authenticated original bodies.
void source_run_physics_early_initializer_runtime(SourcePhysicsContactMemory &,
    std::uint32_t canonical_entry, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
class SourcePhysicsEarlyRendererUnavailable final : public std::runtime_error {
public:
    explicit SourcePhysicsEarlyRendererUnavailable(SourcePhysicsContactRuntimeCall c)
        :std::runtime_error("Required original early initializer renderer overflow storage owner unavailable"),call(c){}
    SourcePhysicsContactRuntimeCall call;
};
// Synchronous TU1.5 composition over exclusively owned retained memory and
// actual native pool/main pages. The caller must retire any async bank user
// first or provide exclusion around the complete call. Arena construction is
// a prerequisite. This does not construct renderer storage, transition the
// allocator mode or admit a world/body. Missing owners retain actual call ABI.
void source_run_physics_early_initializer_with_native_storage(SourceRuntimeMemory &,
    std::uint32_t canonical_entry, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsWorldRuntimeServices &,
    SourcePhysicsPoolHostStorage &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
