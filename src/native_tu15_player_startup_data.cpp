#include "halo2/native_tu15_player_startup_data.hpp"
namespace halo2 {
void mount_native_tu15_player_startup_data(SourceRuntimeMemory& m) {
    constexpr std::array<std::byte,4> allocator{std::byte{0x10},std::byte{0x72},std::byte{0x12},std::byte{0}};
    m.mount(0x45f1d8,allocator,SourceRuntimeMemory::Access::ReadOnly,"source arena allocator method identity consumed by14DFE0");
    m.mount(0x4f54f0,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,"source player state and datum-pool constructor bindings");
    constexpr std::array<char,8> name{"players"};
    m.mount(0x45f6c4,std::as_bytes(std::span(name)),SourceRuntimeMemory::Access::ReadOnly,"authored player datum-pool name");
}
}
