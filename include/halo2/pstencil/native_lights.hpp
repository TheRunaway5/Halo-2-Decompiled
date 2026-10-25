#pragma once
#include "halo2/source_native_light_snapshot_tu15.hpp"
namespace halo2 {
class SourceRuntimeMemory;
namespace pstencil {
// Completed simulation state, including original spatial lists and full salts.
// A view may update its private fades/visit stamps; it must not allocate, tick,
// register or retire lights in this snapshot or publish it back to simulation.
using NativeLightSnapshot=SourceNativeLightSnapshotTu15;
NativeLightSnapshot native_lights_tu15(const SourceRuntimeMemory&,std::size_t clusters);
}
}
