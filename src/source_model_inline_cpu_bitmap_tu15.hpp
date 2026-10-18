#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/tags/types.hpp"
#include <functional>
namespace halo2::private_cpu_bitmap {
// Exact3DAF0 CPU bitmap boundary. The provider must authenticate
// this exact requested full-salt handle + canonical root/current source, then
// stage/refresh actual owned frame0 data through the retained Backing owner.
using InlineBitmapFirstRow = std::function<unsigned(TagHandle requested,
                                                   unsigned canonical_root)>;
// Retains the complete compiled original model body. Only D459D's exact4B
// caller-stack read -> D45A5 -> D5610 -> D563B/root+48 read may be projected.
// All descendant services receive the actual bank after exact adapter identity
// validation. Original/non-MCC entrypoints must continue using the old runner.
void run_model_3daf0_with_inline_cpu_bitmaps(SourceRuntimeMemory&,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,
    const SourcePhysicsContactRecordObserver&,
    const InlineBitmapFirstRow&);
}
