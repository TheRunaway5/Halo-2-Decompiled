#include "halo2/native_tu15_extended_constraint_info.hpp"
namespace halo2 {
namespace {
std::uint32_t extended_info(SourcePhysicsContactMemory&memory,
    std::uint32_t constraint,NativeTu15IslandConstraintInfo info,
    std::uint32_t site,std::uint32_t extension,
    const SourcePhysicsContactRecordObserver&observe){
  native_tu15_get_contact_collector_info(memory,constraint,info,observe);
  std::array<std::byte,4>bytes{};memory.read(info.identity+8u,bytes);std::uint32_t value{};
  for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
  value+=extension;SourcePhysicsContactRecordStore event{site,info.identity+8u,{},{}};
  if(observe){event.before.assign(bytes.begin(),bytes.end());event.written.resize(4);}
  for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
  if(observe)event.written.assign(bytes.begin(),bytes.end());memory.write(info.identity+8u,bytes);if(observe)observe(event);
  return info.identity;
}
}
std::uint32_t native_tu15_get_extended_constraint_info(SourcePhysicsContactMemory&memory,
    std::uint32_t constraint,NativeTu15IslandConstraintInfo info,
    const SourcePhysicsContactRecordObserver&observe){
  return extended_info(memory,constraint,info,0x31f84bu,0x88u,observe);
}
std::uint32_t native_tu15_get_conveyor_constraint_info(SourcePhysicsContactMemory&memory,
    std::uint32_t constraint,NativeTu15IslandConstraintInfo info,
    const SourcePhysicsContactRecordObserver&observe){
  return extended_info(memory,constraint,info,0x31f74bu,0x28u,observe);
}
} // namespace halo2
