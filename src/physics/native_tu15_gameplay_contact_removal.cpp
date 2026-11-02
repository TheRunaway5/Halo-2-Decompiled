#include "halo2/native_tu15_gameplay_contact_removal.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;const SourcePhysicsContactRecordObserver& observe_;
public:
    Memory(SourcePhysicsContactMemory& memory,const SourcePhysicsContactRecordObserver& observe):memory_(memory),observe_(observe){}
    std::uint32_t word(std::uint32_t address,unsigned size=4)const{
        std::array<std::byte,4>bytes{};memory_.read(address,std::span(bytes).first(size));std::uint32_t value{};
        for(unsigned n=0;n<size;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
    }
    void word(std::uint32_t site,std::uint32_t address,std::uint32_t value){
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));
        SourcePhysicsContactRecordStore event{site,address,{},{}};
        if(observe_){event.before.resize(4);memory_.read(address,event.before);event.written.assign(bytes.begin(),bytes.end());}
        memory_.write(address,bytes);if(observe_)observe_(event);
    }
};
bool positive(std::uint32_t value){return std::bit_cast<std::int32_t>(value)>0;}
std::int32_t signed_byte(std::uint32_t value){return std::bit_cast<std::int8_t>(std::uint8_t(value));}
std::uint32_t property(Memory& m,std::uint32_t body,std::uint32_t count,std::uint32_t id){
    if(!positive(count))return 0;
    const auto data=m.word(body+0x30u);std::uint32_t index{};
    do{if(m.word(data+index*8u)==id)return m.word(data+index*8u+4u);++index;}while(index<count);
    return 0;
}
}

void native_tu15_remove_gameplay_contact_row(SourcePhysicsContactMemory& memory,
    std::uint32_t component,std::uint32_t index,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15GameplayContactRemovalServices& services,const SourcePhysicsContactRecordObserver& observe,
    NativeTu15GameplayContactRemovalStats* stats){
    Memory m(memory,observe);const auto data=m.word(component+0x88u),offset=index*72u;
    const auto flags=m.word(data+offset+0x44u,1);const auto part=signed_byte(m.word(data+offset+0x45u,1));
    const auto effect=m.word(data+offset+8u),record=data+offset;
    if(effect!=0xffffffffu){
        const auto owner=m.word(0x52d68cu),instances=m.word(owner+0x44u),instance=instances+(effect&0xffffu)*160u;
        if(!services.release_constraint_reference)throw std::runtime_error("Native TU1.5 gameplay contact removal requires its current22E1B0 owner");
        if(stats)++stats->constraint_releases;services.release_constraint_reference(instance,effect,fp);
        m.word(0x1d89ccu,record+8u,0xffffffffu);
    }
    const auto count=m.word(component+0x8cu),current_data=m.word(component+0x88u),last=count-1u;
    m.word(0x1d89e5u,component+0x8cu,last);
    const auto source=current_data+last*72u,destination=current_data+offset;
    for(unsigned n=0;n<18;++n){const auto value=m.word(source+n*4u);m.word(0x1d89f5u,destination+n*4u,value);}
    if(stats)++stats->rows;
    if(flags&0x20u){
        const auto parts=m.word(component+0x70u),slot=parts+std::bit_cast<std::uint32_t>(part)*96u;
        if(!services.release_part_contact)throw std::runtime_error("Native TU1.5 gameplay contact removal requires its current1E0A60 owner");
        if(stats)++stats->part_releases;services.release_part_contact(slot,part,component,fp);
    }
}

void native_tu15_remove_matching_gameplay_contact(SourcePhysicsContactMemory& memory,
    std::uint32_t other_body,std::uint32_t subject_body,std::uint32_t component,std::uint32_t event,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15GameplayContactRemovalServices& services,
    const SourcePhysicsContactRecordObserver& observe,NativeTu15GameplayContactRemovalStats* stats){
    if(stats)++stats->searches;Memory m(memory,observe);
    const auto other_count=m.word(other_body+0x34u),other_datum=property(m,other_body,other_count,0x2001u);
    const auto subject_count=m.word(subject_body+0x34u),subject_part=property(m,subject_body,subject_count,0x2002u);
    const auto other_part=property(m,other_body,other_count,0x2002u);
    const auto count=m.word(component+0x8cu);
    if(!positive(count))return;
    const auto point=m.word(event,2),data=m.word(component+0x88u);std::uint32_t index{};
    do{
        const auto row=data+index*72u;
        if(m.word(row,2)==point&&m.word(row+0x14u)==other_datum&&
           std::bit_cast<std::uint32_t>(signed_byte(m.word(row+0x45u,1)))==subject_part&&
           std::bit_cast<std::uint32_t>(signed_byte(m.word(row+0x46u,1)))==other_part){
            if(stats)++stats->matches;
            native_tu15_remove_gameplay_contact_row(memory,component,index,fp,services,observe,stats);return;
        }
        ++index;
    }while(index<count);
}

void native_tu15_notify_gameplay_contact_removed(SourcePhysicsContactMemory& memory,
    std::uint32_t event,NativeTu15PhysicsFrameFp& fp,const NativeTu15GameplayContactRemovalServices& services,
    const SourcePhysicsContactRecordObserver& observe,NativeTu15GameplayContactRemovalStats* stats){
    if(stats)++stats->notifications;Memory m(memory,observe);
    const auto first=m.word(event+8u),first_count=m.word(first+0x34u),second=m.word(event+0xcu);
    const auto first_datum=property(m,first,first_count,0x2001u);
    const auto second_count=m.word(second+0x34u),second_datum=property(m,second,second_count,0x2001u);
    const auto subject=m.word(event+0x10u);const auto owner=m.word(0x52be5cu),data=m.word(owner+0x44u);
    const bool is_first=subject==first;
    const auto component=data+((is_first?first_datum:second_datum)&0xffffu)*160u;
    if(m.word(component+4u,1)&1u){
        if(stats)++stats->active_components;
        native_tu15_remove_matching_gameplay_contact(memory,is_first?second:first,is_first?first:second,component,event,fp,services,observe,stats);
    }
}
} // namespace halo2
