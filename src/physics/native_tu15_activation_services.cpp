#include "halo2/native_tu15_activation_services.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
public:
    Memory(SourcePhysicsContactMemory&memory,const SourcePhysicsContactRecordObserver&observer):memory_(memory),observer_(observer){}
    std::uint32_t word(std::uint32_t address,unsigned width=4)const{std::array<std::byte,4>bytes{};memory_.read(address,std::span(bytes).first(width));std::uint32_t result{};for(unsigned n=0;n<width;++n)result|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return result;}
    void word(std::uint32_t instruction,std::uint32_t address,std::uint32_t value,unsigned width=4){std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));SourcePhysicsContactRecordStore event{instruction,address,{},{}};if(observer_){event.before.resize(width);memory_.read(address,event.before);event.written.assign(bytes.begin(),bytes.begin()+width);}memory_.write(address,std::span(bytes).first(width));if(observer_)observer_(event);}
};
}
std::optional<std::uint32_t> native_tu15_activation_type(std::uint32_t owner) noexcept{
    switch(owner){
    case 0xa9be0u:return 19; // Complete original A9BE0: B8 13 00 00 00 C3.
    case 0xadb50u:return 20;case 0xad1d0u:return 17;
    case 0x2e8cf0u:return 4;case 0x31dd70u:return 5;case 0x31edf0u:return 6;
    case 0xad710u:return 10;case 0xae690u:return 21;
    case 0xad740u:return 1;case 0x31d520u:return 2;
    case 0xa3850u:return 0;case 0x31ede0u:return 7;case 0xad390u:return 8;
    case 0xaf940u:return 25;case 0x1cb030u:return 0xffffffffu;
    default:return std::nullopt;
    }
}
std::uint32_t native_tu15_activation_group_filter(SourcePhysicsContactMemory&memory,
 std::uint32_t filter,std::uint32_t result,NativeTu15ContactPayload first,
 NativeTu15ContactPayload second,const SourcePhysicsContactRecordObserver&observer,
 NativeTu15ActivationServiceStats*stats){
    Memory m(memory,observer);if(stats)++stats->filter_calls;
    const auto second_group=m.word(second.identity+0x1cu),first_group=m.word(first.identity+0x1cu);
    if(!((first_group^second_group)&0xffff0000u)&&(first_group&0xffff0000u)){
        m.word(0x2e9a28u,result,0,1);if(stats)++stats->same_group_rejections;
    }else{
        const auto bit=1u<<(second_group&31u),mask=m.word(filter+(first_group&31u)*4u+0x14u);
        m.word(0x2e9a49u,result,(mask&bit)!=0,1);if(stats)++stats->mask_queries;
    }
    return result;
}
std::uint32_t native_tu15_activation_discrete_pair(SourcePhysicsContactMemory&memory,
 std::uint32_t dispatcher,std::uint32_t result,NativeTu15ContactPayload first,
 NativeTu15ContactPayload second,NativeTu15PhysicsFrameFp&fp,
 const NativeTu15ActivationDiscreteServices&services,const SourcePhysicsContactRecordObserver&observer,
 NativeTu15ActivationServiceStats*stats){
    Memory m(memory,observer);if(stats)++stats->discrete_calls;const auto filter=m.word(dispatcher+8u),table=m.word(filter);const NativeTu15ContactVirtualOwner owner{m.word(table+12u)};
    if(!services.test_filter)throw std::runtime_error("Native TU1.5 activation discrete query requires actual filter ownership");
    const auto accepted=services.test_filter(filter,owner,first,second,fp);
    m.word(0x281546u,result,accepted&255u,1);return result;
}
void native_tu15_wake_object_ancestors(SourcePhysicsContactMemory&memory,std::uint32_t datum,
 const SourcePhysicsContactRecordObserver&observer,NativeTu15ActivationServiceStats*stats){
    Memory m(memory,observer);if(stats)++stats->object_wake_calls;const auto pool=m.word(0x4ecbd0u),entries=m.word(pool+0x44u);
    if(m.word(entries+(datum&0xffffu)*12u+2u,1)&2u)return;
    if(datum==0xffffffffu)return;
    auto current_entries=entries;
    do{
        const auto offset=(datum&0xffffu)*12u,header=current_entries+offset+2u;
        m.word(0xb93c8u,header,m.word(header,1)|2u,1);if(stats)++stats->object_headers_marked;
        current_entries=m.word(pool+0x44u);const auto object=m.word(current_entries+offset+8u);datum=m.word(object+0x14u);
    }while(datum!=0xffffffffu);
}
void native_tu15_body_activation_listener(SourcePhysicsContactMemory&memory,NativeTu15PhysicsBody body,
 NativeTu15PhysicsFrameFp&fp,const NativeTu15ActivationBodyServices&services,
 const SourcePhysicsContactRecordObserver&observer,NativeTu15ActivationServiceStats*stats){
    Memory m(memory,observer);if(stats)++stats->body_listener_calls;
    const auto count=m.word(body.identity+0x34u);std::uint32_t datum{};
    if(std::bit_cast<std::int32_t>(count)>0){
        const auto properties=m.word(body.identity+0x30u);std::uint32_t index{};
        do{if(m.word(properties+index*8u)==0x2001u){datum=m.word(properties+index*8u+4u);break;}++index;}while(std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(count));
    }
    if(datum==0xffffffffu)return;
    const auto manager=m.word(0x52be5cu),entries=m.word(manager+0x44u),offset=(datum&0xffffu)*160u;
    const auto flags=m.word(entries+offset+4u);m.word(0x2816efu,entries+offset+4u,flags&0xfffffff7u);
    const auto object=m.word(entries+offset+8u);native_tu15_wake_object_ancestors(memory,object,observer,stats);
    if(!services.publish_body_update)throw std::runtime_error("Native TU1.5 activation listener requires actual body-update ownership");
    if(stats)++stats->body_updates;services.publish_body_update(datum,fp);
}
} // namespace halo2
