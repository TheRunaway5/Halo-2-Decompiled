#include "halo2/native_tu15_renderer_depth.hpp"
namespace halo2 {
void mount_native_tu15_renderer_depth_state_data(SourceRuntimeMemory& memory) {
    // Authenticated TU1.5 virtual-tail zeros. These two scalar SDK caches are
    // written directly by original12560, not an imported driver object bank.
    memory.mount(0x411678,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "source12560 inline depth-write scalar cache");
    memory.mount(0x41165c,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "source12560 inline depth-function scalar cache");
}
}
