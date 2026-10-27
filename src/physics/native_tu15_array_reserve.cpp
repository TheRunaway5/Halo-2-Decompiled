#include "halo2/native_tu15_array_reserve.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
void publish(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&observe,
  std::uint32_t instruction,std::uint32_t address,std::uint32_t value){
  std::array<std::byte,4>bytes{};
  for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));
  SourcePhysicsContactRecordStore store{instruction,address,{},{}};
  if(observe){store.before.resize(4);m.read(address,store.before);store.written.assign(bytes.begin(),bytes.end());}
  m.write(address,bytes);if(observe)observe(store);
}
}
void native_tu15_reserve_array(SourcePhysicsContactMemory&memory,
  const NativeTu15ArrayReserveArguments&arguments,NativeTu15PhysicsFrameFp&fp,
  const NativeTu15ArrayGrowthServices&services,const SourcePhysicsContactRecordObserver&observe,
  NativeTu15ArrayGrowthActivity*activity){
  NativeTu15ArrayGrowthMemory allocator_input(memory,{},observe);
  const auto allocator=allocator_input.allocator(false);
  if(!arguments.stride||!arguments.capacity||!arguments.header)
    throw std::runtime_error("Native TU1.5 array reserve requires its actual input cells");
  const auto stride=arguments.stride();
  const auto capacity=arguments.capacity();
  const NativeTu15ArrayAllocationRequest request{allocator.receiver,allocator_input.allocate_owner(allocator),capacity*stride,0x12};
  if(!services.allocate)throw std::runtime_error("Native TU1.5 array reserve requires its actual allocation owner");
  const auto replacement=services.allocate(request,fp);if(activity)++activity->allocations;
  const auto header=arguments.header();
  NativeTu15ArrayGrowthMemory s(memory,header,observe);
  const auto source=s.data(NativeTu15ArrayDataRead::copy_source);
  const auto count=s.count(NativeTu15ArrayCountRead::after_allocation);
  native_tu15_copy_array_bytes(s,replacement,source,count*stride,activity);
  const auto flags=s.capacity_flags();
  if(!(flags&0x80000000u)){
    const auto releasing=s.allocator(true);
    const auto bytes=(flags&0x7fffffffu)*stride;
    const auto released=s.data(NativeTu15ArrayDataRead::release_source);
    const NativeTu15ArrayReleaseRequest release{releasing.receiver,s.release_owner(releasing),released,bytes,0x12};
    if(!services.release)throw std::runtime_error("Native TU1.5 array reserve requires its actual release owner");
    services.release(release,fp);if(activity)++activity->releases;
  }
  publish(memory,observe,0x2e3238u,header.identity,replacement);
  publish(memory,observe,0x2e323bu,header.identity+8u,capacity);
  if(activity)++activity->completed_growths;
}
void native_tu15_reserve_array(SourcePhysicsContactMemory&memory,NativeTu15ArrayHeader header,
  std::uint32_t capacity,std::uint32_t stride,NativeTu15PhysicsFrameFp&fp,
  const NativeTu15ArrayGrowthServices&services,const SourcePhysicsContactRecordObserver&observe,
  NativeTu15ArrayGrowthActivity*activity){
  native_tu15_reserve_array(memory,NativeTu15ArrayReserveArguments{
    [stride]{return stride;},[capacity]{return capacity;},[header]{return header;}},
    fp,services,observe,activity);
}
} // namespace halo2
