#include "halo2/native_tu15_camera_packet_data.hpp"
#include <array>
#include <stdexcept>
namespace halo2 {
void mount_native_tu15_camera_packet_data(SourceRuntimeMemory& memory) {
    for(const auto& region:memory.checkpoint().regions)
        for(const auto address:{0x47da8cu,0x56d324u,0x56d328u,0x46a6f4u,0x46a6f8u,0x46a75cu})
            if(region.address<std::uint64_t(address)+4u&&
                std::uint64_t(region.address)+region.bytes.size()>address)
                throw std::invalid_argument("Camera packet data conflicts with its cold owner");
    // Actual2454F7 multiplies the current unit's authored horizontal angle
    // by this writable initialized .data scalar. Preserve its original1.0.
    constexpr std::array<std::byte,4> one{std::byte{},std::byte{},std::byte{0x80},std::byte{0x3f}};
    memory.mount(0x47da8cu,one,SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 initialized camera packet horizontal-angle multiplier");
    // Actual CRT rows46DC90/94 load their existing readonly46A164/46A5F8
    // operands before storing these two independent loader-zero destinations.
    memory.mount(0x56d324u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold382360 camera packet lower-angle limit");
    memory.mount(0x56d328u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 cold382380 camera packet upper-angle limit");
    // Actual246CF0 resets an absent-unit observer before the player's first
    // activation tick. Its246DE5/246DF0 pitch operands are the same authored
    // words already retained by native_tu15_operands.generated.cpp. Verified
    // offline against TU1.5 image21b207e7; the runtime opens no executable.
    memory.mount(0x46a6f4u,std::array{std::byte{0x3b},std::byte{0x46},std::byte{0xf1},std::byte{0x3e}},
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 absent-unit camera pitch bias");
    memory.mount(0x46a6f8u,std::array{std::byte{0x7a},std::byte{0xd9},std::byte{0x20},std::byte{0x3f}},
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 absent-unit camera pitch scale");
    //247194 supplies the same authenticated attached_seconds operand to the
    // absent-unit director's following transition; this is not a host delay.
    memory.mount(0x46a75cu,std::array{std::byte{0xc3},std::byte{0xf5},std::byte{0xa8},std::byte{0x3e}},
        SourceRuntimeMemory::Access::ReadOnly,"native TU1.5 absent-unit camera transition operand");
}
}
