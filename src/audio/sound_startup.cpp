#include "halo2/sound_startup.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace halo2 {
namespace {
template<class F> const F &need(const F &f,const char *message) {
  if(!f)throw std::runtime_error(message);
  return f;
}
class Owner {
  SoundStartupState &s;
  const SoundStartupConstants &c;
  const SoundStartupServices &services;
  const SoundStartupObserver &observe;
  std::span<std::byte> memory(std::uint32_t address,std::size_t size) const {
    auto result=need(services.memory,"Original sound startup raw storage unavailable")(address,size);
    if(result.size()!=size)throw std::runtime_error("Original sound startup raw storage extent unavailable");
    return result;
  }
  std::uint32_t read(std::uint32_t address,unsigned size) const {
    const auto bytes=memory(address,size);std::uint32_t value=0;
    for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return value;
  }
  void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned size=4) {
    const auto before=read(address,size);auto bytes=memory(address,size);
    for(unsigned i=0;i<size;++i)bytes[i]=std::byte(value>>(8*i));
    if(observe)observe({pc,address,size,before,value& (size==4?0xffffffffu:((1u<<(8*size))-1))});
  }
  void global(std::uint32_t pc,std::uint32_t address,std::uint32_t &field,std::uint32_t value) {
    const auto before=field;field=value;if(observe)observe({pc,address,4,before,value});
  }
  void clear(std::uint32_t pc,std::uint32_t address,std::uint32_t size) {
    for(std::uint32_t i=0;i<size;i+=4)write(pc,address+i,0,std::min(4u,size-i));
  }
  std::uint32_t allocate(std::uint32_t size,std::uint32_t cursor_pc,
                         const std::function<void()> &before_cursor={}) {
    const auto position=std::uint64_t(s.arena_base)+s.arena_used;
    const auto aligned=(position+3)&~std::uint64_t(3);
    const auto extent=aligned-position+size;
    if(aligned>std::numeric_limits<std::uint32_t>::max() ||
       std::uint64_t(s.arena_used)+extent>std::numeric_limits<std::uint32_t>::max())
      throw std::runtime_error("Original sound static arena exceeds native address backing");
    // Bounds are a host storage boundary, not an original allocation result.
    memory(std::uint32_t(aligned),size);
    if(before_cursor)before_cursor();
    global(cursor_pc,0x517ff4,s.arena_used,s.arena_used+std::uint32_t(extent));
    auto crc=s.arena_crc;
    for(unsigned byte=0;byte<4;++byte) {
      auto table=(crc^std::uint32_t(extent>>(8*byte)))&255;
      for(unsigned bit=0;bit<8;++bit)table=(table>>1)^((table&1)?0xedb88320u:0u);
      crc=(crc>>8)^table;
    }
    global(0x1656ed,0x517ff8,s.arena_crc,crc);
    return std::uint32_t(aligned);
  }
  std::uint32_t pool(std::string_view name,std::uint32_t capacity,std::uint32_t stride) {
    const auto size=0x4c+capacity*stride+((capacity+31)>>5)*4;
    const auto header=allocate(size,0x11e385),elements=header+0x4c;
    clear(0x16d0da,header,0x4c);
    //16D0D0 strncpy32 then forces byte31 NUL. The particular C helper's
    // stores are represented as byte copies, not a different pool initializer.
    for(unsigned i=0;i<32;++i)write(0,header+i,i<name.size()?std::uint8_t(name[i]):0,1);
    write(0x16d0f1,header+0x2a,3,1);write(0x16d0f7,header+31,0,1);
    write(0x16d0fa,header+0x20,capacity);write(0x16d101,header+0x24,stride);
    write(0x16d108,header+0x44,0);write(0x16d10b,header+0x29,0,1);
    write(0x16d115,header+0x28,0,1);write(0x16d118,header+0x2c,0x64407440);
    write(0x16d11f,header+0x48,elements+capacity*stride);
    write(0x16d122,header+0x30,s.allocator_identity);
    write(0x16d0a0,header+0x2a,read(header+0x2a,1)&0xfc,1);
    write(0x16d0af,header+0x44,elements);
    clear(0x16d0ba,elements+capacity*stride,((capacity+31)>>5)*4);
    return header;
  }
  void reset(std::uint32_t header,std::uint32_t valid_pc) {
    write(valid_pc,header+0x29,1,1);
    write(0x16d228,header+0x38,0);write(0x16d22f,header+0x3c,0);
    write(0x16d236,header+0x34,0);
    write(0,header+0x40,read(header,2),2);
    write(0x16d245,header+0x40,read(header+0x40,2)|0x8000,2);
    const auto capacity=read(header+0x20,4),stride=read(header+0x24,4),elements=read(header+0x44,4);
    if(read(header+0x2a,1)&8)throw std::runtime_error("Unexpected debug-poison branch in source-owned sound static pool");
    for(std::uint32_t i=0;i<capacity;++i)write(0x16d289,elements+i*stride,0,2);
    clear(0x16d2ad,read(header+0x48,4),((capacity+31)>>5)*4);
  }
  bool controllers() {
    const auto p=pool("sound playback controllers",64,0x1c);
    const auto root=allocate(0x1c,0x21bc9c,[&]{global(0x21bc96,0x526468,s.controllers,p);});
    global(0x21bcab,0x52646c,s.controller_root,root);
    if(!p || !root)return false;
    reset(p,0x21bcb6);
    const auto sound_flag=read(s.system+0x7f,1);
    const auto time=need(services.utc_seconds,"Original sound controller time owner unavailable")();
    const auto random=need(services.crt_random,"Original sound controller CRT RNG owner unavailable")();
    const auto tick=need(services.tick_milliseconds,"Original sound controller tick owner unavailable")();
    write(0x21bcf6,root+8,random^tick^time);
    write(0x21bcff,root+4,0xffffffffu);
    write(0x21bd06,root+2,((~sound_flag<<7)&255)|(read(root+2,1)&0x7f),1);
    write(0x21bd09,root+3,0x80,1);
    write(0x21bd12,root+0xc,0);write(0x21bd14,root+0x10,0);
    write(0x21bd17,root+0x14,0);write(0x21bd1a,root+0x18,0);
    return true;
  }
  bool simple_pool(std::string_view name,std::uint32_t capacity,std::uint32_t stride,
                   std::uint32_t &slot,std::uint32_t global_pc,std::uint32_t global_va,
                   std::uint32_t valid_pc) {
    const auto p=pool(name,capacity,stride);global(global_pc,global_va,slot,p);
    if(!p)return false;
    reset(p,valid_pc);
    return true;
  }
  void commit() {
    clear(0x12c731,s.system+0x60,16);clear(0x12c731,s.system+0x70,8);
    auto slot=std::uint16_t(0);
    for(unsigned category=0;category<3;++category) {
      const auto bytes=std::span(c.voice_policy).subspan(6+2*category,2);
      const auto count=std::bit_cast<std::int16_t>(std::uint16_t(std::to_integer<unsigned>(bytes[0])|
                              (std::to_integer<unsigned>(bytes[1])<<8)));
      write(0x126cd6,s.system+0x208,read(s.system+0x208,2)+std::uint16_t(count),2);
      for(std::int16_t i=0;i<count;++i) {
        const auto voice=s.static_voices+std::uint32_t(std::bit_cast<std::int16_t>(slot))*0x24;
        ++slot;write(0x126cf7,voice,0xffffffffu);write(0x126cf9,voice+8,category,1);
        write(0x126cfd,voice+0xa,0,1);write(0x126d00,voice+0xb,0,1);
        write(0x126d03,voice+0xc,0xffff,2);write(0x126d07,voice+0xe,0xffff,2);
        write(0x126d0b,voice+4,0xffffffffu);write(0x126d0e,voice+0x1c,0);
        write(0x126d11,voice+0x20,0);write(0x126d14,voice+9,0,1);
      }
    }
    write(0x126d2c,s.system+0x20a,0x33,2);
    write(0x126d35,s.system+0x78,1,1);
  }
public:
  Owner(SoundStartupState &state,const SoundStartupConstants &constants,
        const SoundStartupServices &providers,const SoundStartupObserver &observer)
      :s(state),c(constants),services(providers),observe(observer) {}
  void commit_fragment() {commit();}
  void run() {
    if(!s.references) {
      const auto system=allocate(0x20c,0x126b65);
      clear(0x126b79,system,0x20c);global(0x126b7b,0x4ed950,s.system,system);
      write(0x126b81,system+0x79,1,1);write(0x126b85,system+0x7d,0,1);
      need(services.cache_initialize,"Original sound cache constructor unavailable")(s);
      write(0x126ba0,s.system+0x1b0,0xffffffffu);write(0x126ba6,s.system+0x1b4,0);
      write(0x126bae,s.system+0x1b8,c.ambient_transition_bits);
      write(0x126bbb,s.system+0x1cc,0xffffffffu);write(0x126bc1,s.system+0x1d0,0);
      write(0x126bc9,s.system+0x1d4,0);
      const auto sources=pool("sound sources",384,0xbc);
      const auto voices=allocate(0x1200,0x126c19,[&]{global(0x126c13,0x4ed94c,s.sources,sources);});
      global(0x126c26,0x4ed948,s.static_voices,voices);
      if(sources && voices) {
        reset(sources,0x126c3a);clear(0x126c50,s.static_voices,0x1200);
        if(controllers() &&
           simple_pool("looping sounds",128,0xd4,s.looping,0x21c1e6,0x5098e8,0x21c1f0) &&
           simple_pool("sounds effects",16,0x14,s.effects,0x21f5b3,0x526470,0x21f5bd)) {
          const auto count=std::bit_cast<std::int16_t>(std::uint16_t(
              std::to_integer<unsigned>(c.voice_policy[12])|
              (std::to_integer<unsigned>(c.voice_policy[13])<<8)));
          if(count<=0)throw std::runtime_error("Unsupported source sound voice pool capacity");
          if(simple_pool("sound voices",std::uint32_t(count),0x6c,s.shared_voices,
                         0x21fec5,0x5098ec,0x21fecf) &&
             need(services.native_platform_initialize,
                  "Native sound platform constructor unavailable")(c.voice_policy))commit();
        }
      }
    }
    global(0x126d3c,0x4ed944,s.references,s.references+1);
  }
};
}
void source_sound_startup(SoundStartupState &state,const SoundStartupConstants &constants,
                         const SoundStartupServices &services,const SoundStartupObserver &observer) {
  Owner(state,constants,services,observer).run();
}
void source_sound_startup_commit_fragment(SoundStartupState &state,
    const SoundStartupConstants &constants,const SoundStartupServices &services,
    const SoundStartupObserver &observer) {
  Owner(state,constants,services,observer).commit_fragment();
}
} // namespace halo2
