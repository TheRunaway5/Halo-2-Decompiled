#include "halo2/native_tu15_array_growth.hpp"
#include <stdexcept>
namespace halo2 {
void native_tu15_copy_array_bytes(NativeTu15ArrayCopyStorage&s,std::uint32_t destination,
  std::uint32_t source,std::uint32_t bytes,NativeTu15ArrayGrowthActivity*activity){
  auto words=bytes>>2u;
  while(words){const auto value=s.copy_word(source);s.copy_word(destination,value);
    source+=4u;destination+=4u;--words;if(activity)++activity->copied_words;}
  auto tail=bytes&3u;
  while(tail){const auto value=s.copy_byte(source);s.copy_byte(destination,value);
    ++source;++destination;--tail;if(activity)++activity->copied_bytes;}
}
void native_tu15_grow_array(NativeTu15ArrayGrowthStorage&s,std::uint32_t stride,
  NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&services,
  NativeTu15ArrayGrowthActivity*activity){
  const auto initial_count=s.count(NativeTu15ArrayCountRead::initial);
  const auto next_capacity=initial_count?initial_count+initial_count:1u;
  const auto allocator=s.allocator(false);const auto allocation_bytes=next_capacity*stride;
  const NativeTu15ArrayAllocationRequest request{allocator.receiver,s.allocate_owner(allocator),allocation_bytes,0x12};
  if(!services.allocate)throw std::runtime_error("Native TU1.5 array requires its actual allocation owner");
  const auto replacement=services.allocate(request,fp);if(activity)++activity->allocations;
  const auto old_data=s.data(NativeTu15ArrayDataRead::copy_source);
  const auto copy_count=s.count(NativeTu15ArrayCountRead::after_allocation);
  native_tu15_copy_array_bytes(s,replacement,old_data,copy_count*stride,activity);
  const auto capacity=s.capacity_flags();
  if(!(capacity&0x80000000u)){
    const auto releasing=s.allocator(true);const auto release_bytes=(capacity&0x7fffffffu)*stride;
    const auto released=s.data(NativeTu15ArrayDataRead::release_source);
    const NativeTu15ArrayReleaseRequest release{releasing.receiver,s.release_owner(releasing),released,release_bytes,0x12};
    if(!services.release)throw std::runtime_error("Native TU1.5 array requires its actual release owner");
    services.release(release,fp);if(activity)++activity->releases;
  }
  s.publish_data(replacement);s.publish_capacity(next_capacity);if(activity)++activity->completed_growths;
}
NativeTu15ArrayGrowthMemory::NativeTu15ArrayGrowthMemory(SourcePhysicsContactMemory&m,
  NativeTu15ArrayHeader header,const SourcePhysicsContactRecordObserver&observer):memory_(m),header_(header),observer_(observer){}
std::uint32_t NativeTu15ArrayGrowthMemory::read(std::uint32_t address,unsigned bytes)const{
  std::array<std::byte,4>raw{};memory_.read(address,std::span(raw).first(bytes));std::uint32_t value{};
  for(unsigned i=0;i<bytes;++i)value|=std::to_integer<unsigned>(raw[i])<<(i*8u);return value;
}
void NativeTu15ArrayGrowthMemory::write(std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned bytes){
  std::array<std::byte,4>raw{};for(unsigned i=0;i<bytes;++i)raw[i]=std::byte(value>>(i*8u));
  SourcePhysicsContactRecordStore store{site,address,{},{}};
  if(observer_){store.before.resize(bytes);memory_.read(address,store.before);store.written.assign(raw.begin(),raw.begin()+bytes);}
  memory_.write(address,std::span(raw).first(bytes));if(observer_)observer_(store);
}
std::uint32_t NativeTu15ArrayGrowthMemory::count(NativeTu15ArrayCountRead)const{return read(header_.identity+4u);}
std::uint32_t NativeTu15ArrayGrowthMemory::data(NativeTu15ArrayDataRead)const{return read(header_.identity);}
std::uint32_t NativeTu15ArrayGrowthMemory::capacity_flags()const{return read(header_.identity+8u);}
NativeTu15ArrayAllocator NativeTu15ArrayGrowthMemory::allocator(bool)const{const auto receiver=read(0x48c938u);return{receiver,read(receiver)};}
std::uint32_t NativeTu15ArrayGrowthMemory::allocate_owner(NativeTu15ArrayAllocator owner)const{return read(owner.method_table+0x10u);}
std::uint32_t NativeTu15ArrayGrowthMemory::release_owner(NativeTu15ArrayAllocator owner)const{return read(owner.method_table+0x14u);}
std::uint32_t NativeTu15ArrayGrowthMemory::copy_word(std::uint32_t p)const{return read(p);}
std::uint8_t NativeTu15ArrayGrowthMemory::copy_byte(std::uint32_t p)const{return std::uint8_t(read(p,1));}
void NativeTu15ArrayGrowthMemory::copy_word(std::uint32_t p,std::uint32_t v){write(0x2e2e43u,p,v);}
void NativeTu15ArrayGrowthMemory::copy_byte(std::uint32_t p,std::uint8_t v){write(0x2e2e4au,p,v,1);}
void NativeTu15ArrayGrowthMemory::publish_data(std::uint32_t p){write(0x2e32b3u,header_.identity,p);}
void NativeTu15ArrayGrowthMemory::publish_capacity(std::uint32_t capacity){write(0x2e32b6u,header_.identity+8u,capacity);}
} // namespace halo2
