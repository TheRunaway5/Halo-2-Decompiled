#include "halo2/source_level_handles_startup_tu15.hpp"
namespace halo2 {
void mount_native_tu15_level_handles_startup_data(SourceRuntimeMemory& m) {
    m.mount(0x4fadb8,std::vector<std::byte>(0x208),SourceRuntimeMemory::Access::ReadWrite,"source 64 level-handle pairs and two pool constructor bindings");
    constexpr std::array<char,16> level{"level handles"};
    constexpr std::array<char,28> multiplayer{"multiplayer level handles"};
    m.mount(0x4607ec,std::as_bytes(std::span(level)),SourceRuntimeMemory::Access::ReadOnly,"authored level-handle pool name and alignment");
    m.mount(0x4607d0,std::as_bytes(std::span(multiplayer)),SourceRuntimeMemory::Access::ReadOnly,"authored multiplayer level-handle pool name and alignment");
}
}
