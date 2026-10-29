#include "halo2/native_campaign_ai_vision_records_tu15.hpp"
#include <array>
#include <bit>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t value{};
    for(unsigned k=0;k<n;++k)value|=std::uint32_t(std::to_integer<unsigned>(b[k]))<<(k*8u);
    return value;
}
void write(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t value,unsigned n=4){
    std::array<std::byte,4>b{};for(unsigned k=0;k<n;++k)b[k]=std::byte(value>>(k*8u));m.write(p,std::span(b).first(n));
}
std::int32_t signed16(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
}
NativeCampaignAiVisionAttachmentTu15 native_campaign_ai_vision_attachment_index_tu15(
    const SourcePhysicsContactMemory&m,std::uint32_t object){
    const auto pool=read(m,0x4ecbd0u),rows=read(m,pool+0x44u);
    const auto body=read(m,rows+(object&0xffffu)*12u+8u);
    if(!(read(m,body+0x10au,1)&4u))return {false,0xffffu};
    // Logical division after sign extension is the actual authored count rule.
    const auto count=std::uint32_t(signed16(read(m,body+0x120u,2)))>>3u;
    const auto records=body+std::uint32_t(signed16(read(m,body+0x122u,2)));
    for(std::uint16_t ordinal=0;std::int32_t(signed16(ordinal))<std::int32_t(count);++ordinal){
        const auto at=records+std::uint32_t(signed16(ordinal))*8u+4u;
        if(read(m,at,2)&0xfff8u){
            const auto packed=read(m,at,2);
            const auto index=std::uint16_t(packed>>3u);
            if(index<0x7fffu)return {true,index};
            break;
        }
    }
    return {false,0xffffu};
}
bool native_campaign_ai_vision_attachment_index_store_tu15(
    SourcePhysicsContactMemory&m,std::uint32_t object,std::uint32_t output){
    const auto result=native_campaign_ai_vision_attachment_index_tu15(m,object);
    if(output)write(m,output,result.present?result.index:0xffffu,2);
    return result.present;
}
std::uint32_t native_campaign_ai_vision_character_block_tu15(
    const SourcePhysicsContactMemory&m,std::uint32_t character){
    if(character==0xffffffffu)return 0;
    const auto tags=read(m,0x4f0414u);
    do{
        const auto data=read(m,tags+(character&0xffffu)*16u+8u);
        if(std::bit_cast<std::int32_t>(read(m,data+0x3cu))>0)return read(m,data+0x40u);
        character=read(m,data+8u);
    }while(character!=0xffffffffu);
    return 0;
}
std::int16_t native_campaign_ai_vision_player_slot_tu15(
    const SourcePhysicsContactMemory&m,std::uint32_t player){
    const auto rows=read(m,0x5020a4u);
    for(std::int16_t slot=0;slot<2;++slot)if(read(m,rows+std::uint32_t(slot)*0x1cu)==player)return slot;
    return -1;
}
void native_campaign_ai_vision_clump_timestamp_tu15(
    SourcePhysicsContactMemory&m,std::uint32_t clump){
    const auto pool=read(m,0x50eef8u),rows=read(m,pool+0x44u);
    const auto clock=read(m,0x51df54u),tick=read(m,clock+8u);
    write(m,rows+(clump&0xffffu)*80u+0x38u,tick);
}
}
