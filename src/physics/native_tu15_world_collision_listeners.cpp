#include "halo2/native_tu15_world_collision_listeners.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
  SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
public:
  Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
  std::uint32_t word(std::uint32_t address)const{
    std::array<std::byte,4>raw{};memory_.read(address,raw);std::uint32_t result{};
    for(unsigned i=0;i<4;++i)result|=std::to_integer<unsigned>(raw[i])<<(8u*i);return result;
  }
  void word(std::uint32_t site,std::uint32_t address,std::uint32_t value){
    std::array<std::byte,4>raw{};for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(8u*i));
    SourcePhysicsContactRecordStore event{site,address,{},{}};
    if(observer_){event.before.resize(4);memory_.read(address,event.before);event.written.assign(raw.begin(),raw.end());}
    memory_.write(address,raw);if(observer_)observer_(event);
  }
};
bool nonnegative(std::uint32_t value){return std::bit_cast<std::int32_t>(value)>=0;}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
}
namespace {
template<class Callback>
void notify(Memory&m,std::uint32_t w,std::uint32_t array_offset,
  std::uint32_t count_offset,std::uint32_t count_site,std::uint32_t copy_site,
  Callback callback){
  for(auto index=m.word(w+count_offset)-1u;nonnegative(index);--index){
    const auto array=m.word(w+array_offset);const NativeTu15WorldCollisionListener listener{m.word(array+index*4u)};
    if(listener.identity){
      const NativeTu15PhysicsVirtualOwner owner{m.word(m.word(listener.identity)+4u)};
      // Actual08FDB0 is the complete authenticated RET8 owner. No callback
      // output is supplied: this source leaf has no work before its return.
      if(owner.identity!=0x8fdb0u)callback(listener,owner);
    }
  }
  for(auto index=m.word(w+count_offset)-1u;nonnegative(index);--index){
    const auto array=m.word(w+array_offset);
    if(!m.word(array+index*4u)){
      const auto count=m.word(w+count_offset)-1u;m.word(count_site,w+count_offset,count);
      if(less(index,count)){
        auto cursor=index;
        do{
          const auto current=m.word(w+array_offset);const auto next=m.word(current+cursor*4u+4u);
          m.word(copy_site,current+cursor*4u,next);
          const auto current_count=m.word(w+count_offset);++cursor;
          if(!less(cursor,current_count))break;
        }while(true);
      }
    }
  }
}
}
void native_tu15_notify_world_collision_listeners(SourcePhysicsContactMemory&memory,
  NativeTu15PhysicsWorld world,NativeTu15PhysicsStepPacket step,NativeTu15PhysicsFrameFp&fp,
  const NativeTu15WorldCollisionListenerServices&services,const SourcePhysicsContactRecordObserver&observer){
  Memory m(memory,observer);
  notify(m,world.identity,0x90u,0x94u,0x32290bu,0x32292du,[&](auto listener,auto owner){
    if(!services.notify)throw std::runtime_error("Native TU1.5 post-collision listener requires its actual callback owner");
    services.notify(listener,owner,world,step,fp);
  });
}
void native_tu15_notify_island_collision_listeners(SourcePhysicsContactMemory&memory,
  NativeTu15PhysicsWorld world,NativeTu15PhysicsIsland island,NativeTu15PhysicsStepPacket step,
  NativeTu15PhysicsFrameFp&fp,const NativeTu15WorldCollisionListenerServices&services,
  const SourcePhysicsContactRecordObserver&observer){
  Memory m(memory,observer);
  notify(m,world.identity,0xa8u,0xacu,0x322a38u,0x322a4fu,[&](auto listener,auto owner){
    if(!services.notify_island)throw std::runtime_error("Native TU1.5 island post-collision listener requires its actual callback owner");
    services.notify_island(listener,owner,island,step,fp);
  });
}
} // namespace halo2
