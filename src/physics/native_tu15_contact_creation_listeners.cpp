#include "halo2/native_tu15_contact_creation_listeners.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
    std::uint32_t word(std::uint32_t p)const {
        std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t v{};
        for(unsigned k=0;k<4;++k)v|=std::to_integer<unsigned>(b[k])<<(8u*k);return v;
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t v){
        std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(v>>(8u*k));
        SourcePhysicsContactRecordStore e{pc,p,{},{}};
        if(o_){e.before.resize(4);m_.read(p,e.before);}
        m_.write(p,b);if(o_){e.written.assign(b.begin(),b.end());o_(e);}
    }
};
struct Layout {std::uint32_t data,count,slot,publish_count,move_entry;};
bool negative(std::uint32_t v){return bool(v&0x80000000u);}
bool signed_less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
void dispatch(SourcePhysicsContactMemory&memory,std::uint32_t owner,std::uint32_t argument,
    Layout layout,NativeTu15PhysicsFrameFp&fp,const NativeTu15ContactCreationListenerServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ContactCreationListenerStats*stats){
    Memory m(memory,observe);auto index=m.word(owner+layout.count)-1u;
    while(!negative(index)){
        const auto data=m.word(owner+layout.data),listener=m.word(data+index*4u);
        if(listener){
            const auto table=m.word(listener),target=m.word(table+layout.slot);
            if(stats)++stats->notifications;
            if(!services.notify)throw std::runtime_error("Native TU1.5 contact creation requires listener owner");
            services.notify(listener,{target},argument,fp);
        }
        --index;
    }
    index=m.word(owner+layout.count)-1u;
    while(!negative(index)){
        const auto data=m.word(owner+layout.data);
        if(!m.word(data+index*4u)){
            const auto last=m.word(owner+layout.count)-1u;
            m.word(layout.publish_count,owner+layout.count,last);
            auto destination=index;
            if(signed_less(destination,last)){
                for(;;){
                    const auto current=m.word(owner+layout.data);
                    const auto next=m.word(current+destination*4u+4u);
                    m.word(layout.move_entry,current+destination*4u,next);
                    const auto count=m.word(owner+layout.count);
                    ++destination;
                    if(!signed_less(destination,count))break;
                }
            }
            if(stats)++stats->removed_nulls;
        }
        --index;
    }
}
template<class F>void attempt(NativeTu15ContactCreationListenerStats*stats,F&&function){
    if(stats)++stats->attempts;
    try{function();if(stats)++stats->completed;}catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace

void native_tu15_notify_world_contact_creation(SourcePhysicsContactMemory&memory,
    std::uint32_t world,std::uint32_t event,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ContactCreationListenerServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ContactCreationListenerStats*stats){
    attempt(stats,[&]{Memory(memory,observe).word(0x32244bu,event+0x10u,0u);
        dispatch(memory,world,event,{0xb4u,0xb8u,0u,0x322497u,0x3224aeu},fp,services,observe,stats);});
}
void native_tu15_notify_body_contact_creation(SourcePhysicsContactMemory&memory,
    std::uint32_t body,std::uint32_t event,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ContactCreationListenerServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ContactCreationListenerStats*stats){
    attempt(stats,[&]{Memory(memory,observe).word(0x31fe7bu,event+0x10u,body);
        dispatch(memory,body,event,{0x60u,0x64u,0u,0x31feb1u,0x31fecau},fp,services,observe,stats);});
}
void native_tu15_notify_world_constraint_addition(SourcePhysicsContactMemory&memory,
    std::uint32_t world,std::uint32_t constraint,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ContactCreationListenerServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ContactCreationListenerStats*stats){
    attempt(stats,[&]{dispatch(memory,world,constraint,{0x60u,0x64u,8u,0x3223a1u,0x3223bau},fp,services,observe,stats);});
}
} // namespace halo2
