#pragma once
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
class Cache;
// Authenticated arithmetic operands and actual HUD/bitmap metadata only.
// This is a native presentation resource bank, with no player allocation,
// physics world, game clock advancement or authority-restoration capability.
// The compiled consumer still requires a real collision service if an
// authored style actually requests one; no clear-ray result is supplied.
SourceRuntimeMemory native_objective_waypoint_memory_tu15(const Cache&);
}
