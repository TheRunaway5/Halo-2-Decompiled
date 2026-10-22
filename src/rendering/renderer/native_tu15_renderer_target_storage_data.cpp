#include "halo2/source_renderer_target_storage_tu15.hpp"
#include <array>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    const auto bytes=memory.view(address,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return value;
}
std::uint32_t format_word(std::uint32_t w,std::uint32_t h,std::uint32_t levels,bool linear,bool depth) {
    // Original1D6D0 builds the header format from these exact inputs. Its
    // swizzled dimensions are the smallest powers of two at least as large.
    const auto format=linear?(depth?0x2eu:0x12u):(depth?0x2eu:6u);
    auto value=(levels<<16)|(format<<8)|0x29u;
    if(!linear) {
        unsigned u=1,v=1;
        while((1u<<u)<w)++u;
        while((1u<<v)<h)++v;
        value|=(u|(v<<4))<<20;
    }
    return value;
}
}
void mount_native_tu15_renderer_target_storage_data(SourceRuntimeMemory& memory) {
    memory.mount(0x4c1378,std::array<std::byte,28*0x98>{},SourceRuntimeMemory::Access::ReadWrite,
        "source1D750 loader-zeroed target definition storage");
    memory.mount(0x56e190,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "source1DF00 missing target storage observation counter");
}
std::vector<NativeTu15TargetStorageDefinition> native_tu15_renderer_target_definitions(const SourceRuntimeMemory& memory) {
    std::vector<NativeTu15TargetStorageDefinition> result;
    constexpr std::array<unsigned,18> expected{1,5,6,7,8,9,10,11,12,13,15,19,21,22,23,25,26,27};
    for(unsigned i=0;i<28;++i) {
        const auto base=0x4c1378u+i*0x98u,raw=word(memory,base+0x88);
        if(!raw)continue;
        const auto w=word(memory,base+0x80),h=word(memory,base+0x84),levels=word(memory,base+0x78);
        const auto format=word(memory,base+0x10),size=word(memory,base+0x14),bytes=word(memory,base+0x7c);
        if(!w||!h||w>16384||h>16384||!levels||levels>4||!bytes||
           std::to_integer<unsigned>(memory.view(base+0x95,1)[0])||
           (word(memory,base+4)&0x70000)!=0x40000||(raw&0xfffffff)!=word(memory,base+8))
            throw std::runtime_error("Constructed renderer target has an unsupported/retired definition");
        const bool linear=memory.view(base+0x94,1)[0]!=std::byte{};
        const bool depth=((format>>8)&255u)==0x2eu;
        const auto expected_size=linear?((w-1)|((h-1)<<12)|((w*4/64-1)<<24)):0u;
        if(format!=format_word(w,h,levels,linear,depth)||size!=expected_size||
           word(memory,base+4)!=0x40001u+levels)
            throw std::runtime_error("Constructed renderer target header disagrees with its dimensions/format");
        NativeTu15TargetStorageDefinition value{i,w,h,format,size,linear?((size>>24)+1)*64:0,linear,
            memory.native_address(raw,bytes),bytes,{}};
        const auto allocation_bytes=((std::uint64_t(w)+63)&~std::uint64_t(63))*h*4;
        std::uint64_t offset=0;
        for(unsigned level=0;level<levels;++level) {
            const auto surface=base+0x18+level*24;
            const auto part=allocation_bytes>>(level*2);
            if(offset+part>bytes||word(memory,surface+20)!=base+4||
               word(memory,surface)!=0x50001||
               word(memory,surface+12)!=format_word(w>>level,h>>level,1,linear,depth)||
               word(memory,surface+16)!=size||
               word(memory,surface+4)!=(raw&0xfffffff)+offset)
                throw std::runtime_error("Constructed renderer target mip loses its parent/range");
            const auto pixels=memory.native_address(raw+std::uint32_t(offset),std::size_t(part));
            value.surfaces.push_back({w>>level,h>>level,word(memory,surface+12),word(memory,surface+16),pixels,part});
            offset+=part;
        }
        if(offset!=bytes)throw std::runtime_error("Constructed renderer target storage footprint differs");
        result.push_back(std::move(value));
    }
    if(result.size()!=expected.size())throw std::runtime_error("Renderer target construction lacks owned storage for all18 definitions");
    for(unsigned i=0;i<expected.size();++i)if(result[i].index!=expected[i])
        throw std::runtime_error("Renderer target construction published another definition set");
    return result;
}
}
