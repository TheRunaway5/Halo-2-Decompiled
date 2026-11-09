#include "halo2/native_tu15_array_reserve.hpp"
#include <stdexcept>
#include <optional>
namespace halo2 {
void native_tu15_reserve_array_in_storage(NativeTu15ArrayGrowthStorage&allocator_input,
  const NativeTu15ArrayReserveArguments&arguments,
  const std::function<NativeTu15ArrayGrowthStorage&(NativeTu15ArrayHeader)>& storage,
  NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&services,
  NativeTu15ArrayGrowthActivity*activity){
  const auto allocator=allocator_input.allocator(false);
  if(!arguments.stride||!arguments.capacity||!arguments.header)
    throw std::runtime_error("Native TU1.5 array reserve requires its actual input cells");
  const auto stride=arguments.stride();
  const auto capacity=arguments.capacity();
  const NativeTu15ArrayAllocationRequest request{allocator.receiver,allocator_input.allocate_owner(allocator),capacity*stride,0x12};
  if(!services.allocate)throw std::runtime_error("Native TU1.5 array reserve requires its actual allocation owner");
  const auto replacement=services.allocate(request,fp);if(activity)++activity->allocations;
  const auto header=arguments.header();
  auto &s=storage(header);
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
  s.publish_data_at(0x2e3238u,replacement);
  s.publish_capacity_at(0x2e323bu,capacity);
  if(activity)++activity->completed_growths;
}
void native_tu15_reserve_array(SourcePhysicsContactMemory&memory,
  const NativeTu15ArrayReserveArguments&arguments,NativeTu15PhysicsFrameFp&fp,
  const NativeTu15ArrayGrowthServices&services,const SourcePhysicsContactRecordObserver&observe,
  NativeTu15ArrayGrowthActivity*activity){
  NativeTu15ArrayGrowthMemory allocator_input(memory,{},observe);
  std::optional<NativeTu15ArrayGrowthMemory> selected;
  native_tu15_reserve_array_in_storage(allocator_input,arguments,[&](NativeTu15ArrayHeader header)->NativeTu15ArrayGrowthStorage&{
    selected.emplace(memory,header,observe);return *selected;
  },fp,services,activity);
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
