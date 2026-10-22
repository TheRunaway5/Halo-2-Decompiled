#include "halo2/native_tu15_view_storage_data.hpp"
namespace halo2 {
void mount_native_tu15_view_storage_data(SourceRuntimeMemory& memory) {
    memory.mount(0x4f9b54,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"source1888A0 local-view storage binding");
    memory.mount(0x52be64,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"source1EECF0 action-cache storage binding");
    memory.mount(0x4f63b8,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"source159F80 storage binding");
    memory.mount(0x50ed30,std::array<std::byte,0x9c>{},SourceRuntimeMemory::Access::ReadWrite,"source159F80 static constructor destination");
}
}
