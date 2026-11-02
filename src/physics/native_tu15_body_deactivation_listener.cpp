#include "halo2/native_tu15_body_deactivation_listener.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
bool positive(std::uint32_t value){return std::bit_cast<std::int32_t>(value)>0;}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
    std::uint32_t read(std::uint32_t p,unsigned size=4)const{
        std::array<std::byte,4>bytes{};m_.read(p,std::span(bytes).first(size));std::uint32_t value{};
        for(unsigned n=0;n<size;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
    }
    void write(std::uint32_t site,std::uint32_t p,std::uint32_t value,unsigned size=4){
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<size;++n)bytes[n]=std::byte(value>>(8u*n));
        SourcePhysicsContactRecordStore event{site,p,{},{}};
        if(o_){event.before.resize(size);m_.read(p,event.before);event.written.assign(bytes.begin(),bytes.begin()+size);}
        m_.write(p,std::span(bytes).first(size));if(o_)o_(event);
    }
};
}
void native_tu15_body_deactivation_listener(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsListener listener,NativeTu15PhysicsBody body,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15BodyDeactivationListenerServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15BodyDeactivationListenerStats*stats){
    Memory m(memory,observe);if(stats)++stats->calls;
    if(m.read(listener.identity+0x18u,1)){
        m.write(0x2815fdu,body.identity+6u,m.read(body.identity+6u,2)+1u,2);
        const auto flags=m.read(listener.identity+0x10u),header=listener.identity+8u;
        const auto count=m.read(header+4u);
        if(count==(flags&0x7fffffffu)){
            if(stats)++stats->array_growths;
            NativeTu15ArrayGrowthMemory array(memory,{header},observe);
            native_tu15_grow_array(array,4u,fp,services.allocation);
        }
        const auto insertion=m.read(header+4u),data=m.read(header);
        m.write(0x281623u,data+insertion*4u,body.identity);
        m.write(0x281626u,header+4u,m.read(header+4u)+1u);
        if(stats)++stats->deferred_bodies;
    }
    if(stats)++stats->property_queries;
    const auto datum=native_tu15_activation_body_datum(memory,body);
    if(datum==0xffffffffu){if(stats)++stats->missing_datums;return;}
    const auto manager=m.read(0x52be5cu),components=m.read(manager+0x44u),component=components+(datum&0xffffu)*160u;
    if(!(m.read(component+4u)&0x20u))return;
    auto count=m.read(component+0x74u);
    if(!positive(count))return;
    std::uint32_t index{};
    do{
        if(!services.publish_component_body_motion){
            if(stats)++stats->unbound_services;
            throw std::runtime_error("Native TU1.5 body deactivation requires component motion owner1D7260");
        }
        if(stats)++stats->component_motion_publications;
        services.publish_component_body_motion(component,index,fp);
        count=m.read(component+0x74u);++index;
    }while(less(index,count));
}
}
