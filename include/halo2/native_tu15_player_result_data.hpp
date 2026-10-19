#pragma once
#include "halo2/source_runtime_memory.hpp"
namespace halo2 {
// Complete authored result descriptor data only. Original game/result owners
// retain their live storage, flags, player/team updates and recording lifecycle.
void mount_native_tu15_player_result_data(SourceRuntimeMemory&);
}
