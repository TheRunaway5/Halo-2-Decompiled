#include "halo2/source_bink_memory_startup_tu15.hpp"
namespace halo2 {
void mount_native_tu15_bink_memory_startup_data(SourceRuntimeMemory& m) {
    m.mount(0x4f5a58,std::vector<std::byte>(0xe0),SourceRuntimeMemory::Access::ReadWrite,"source Bink memory startup constructor destination");
    // These are source callback identities, never narrowed native pointers.
    // Actual377460 publishes them from its live caller's two arguments.
    m.mount(0x49130c,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,"source Bink allocator and release callback identity bindings");
}
}
