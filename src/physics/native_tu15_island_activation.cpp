#include "halo2/native_tu15_island_activation.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width=4){std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(n*8u));SourcePhysicsContactRecordStore event{pc,p,{},{}};if(observer_){event.before.resize(width);memory_.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}memory_.write(p,std::span(b).first(width));if(observer_)observer_(event);}
};
template<class F>void require(const F&f,const char*message){if(!f)throw std::runtime_error(message);}
// Both98-site notification owners have the same signed reverse/null-removal
// algorithm, with distinct authenticated store sites for each actual cohort.
void compact(Memory&m,std::uint32_t header,bool body,NativeTu15IslandActivationStats*stats){
    auto index=m.word(header+4u)-1u;
    if(less(index,0))return;
    do{
        const auto data=m.word(header);
        if(!m.word(data+index*4u)){
            const auto remaining=m.word(header+4u)-1u;
            m.word(body?0x322710u:0x3226a1u,header+4u,remaining);
            if(stats)++stats->null_entries_removed;
            auto cursor=index;
            if(less(cursor,remaining))do{
                const auto current=m.word(header),value=m.word(current+cursor*4u+4u);
                m.word(body?0x322720u:0x3226bau,current+cursor*4u,value);
                const auto count=m.word(header+4u);++cursor;
                if(!less(cursor,count))break;
            }while(true);
        }
        --index;
    }while(!less(index,0));
}
}
bool native_tu15_reserved_body_reference(SourcePhysicsContactMemory&memory,NativeTu15PhysicsBody body){
    const SourcePhysicsContactRecordObserver none;Memory m(memory,none);constexpr std::uint32_t first=0x80061000u;
    if(less(body.identity,first))return false;
    const auto enabled=m.word(0x556588u,1),bytes=m.word(0x5565acu);
    const auto end=first+(enabled?bytes:0u);return !less(end,body.identity);
}
void native_tu15_notify_island_activation(SourcePhysicsContactMemory&memory,NativeTu15PhysicsWorld world,
 NativeTu15PhysicsIsland island,NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandActivationServices&s,
 const SourcePhysicsContactRecordObserver&observe,NativeTu15IslandActivationStats*stats){
    Memory m(memory,observe);auto listener_index=m.word(world.identity+0x7cu)-1u;
    if(!less(listener_index,0))do{
        const auto list=m.word(world.identity+0x78u);const NativeTu15PhysicsListener listener{m.word(list+listener_index*4u)};
        if(listener.identity){const auto table=m.word(listener.identity);const NativeTu15PhysicsVirtualOwner owner{m.word(table+4u)};
            if(stats)++stats->island_notifications;
            if(owner.identity==0x2e4d60u){if(stats)++stats->native_default_notifications;}
            else{require(s.island_activated,"Native TU1.5 activation requires actual world listener");s.island_activated(listener,owner,island,fp);}}
        --listener_index;
    }while(!less(listener_index,0));
    compact(m,world.identity+0x78u,false,stats);
    auto count=m.word(island.identity+0x40u);std::uint32_t body_index{};
    if(!less(0,count))return;
    do{
        const auto bodies=m.word(island.identity+0x3cu),body=m.word(bodies+body_index*4u);const auto listeners=body+0x70u;
        listener_index=m.word(body+0x74u)-1u;
        if(!less(listener_index,0))do{
            const auto list=m.word(listeners);const NativeTu15PhysicsListener listener{m.word(list+listener_index*4u)};
            if(listener.identity){const auto current_bodies=m.word(island.identity+0x3cu);const NativeTu15PhysicsBody current_body{m.word(current_bodies+body_index*4u)};const auto table=m.word(listener.identity);const NativeTu15PhysicsVirtualOwner owner{m.word(table+8u)};
                if(stats)++stats->body_notifications;
                if(owner.identity==0x2e4d60u){if(stats)++stats->native_default_notifications;}
                else{require(s.body_activated,"Native TU1.5 activation requires actual body listener");s.body_activated(listener,owner,current_body,fp);}}
            --listener_index;
        }while(!less(listener_index,0));
        compact(m,listeners,true,stats);count=m.word(island.identity+0x40u);++body_index;
    }while(less(body_index,count));
}
void native_tu15_activate_physics_island(SourcePhysicsContactMemory&memory,NativeTu15PhysicsWorld world,
 NativeTu15PhysicsIsland island,NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandActivationServices&s,
 const SourcePhysicsContactRecordObserver&observe,NativeTu15IslandActivationStats*stats){
    Memory m(memory,observe);if(stats)++stats->island_calls;
    const auto capacity=m.word(world.identity+0x10u)&0x7fffffffu,count=m.word(world.identity+0xcu);
    if(count==capacity){require(s.grow_array,"Native TU1.5 activation requires actual active-array growth");if(stats)++stats->growths;s.grow_array({world.identity+8u},4u,fp);}
    const auto current_count=m.word(world.identity+0xcu),active=m.word(world.identity+8u);
    m.word(0x316d6bu,active+current_count*4u,island.identity);m.word(0x316d6eu,world.identity+0xcu,m.word(world.identity+0xcu)+1u);
    const auto inactive=m.word(world.identity+0x14u),inactive_count=m.word(world.identity+0x18u),moved=m.word(inactive+inactive_count*4u-4u),index=m.word(island.identity+0x24u);
    m.word(0x316d7eu,inactive+index*4u,moved);
    const auto current_inactive=m.word(world.identity+0x14u),current_index=m.word(island.identity+0x24u),current_moved=m.word(current_inactive+current_index*4u);
    m.word(0x316d8au,current_moved+0x24u,current_index);m.word(0x316d8du,world.identity+0x18u,m.word(world.identity+0x18u)-1u);
    const auto new_index=m.word(world.identity+0xcu)-1u;m.word(0x316d94u,island.identity+0x24u,new_index);
    m.word(0x316d97u,island.identity+0x29u,1,1);m.word(0x316d9fu,island.identity+0x34u,0);m.word(0x316da2u,island.identity+0x38u,0);
    native_tu15_notify_island_activation(memory,world,island,fp,s,observe,stats);
    m.word(0x316daeu,island.identity+0x30u,0,1);
}
void native_tu15_remove_deferred_physics_body(SourcePhysicsContactMemory&memory,NativeTu15PhysicsWorld world,
 NativeTu15PhysicsBody body,NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandActivationServices&s,
 const SourcePhysicsContactRecordObserver&observe,NativeTu15IslandActivationStats*stats){
    Memory m(memory,observe);auto count=m.word(world.identity+0x10cu);std::uint32_t index{};
    if(!less(0,count))return;
    do{
        const auto list=m.word(world.identity+0x108u);
        if(m.word(list+index*4u)==body.identity){
            const auto search_count=m.word(world.identity+0x10cu);std::uint32_t found=0xffffffffu;
            if(less(0,search_count)){const auto search_list=m.word(world.identity+0x108u);std::uint32_t candidate{};do{if(m.word(search_list+candidate*4u)==body.identity){found=candidate;break;}++candidate;}while(less(candidate,search_count));}
            const auto remaining=m.word(world.identity+0x10cu)-1u;m.word(0x316626u,world.identity+0x10cu,remaining);
            const auto current_list=m.word(world.identity+0x108u),moved=m.word(current_list+remaining*4u);m.word(0x316637u,current_list+found*4u,moved);
            m.word(0x31663au,body.identity+6u,m.word(body.identity+6u,2)-1u,2);
            if(stats)++stats->deferred_removals;
            if(!m.word(body.identity+6u,2)){
                if(native_tu15_reserved_body_reference(memory,body)){m.word(0x316652u,body.identity+6u,1,2);if(stats)++stats->protected_references;}
                else{const auto table=m.word(body.identity);const NativeTu15PhysicsVirtualOwner owner{m.word(table)};require(s.destroy_body,"Native TU1.5 deferred removal requires actual body destructor");if(stats)++stats->destroy_calls;s.destroy_body(body,owner,1,fp);}
            }
        }
        count=m.word(world.identity+0x10cu);++index;
    }while(less(index,count));
}
void native_tu15_activate_physics_body(SourcePhysicsContactMemory&memory,NativeTu15PhysicsBody body,
 NativeTu15PhysicsFrameFp&fp,const NativeTu15IslandActivationServices&s,
 const SourcePhysicsContactRecordObserver&observe,NativeTu15IslandActivationStats*stats){
    Memory m(memory,observe);if(stats)++stats->body_calls;const NativeTu15PhysicsWorld world{m.word(body.identity+8u)};
    if(!world.identity)return;native_tu15_remove_deferred_physics_body(memory,world,body,fp,s,observe,stats);
    const NativeTu15PhysicsIsland island{m.word(body.identity+0x44u)};
    if(m.word(island.identity+0x29u,1)||m.word(island.identity+0x24u)==0xffffffffu)return;
    native_tu15_activate_physics_island(memory,{m.word(body.identity+8u)},island,fp,s,observe,stats);
}
} // namespace halo2
