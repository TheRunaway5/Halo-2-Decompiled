#pragma once
#include "halo2/source_runtime_memory.hpp"
namespace halo2 {
// Original immutable channel offsets/import tokens only. No codec, channel,
// interrupt, callback record or ready word is constructed by this loader.
void mount_native_tu15_sound_codec_initialization_data(SourceRuntimeMemory&);
}
