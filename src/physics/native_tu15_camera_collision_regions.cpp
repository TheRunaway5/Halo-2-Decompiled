#include "halo2/native_tu15_camera_collision_regions.hpp"
#include <bit>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t n{};for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(8*k);return n;}
std::uint32_t sign_byte(std::uint32_t n){return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(n))));}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,std::uint32_t pc,std::uint32_t p,std::uint32_t n,unsigned width=4){std::array<std::byte,4>b{};for(unsigned k=0;k<width;++k)b[k]=std::byte(n>>(8*k));SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o){e.before.resize(width);m.read(p,e.before);e.written.assign(b.begin(),b.begin()+width);}m.write(p,std::span(b).first(width));if(o)o(e);}
}
void native_tu15_begin_camera_collision_regions(SourcePhysicsContactMemory&m,std::uint32_t collision,
    std::uint32_t state,std::uint32_t iterator,std::uint32_t model,const SourcePhysicsContactRecordObserver&o){
    store(m,o,0x1f7e9bu,iterator,collision);store(m,o,0x1f7e9du,iterator+20u,0xffffffffu);
    if(!state){for(unsigned n=0;n<4;++n){constexpr std::array pc{0x1f7eabu,0x1f7eadu,0x1f7eb0u,0x1f7eb3u};store(m,o,pc[n],iterator+4u+n*4u,0);}return;}
    if(!model){for(unsigned n=0;n<4;++n){constexpr std::array pc{0x1f7ec3u,0x1f7ec8u,0x1f7eceu,0x1f7ed4u};store(m,o,pc[n],iterator+4u+n*4u,word(m,state+n*4u));}return;}
    const auto original_count=word(m,model+0x70u);const auto count=std::int32_t(original_count)>16?16u:original_count;
    for(unsigned n=0;n<4;++n){constexpr std::array pc{0x1f7ef5u,0x1f7ef7u,0x1f7efau,0x1f7efdu};store(m,o,pc[n],iterator+4u+n*4u,0xffffffffu);}
    for(std::uint32_t index=0;std::int32_t(index)<std::int32_t(count);++index){
        const auto mappings=word(m,model+0x74u),row=mappings+index*16u;
        const auto slot=word(m,row+4u,1),permutation=sign_byte(word(m,state+index,1));
        if(slot==0xffu||std::int32_t(permutation)<0)continue;
        if(std::int32_t(permutation)>=std::int32_t(word(m,row+8u)))continue;
        const auto permutations=word(m,row+12u),selected=word(m,permutations+permutation*8u+5u,1);
        if(selected!=0xffu)store(m,o,0x1f7f3fu,iterator+4u+sign_byte(slot),selected,1);
    }
}
bool native_tu15_next_camera_collision_region(SourcePhysicsContactMemory&m,std::uint32_t iterator,
    const SourcePhysicsContactRecordObserver&o){
    const auto encoded=word(m,iterator+20u);std::uint32_t region=0xffffffffu,permutation{},bsp{},record{};
    bool ready{};
    if(encoded!=0xffffffffu){
        region=word(m,iterator+23u,1);permutation=word(m,iterator+22u,1);
        const auto collision=word(m,iterator),regions=word(m,collision+32u),old_bsp=word(m,iterator+21u,1),permutations=word(m,regions+region*12u+8u);
        record=permutations+permutation*20u;
        if(record){const auto count=word(m,record+4u);bsp=old_bsp+1u;if(std::int32_t(bsp)<std::int32_t(count))ready=true;}
    }
    if(!ready){
        const auto collision=word(m,iterator),count=word(m,collision+28u);
        for(;;){
            region=region==0xffffffffu?0u:region+1u;
            if(std::int32_t(region)>=std::int32_t(count))break;
            const auto regions=word(m,collision+32u),row=regions+region*12u;
            permutation=sign_byte(word(m,iterator+region+4u,1));if(std::int32_t(permutation)<0)continue;
            if(std::int32_t(permutation)>=std::int32_t(word(m,row+4u)))continue;
            const auto permutations=word(m,row+8u);record=permutations+permutation*20u;
            const auto bsps=word(m,record+4u);if(std::int32_t(bsps)<=0)continue;bsp=0;ready=true;break;
        }
    }
    if(!ready||region==0xffffffffu){store(m,o,0x1f7fdcu,iterator+24u,0);const auto current=word(m,iterator+24u);store(m,o,0x1f7fecu,iterator+20u,0xffffffffu);return current!=0;}
    const auto bsps=word(m,record+8u),row=bsps+bsp*0x44u,primitive=word(m,row,1);
    auto key=(permutation&0xffu)|(region<<8u);key=(key<<8u)|(bsp&0xffu);key=(key<<8u)|(primitive&0xffu);
    store(m,o,0x1f802au,iterator+20u,key);const auto descriptor=row+4u;store(m,o,0x1f8034u,iterator+24u,descriptor);return descriptor!=0;
}
} // namespace halo2
