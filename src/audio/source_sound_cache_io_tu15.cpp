#include "halo2/source_sound_cache_io_tu15.hpp"
#include <limits>
#include <stdexcept>

namespace halo2 {namespace {
std::uint32_t add(std::uint32_t base,std::uint32_t offset) {
    if(std::uint64_t(base)+offset>std::numeric_limits<std::uint32_t>::max())
        throw std::out_of_range("TU1.5 sound-cache operand wraps");
    return base+offset;
}
std::uint32_t word(const SourcePhysicsContactMemory &memory,std::uint32_t at,unsigned count=4) {
    std::array<std::byte,4> bytes{};memory.read(at,std::span(bytes).first(count));
    std::uint32_t result{};for(unsigned i=0;i<count;++i)result|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return result;
}
std::uint32_t row(const SourcePhysicsContactMemory &memory,std::uint32_t pool,std::uint32_t handle,std::uint32_t stride) {
    const auto index=handle&65535u;
    if(!pool || handle==0xffffffffu || word(memory,add(pool,0x24))!=stride ||
        index>=word(memory,add(pool,0x38)) || index>=word(memory,add(pool,0x20)))
        throw std::invalid_argument("TU1.5 sound-cache datum extent differs");
    const auto bits=word(memory,add(pool,0x48));
    if(!(word(memory,add(bits,(index>>5)*4))&(1u<<(index&31))))
        throw std::invalid_argument("TU1.5 sound-cache datum is not occupied");
    const auto address=add(word(memory,add(pool,0x44)),index*stride);
    if(word(memory,address,2)!=(handle>>16))
        throw std::invalid_argument("TU1.5 sound-cache datum salt differs");
    return address;
}
}
bool source_try_sound_cache_io_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &abi,
    const SourceSoundCacheIoTu15Services &services) {
    if(call.instruction!=0x21f4bdu)return false;
    if(call.target!=0x1a3b70u || call.tail)
        throw std::invalid_argument("TU1.5 sound-cache read target differs");
    if(!services.read || !services.retain_completed_read)
        throw std::runtime_error("TU1.5 sound-cache read/receipt owner unavailable");
    const auto sp=abi.gp[4],chunk=abi.gp[3],handle=abi.gp[5];
    const auto returned_sp=add(sp,28u);
    const auto entry=row(memory,word(memory,0x50ebdcu),handle,0x10u);
    const auto allocator=word(memory,0x50ebe4u);
    if(!allocator)throw std::invalid_argument("TU1.5 sound-cache allocator unavailable");
    const auto page=row(memory,word(memory,add(allocator,0x64u)),handle,0x18u);
    const auto shift=word(memory,add(allocator,0x34u))&31u;
    const auto destination=word(memory,0x50ebe0u)+(word(memory,add(page,8u))<<shift);
    const auto raw_size=word(memory,add(chunk,4u))&0x3fffffffu;
    const auto size=(raw_size+511u)&~511u;
    const auto done=add(entry,2u),encoded=word(memory,chunk);
    if(word(memory,sp)!=0x21f4c2u || word(memory,add(sp,4))!=size ||
        word(memory,add(sp,8))!=destination || word(memory,add(sp,12))!=0 ||
        word(memory,add(sp,16))!=done || word(memory,add(sp,20))!=5u || word(memory,add(sp,24))!=4u ||
        abi.gp[1]!=encoded || abi.gp[6]!=done || abi.gp[7]!=destination ||
        word(memory,add(chunk,8))!=handle || word(memory,add(entry,12))!=chunk ||
        word(memory,add(entry,8))==0xffffffffu || word(memory,done,1)!=0 ||
        !size || std::uint64_t(destination)+size>0x100000000ull)
        throw std::invalid_argument("TU1.5 sound-cache read differs from its actual allocated caller");
    const auto bytes=services.read(encoded,size);
    if(bytes.size()!=size)throw std::runtime_error("TU1.5 sound-cache native read is incomplete");
    memory.write(destination,bytes);
    const auto receipt=services.retain_completed_read(chunk,destination,size);
    const std::array completed{std::byte{1}};memory.write(done,completed);
    abi.gp[0]=receipt;abi.gp[4]=returned_sp;return true;
}
}
