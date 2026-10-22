#include "halo2/native_tu15_sound_ac97_output_data.hpp"
#include <array>
#include <algorithm>
#include <stdexcept>
namespace halo2 {namespace {
void literal(SourceRuntimeMemory& memory,std::uint32_t address,std::span<const std::byte> expected){
    bool present=false;
    memory.visit_committed([&](auto p,auto bytes){if(std::uint64_t(p)<std::uint64_t(address)+expected.size()&&std::uint64_t(p)+bytes.size()>address)present=true;});
    if(present){
        if(!memory.is_read_only(address,expected.size())||!std::ranges::equal(memory.view(address,expected.size()),expected))
            throw std::invalid_argument("Existing native AC97 immutable operand has different bytes/access/extent");
        return;
    }
    memory.mount(address,expected,SourceRuntimeMemory::Access::ReadOnly,"exact TU1.5 AC97 channel/refcount immutable operand");
}
}
void mount_native_tu15_sound_ac97_output_data(SourceRuntimeMemory& memory){
    constexpr std::array offsets{std::byte{0x10},std::byte{},std::byte{},std::byte{},std::byte{0x70},std::byte{},std::byte{},std::byte{}};
    constexpr std::array retain{std::byte{0xec},std::byte{0x3d},std::byte{0x38},std::byte{}};
    literal(memory,0x390b94,offsets);literal(memory,0x4212f8,retain);
}
}
