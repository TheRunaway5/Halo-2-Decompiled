#include "halo2/native_tu15_sound_codec_initialization_data.hpp"
#include <array>
#include <algorithm>
#include <stdexcept>
namespace halo2 { namespace {
template<std::size_t N>void literal(SourceRuntimeMemory& memory,std::uint32_t address,const std::array<std::uint8_t,N>& expected){
    std::size_t covered{};memory.visit_committed([&](auto first,auto bytes){const auto begin=std::max<std::uint64_t>(first,address),end=std::min<std::uint64_t>(std::uint64_t(first)+bytes.size(),std::uint64_t(address)+N);if(begin<end)covered+=end-begin;});
    if(covered){if(covered!=N)throw std::invalid_argument("Partially owned codec immutable operand");for(unsigned i=0;i<N;++i)if(!memory.is_read_only(address+i,1)||memory.view(address+i,1)[0]!=std::byte(expected[i]))throw std::invalid_argument("Different codec immutable operand bytes or access");return;}
    memory.mount(address,std::as_bytes(std::span(expected)),SourceRuntimeMemory::Access::ReadOnly,"original TU1.5 codec constructor immutable operand");
}
}
void mount_native_tu15_sound_codec_initialization_data(SourceRuntimeMemory& memory){
    literal(memory,0x390b94,std::array<std::uint8_t,8>{0x10,0,0,0,0x70,0,0,0});
    literal(memory,0x41b70c,std::array<std::uint8_t,4>{0x6b,0,0,0x80});
    literal(memory,0x41b7a8,std::array<std::uint8_t,4>{0x2c,0,0,0x80});
    literal(memory,0x41b7b4,std::array<std::uint8_t,4>{0x6d,0,0,0x80});
    literal(memory,0x41b7b0,std::array<std::uint8_t,4>{0x62,0,0,0x80});
    literal(memory,0x41b670,std::array<std::uint8_t,4>{0x2f,0,0,0x80});
}
}
