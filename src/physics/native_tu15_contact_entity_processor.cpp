#include "halo2/native_tu15_contact_entity_processor.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t n{};
    for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(8u*k);return n;
}
bool positive(std::uint32_t n){return std::bit_cast<std::int32_t>(n)>0;}
std::uint32_t signed_byte(const SourcePhysicsContactMemory&m,std::uint32_t p){
    return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(word(m,p,1)))));
}
}
void native_tu15_process_entity_contacts(const SourcePhysicsContactMemory&m,
    NativeTu15ContactCompletionPacket event,std::uint32_t component,
    std::uint32_t own,std::uint32_t other,std::uint32_t own_slot,std::uint32_t other_slot,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ContactEntityProcessorServices&services,
    NativeTu15ContactEntityProcessorStats*stats){
    if(stats)++stats->calls;
    auto descriptor=word(m,word(m,event.identity+12u));
    if(!positive(word(m,descriptor+4u)))return;
    std::uint32_t contact_index{},contact_offset{};
    do{
        const auto contact=word(m,descriptor)+contact_offset;
        auto count=word(m,component+0x8cu);
        if(stats)++stats->contacts;
        if(positive(count)){
            std::uint32_t row_index{},row_offset{};
            do{
                const auto row=word(m,component+0x88u)+row_offset;
                if(stats)++stats->candidate_rows;
                if(word(m,row,2)==word(m,contact+0x20u,2)&&
                   word(m,row+0x14u)==other&&signed_byte(m,row+0x45u)==own_slot&&
                   signed_byte(m,row+0x46u)==other_slot){
                    if(stats)++stats->matched_rows;
                    const auto first=word(m,event.identity+4u);
                    if(word(m,first+0x18u)==1u){
                        const auto body=word(m,first+0x20u);
                        if(body){
                            const bool reversed=body!=word(m,event.identity+0x10u);
                            // The original performs this root-word read before
                            // dispatch, though its private cached value is dead.
                            if(reversed)(void)word(m,first);
                            else (void)word(m,word(m,event.identity+8u));
                            const auto manager=word(m,event.identity);
                            if(!services.update_contact)throw std::runtime_error("Native TU1.5 entity contact processor requires actual contact update");
                            if(stats)++stats->updates;
                            services.update_contact(manager,row,reversed,own,own_slot,fp);
                        }
                    }
                }
                count=word(m,component+0x8cu);++row_index;row_offset+=0x48u;
            }while(std::bit_cast<std::int32_t>(row_index)<std::bit_cast<std::int32_t>(count));
        }
        descriptor=word(m,word(m,event.identity+12u));
        const auto contacts=word(m,descriptor+4u);++contact_index;contact_offset+=0x30u;
        if(std::bit_cast<std::int32_t>(contact_index)>=std::bit_cast<std::int32_t>(contacts))break;
    }while(true);
}
} // namespace halo2
