#include "halo2/native_tu15_player_startup_data.hpp"
namespace halo2 {
void mount_native_tu15_player_startup_data(SourceRuntimeMemory& m) {
    constexpr std::array<std::byte,4> allocator{std::byte{0x10},std::byte{0x72},std::byte{0x12},std::byte{0}};
    m.mount(0x45f1d8,allocator,SourceRuntimeMemory::Access::ReadOnly,"source arena allocator method identity consumed by14DFE0");
    m.mount(0x4f54f0,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,"source player state and datum-pool constructor bindings");
    // Original151BF0 compares151C1A/stores151C82 into this one DWORD.
    // It is loader-zeroed .data beyond the authenticated raw file extent;
    // the original fallback spawn owner supplies each subsequent timestamp.
    m.mount(0x56e208u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "source151BF0 fallback spawn timestamp,original loader-zeroed DWORD");
    constexpr std::array<char,8> name{"players"};
    m.mount(0x45f6c4,std::as_bytes(std::span(name)),SourceRuntimeMemory::Access::ReadOnly,"authored player datum-pool name");
}
}
