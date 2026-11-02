#include "halo2/native_tu15_component_state.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t value{};
        for(unsigned i=0;i<width;++i)value|=std::to_integer<unsigned>(b[i])<<(8u*i);return value;
    }
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t value,unsigned width=4){
        std::array<std::byte,4>b{};for(unsigned i=0;i<width;++i)b[i]=std::byte(value>>(8u*i));
        SourcePhysicsContactRecordStore event{site,p,{},{}};
        if(observe_){event.before.resize(width);memory_.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}
        memory_.write(p,std::span(b).first(width));if(observe_)observe_(event);
    }
};
[[noreturn]] void missing(NativeTu15ComponentStateStats*stats,std::uint32_t owner){
    if(stats){++stats->unbound_services;++stats->unbound_owners[owner];}
    throw std::runtime_error("Native TU1.5 component state requires native owner="+std::to_string(owner));
}
}
void native_tu15_set_component_state_flags(SourcePhysicsContactMemory&memory,
    std::uint32_t datum,std::uint32_t flags,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15ComponentStateServices&services,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ComponentStateStats*stats){
    if(stats)++stats->flag_calls;Memory m(memory,observe);
    const auto objects=m.word(0x4ecbd0u),headers=m.word(objects+0x44u),object=m.word(headers+(datum&0xffffu)*12u+8u),component=m.word(object+0xb4u);
    if(component==0xffffffffu)return;
    const auto pool=m.word(0x52be5cu),rows=m.word(pool+0x44u),row=rows+(component&0xffffu)*160u;
    const bool active=((m.word(row+4u)>>5u)&1u)!=0;
    if(active){if(!services.remove)missing(stats,0x1d9510u);if(stats)++stats->removals;services.remove(row,fp,depth);}
    if(!services.reset)missing(stats,0x1d8470u);if(stats)++stats->resets;services.reset(row,fp,depth);
    m.word(0x1cc369u,row+4u,flags);const auto current_component=m.word(object+0xb4u);
    if(!services.refresh)missing(stats,0x1d73d0u);if(stats)++stats->refreshes;services.refresh(current_component,fp,depth);
    if(active){if(!services.activate)missing(stats,0x1d97f0u);if(stats)++stats->activations;services.activate(row,fp,depth);}
}
void native_tu15_defer_component_object(SourcePhysicsContactMemory&memory,
    std::uint32_t datum,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15ComponentStateServices&services,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ComponentStateStats*stats){
    if(stats)++stats->defer_calls;Memory m(memory,observe);
    const auto objects=m.word(0x4ecbd0u),headers=m.word(objects+0x44u),object=m.word(headers+(datum&0xffffu)*12u+8u),component=m.word(object+0xb4u);
    if(component!=0xffffffffu){
        const auto pool=m.word(0x52be5cu),rows=m.word(pool+0x44u),row=rows+(component&0xffffu)*160u;
        if(!services.activate)missing(stats,0x1d97f0u);if(stats)++stats->activations;services.activate(row,fp,depth);
    }
    m.word(0xba8ffu,object+0xc0u,m.word(object+0xc0u,1)|0x40u,1);
}
} // namespace halo2
