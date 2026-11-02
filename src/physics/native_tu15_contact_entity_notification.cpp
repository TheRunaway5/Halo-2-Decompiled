#include "halo2/native_tu15_contact_entity_notification.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&memory,std::uint32_t address,unsigned width=4){
    std::array<std::byte,4>raw{};memory.read(address,std::span(raw).first(width));
    std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(raw[n])<<(n*8u);
    return value;
}
std::uint32_t entity(const SourcePhysicsContactMemory&memory,std::uint32_t payload){
    auto parent=word(memory,payload+12u);
    while(parent){payload=parent;parent=word(memory,payload+12u);}
    return word(memory,payload+24u)==1u?word(memory,payload+32u):0;
}
std::uint32_t property(const SourcePhysicsContactMemory&memory,std::uint32_t body,std::uint32_t key){
    const auto count=word(memory,body+0x34u);
    if(std::bit_cast<std::int32_t>(count)<=0)return 0;
    const auto array=word(memory,body+0x30u);std::uint32_t index{};
    do{
        if(word(memory,array+index*8u)==key)return word(memory,array+index*8u+4u);
        ++index;
    }while(std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(count));
    return 0;
}
}
void native_tu15_notify_contact_entities(const SourcePhysicsContactMemory&memory,
    NativeTu15ContactCompletionPacket event,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ContactEntityNotificationServices&services,NativeTu15ContactEntityNotificationStats*stats){
    if(stats)++stats->calls;
    const auto first=entity(memory,word(memory,event.identity+4u));
    const auto second=entity(memory,word(memory,event.identity+8u));
    if(!first||!second){if(stats)++stats->missing_body_pairs;return;}
    const auto first_datum=property(memory,first,0x2001u);
    const auto second_datum=property(memory,second,0x2001u);
    const auto first_slot=property(memory,first,0x2002u);
    const auto second_slot=property(memory,second,0x2002u);
    if(stats)stats->property_queries+=4u;
    const auto subject=word(memory,event.identity+16u),pool=word(memory,0x52be5cu);
    const bool use_first=subject==first;
    const auto rows=word(memory,pool+0x44u),datum=use_first?first_datum:second_datum;
    const auto component=rows+(datum&0xffffu)*160u;
    if(!(word(memory,component+4u,1)&1u)){if(stats)++stats->disabled_components;return;}
    if(!services.process_contacts)throw std::runtime_error("Native TU1.5 contact entity notification requires its genuine contact processor");
    if(stats)++stats->dispatch_calls;
    services.process_contacts(event,component,datum,use_first?second_datum:first_datum,
        use_first?first_slot:second_slot,use_first?second_slot:first_slot,fp);
}
} // namespace halo2
