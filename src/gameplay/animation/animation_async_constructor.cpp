#include "halo2/animation_async_constructor.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourceAnimationAsyncConstructionServices &s,
                   std::uint32_t address) {
  if(!s.memory.read)throw std::runtime_error("Original async storage unavailable");
  const auto raw=s.memory.read(address,4);
  if(raw.size()<4)throw std::runtime_error("Truncated original async storage");
  std::uint32_t v=0;
  for(unsigned i=0;i<4;++i)v|=std::uint32_t(std::to_integer<unsigned>(raw[i]))<<(i*8);
  return v;
}
void store(const SourceAnimationAsyncConstructionServices &s,
           std::uint32_t address,std::uint32_t value,std::uint32_t instruction,
           const AnimationResourceObserver &observer) {
  const auto before=word(s,address);
  if(!s.memory.write)throw std::runtime_error("Mutable original async storage unavailable");
  std::array<std::byte,4> raw;
  for(unsigned i=0;i<4;++i)raw[i]=std::byte((value>>(i*8))&255);
  s.memory.write(address,raw);
  if(observer)observer({instruction,address,before,word(s,address)});
}
} // namespace
void construct_source_animation_async_queue(
    const SourceAnimationAsyncConstructionServices &s,
    const AnimationResourceObserver &observer) {
  const auto &b=s.bindings;
  for(unsigned i=0;i<0x960;++i)store(s,b.nodes+i*4,0,0x1221bd,observer);
  for(unsigned i=0;i<150;++i) {
    store(s,b.nodes+i*64+0x3c,b.nodes+(i+1)*64,0x1221cb,observer);
    store(s,b.nodes+i*64+4,0xffffffffu,0x1221cd,observer);
  }
  store(s,b.nodes+149*64+0x3c,0,0x1221e0,observer);
  store(s,b.free_head,b.nodes,0x1221ea,observer);
  store(s,b.work_head,0,0x1221f4,observer);
  if(!s.native_create_mutex)throw std::runtime_error("Native XAPI mutex creation owner unavailable");
  const auto free_mutex=s.native_create_mutex(0,0,0);
  store(s,b.free_mutex_slot,free_mutex,0x122209,observer);
  const auto work_mutex=s.native_create_mutex(0,0,0);
  store(s,b.work_mutex_slot,work_mutex,0x12221e,observer);
  if(!s.native_create_semaphore)throw std::runtime_error("Native XAPI semaphore creation owner unavailable");
  const auto semaphore=s.native_create_semaphore(0,0,150,0);
  store(s,b.semaphore_slot,semaphore,0x12223a,observer);
  store(s,b.worker_storage_descriptor,0,0x12223f,observer);
  store(s,b.worker_storage_descriptor+4,b.worker_storage,0x122249,observer);
  store(s,b.worker_storage_descriptor+8,0x8000,0x122253,observer);
  if(!s.native_create_worker)throw std::runtime_error("Native worker and source callback/TLS owner unavailable");
  const auto worker=s.native_create_worker(0,0x4000,b.worker_entry,0,0,0);
  store(s,b.worker_slot,worker,0x122265,observer);
  if(!s.native_set_worker_priority)throw std::runtime_error("Native worker priority owner unavailable");
  s.native_set_worker_priority(worker,1);
}
} // namespace halo2
