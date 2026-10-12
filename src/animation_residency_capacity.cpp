#include "halo2/animation_residency_capacity.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
void require(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
std::uint32_t word(const SourceAnimationCapacityServices &s,
                   std::uint32_t address, unsigned bytes = 4) {
  require(bool(s.memory.read), "Original animation capacity backing unavailable");
  auto b=s.memory.read(address,bytes);
  require(b.size()>=bytes,"Truncated original animation capacity backing");
  std::uint32_t v=0;
  for(unsigned i=0;i<bytes;++i)
    v|=std::uint32_t(std::to_integer<unsigned>(b[i]))<<(i*8);
  return v;
}
std::int32_t signed_word(std::uint32_t v) {return std::bit_cast<std::int32_t>(v);}
void store(const SourceAnimationCapacityServices &s, std::uint32_t address,
           std::uint32_t value, std::uint32_t instruction,
           const AnimationResourceObserver &o) {
  const auto before=word(s,address);
  require(bool(s.memory.write),"Original mutable animation capacity backing unavailable");
  std::array<std::byte,4> b;
  for(unsigned i=0;i<4;++i)b[i]=std::byte((value>>(i*8))&255);
  s.memory.write(address,b);
  if(o)o({instruction,address,before,word(s,address)});
}
} // namespace
void resize_source_animation_cache(
    std::uint32_t cache, std::uint32_t capacity,
    const SourceAnimationCapacityServices &s,
    const AnimationResourceObserver &o) {
  const auto pool=word(s,cache+0x64);
  std::uint32_t index=0xffffffffu;
  for(;;) {
    auto next=index+1u;
    if(signed_word(next)<0) {
      store(s,cache+0x30,capacity,0x13f09d,o);return;
    }
    const auto high=word(s,pool+0x38);
    if(signed_word(next)>=signed_word(high)) {
      store(s,cache+0x30,capacity,0x13f0ab,o);return;
    }
    const auto bitmap=word(s,pool+0x48);
    while(!(word(s,bitmap+(next>>5)*4)&(1u<<(next&31)))) {
      ++next;
      if(signed_word(next)>=signed_word(high)) {
        store(s,cache+0x30,capacity,0x13f060,o);return;
      }
    }
    if(next==0xffffffffu) {
      store(s,cache+0x30,capacity,0x13f060,o);return;
    }
    const auto stride=word(s,pool+0x24);
    const auto elements=word(s,pool+0x44);
    const auto slot=elements+stride*next;
    const auto datum=(word(s,slot,2)<<16)|next;
    const auto offset=word(s,slot+8);
    const auto count=word(s,slot+4);
    const auto end=offset+count;
    index=next;
    if(signed_word(end)>signed_word(capacity)) {
      require(bool(s.retire),"Original13EFA0 resource retirement owner unavailable");
      s.retire(cache,datum);
    }
  }
}
void configure_source_animation_capacity(
    std::int32_t mode, const SourceAnimationCapacityServices &s,
    const AnimationResourceObserver &o) {
  const auto &b=s.bindings;
  if(!word(s,b.cache_pointer_slot))return;
  const auto bytes=mode>1 ? 0x300000u : 0x400000u;
  const auto blocks=mode>1 ? 0x600u : 0x800u;
  store(s,b.block_capacity_slot,blocks,mode>1?0x124999:0x1249aa,o);
  const auto index=word(s,b.physical_index);
  const auto lower=word(s,b.physical_lower_table+index*4);
  store(s,b.requested_bytes_slot,bytes,0x1249ca,o);
  const auto cursor=b.physical_upper_table+index*4;
  const auto rounded=(bytes+0xfffu)&0xfffff000u;
  const auto candidate=word(s,cursor)-rounded;
  std::uint32_t backing=0;
  if(signed_word(candidate)>=signed_word(lower)) {
    store(s,cursor,candidate,0x1249ed,o);
    backing=candidate;
    if(candidate) {
      backing=candidate|0x80000000u;
      if(backing) {
        require(bool(s.protect),"Original ordinal182 applied protection owner unavailable");
        s.protect(backing,rounded,4);
      }
    }
  }
  // Original reads blocks before the backing store, then rereads cache.
  const auto retained_blocks=word(s,b.block_capacity_slot);
  store(s,b.backing_pointer_slot,backing,0x124a0b,o);
  resize_source_animation_cache(word(s,b.cache_pointer_slot),retained_blocks,s,o);
}
} // namespace halo2
