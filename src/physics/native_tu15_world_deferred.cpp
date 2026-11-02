#include "halo2/native_tu15_world_deferred.hpp"
#include "halo2/native_tu15_island_collision.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;
    const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o)
        :memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned bytes=4)const{
        std::array<std::byte,4>raw{};memory_.read(p,std::span(raw).first(bytes));std::uint32_t value{};
        for(unsigned n=0;n<bytes;++n)value|=std::to_integer<unsigned>(raw[n])<<(n*8u);
        return value;
    }
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t value,unsigned bytes=4){
        std::array<std::byte,4>raw{};for(unsigned n=0;n<bytes;++n)raw[n]=std::byte(value>>(n*8u));
        SourcePhysicsContactRecordStore event{site,p,{},{}};
        if(observe_){event.before.resize(bytes);memory_.read(p,event.before);
            event.written.assign(raw.begin(),raw.begin()+bytes);}
        memory_.write(p,std::span(raw).first(bytes));if(observe_)observe_(event);
    }
};
bool positive(std::uint32_t n){return std::bit_cast<std::int32_t>(n)>0;}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
template<class Function>void require(const Function&f,const char*message){if(!f)throw std::runtime_error(message);}
void release_reference(SourcePhysicsContactMemory&memory,Memory&m,NativeTu15PhysicsBody body,
    std::uint32_t decrement_site,std::uint32_t protected_site,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15WorldDeferredServices&s,NativeTu15WorldDeferredStats*stats){
    const auto previous=m.word(body.identity+6u,2);
    m.word(decrement_site,body.identity+6u,(previous-1u)&0xffffu,2);
    if(stats)++stats->reference_releases;
    if(m.word(body.identity+6u,2))return;
    if(native_tu15_reserved_body_reference(memory,body)){
        m.word(protected_site,body.identity+6u,1u,2);return;
    }
    const NativeTu15PhysicsVirtualOwner owner{m.word(m.word(body.identity))};
    require(s.destroy_reference,"Native TU1.5 deferred reference requires its actual destruction owner");
    if(stats)++stats->destructions;s.destroy_reference(body,owner,1u,fp);
}
}
void native_tu15_begin_world_merge_monitor(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsIsland island,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);m.word(0x282bc4u,0x56dfdcu,island.identity);
    m.word(0x282bceu,0x56dfd0u,native_tu15_island_allocated_bytes(memory));
}
void native_tu15_end_world_merge_monitor(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsIsland island,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);m.word(0x282be0u,0x56dfdcu,0);
    const auto after=native_tu15_island_allocated_bytes(memory),before=m.word(0x56dfd0u);
    if(after-before){m.word(0x282bfdu,island.identity+0xaeu,1,1);m.word(0x282c03u,0x56dfccu,1,1);}
}
void native_tu15_drain_world_operations(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsWorld world,NativeTu15PhysicsFrameFp&fp,const NativeTu15WorldDeferredServices&s,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15WorldDeferredStats*stats){
    Memory m(memory,observe);if(stats)++stats->operation_calls;
    if(!positive(m.word(world.identity+0x24u))){m.word(0x3197a6u,world.identity+0x24u,0);return;}
    std::uint32_t index{};
    do{
        const auto pairs=m.word(world.identity+0x20u);
        const auto first_body=m.word(pairs+index*8u),second_body=m.word(pairs+index*8u+4u);
        const NativeTu15PhysicsIsland first{m.word(first_body+0x44u)},second{m.word(second_body+0x44u)};
        if(first.identity!=second.identity){
            // Actual282BC0/282BE0 bracket the merge. Their allocation delta
            // is measured from real native heap bookkeeping, never supplied.
            native_tu15_begin_world_merge_monitor(memory,first,observe);
            require(s.merge_islands,"Native TU1.5 deferred operation requires actual3190D0");
            if(stats)++stats->merges;s.merge_islands(world,first,second,fp);
            native_tu15_end_world_merge_monitor(memory,first,observe);
        }
        const auto count=m.word(world.identity+0x24u);++index;
        if(!less(index,count))break;
    }while(true);
    m.word(0x31979cu,world.identity+0x24u,0);
}
void native_tu15_drain_world_added_actions(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsWorld world,NativeTu15PhysicsFrameFp&fp,const NativeTu15WorldDeferredServices&s,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15WorldDeferredStats*stats){
    Memory m(memory,observe);if(stats)++stats->action_calls;
    if(positive(m.word(world.identity+0x10cu))){
        std::uint32_t index{};
        do{
            const auto queue=m.word(world.identity+0x108u),body=m.word(queue+index*4u);
            const NativeTu15PhysicsIsland island{m.word(body+0x44u)};
            if(island.identity&&m.word(island.identity+0x29u,1)){
                require(s.deactivate_island,"Native TU1.5 added action requires actual316CD0");
                if(stats)++stats->deactivations;s.deactivate_island(world,island,fp);
            }
            const auto current=m.word(world.identity+0x108u);
            release_reference(memory,m,{m.word(current+index*4u)},0x316ec9u,0x316ee1u,fp,s,stats);
            const auto count=m.word(world.identity+0x10cu);++index;
            if(!less(index,count))break;
        }while(true);
    }
    m.word(0x316efdu,world.identity+0x10cu,0);
}
void native_tu15_drain_world_deletes(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsWorld world,NativeTu15DeferredResult result,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15WorldDeferredServices&s,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15WorldDeferredStats*stats){
    Memory m(memory,observe);if(stats)++stats->delete_calls;
    if(positive(m.word(world.identity+0xf4u))){
        std::uint32_t index{};
        do{
            const auto queue=m.word(world.identity+0xf0u);const NativeTu15PhysicsBody body{m.word(queue+index*4u)};
            if(m.word(body.identity+0x44u)){
                require(s.remove_body,"Native TU1.5 deferred delete requires actual3198A0");
                if(stats)++stats->body_removals;s.remove_body(world,result,body,fp);
            }
            const auto current=m.word(world.identity+0xf0u);
            release_reference(memory,m,{m.word(current+index*4u)},0x319d08u,0x319d20u,fp,s,stats);
            const auto count=m.word(world.identity+0xf4u);++index;
            if(!less(index,count))break;
        }while(true);
    }
    m.word(0x319d3cu,world.identity+0xf4u,0);
}
void native_tu15_drain_world_updates(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsWorld world,NativeTu15DeferredResult result,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15WorldDeferredServices&s,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15WorldDeferredStats*stats){
    Memory m(memory,observe);if(stats)++stats->update_calls;
    if(positive(m.word(world.identity+0x100u))){
        std::uint32_t index{};
        do{
            const auto queue=m.word(world.identity+0xfcu);const NativeTu15PhysicsBody update{m.word(queue+index*4u)};
            const auto body=m.word(update.identity+0xcu);
            if(m.word(body+0x44u)){
                require(s.remove_update,"Native TU1.5 deferred update requires actual316AA0");
                if(stats)++stats->update_removals;s.remove_update(world,result,update,fp);
            }
            const auto current=m.word(world.identity+0xfcu);
            release_reference(memory,m,{m.word(current+index*4u)},0x316e4bu,0x316e63u,fp,s,stats);
            const auto count=m.word(world.identity+0x100u);++index;
            if(!less(index,count))break;
        }while(true);
    }
    m.word(0x316e7fu,world.identity+0x100u,0);
}
} // namespace halo2
