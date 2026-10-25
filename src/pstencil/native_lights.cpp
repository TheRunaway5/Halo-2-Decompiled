#include "halo2/pstencil/native_lights.hpp"
namespace halo2::pstencil {
NativeLightSnapshot native_lights_tu15(const SourceRuntimeMemory& memory,std::size_t clusters) {
    return source_native_light_snapshot_tu15(memory,clusters);
}
}
