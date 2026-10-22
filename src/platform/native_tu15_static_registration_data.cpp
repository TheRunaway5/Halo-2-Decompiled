#include "halo2/source_static_registration_tu15.hpp"
#include "halo2/source_network_session_startup_tu15.hpp"
namespace halo2 {
void mount_native_tu15_static_registration_data(SourceRuntimeMemory& memory) {
    mount_native_tu15_network_session_presence_storage(memory);
    memory.mount(0x4dc040,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,"source69220 registration publication byte");
    memory.mount(0x4dc04c,std::array<std::byte,12>{},SourceRuntimeMemory::Access::ReadWrite,"source69220 registration bindings");
    memory.mount(0x555808,std::array<std::byte,0x108>{},SourceRuntimeMemory::Access::ReadWrite,"source69220 two registration arrays");
}
}
