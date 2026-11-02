#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool native_campaign_view_rectangle_owns_entry_tu15(std::uint32_t);
// Actual2B8B4 and13C8A callers only: EAX=camera, ESI=private rectangle.
// Both callers overwrite volatile GP/SIMD state before use. This transports
// actual operands, FP status and RET; it does not reconstruct dead registers.
void native_campaign_view_rectangle_run_tu15(SourcePhysicsContactMemory &,
    std::uint32_t entry,std::uint32_t caller,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
std::uint64_t native_campaign_view_rectangle_calls_tu15();
}
