#include "halo2/native_campaign_ai_query_scheduler_tu15.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
#include <array>
#include <bit>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
std::int16_t signed_word(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
std::int32_t signed_dword(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
std::uint32_t offset(std::int16_t index){return std::uint32_t(std::int32_t(index))*12u;}
std::uint32_t actor_row(SourcePhysicsContactMemory &memory,std::uint32_t actor) {
    const auto pool=read(memory,0x5020c8u),rows=read(memory,pool+0x44u);return rows+(actor&0xffffu)*0x888u;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=4) {
    std::array<std::byte,4> bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
    if(!observe){memory.write(address,std::span(bytes).first(width));return;}
    SourcePhysicsContactRecordStore event{site,address,{}, {bytes.begin(),bytes.begin()+width}};
    event.before.resize(width);memory.read(address,event.before);memory.write(address,event.written);observe(event);
}
}
std::int16_t native_campaign_ai_query_find_slot_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t actor,std::uint32_t query) {
    const auto row=actor_row(memory,actor);
    for(std::int16_t index=0;index<4;++index) {
        const auto slot=row+0x194u+offset(index);
        if(signed_word(read(memory,slot,2))!=-1&&read(memory,slot+4u)==query)return index;
    }
    return -1;
}
void native_campaign_ai_query_release_slot_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t actor,std::int16_t index,const SourcePhysicsContactRecordObserver &observe) {
    const auto row=actor_row(memory,actor),slot=row+0x194u+offset(index);
    const auto query=read(memory,slot+4u),pool=read(memory,0x50eefcu),rows=read(memory,pool+0x44u);
    const auto member=signed_word(read(memory,slot+2u,2));const auto query_row=rows+(query&0xffffu)*0xbcu;
    publish(memory,observe,0x278a96,query_row+offset(member)+4u,0xffffffffu);
    const auto after_publication=signed_word(read(memory,slot+2u,2));
    publish(memory,observe,0x278aa5,query_row+offset(after_publication)+8u,1u,2);
    publish(memory,observe,0x278aac,slot,0xffffu,2);
}
std::int16_t native_campaign_ai_query_select_slot_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t actor,std::int16_t priority,const SourcePhysicsContactRecordObserver &observe) {
    const auto row=actor_row(memory,actor);std::int16_t best_priority=8,best_slot=-1;
    for(std::int16_t index=0;index<4;++index) {
        const auto slot=row+0x194u+offset(index);
        if(signed_word(read(memory,slot,2))==-1)return index;
        const auto expiration=read(memory,slot+8u),clock=read(memory,0x51df54u);
        if(signed_dword(expiration)<signed_dword(read(memory,clock+8u))) {
            native_campaign_ai_query_release_slot_tu15(memory,actor,index,observe);return index;
        }
        const auto query=read(memory,slot+4u);const auto member=signed_word(read(memory,slot+2u,2));
        const auto pool=read(memory,0x50eefcu),rows=read(memory,pool+0x44u);
        const auto candidate=signed_word(read(memory,rows+(query&0xffffu)*0xbcu+offset(member)+0xau,2));
        if(candidate<best_priority){best_priority=candidate;best_slot=index;}
    }
    if(best_priority<priority&&best_slot!=-1) {
        native_campaign_ai_query_release_slot_tu15(memory,actor,best_slot,observe);return best_slot;
    }
    return -1;
}
bool native_campaign_ai_query_reserve_member_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t query,std::uint32_t actor,std::int16_t priority,std::uint32_t expiration_bits,
    const SourcePhysicsContactRecordObserver &observe) {
    if(native_campaign_ai_query_select_slot_tu15(memory,actor,priority,observe)==-1)return false;
    const auto pool=read(memory,0x50eefcu),rows=read(memory,pool+0x44u),row=rows+(query&0xffffu)*0xbcu;
    std::int16_t index=0;
    if(read(memory,row+4u)!=0xffffffffu) {
        do {
            if(index>=10)return false;
            ++index;
        }while(read(memory,row+offset(index)+4u)!=0xffffffffu);
        if(index>=10)return false;
    }
    const auto member=row+offset(index);
    publish(memory,observe,0x278859,member+4u,actor);
    publish(memory,observe,0x27885d,member+0xcu,expiration_bits);
    publish(memory,observe,0x278862,member+0xau,std::uint16_t(priority),2);return true;
}
namespace {
struct RetainedBroadcastRequest {
    SourcePhysicsContactMemory &memory;std::uint32_t address;
    std::uint32_t query()const{return read(memory,address+4u);}
    std::uint16_t kind()const{return std::uint16_t(read(memory,address,2));}
    std::uint32_t expiration()const{return read(memory,address+8u);}
};
struct NativeBroadcastRequest {
    const NativeCampaignAiQueryBroadcastRequest &value;
    std::uint32_t query()const{return value.query;}
    std::uint16_t kind()const{return value.kind;}
    std::uint32_t expiration()const{return value.expiration_bits;}
};
template<class Request> void broadcast(SourcePhysicsContactMemory &memory,
    const Request &request,std::uint32_t excluded_actor,const SourcePhysicsContactRecordObserver &observe) {
    const auto query=request.query(),pool=read(memory,0x50eefcu),rows=read(memory,pool+0x44u);
    const auto row=rows+(query&0xffffu)*0xbcu;
    for(std::int16_t member=0;member<10;++member) {
        const auto actor=read(memory,row+offset(member)+4u);
        if(actor==0xffffffffu||actor==excluded_actor)continue;
        const auto actor_address=actor_row(memory,actor);
        const auto priority=signed_word(read(memory,row+offset(member)+0xau,2));
        const auto index=native_campaign_ai_query_select_slot_tu15(memory,actor,priority,observe);
        if(index==-1)continue;
        const auto slot=actor_address+0x194u+offset(index);
        const auto current_query=request.query();publish(memory,observe,0x278994,slot+4u,current_query);
        const auto kind=request.kind();publish(memory,observe,0x2789a0,slot,kind,2);
        const auto expiration=request.expiration();publish(memory,observe,0x2789aa,slot+8u,expiration);
        publish(memory,observe,0x2789b0,slot+2u,std::uint16_t(member),2);
    }
}
}
void native_campaign_ai_query_broadcast_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t request,std::uint32_t excluded_actor,const SourcePhysicsContactRecordObserver &observe) {
    broadcast(memory,RetainedBroadcastRequest{memory,request},excluded_actor,observe);
}
void native_campaign_ai_query_broadcast_tu15(SourcePhysicsContactMemory &memory,
    const NativeCampaignAiQueryBroadcastRequest &request,std::uint32_t excluded_actor,
    const SourcePhysicsContactRecordObserver &observe) {
    broadcast(memory,NativeBroadcastRequest{request},excluded_actor,observe);
}
void native_campaign_ai_query_retire_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t state,const SourcePhysicsContactRecordObserver &observe) {
    const auto query=read(memory,state+0x10u),pool=read(memory,0x50eefcu),rows=read(memory,pool+0x44u);
    const auto row=rows+(query&0xffffu)*0xbcu;const auto member=signed_word(read(memory,state+0x18u,2));
    publish(memory,observe,0x278b74,row+offset(member)+4u,0xffffffffu);
    const auto after_publication=signed_word(read(memory,state+0x18u,2));
    publish(memory,observe,0x278b83,row+offset(after_publication)+8u,1u,2);
    const auto count=std::uint16_t(read(memory,row+0x7cu,2)-1u);
    publish(memory,observe,0x278b8a,row+0x7cu,count,2);(void)read(memory,row+0x7cu,2);
    if(count!=0)return;
    const auto actors=read(memory,0x5020c8u);
    for(unsigned index=0;index<10;++index) {
        const auto current=row+index*12u+4u,actor=read(memory,current);
        if(actor==0xffffffffu||read(memory,current+4u,2)!=0)continue;
        const auto actor_index=actor&0xffffu;
        if(signed_dword(actor_index)>=signed_dword(read(memory,actors+0x38u)))continue;
        const auto stride=read(memory,actors+0x24u),actor_rows=read(memory,actors+0x44u);
        const auto actor_address=actor_rows+actor_index*stride;
        const auto salt=signed_word(read(memory,actor_address,2));
        if(salt==0||salt!=signed_word(actor>>16u))continue;
        const auto current_query=read(memory,state+0x10u);
        const auto slot_index=native_campaign_ai_query_find_slot_tu15(memory,actor,current_query);
        if(slot_index==-1)continue;
        const auto slot=actor_address+0x194u+offset(slot_index);
        publish(memory,observe,0x278c0a,slot,0xffffu,2);
        publish(memory,observe,0x278c11,slot+4u,0xffffffffu);
        publish(memory,observe,0x278c14,slot+2u,0xffffu,2);
    }
    const auto current_query=read(memory,state+0x10u),current_pool=read(memory,0x50eefcu);
    native_tu15_data_array_delete(memory,current_pool,current_query);
}
}
