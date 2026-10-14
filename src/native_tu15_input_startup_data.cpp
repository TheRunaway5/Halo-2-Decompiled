#include "halo2/source_input_startup_tu15.hpp"
#include <array>
namespace halo2 {
void mount_native_tu15_input_startup_data(SourceRuntimeMemory& m) {
    // Constructor destinations, not preconstructed bindings or input samples.
    m.mount(0x4f2970,std::vector<std::byte>(0x2c8),SourceRuntimeMemory::Access::ReadWrite,"source controller feedback, bindings and input constructor destinations");
    m.mount(0x52d4a8,std::vector<std::byte>(0x1bc),SourceRuntimeMemory::Access::ReadWrite,"source input preset and presence constructor destinations");
    constexpr std::array<std::byte,16> maps{std::byte{0},std::byte{1},std::byte{2},std::byte{3},std::byte{4},std::byte{5},std::byte{6},std::byte{7},std::byte{1},std::byte{2},std::byte{4},std::byte{8},std::byte{16},std::byte{32},std::byte{64},std::byte{128}};
    m.mount(0x44ad30,maps,SourceRuntimeMemory::Access::ReadOnly,"authored analog index and digital mask tables");
    constexpr std::array<std::byte,8> constants{std::byte{0},std::byte{0},std::byte{0xf0},std::byte{0x42},std::byte{0},std::byte{0},std::byte{0x70},std::byte{0x42}};
    m.mount(0x455a90,constants,SourceRuntimeMemory::Access::ReadOnly,"authored input preset floating point constants");
}
}
