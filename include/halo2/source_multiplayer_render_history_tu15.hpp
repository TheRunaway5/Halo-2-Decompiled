#pragma once
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Stage the recipient's actual native renderer owners into a separately
// prepared complete multiplayer world. No object allocation, visibility,
// resource completion or model choice is supplied. The enclosing authority
// transaction holds both banks quiescent and discards a rejected replacement.
void source_stage_multiplayer_render_history_tu15(const SourceRuntimeMemory &current,
    SourceRuntimeMemory &replacement);
}
