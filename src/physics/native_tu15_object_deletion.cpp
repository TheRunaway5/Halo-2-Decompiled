#include "halo2/native_tu15_object_deletion.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;const SourcePhysicsContactRecordObserver& observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t n{};
        for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(8u*k);return n;
    }
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t n,unsigned width=4){
        std::array<std::byte,4>b{};for(unsigned k=0;k<width;++k)b[k]=std::byte(n>>(8u*k));SourcePhysicsContactRecordStore e{site,p,{},{}};
        if(observe_){e.before.resize(width);memory_.read(p,e.before);e.written.assign(b.begin(),b.begin()+width);}
        memory_.write(p,std::span(b).first(width));if(observe_)observe_(e);
    }
};
std::int32_t signed32(std::uint32_t n){return std::bit_cast<std::int32_t>(n);}
[[noreturn]] void missing(NativeTu15ObjectDeletionStats*s,const char*owner){if(s)++s->unbound_services;throw std::runtime_error(owner);}
void spatial(Memory&m,std::uint32_t datum,NativeTu15ObjectDeletionScratch scratch,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ObjectDeletionServices&s,NativeTu15ObjectDeletionStats*stats){
    if(stats)++stats->spatial_queries;const auto pool=m.word(0x4ecbd0u),rows=m.word(pool+0x44u),header=rows+(datum&0xffffu)*12u;
    const auto flags=m.word(header+2u,1),object=m.word(header+8u);
    if((flags&0x40u)&&(m.word(object+4u)&0x200u)){
        if(!s.spatial_packet)missing(stats,"Native object deletion requires actualBA910 spatial packet owner");
        if(stats)++stats->spatial_packets;s.spatial_packet(datum,scratch.spatial_packet,fp);
        const auto actor=m.word(object+0x60u);
        if(!s.remove_spatial)missing(stats,"Native object deletion requires actual1D30A0 spatial removal owner");
        if(stats)++stats->spatial_removals;s.remove_spatial(actor,datum,scratch.spatial_packet,fp);
    }
}
void set_dependent_flag(SourcePhysicsContactMemory&memory,Memory&m,std::uint32_t datum,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ObjectDeletionServices&s,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ObjectDeletionStats*stats){
    auto pool=m.word(0x4ecbd0u);auto rows=m.word(pool+0x44u);const auto offset=(datum&0xffffu)*12u;
    const auto type=m.word(rows+offset+3u,1),object=m.word(rows+offset+8u),mask=1u<<(type&31u);
    if((mask&0x1883u)&&(m.word(object+0xc0u,1)&0x40u)){
        const auto component=m.word(object+0xb4u);
        if(component!=0xffffffffu){
            const auto components=m.word(0x52be5cu),component_rows=m.word(components+0x44u);
            if(!s.release_physics_component)missing(stats,"Native object deletion requires actual1D9F10 component owner");
            s.release_physics_component(component_rows+(component&0xffffu)*160u,fp);
            pool=m.word(0x4ecbd0u);
        }
    }
    m.word(0xbbc38u,object+0xc1u,m.word(object+0xc1u,1)&0xfeu,1);
    native_tu15_wake_object_ancestors(memory,datum,observe);
    if(mask&0x3cu){
        rows=m.word(pool+0x44u);const auto current=m.word(rows+offset+8u),actor=m.word(current+0xd4u);
        if(actor!=0xffffffffu){if(!s.actor_flags)missing(stats,"Native object deletion requires actualB7E50 actor flags owner");s.actor_flags(actor,0x400u,fp);}
    }
}
void release_player(Memory&m,std::uint32_t datum,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ObjectDeletionServices&s,NativeTu15ObjectDeletionStats*stats){
    const auto pool=m.word(0x4ecbd0u),rows=m.word(pool+0x44u),object=m.word(rows+(datum&0xffffu)*12u+8u);
    if(m.word(object+0x3dcu,1)==3u){
        const auto packet=object+0x3ecu;m.word(0xe7189u,packet+0xdu,0,1);m.word(0xe718cu,packet+0xcu,0,1);
        if(!s.release_player_state)missing(stats,"Native object deletion requires actual1F5620 player state owner");s.release_player_state(datum,packet,fp);
        if(!s.release_player_control)missing(stats,"Native object deletion requires actualE3E20 player control owner");s.release_player_control(datum,fp);
    }
}
void dependents(SourcePhysicsContactMemory&memory,Memory&m,std::uint32_t datum,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ObjectDeletionServices&s,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ObjectDeletionStats*stats){
    if(stats)++stats->dependent_calls;const auto pool=m.word(0x4ecbd0u),rows=m.word(pool+0x44u),object=m.word(rows+(datum&0xffffu)*12u+8u);
    const auto definition=m.word(object),tags=m.word(0x4f0414u),tag=m.word(tags+(definition&0xffffu)*16u+8u);
    if(!(m.word(tag+2u,1)&0x40u))return;
    const auto state=m.word(0x51daf0u);auto count=m.word(state+0x1d80u);
    if(signed32(count)<=0)return;
    std::uint32_t index{},entry=state+0x1d00u;
    for(;;){
        if(m.word(entry)==datum)break;
        count=m.word(state+0x1d80u);++index;entry+=4u;if(signed32(index)>=signed32(count))return;
    }
    m.word(0x10e51eu,object+0xc0u,m.word(object+0xc0u,1)&0xf7u,1);
    count=m.word(state+0x1d80u);const auto last=m.word(state+count*4u+0x1cfcu);
    m.word(0x10e532u,state+index*4u+0x1d00u,last);m.word(0x10e539u,state+0x1d80u,m.word(state+0x1d80u)-1u);
    auto previous=0xffffffffu;
    for(;;){
        auto next=previous+1u;if(signed32(next)<0)return;const auto extent=m.word(pool+0x38u);if(signed32(next)>=signed32(extent))return;
        const auto occupancy=m.word(pool+0x48u);
        for(;;){
            const auto shift=std::uint32_t(signed32(next)>>5u);
            if(m.word(occupancy+shift*4u)&(1u<<(next&31u)))break;
            ++next;if(signed32(next)>=signed32(extent))return;
        }
        if(next==0xffffffffu)return;
        const auto stride=m.word(pool+0x24u),current_rows=m.word(pool+0x44u),header=current_rows+next*stride;
        const auto salt=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(m.word(header,2))));
        const auto other=(std::uint32_t(salt)<<16u)|next;previous=next;
        const auto live_pool=m.word(0x4ecbd0u),live_rows=m.word(live_pool+0x44u),other_object=m.word(live_rows+(other&0xffffu)*12u+8u);
        const auto flags=m.word(header+2u,1);if(!(flags&1u)||(flags&0x10u))continue;
        const auto body_flags=m.word(other_object+0xc0u,2);if(!(body_flags&6u))continue;
        if(m.word(other_object+0xb8u)!=datum)continue;
        const auto type=m.word(header+3u,1),mask=1u<<(type&31u);
        if(mask&0x1cu){set_dependent_flag(memory,m,other,fp,s,observe,stats);native_tu15_wake_object_ancestors(memory,other,observe);}
        else if(mask&1u){release_player(m,other,fp,s,stats);set_dependent_flag(memory,m,other,fp,s,observe,stats);native_tu15_wake_object_ancestors(memory,other,observe);}
    }
}
void actor(Memory&m,std::uint32_t datum,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ObjectDeletionServices&s,NativeTu15ObjectDeletionStats*stats){
    if(stats)++stats->actor_calls;const auto pool=m.word(0x4ecbd0u),rows=m.word(pool+0x44u),offset=(datum&0xffffu)*12u,object=m.word(rows+offset+8u),attachment=m.word(object+0xd4u);
    if(attachment==0xffffffffu)return;
    if(!s.unregister_actor)missing(stats,"Native object deletion requires actualB7F00 actor unregister owner");s.unregister_actor(attachment,fp);
    const auto current_pool=m.word(0x4ecbd0u),current_rows=m.word(current_pool+0x44u),current=m.word(current_rows+offset+8u);
    const auto notification_pool=m.word(0x4ecbd0u),notification_rows=m.word(notification_pool+0x44u),notified=m.word(notification_rows+offset+8u);
    const auto type=std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(m.word(notified+0xaau,1))));
    const auto definition=m.word(0x474b90u+std::uint32_t(type)*4u);auto list=definition+0x84u;auto value=m.word(list);std::uint32_t index{};
    while(value){
        const auto current_value=m.word(list),owner=m.word(current_value+0x54u);
        if(owner){if(!s.actor_notification)missing(stats,"Native object deletion requires actual actor notification owner");s.actor_notification(datum,owner,fp);}
        ++index;const auto signed_index=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(index)));
        list=definition+std::uint32_t(signed_index)*4u+0x84u;value=m.word(list);
    }
    m.word(0xb1cccu,current+0xd4u,0xffffffffu);m.word(0xb1cd6u,current+0xd8u,0,1);
}
void projectile(Memory&m,std::uint32_t datum,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ObjectDeletionServices&s,NativeTu15ObjectDeletionStats*stats){
    if(stats)++stats->projectile_calls;const auto pool=m.word(0x4ecbd0u),rows=m.word(pool+0x44u),offset=(datum&0xffffu)*12u,object=m.word(rows+offset+8u);
    m.word(0x2f30f4u,0x46e468u,0,1);
    if(!(m.word(object+0x12cu,1)&2u)){
        const auto current_pool=m.word(0x4ecbd0u),current_rows=m.word(current_pool+0x44u),current=m.word(current_rows+offset+8u);
        const auto index=m.word(current+0x1au,2);
        if(index!=0xffffu){
            const auto state=m.word(0x4ecc20u),controllers=m.word(state+0x54u);
            const auto signed_index=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(index)));
            const auto slot=m.word(controllers+std::uint32_t(signed_index)*0x5cu+0x58u,2);
            const auto entry=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(slot))))-1u;
            if(entry!=0xffffffffu){
                if(m.word(0x52be74u)){
                    if(!s.release_projectile_controller)missing(stats,"Native object deletion requires actual1CE340 projectile controller owner");s.release_projectile_controller(datum,fp);
                }
                const auto entries=m.word(0x52be70u);m.word(0x10f37fu,entries+entry*4u,0xffffffffu);
            }
        }
        m.word(0x10f386u,object+0x12cu,m.word(object+0x12cu)|2u);
    }
}
}
bool native_tu15_object_or_ancestor_deleting(SourcePhysicsContactMemory&memory,
    std::uint32_t datum,NativeTu15ObjectDeletionStats*stats){
    if(stats)++stats->ancestor_queries;const SourcePhysicsContactRecordObserver none;Memory m(memory,none);const auto pool=m.word(0x4ecbd0u),rows=m.word(pool+0x44u);
    for(;;){const auto header=rows+(datum&0xffffu)*12u;const bool deleting=(m.word(header+2u,1)&0x10u)!=0;
        const auto object=m.word(header+8u);if(deleting)return true;if(m.word(object+4u,1)&1u)return true;
        datum=m.word(object+0x14u);if(datum==0xffffffffu)return false;}
}
bool native_tu15_object_root_attached(SourcePhysicsContactMemory&memory,
    std::uint32_t datum,NativeTu15ObjectDeletionStats*stats){
    if(stats)++stats->root_queries;const SourcePhysicsContactRecordObserver none;Memory m(memory,none);const auto pool=m.word(0x4ecbd0u);auto root=0xffffffffu;
    if(datum!=0xffffffffu){const auto rows=m.word(pool+0x44u);do{root=datum;const auto object=m.word(rows+(datum&0xffffu)*12u+8u);datum=m.word(object+0x14u);}while(datum!=0xffffffffu);}
    const auto rows=m.word(pool+0x44u);return(m.word(rows+(root&0xffffu)*12u+2u,1)&0x40u)!=0;
}
void native_tu15_clear_object_tracking(SourcePhysicsContactMemory&memory,std::uint32_t datum,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ObjectDeletionStats*stats){
    if(stats)++stats->tracking_clears;Memory m(memory,observe);const auto pool=m.word(0x4ecbd0u),rows=m.word(pool+0x44u),object=m.word(rows+(datum&0xffffu)*12u+8u);
    if(m.word(object+0xacu,2)==0xffffu)return;const auto state=m.word(0x4ecc20u);
    m.word(0xc10e8u,object+0xacu,0xffffu,2);auto count=m.word(state+0x48u);
    if(signed32(count)>0){const auto entries=m.word(0x4eaba0u);std::uint32_t index{},offset{};do{
        if(m.word(entries+offset*4u)==datum)m.word(0xc1108u,entries+offset*4u,0xffffffffu);
        count=m.word(state+0x48u);++index;offset=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(index))));
    }while(signed32(offset)<signed32(count));}
}
void native_tu15_clear_object_player(SourcePhysicsContactMemory&memory,std::uint32_t datum,
    std::int32_t player,const SourcePhysicsContactRecordObserver&observe,NativeTu15ObjectDeletionStats*stats){
    if(stats)++stats->player_clears;Memory m(memory,observe);const auto pool=m.word(0x4ecbd0u),rows=m.word(pool+0x44u),mode=m.word(0x4f3218u),object=m.word(rows+(datum&0xffffu)*12u+8u);
    if(m.word(mode+0xcu,1)!=4){const auto players=m.word(0x4f63b8u);m.word(0x15dc6fu,players+std::uint32_t(player)*8u+0x7dcu,0xffffffffu);m.word(0x15dc76u,object+0xafu,0xffu,1);}
    else m.word(0x15dc80u,object+0xafu,0xffu,1);
}
void native_tu15_update_object_delete_schedule(SourcePhysicsContactMemory&memory,
    std::uint32_t datum,bool enabled,std::uint32_t ticks,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ObjectDeletionStats*stats){
    if(stats)++stats->schedule_updates;Memory m(memory,observe);const auto pool=m.word(0x4ecbd0u),rows=m.word(pool+0x44u),header=rows+(datum&0xffffu)*12u,object=m.word(header+8u);auto flags=m.word(object+4u);
    if(enabled){
        if(!(flags&0xc000u)){
            const auto state=m.word(0x4eabc4u),head=m.word(state+8u);m.word(0xbd9c0u,object+0x1cu,head);m.word(0xbd9c3u,state+8u,datum);
            m.word(0xbd9c6u,object+4u,m.word(object+4u)|0x4000u);
            if(m.word(header+2u,1)&1u)m.word(0xbd9d3u,state+4u,m.word(state+4u,2)+1u,2);
        }
        const auto clock=m.word(0x51df54u),now=m.word(clock+8u),expiry=ticks+now,previous=m.word(object+0x20u);
        m.word(0xbd9eeu,object+0x20u,signed32(previous)>signed32(expiry)?previous:expiry);
    }else if(flags&0x4000u){
        const auto state=m.word(0x4eabc4u);auto link=state+8u;
        if(m.word(link)!=datum)do{const auto current=m.word(link),entry=m.word(rows+(current&0xffffu)*12u+8u);const auto next=m.word(entry+0x1cu);link=entry+0x1cu;if(next==datum)break;}while(true);
        const auto next=m.word(object+0x1cu);m.word(0xbda2bu,link,next);m.word(0xbda2du,object+0x1cu,0xffffffffu);
        m.word(0xbda34u,object+4u,m.word(object+4u)&0xffffbfffu);
        if(m.word(header+2u,1)&1u)m.word(0xbda41u,state+4u,m.word(state+4u,2)-1u,2);
    }
}
void native_tu15_clear_object_references(SourcePhysicsContactMemory&memory,std::uint32_t datum,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ObjectDeletionStats*stats){
    if(stats)++stats->reference_clears;if(datum==0xffffffffu)return;Memory m(memory,observe);
    const auto pool=m.word(0x4ecbd0u),rows=m.word(pool+0x44u),object=m.word(rows+(datum&0xffffu)*12u+8u);
    if(m.word(object+0xc0u,2)&0x200u){const auto state=m.word(0x51daf4u);auto entry=state+0x280u;
        for(unsigned n=0;n<32;++n){if(m.word(entry)==datum)m.word(0x10fdd4u,entry,0xffffffffu);entry+=4u;}
        m.word(0x10fde0u,object+0xc1u,m.word(object+0xc1u,1)&0xfdu,1);
    }
}
void native_tu15_delete_object(SourcePhysicsContactMemory&memory,std::uint32_t datum,
    NativeTu15ObjectDeletionScratch scratch,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ObjectDeletionServices&s,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ObjectDeletionStats*stats){
    if(stats)++stats->calls;Memory m(memory,observe);const auto pool=m.word(0x4ecbd0u),rows=m.word(pool+0x44u),header=rows+(datum&0xffffu)*12u,object=m.word(header+8u);
    if(!native_tu15_object_or_ancestor_deleting(memory,datum,stats)&&native_tu15_object_root_attached(memory,datum,stats)){
        if(!s.detach_object)missing(stats,"Native object deletion requires actualC0F60 detach owner");
        if(stats)++stats->detach_calls;s.detach_object(datum,fp);
    }
    m.word(0xba5b5u,header+2u,m.word(header+2u,1)|0x10u,1);spatial(m,datum,scratch,fp,s,stats);
    if((1u<<(m.word(header+3u,1)&31u))&0x40u)projectile(m,datum,fp,s,stats);
    native_tu15_clear_object_tracking(memory,datum,observe,stats);
    m.word(0xba5d9u,object+0xabu,0xffu,1);m.word(0xba5e0u,object+0xa8u,0xffffu,2);m.word(0xba5e9u,object+0xa4u,0xffffffffu);
    const auto player=m.word(object+0xafu,1);if(player!=0xffu)native_tu15_clear_object_player(memory,datum,std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(player))),observe,stats);
    dependents(memory,m,datum,fp,s,observe,stats);
    native_tu15_update_object_delete_schedule(memory,datum,false,0xffffffffu,observe,stats);
    actor(m,datum,fp,s,stats);
    native_tu15_clear_object_references(memory,datum,observe,stats);
}
}
