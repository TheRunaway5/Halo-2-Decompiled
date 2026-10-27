#include "halo2/native_tu15_island_collision.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t read(SourcePhysicsContactMemory&m,std::uint32_t address,unsigned bytes=4){
  std::array<std::byte,4>raw{};m.read(address,std::span(raw).first(bytes));std::uint32_t value{};
  for(unsigned i=0;i<bytes;++i)value|=std::to_integer<unsigned>(raw[i])<<(i*8u);return value;
}
void write(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&observer,
  std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned bytes=4){
  std::array<std::byte,4>raw{};for(unsigned i=0;i<bytes;++i)raw[i]=std::byte(value>>(i*8u));
  SourcePhysicsContactRecordStore event{site,address,{},{}};
  if(observer){event.before.resize(bytes);m.read(address,event.before);event.written.assign(raw.begin(),raw.begin()+bytes);}
  m.write(address,std::span<const std::byte>(raw).first(bytes));if(observer)observer(event);
}
std::uint32_t total(SourcePhysicsContactMemory&m,std::uint32_t global){
  const auto heap=read(m,global);if(!heap)return 0;
  auto count=read(m,heap+0x20u);if(std::bit_cast<std::int32_t>(count)<=0)return 0;
  const auto bytes=read(m,heap+0x1cu);const auto base=read(m,heap+0x18u);
  auto cursor=bytes+base-4u;std::uint32_t result{};
  do{result+=read(m,cursor);cursor-=8u;--count;}while(count);
  return result;
}
}
std::uint32_t native_tu15_island_allocated_bytes(SourcePhysicsContactMemory&memory){
  const auto secondary=total(memory,0x486070u);const auto defaults=total(memory,0x486074u);
  return defaults+secondary;
}
void native_tu15_begin_island_collision_monitor(SourcePhysicsContactMemory&memory,
  NativeTu15PhysicsIsland island,const SourcePhysicsContactRecordObserver&observer){
  write(memory,observer,0x282b74u,0x56dfd8u,island.identity);
  const auto bytes=native_tu15_island_allocated_bytes(memory);
  write(memory,observer,0x282b7eu,0x56dfd4u,bytes);
}
void native_tu15_end_island_collision_monitor(SourcePhysicsContactMemory&memory,
  NativeTu15PhysicsIsland island,const SourcePhysicsContactRecordObserver&observer){
  write(memory,observer,0x282b90u,0x56dfd8u,0);
  const auto bytes=native_tu15_island_allocated_bytes(memory);const auto before=read(memory,0x56dfd4u);
  if(bytes-before){write(memory,observer,0x282badu,island.identity+0xaeu,1,1);write(memory,observer,0x282bb3u,0x56dfccu,1,1);}
}
void native_tu15_collide_physics_island(SourcePhysicsContactMemory&memory,
  NativeTu15PhysicsIsland island,NativeTu15CollisionDispatcher dispatcher,
  NativeTu15PhysicsStepConfiguration configuration,NativeTu15PhysicsFrameFp&fp,
  const NativeTu15IslandCollisionServices&services,const SourcePhysicsContactRecordObserver&observer){
  native_tu15_begin_island_collision_monitor(memory,island,observer);
  if(!services.first_phase)throw std::runtime_error("Native TU1.5 island collision requires actual321700 owner");
  services.first_phase(island,dispatcher,configuration,fp);
  const auto world=read(memory,island.identity+0x20u);const auto flag=read(memory,world+0x23eu,1);
  if(flag){
    if(!services.flag_set_phase)throw std::runtime_error("Native TU1.5 island collision requires actual321A70 owner");
    services.flag_set_phase(island,configuration,fp);
  }else{
    if(!services.flag_clear_phase)throw std::runtime_error("Native TU1.5 island collision requires actual321D20 owner");
    services.flag_clear_phase(island,configuration,fp);
  }
  native_tu15_end_island_collision_monitor(memory,island,observer);
}
} // namespace halo2
