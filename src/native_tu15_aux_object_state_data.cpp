#include "halo2/source_aux_object_state_tu15.hpp"
namespace halo2 {
void mount_native_tu15_aux_object_state_data(SourceRuntimeMemory& memory) {
    memory.mount(0x51daf0,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,"source10E3B0/10FA30 arena state bindings");
    memory.mount(0x52be6c,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"source1F4700 arena state binding");
}
}
