#include "halo2/native_tu15_data_array_delete.hpp"
#include <array>
#include <bit>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t p,unsigned n=4) {
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};
    for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8u);
    return v;
}
void write(SourcePhysicsContactMemory &m,std::uint32_t p,std::uint32_t v,unsigned n=4) {
    std::array<std::byte,4>b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(i*8u));
    m.write(p,std::span(b).first(n));
}
std::int32_t signed32(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
}
void native_tu15_data_array_delete(SourcePhysicsContactMemory &m,
        std::uint32_t pool,std::uint32_t datum) {
    const auto rows=read(m,pool+0x44u),stride=read(m,pool+0x24u),index=datum&0xffffu;
    auto record=rows+index*stride;
    if(read(m,pool+0x2au,1)&8u) {
        for(std::uint32_t i=0;i<(stride>>2u);++i)write(m,record+i*4u,0xbabababau);
        for(std::uint32_t i=0;i<(stride&3u);++i)write(m,record+(stride&~3u)+i,0xbau,1);
    }
    const auto occupancy=read(m,pool+0x48u)+(index>>5u)*4u;
    write(m,occupancy,read(m,occupancy)&~(1u<<(index&31u)));
    write(m,record,0,2);
    if(signed32(index)<signed32(read(m,pool+0x34u)))write(m,pool+0x34u,index);
    if(index+1u==read(m,pool+0x38u)) {
        const auto current_stride=read(m,pool+0x24u);
        for(;;) {
            const auto extent=read(m,pool+0x38u)-1u;
            record-=current_stride;write(m,pool+0x38u,extent);
            if(signed32(extent)<=0||read(m,record,2)!=0)break;
        }
    }
    write(m,pool+0x3cu,read(m,pool+0x3cu)-1u);
}
}
