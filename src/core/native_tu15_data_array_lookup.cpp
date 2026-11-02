#include "halo2/native_tu15_data_array_lookup.hpp"
#include <array>
#include <bit>
namespace halo2 { namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
}
std::uint32_t native_tu15_data_array_try_lookup(const SourcePhysicsContactMemory &memory,
    std::uint32_t pool,std::uint32_t datum) {
    if(datum==0xffffffffu)return 0;
    const auto count=std::bit_cast<std::int32_t>(read(memory,pool+0x38u));
    const auto index=datum&0xffffu;
    if(std::int32_t(index)>=count)return 0;
    const auto stride=read(memory,pool+0x24u),rows=read(memory,pool+0x44u);
    const auto record=rows+stride*index;
    const auto salt=std::uint16_t(read(memory,record,2));
    if(salt==0 || salt!=std::uint16_t(datum>>16u))return 0;
    return record;
}
}
