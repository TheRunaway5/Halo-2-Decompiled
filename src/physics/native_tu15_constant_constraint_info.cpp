#include "halo2/native_tu15_constant_constraint_info.hpp"
namespace halo2 {
std::uint32_t native_tu15_get_constant_constraint_info(SourcePhysicsContactMemory&memory,
    NativeTu15IslandConstraintInfo info,const SourcePhysicsContactRecordObserver&observe){
  const auto store=[&](std::uint32_t site,std::uint32_t offset,std::uint32_t value){
    std::array<std::byte,4>bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
    SourcePhysicsContactRecordStore event{site,info.identity+offset,{},{}};
    if(observe){event.before.resize(4);memory.read(event.address,event.before);event.written.assign(bytes.begin(),bytes.end());}
    memory.write(event.address,bytes);if(observe)observe(event);
  };
  store(0x31d634u,0u,0u);store(0x31d63au,8u,0x44u);
  store(0x31d641u,4u,0x110u);store(0x31d648u,12u,7u);
  return info.identity;
}
} // namespace halo2
