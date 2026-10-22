#include "halo2/source_sound_configuration_tu15.hpp"
#include <array>

namespace halo2 {
void mount_native_tu15_sound_configuration_data(SourceRuntimeMemory& memory) {
    constexpr std::array<std::byte,4> loader_zero{};
    for(const auto address:{0x391460u,0x38f848u,0x390bdcu})
        memory.mount(address,loader_zero,SourceRuntimeMemory::Access::ReadWrite,
            "authenticated cold source sound configuration destination");
}
}
