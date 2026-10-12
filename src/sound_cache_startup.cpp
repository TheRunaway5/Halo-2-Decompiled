#include "halo2/sound_cache_startup.hpp"
#include <bit>
#include <stdexcept>
#include <string_view>

namespace halo2 {
namespace {
template<class F> const F &need(const F &f,const char *message) {
  if(!f)throw std::runtime_error(message);
  return f;
}
class Owner {
  SoundCacheStartupState &s;
  const SoundCacheStartupConstants &c;
  const SoundCacheStartupServices &services;
  const SoundCacheStartupObserver &observe;
  std::span<std::byte> memory(std::uint32_t address,std::size_t extent)const {
    auto bytes=need(services.memory,"Original sound cache raw storage unavailable")(address,extent);
    if(bytes.size()!=extent)throw std::runtime_error("Original sound cache raw storage extent unavailable");
    return bytes;
  }
  std::uint32_t read(std::uint32_t address,unsigned extent)const {
    const auto bytes=memory(address,extent);std::uint32_t out=0;
    for(unsigned i=0;i<extent;++i)out|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return out;
  }
  void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned extent=4) {
    const auto before=read(address,extent);auto bytes=memory(address,extent);
    for(unsigned i=0;i<extent;++i)bytes[i]=std::byte(value>>(8*i));
    if(observe)observe({pc,address,extent,before,value&(extent==4?0xffffffffu:((1u<<(8*extent))-1))});
  }
  void global(std::uint32_t pc,std::uint32_t address,std::uint32_t &slot,std::uint32_t value) {
    const auto before=slot;slot=value;if(observe)observe({pc,address,4,before,value});
  }
  void clear(std::uint32_t pc,std::uint32_t address,std::uint32_t dwords) {
    for(std::uint32_t i=0;i<dwords;++i)write(pc,address+4*i,0);
  }
  void name(std::uint32_t address,std::string_view value) {
    // Source strncpy32 stores are explicitly represented as a C primitive,
    // not assigned different original helper instruction identities.
    for(unsigned i=0;i<32;++i)write(0,address+i,i<value.size()?std::uint8_t(value[i]):0,1);
  }
  void pool(std::uint32_t header,std::string_view label,std::uint32_t capacity,
            std::uint32_t stride,std::uint32_t allocator) {
    // All constructor calls here use actual source alignment shift0.
    const auto elements=header+0x4c,bitmap=elements+capacity*stride;
    clear(0x16d0da,header,0x13);name(header,label);
    write(0x16d0f1,header+0x2a,read(header+0x2a,1)|3,1);
    write(0x16d0f7,header+0x1f,0,1);write(0x16d0fa,header+0x20,capacity);
    write(0x16d101,header+0x24,stride);write(0x16d108,header+0x44,0);
    write(0x16d10b,header+0x29,0,1);write(0x16d115,header+0x28,0,1);
    write(0x16d118,header+0x2c,0x64407440);write(0x16d11f,header+0x48,bitmap);
    write(0x16d122,header+0x30,allocator);
    write(0x16d0a0,header+0x2a,read(header+0x2a,1)&0xfc,1);
    write(0x16d0af,header+0x44,elements);clear(0x16d0ba,bitmap,(capacity+31)>>5);
  }
  void reset(std::uint32_t header) {
    write(0x16d228,header+0x38,0);write(0x16d22f,header+0x3c,0);
    write(0x16d236,header+0x34,0);write(0,header+0x40,read(header,2),2);
    write(0x16d245,header+0x40,read(header+0x40,2)|0x8000,2);
    const auto capacity=read(header+0x20,4),stride=read(header+0x24,4),elements=read(header+0x44,4);
    // This constructor just wrote flags0. Unsupported raw helper/debug input
    // cannot be smuggled in as a different source cache construction path.
    if(read(header+0x2a,1)&8)throw std::runtime_error("Unexpected source cache pool debug flag");
    for(std::uint32_t i=0;i<capacity;++i)write(0x16d289,elements+i*stride,0,2);
    clear(0x16d2ad,read(header+0x48,4),(capacity+31)>>5);
  }
  void pages(std::uint32_t header,std::uint32_t general_allocator) {
    const auto embedded=header+0x70;
    pool(embedded,"xbox sound cache",512,0x18,s.static_allocator_identity);
    write(0x13e90c,embedded+0x29,1,1);reset(embedded);
    clear(0x13e925,header,0x1c);name(header,"xbox sound cache");
    write(0x13e934,header+0x20,c.delete_callback_identity);
    write(0x13e93b,header+0x28,0);write(0x13e942,header+0x34,0x0e);
    write(0x13e945,header+0x24,c.busy_callback_identity);
    write(0x13e94c,header+0x64,embedded);write(0x13e94f,header+0x38,1);
    write(0x13e952,header+0x30,0xc0);write(0x13e955,header+0x68,0x77656565);
    write(0x13e961,header+0x1f,0,1);write(0x13e964,header+0x2c,0);
    write(0x13e96b,header+0x6c,general_allocator);
    write(0x13e971,header+0x3c,0xffffffffu);write(0x13e974,header+0x40,0xffffffffu);
    for(unsigned i=0;i<8;++i)write(0x13e97e + 3*i,header+0x44+4*i,0x7fffffffu);
  }
public:
  Owner(SoundCacheStartupState &state,const SoundCacheStartupConstants &constants,
        const SoundCacheStartupServices &providers,const SoundCacheStartupObserver &observer)
      :s(state),c(constants),services(providers),observe(observer) {}
  void run() {
    const auto first_owner=s.general_allocator_identity;
    const auto first=need(services.raw_allocate,"Original sound cache general allocator unavailable")(
        first_owner,0x208c);
    if(first) {
      pool(first,"xbox sound",512,0x10,first_owner);
      write(0x21a712,first+0x2a,read(first+0x2a,1)|4,1);
    }
    const auto second_owner=s.general_allocator_identity;
    global(0x21a724,0x5098dc,s.entry_pool,first);
    const auto second=need(services.raw_allocate,"Original sound cache general allocator unavailable")(
        second_owner,0x30fc);
    // Original13E8E0 dereferences its second allocation unconditionally.
    // Missing backing stops on that exact address, never creates fake pages.
    pages(second,second_owner);write(0x21a754,second+0x2c,2);
    const auto index=s.physical_arena_index;
    if(index>=s.physical_upper_limits.size())throw std::runtime_error("Original sound cache physical index backing unavailable");
    const auto candidate=s.physical_upper_limits[index]-0x300000u;
    const auto lower=s.physical_lower_limits[index];
    global(0x21a776,0x5098e4,s.page_allocator,second);
    std::uint32_t buffer=0;
    if(std::bit_cast<std::int32_t>(candidate)>=std::bit_cast<std::int32_t>(lower)) {
      global(0x21a789,0x4eda10+4*index,s.physical_upper_limits[index],candidate);
      if(candidate) {
        buffer=candidate|0x80000000u;
        if(buffer)need(services.raw_protect,"Original sound cache physical protection unavailable")(buffer,0x300000,4);
      }
    }
    global(candidate==0&&std::bit_cast<std::int32_t>(candidate)>=std::bit_cast<std::int32_t>(lower)?
           0x21a7ad:0x21a7a4,0x5098e0,s.physical_buffer,buffer);
  }
};
}
void source_sound_cache_startup(SoundCacheStartupState &state,
    const SoundCacheStartupConstants &constants,const SoundCacheStartupServices &services,
    const SoundCacheStartupObserver &observer) {
  Owner(state,constants,services,observer).run();
}
} // namespace halo2
