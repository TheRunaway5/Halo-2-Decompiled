#include "halo2/source_network_session_startup_tu15.hpp"
#include <array>
#include <stdexcept>
namespace halo2 {
void mount_native_tu15_network_session_presence_storage(SourceRuntimeMemory& memory) {
    constexpr std::uint32_t address=0x4dc3e8u;
    constexpr const char* owner="network session cold initialized flag";
    std::array<std::byte,1> actual{};
    try { memory.read(address,actual); }
    catch(const std::out_of_range&) {
        // Same one-byte cold allocation as the existing canonical data owner.
        // This does not execute or report success for a network constructor.
        memory.mount(address,std::array<std::byte,1>{},
            SourceRuntimeMemory::Access::ReadWrite,owner);
        return;
    }
    // Preserve every actual live byte, including the real constructor's1.
    // A containing/foreign/readonly/reserved owner is not interchangeable.
    SourceRuntimeMemory::CheckpointRegion expected{address,
        SourceRuntimeMemory::Access::ReadWrite,owner,0,
        SourceRuntimeSnapshotBytes(std::span<const std::byte>(actual)),{}};
    if(!memory.matches_checkpoint_region(expected))
        throw std::invalid_argument("Network session presence storage conflicts with its canonical owner");
    memory.validate_mutable_view(address,1);
}
}
