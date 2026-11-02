#include "halo2/native_tu15_component_reset.hpp"
#include <bit>
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
bool positive(std::uint32_t n){return std::bit_cast<std::int32_t>(n)>0;}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
}
void native_tu15_reset_physics_component(SourcePhysicsContactMemory&memory,std::uint32_t component,
    NativeTu15ComponentResetScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15ComponentResetServices&services,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ComponentResetStats*stats){
    if(stats)++stats->calls;Memory m(memory,observe);
    const auto missing=[&](std::uint32_t owner)->void{
        if(stats){++stats->unbound_services;++stats->unbound_owners[owner];}
        throw std::runtime_error("Native TU1.5 component reset requires native owner="+std::to_string(owner));
    };
    const auto destroy=[&](std::uint32_t body){
        const auto table=m.word(body),owner=m.word(table);
        // Authenticated RET4, no DATA/FP effects; this caller discards EAX.
        if(owner==0x2e4d60u){if(stats)++stats->body_destructions;return;}
        if(!services.destroy_body)missing(owner);if(stats)++stats->body_destructions;
        services.destroy_body({body},{owner},1u,fp,depth);
    };
    auto count=m.word(component+0x80u);
    if(positive(count)){
        std::uint32_t index{},offset{};
        do{
            const auto records=m.word(component+0x7cu),body=m.word(records+offset+8u),record=records+offset;
            m.word(0x1d8499u,body+6u,m.word(body+6u,2)-1u,2);if(stats)++stats->references;
            if(!m.word(body+6u,2)){
                if(native_tu15_reserved_body_reference(memory,{body})){m.word(0x1d84b1u,body+6u,1u,2);if(stats)++stats->protected_resets;}
                else destroy(body);
            }
            m.word(0x1d84c5u,record+8u,0u);count=m.word(component+0x80u);++index;offset+=12u;
        }while(less(index,count));
    }
    count=m.word(component+0x74u);
    if(positive(count)){
        std::uint32_t index{},offset{};
        do{
            const auto records=m.word(component+0x70u),body=m.word(records+offset+0x40u),record=records+offset;
            const auto remove=[&](std::uint32_t key,std::uint32_t result){
                auto properties=m.word(body+0x34u);std::uint32_t property{};bool found=false;
                if(positive(properties)){
                    auto cursor=m.word(body+0x30u);
                    do{
                        if(m.word(cursor)==key){found=true;break;}
                        properties=m.word(body+0x34u);++property;cursor+=8u;
                    }while(less(property,properties));
                }
                if(found){native_tu15_remove_body_property(memory,body,key,result,observe);if(stats)++stats->property_removals;}
                native_tu15_add_body_property(memory,body,key,0xffffffffu,fp,services.allocation,observe);
                if(stats)++stats->property_publications;
            };
            remove(0x2001u,scratch.removed_component);remove(0x2002u,scratch.removed_slot);
            m.word(0x1d857du,body+6u,m.word(body+6u,2)-1u,2);if(stats)++stats->references;
            if(!m.word(body+6u,2)){
                if(native_tu15_reserved_body_reference(memory,{body})){m.word(0x1d85c3u,body+6u,1u,2);if(stats)++stats->protected_resets;}
                else destroy(body);
            }
            m.word(0x1d85d3u,record+0x40u,0u);const auto capacity=m.word(record+0x50u);
            if(!less(capacity,0u)){
                const auto allocator=m.word(0x48c938u),table=m.word(allocator),data=m.word(record+0x48u),owner=m.word(table+0x14u);
                if(!services.allocation.release)missing(owner);if(stats)++stats->record_releases;
                services.allocation.release({allocator,owner,data,capacity&0x7fffffffu,0x12u},fp);
            }
            count=m.word(component+0x74u);++index;offset+=96u;
        }while(less(index,count));
    }
    auto body=m.word(component+0x98u);
    m.word(0x1d8617u,component+4u,0u);m.word(0x1d861au,component+0x1cu,0u,1);
    m.word(0x1d861du,component+0x18u,0xffu,1);m.word(0x1d8621u,component+0x19u,0xffu,1);
    m.word(0x1d8625u,component+0x74u,0u);m.word(0x1d8628u,component+0x80u,0u);
    if(body){
        m.word(0x1d8630u,body+6u,m.word(body+6u,2)-1u,2);if(stats)++stats->references;
        if(!m.word(body+6u,2)){
            if(native_tu15_reserved_body_reference(memory,{body})){m.word(0x1d8647u,body+6u,1u,2);if(stats)++stats->protected_resets;}
            else destroy(body);
        }
        m.word(0x1d865du,component+0x98u,0u);
    }
    body=m.word(component+0x9cu);
    if(body){
        m.word(0x1d866du,body+6u,m.word(body+6u,2)-1u,2);if(stats)++stats->references;
        if(!m.word(body+6u,2)){
            if(native_tu15_reserved_body_reference(memory,{body})){m.word(0x1d8684u,body+6u,1u,2);if(stats)++stats->protected_resets;}
            else destroy(body);
        }
        m.word(0x1d8698u,component+0x9cu,0u);
    }
}
} // namespace halo2
