#include "halo2/native_tu15_island_transition.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
bool nonnegative(std::uint32_t value){return std::bit_cast<std::int32_t>(value)>=0;}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
void compact_null_listeners(NativeTu15IslandTransitionStorage&s,NativeTu15PhysicsArrayHeader list,bool world){
    auto cursor=s.count(list)-1u;
    if(!nonnegative(cursor))return;
    do{
        if(s.element(s.data(list),cursor)==0){
            const auto next_count=s.count(list)-1u;
            s.count(list,next_count,world?0x322781u:0x3227f0u);
            if(less(cursor,next_count)){
                auto destination=cursor;
                do{
                    const auto data=s.data(list);
                    const auto next=s.element(data,destination+1u);
                    s.element(data,destination,next,world?0x32279au:0x322800u);
                    const auto current_count=s.count(list);
                    ++destination;
                    if(!less(destination,current_count))break;
                }while(true);
            }
        }
        --cursor;
    }while(nonnegative(cursor));
}
}
void native_tu15_notify_island_deactivation(NativeTu15IslandTransitionStorage&s,
    NativeTu15PhysicsWorld world,NativeTu15PhysicsIsland island,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15IslandTransitionServices&services){
    const auto world_list=s.island_listeners(world);
    auto cursor=s.count(world_list)-1u;
    if(nonnegative(cursor))do{
        const NativeTu15PhysicsListener listener{s.element(s.data(world_list),cursor)};
        if(listener.identity){
            const auto owner=s.listener_owner(listener,NativeTu15DeactivationNotification::island);
            if(owner.identity!=0x2e4d60u){
                if(!services.island_deactivated)throw std::runtime_error("Native TU1.5 transition requires its island deactivation listener owner");
                services.island_deactivated(listener,owner,island,fp);
            }
        }
        --cursor;
    }while(nonnegative(cursor));
    compact_null_listeners(s,world_list,true);
    const auto bodies=s.bodies(island);
    auto count=s.count(bodies);
    if(!less(0,count))return;
    std::uint32_t body_index{};
    do{
        const NativeTu15PhysicsBody body{s.element(s.data(bodies),body_index)};
        // Actual3227B9 keeps this header through callbacks. A callback can
        // replace the island's body pointer, but cannot retarget compaction.
        const auto body_list=s.body_listeners(body);
        cursor=s.count(body_list)-1u;
        if(nonnegative(cursor))do{
            const NativeTu15PhysicsListener listener{s.element(s.data(body_list),cursor)};
            if(listener.identity){
                const NativeTu15PhysicsBody argument{s.element(s.data(bodies),body_index)};
                const auto owner=s.listener_owner(listener,NativeTu15DeactivationNotification::body);
                if(owner.identity!=0x2e4d60u){
                    if(!services.body_deactivated)throw std::runtime_error("Native TU1.5 transition requires its body deactivation listener owner");
                    services.body_deactivated(listener,owner,argument,fp);
                }
            }
            --cursor;
        }while(nonnegative(cursor));
        compact_null_listeners(s,body_list,false);
        count=s.count(bodies);++body_index;
    }while(less(body_index,count));
}
void native_tu15_deactivate_physics_island(NativeTu15IslandTransitionStorage&s,
    NativeTu15PhysicsWorld world,NativeTu15PhysicsIsland island,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15IslandTransitionServices&services){
    const auto inactive=s.inactive_islands(world);
    const auto capacity=s.capacity(inactive)&0x7fffffffu;
    if(s.count(inactive)==capacity){
        if(!services.grow_array)throw std::runtime_error("Native TU1.5 transition requires actual array growth2E3250");
        services.grow_array(inactive,4,fp);
    }
    const auto insertion=s.count(inactive);
    const auto destination=s.data(inactive);
    s.element(destination,insertion,island.identity,0x316cfau);
    s.count(inactive,s.count(inactive)+1u,0x316cfdu);
    const auto active=s.active_islands(world);
    auto data=s.data(active);
    const auto count=s.count(active);
    const auto last=s.element(data,count-1u);
    auto index=s.index(island);
    s.element(data,index,last,0x316d0du);
    index=s.index(island);
    data=s.data(active);
    const NativeTu15PhysicsIsland moved{s.element(data,index)};
    s.index(moved,index,0x316d19u);
    s.count(active,s.count(active)-1u,0x316d1cu);
    const auto inactive_index=s.count(inactive)-1u;
    s.index(island,inactive_index,0x316d24u);
    s.inactive(island);
    native_tu15_notify_island_deactivation(s,world,island,fp,services);
}
NativeTu15IslandTransitionMemory::NativeTu15IslandTransitionMemory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
std::uint32_t NativeTu15IslandTransitionMemory::read(std::uint32_t p,unsigned n)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(b[i])<<(8u*i);return v;}
void NativeTu15IslandTransitionMemory::write(std::uint32_t site,std::uint32_t p,std::uint32_t v,unsigned n){std::array<std::byte,4>b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(8u*i));SourcePhysicsContactRecordStore store{site,p,{},{}};if(observer_){store.before.resize(n);memory_.read(p,store.before);store.written.assign(b.begin(),b.begin()+n);}memory_.write(p,std::span(b).first(n));if(observer_)observer_(store);}
NativeTu15PhysicsArrayHeader NativeTu15IslandTransitionMemory::inactive_islands(NativeTu15PhysicsWorld w)const{return{w.identity+0x14u};}
NativeTu15PhysicsArrayHeader NativeTu15IslandTransitionMemory::active_islands(NativeTu15PhysicsWorld w)const{return{w.identity+8u};}
NativeTu15PhysicsArrayHeader NativeTu15IslandTransitionMemory::island_listeners(NativeTu15PhysicsWorld w)const{return{w.identity+0x78u};}
NativeTu15PhysicsArrayHeader NativeTu15IslandTransitionMemory::bodies(NativeTu15PhysicsIsland i)const{return{i.identity+0x3cu};}
NativeTu15PhysicsArrayHeader NativeTu15IslandTransitionMemory::body_listeners(NativeTu15PhysicsBody b)const{return{b.identity+0x70u};}
std::uint32_t NativeTu15IslandTransitionMemory::capacity(NativeTu15PhysicsArrayHeader a)const{return read(a.identity+8u);}
std::uint32_t NativeTu15IslandTransitionMemory::count(NativeTu15PhysicsArrayHeader a)const{return read(a.identity+4u);}
NativeTu15PhysicsArray NativeTu15IslandTransitionMemory::data(NativeTu15PhysicsArrayHeader a)const{return{read(a.identity)};}
std::uint32_t NativeTu15IslandTransitionMemory::element(NativeTu15PhysicsArray a,std::uint32_t i)const{return read(a.identity+i*4u);}
void NativeTu15IslandTransitionMemory::element(NativeTu15PhysicsArray a,std::uint32_t i,std::uint32_t v,std::uint32_t site){write(site,a.identity+i*4u,v);}
void NativeTu15IslandTransitionMemory::count(NativeTu15PhysicsArrayHeader a,std::uint32_t v,std::uint32_t site){write(site,a.identity+4u,v);}
std::uint32_t NativeTu15IslandTransitionMemory::index(NativeTu15PhysicsIsland i)const{return read(i.identity+0x24u);}
void NativeTu15IslandTransitionMemory::index(NativeTu15PhysicsIsland i,std::uint32_t v,std::uint32_t site){write(site,i.identity+0x24u,v);}
void NativeTu15IslandTransitionMemory::inactive(NativeTu15PhysicsIsland i){write(0x316d28u,i.identity+0x29u,0,1);}
NativeTu15PhysicsVirtualOwner NativeTu15IslandTransitionMemory::listener_owner(NativeTu15PhysicsListener l,NativeTu15DeactivationNotification n)const{return{read(read(l.identity)+(n==NativeTu15DeactivationNotification::island?8u:4u))};}
} // namespace halo2
