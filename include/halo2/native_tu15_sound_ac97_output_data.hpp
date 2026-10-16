#pragma once
#include "halo2/source_runtime_memory.hpp"
namespace halo2 {
// Exact immutable operands only. No channel/controller construction, codec
// register image, timer object, ready flag or processor program is mounted.
void mount_native_tu15_sound_ac97_output_data(SourceRuntimeMemory&);
}
