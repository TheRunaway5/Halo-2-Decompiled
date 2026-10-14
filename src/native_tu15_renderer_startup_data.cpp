#include "halo2/native_tu15_renderer_startup_data.hpp"
namespace halo2 {
void mount_native_tu15_renderer_startup_data(SourceRuntimeMemory& m) {
    m.mount(0x51df44,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"source renderer raw storage constructor binding");
    m.mount(0x556e18,std::vector<std::byte>(0x55c0),SourceRuntimeMemory::Access::ReadWrite,"source two renderer descriptor constructor destinations");
}
}
