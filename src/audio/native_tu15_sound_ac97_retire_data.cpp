#include "halo2/native_tu15_sound_ac97_retire_data.hpp"
#include <array>
#include <algorithm>
#include <stdexcept>
namespace halo2 {
void mount_native_tu15_sound_ac97_retire_data(SourceRuntimeMemory& memory){
    constexpr std::array raw{std::byte{0x0a},std::byte{0x3e},std::byte{0x38},std::byte{}};
    bool present=false;memory.visit_committed([&](auto p,auto bytes){if(std::uint64_t(p)<0x421300&&std::uint64_t(p)+bytes.size()>0x4212fc)present=true;});
    if(present){if(!memory.is_read_only(0x4212fc,4)||!std::ranges::equal(memory.view(0x4212fc,4),raw))throw std::invalid_argument("Native AC97 shared-pool release vtable operand differs");return;}
    memory.mount(0x4212fc,raw,SourceRuntimeMemory::Access::ReadOnly,"exact TU1.5 shared-pool release method operand");
}
}
