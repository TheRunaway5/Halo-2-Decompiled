#include "halo2/source_geometry_resource.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cstring>

namespace halo2 {
namespace {
template<class T>T read(const SourceGeometryResourceMemory& m,std::uint32_t at) {
    if(!m.read)throw FormatError("source geometry publication reader absent");
    const auto bytes=m.read(at,sizeof(T));
    if(bytes.size()!=sizeof(T))throw FormatError("source geometry publication input truncated");
    T value;std::memcpy(&value,bytes.data(),sizeof(T));return value;
}
template<class T>void put(const SourceGeometryResourceMemory& m,std::uint32_t at,T value) {
    if(!m.write)throw FormatError("source geometry publication writer absent");
    auto bytes=m.write(at,sizeof(T));
    if(bytes.size()!=sizeof(T))throw FormatError("source geometry publication output truncated");
    std::memcpy(bytes.data(),&value,sizeof(T));
}
std::int32_t divide(std::uint32_t numerator,std::uint32_t denominator) {
    if(!denominator)throw FormatError("source geometry publication zero divisor");
    // Original CDQ/IDIV. Both divisors are zero-extended8/16-bit fields.
    return std::bit_cast<std::int32_t>(numerator)/std::int32_t(denominator);
}
}
void source_geometry_resource_publish(std::uint32_t header,std::uint32_t allocation,
    const SourceGeometryResourceMemory& m) {
    const auto root=header+std::uint32_t(std::int32_t(read<std::int16_t>(m,header+0x1c)));
    const auto data=allocation+8;
    put(m,root,1u);put(m,root+4,data);
    for(std::uint32_t index=0;std::bit_cast<std::int32_t>(index)<read<std::int32_t>(m,header+0x10);++index) {
        // The source reloads the descriptor pointer/count each iteration;
        // aliases to published headers therefore observe prior stores.
        const auto descriptor=read<std::uint32_t>(m,header+0x14)+index*16u;
        const auto offset=read<std::uint32_t>(m,descriptor+0xc);
        if(offset==0xffffffffu)continue;
        const auto payload=read<std::uint32_t>(m,header+8)+offset+allocation+8;
        const auto kind=read<std::uint8_t>(m,descriptor);
        const auto locator=read<std::uint16_t>(m,descriptor+4);
        if(kind<=1) {
            const auto size=read<std::uint32_t>(m,descriptor+8);
            const auto count=kind==0?std::uint32_t(divide(size,read<std::uint16_t>(m,descriptor+6))):size;
            put(m,data+locator,count);put(m,data+locator+4,payload);
            continue;
        }
        const auto selector=read<std::uint16_t>(m,descriptor+6);
        const auto buffer=selector==65535?data+locator:
            read<std::uint32_t>(m,data+locator+4)+std::uint32_t(selector)*32u;
        const auto numerator=read<std::uint32_t>(m,descriptor+8)-read<std::uint32_t>(m,buffer+8);
        const auto count=divide(numerator,read<std::uint8_t>(m,buffer+1));
        put(m,buffer+0xc,payload-read<std::uint32_t>(m,buffer+4));
        put(m,buffer+2,std::uint16_t(count));
        put(m,buffer+0x10,buffer+0x14);put(m,buffer+0x14,1u);
        const auto relative=payload-read<std::uint32_t>(m,buffer+4);
        put(m,buffer+0x18,relative);put(m,buffer+0x1c,0u);
        put(m,buffer+0x18,relative&0x0fffffffu);
    }
}
}
