#include "halo2/native_tu15_world_step_listeners.hpp"
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
    std::uint32_t word(std::uint32_t address)const{
        std::array<std::byte,4>raw{};memory_.read(address,raw);std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(raw[n])<<(n*8u);
        return value;
    }
    void word(std::uint32_t site,std::uint32_t address,std::uint32_t value){
        std::array<std::byte,4>raw{};
        for(unsigned n=0;n<4;++n)raw[n]=std::byte(value>>(n*8u));
        SourcePhysicsContactRecordStore event{site,address,{},{}};
        if(observe_){event.before.resize(4);memory_.read(address,event.before);
            event.written.assign(raw.begin(),raw.end());}
        memory_.write(address,raw);if(observe_)observe_(event);
    }
};
bool nonnegative(std::uint32_t n){return std::bit_cast<std::int32_t>(n)>=0;}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
template<class Notify>
void callback(Memory&m,NativeTu15WorldCollisionListener listener,Notify notify){
    const NativeTu15PhysicsVirtualOwner owner{m.word(m.word(listener.identity)+4u)};
    // The authenticated08FDB0 RET8 leaf performs no data/FP work.
    if(owner.identity!=0x8fdb0u)notify(listener,owner);
}
template<class Notify>
void cohort(Memory&m,NativeTu15PhysicsWorld world,std::uint32_t array_offset,
    std::uint32_t count_offset,std::uint32_t count_site,std::uint32_t copy_site,
    Notify notify){
    for(auto index=m.word(world.identity+count_offset)-1u;nonnegative(index);--index){
        const auto array=m.word(world.identity+array_offset);
        const NativeTu15WorldCollisionListener listener{m.word(array+index*4u)};
        if(listener.identity)callback(m,listener,notify);
    }
    for(auto index=m.word(world.identity+count_offset)-1u;nonnegative(index);--index){
        const auto array=m.word(world.identity+array_offset);
        if(m.word(array+index*4u))continue;
        const auto count=m.word(world.identity+count_offset)-1u;
        m.word(count_site,world.identity+count_offset,count);
        if(!less(index,count))continue;
        auto cursor=index;
        do{
            const auto current=m.word(world.identity+array_offset);
            const auto next=m.word(current+cursor*4u+4u);
            m.word(copy_site,current+cursor*4u,next);
            const auto current_count=m.word(world.identity+count_offset);++cursor;
            if(!less(cursor,current_count))break;
        }while(true);
    }
}
}
void native_tu15_notify_world_step_listeners(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsWorld world,NativeTu15PhysicsStepPacket step,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15WorldCollisionListenerServices&s,
    const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);
    const auto notify=[&](auto listener,auto owner){
        if(!s.notify)throw std::runtime_error("Native TU1.5 world-step listener requires its actual callback owner");
        s.notify(listener,owner,world,step,fp);
    };
    cohort(m,world,0x84u,0x88u,0x32286bu,0x32288du,notify);
    // Actual3161F0 is a closed field getter, read only after cohort callbacks
    // and compaction. Callback changes to this listener remain visible.
    const NativeTu15WorldCollisionListener final_listener{m.word(world.identity+0xc0u)};
    if(final_listener.identity)callback(m,final_listener,notify);
}
void native_tu15_notify_island_step_listeners(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsWorld world,NativeTu15PhysicsIsland island,
    NativeTu15PhysicsStepPacket step,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15WorldCollisionListenerServices&s,
    const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);
    cohort(m,world,0x9cu,0xa0u,0x3229a8u,0x3229bfu,[&](auto listener,auto owner){
        if(!s.notify_island)throw std::runtime_error("Native TU1.5 island-step listener requires its actual callback owner");
        s.notify_island(listener,owner,island,step,fp);
    });
}
} // namespace halo2
