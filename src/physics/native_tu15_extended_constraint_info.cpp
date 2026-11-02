#include "halo2/native_tu15_extended_constraint_info.hpp"
namespace halo2 {
std::uint32_t native_tu15_get_extended_constraint_info(SourcePhysicsContactMemory&memory,
    std::uint32_t constraint,NativeTu15IslandConstraintInfo info,
    const SourcePhysicsContactRecordObserver&observe){
  native_tu15_get_contact_collector_info(memory,constraint,info,observe);
  std::array<std::byte,4>bytes{};memory.read(info.identity+8u,bytes);std::uint32_t value{};
  for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
  value+=0x88u;SourcePhysicsContactRecordStore event{0x31f84bu,info.identity+8u,{},{}};
  if(observe){event.before.assign(bytes.begin(),bytes.end());event.written.resize(4);}
  for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
  if(observe)event.written.assign(bytes.begin(),bytes.end());memory.write(info.identity+8u,bytes);if(observe)observe(event);
  return info.identity;
}
} // namespace halo2
