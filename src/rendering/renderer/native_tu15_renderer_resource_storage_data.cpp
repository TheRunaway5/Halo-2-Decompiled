#include "halo2/source_renderer_resource_storage_tu15.hpp"
namespace halo2 {
void mount_native_tu15_renderer_resource_storage_data(SourceRuntimeMemory& memory) {
    memory.mount(0x494330,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,"source1D640 resource-storage initialized flag");
    memory.mount(0x494338,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"source1D640 first resource-storage counter");
    memory.mount(0x49433c,std::array<std::byte,0x27000>{},SourceRuntimeMemory::Access::ReadWrite,"source1D640 first resource-storage bank");
    for(auto p:std::array<std::uint32_t,5>{0x4bb33c,0x4bb344,0x4bb348,0x4bb34c,0x4bb370})
        memory.mount(p,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"source1D640 resource-storage scalar");
    memory.mount(0x4bb340,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,"source1D640 secondary resource-storage flag");
    memory.mount(0x4bb374,std::array<std::byte,0x6000>{},SourceRuntimeMemory::Access::ReadWrite,"source1D640 second resource-storage bank");
}
}
