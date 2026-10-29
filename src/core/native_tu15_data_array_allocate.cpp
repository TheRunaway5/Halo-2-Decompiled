#include "halo2/native_tu15_data_array_allocate.hpp"
#include <array>
#include <bit>
#include <optional>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t at,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(at,std::span(bytes).first(width));
    std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void write(SourcePhysicsContactMemory &memory,std::uint32_t at,std::uint32_t value,unsigned width=4) {
    std::array<std::byte,4> bytes{};
    for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
    memory.write(at,std::span(bytes).first(width));
}
std::int32_t signed32(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
std::uint32_t occupancy_at(std::uint32_t bitmap,std::uint32_t index) {
    return bitmap+std::uint32_t(signed32(index)>>5u)*4u;
}
}
std::uint32_t native_tu15_data_array_allocate(SourcePhysicsContactMemory &memory,std::uint32_t pool) {
    auto cursor=read(memory,pool+0x34u);
    const auto extent=read(memory,pool+0x38u);
    std::optional<std::uint32_t> available;
    if(signed32(cursor)<signed32(extent)) {
        const auto bitmap=read(memory,pool+0x48u);
        while(true) {
            if(!(read(memory,occupancy_at(bitmap,cursor))&(1u<<(cursor&31u)))) {
                if(cursor!=0xffffffffu)available=cursor;
                break;
            }
            const auto current_extent=read(memory,pool+0x38u);
            ++cursor;
            if(signed32(cursor)>=signed32(current_extent))break;
        }
    }
    if(!available) {
        if(signed32(extent)>=signed32(read(memory,pool+0x20u)) || extent==0xffffffffu)
            return 0xffffffffu;
        available=extent;
    }
    const auto index=*available;
    const auto stride=read(memory,pool+0x24u);
    const auto rows=read(memory,pool+0x44u);
    const auto record=rows+index*stride;
    const auto bitmap=read(memory,pool+0x48u);
    const auto occupied=occupancy_at(bitmap,index);
    write(memory,occupied,read(memory,occupied)|(1u<<(index&31u)));
    const auto used=read(memory,pool+0x3cu);
    const auto current_extent=read(memory,pool+0x38u);
    write(memory,pool+0x3cu,used+1u);
    write(memory,pool+0x34u,index+1u);
    if(signed32(current_extent)<=signed32(index))write(memory,pool+0x38u,index+1u);
    // Occupancy/header stores can alias the stride or record. Preserve both
    // the earlier record address and this fresh clear-length lookup.
    const auto clear_bytes=read(memory,pool+0x24u);
    for(std::uint32_t n=0;n<(clear_bytes>>2u);++n)write(memory,record+n*4u,0);
    for(std::uint32_t n=0;n<(clear_bytes&3u);++n)write(memory,record+(clear_bytes&~3u)+n,0,1);
    write(memory,record,read(memory,pool+0x40u,2),2);
    write(memory,pool+0x40u,std::uint16_t(read(memory,pool+0x40u,2)+1u),2);
    if(read(memory,pool+0x40u,2)==0xffffu)write(memory,pool+0x40u,0x8000u,2);
    return (read(memory,record,2)<<16u)|index;
}
}
