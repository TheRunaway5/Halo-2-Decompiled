#include "halo2/sound_looping.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
float scalar(std::uint32_t x){return std::bit_cast<float>(x);}
std::int32_t integer(std::uint32_t x){return std::bit_cast<std::int32_t>(x);}
class Owner {
  const SoundLoopingBindings &b;
  const SoundLoopingServices &service;
  const SoundDriverStartupObserver &observe;
  std::span<std::byte> memory(std::uint32_t a,std::size_t n)const{
    if(!service.memory)throw std::runtime_error("Original looping-sound memory unavailable");
    auto out=service.memory(a,n);if(out.size()!=n)throw std::runtime_error("Original looping-sound memory extent unavailable");return out;
  }
  std::uint32_t read(std::uint32_t a,unsigned n=4)const{
    auto bytes=memory(a,n);std::uint32_t out=0;for(unsigned i=0;i<n;++i)out|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);return out;
  }
  void write(std::uint32_t pc,std::uint32_t a,std::uint32_t v,unsigned n=4)const{
    const auto before=read(a,n);auto bytes=memory(a,n);if(n<4)v&=(1u<<(8*n))-1;for(unsigned i=0;i<n;++i)bytes[i]=std::byte(v>>(8*i));if(observe)observe({pc,a,n,before,v});
  }
  void fill(std::uint32_t pc,std::uint32_t a,std::uint32_t count,std::uint32_t value=0)const{
    for(std::uint32_t i=0;i<count;++i)write(pc,a+4*i,value);
  }
  void checksum(std::uint32_t value)const{
    auto crc=read(b.arena_crc_pointer);for(unsigned i=0;i<4;++i){crc^=(value>>(8*i))&255;for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xedb88320u:0);}
    write(0x1656ed,b.arena_crc_pointer,crc);
  }
  void pool_name(std::uint32_t a,std::string_view name)const{
    // Actual caller uses a word-aligned immutable string and count32. These
    // stores retain that original strncpy primitive's bounded chunk sequence.
    if(name.size()>=32)throw std::runtime_error("Original loop pool name extent unavailable");
    for(unsigned i=0;i<8;++i){std::uint32_t value=0;for(unsigned j=0;j<4;++j)if(4*i+j<name.size())value|=std::uint32_t(std::uint8_t(name[4*i+j]))<<(8*j);
      const auto pc=4*i+4<=name.size()?0x326a15u:4*i>name.size()?0x326a79u:std::array{0x326a6bu,0x326a65u,0x326a5bu,0x326a51u}[name.size()%4];write(pc,a+4*i,value);}
  }
  void initialize_pool(std::uint32_t p,std::uint32_t allocator,std::string_view name)const{
    fill(0x16d0da,p,0x13);pool_name(p,name);
    write(0x16d0f1,p+0x2a,read(p+0x2a,1)|3,1);write(0x16d0f7,p+0x1f,0,1);write(0x16d0fa,p+0x20,1024);
    write(0x16d101,p+0x24,24);write(0x16d108,p+0x44,0);write(0x16d10b,p+0x29,0,1);write(0x16d115,p+0x28,0,1);
    write(0x16d118,p+0x2c,0x64407440);write(0x16d11f,p+0x48,p+0x604c);write(0x16d122,p+0x30,allocator);
    write(0x16d0a0,p+0x2a,read(p+0x2a,1)&0xfc,1);write(0x16d0af,p+0x44,p+0x4c);fill(0x16d0ba,p+0x604c,32);
  }
  void reset_pool(std::uint32_t p)const{
    write(0x16d228,p+0x38,0);write(0x16d22f,p+0x3c,0);write(0x16d236,p+0x34,0);
    // This fixed pool name starts with two nonzero bytes. Original strncpy2
    // performs byte stores at the actual scalar primitive instruction.
    write(0x3269c8,p+0x40,read(p,1),1);write(0x3269c8,p+0x41,read(p+1,1),1);
    write(0x16d245,p+0x40,read(p+0x40,2)|0x8000,2);
    const auto capacity=read(p+0x20),stride=read(p+0x24),elements=read(p+0x44);
    if(read(p+0x2a,1)&8){const auto bytes=capacity*stride;fill(0x16d268,elements,bytes>>2,0xbabababa);for(unsigned n=0;n<(bytes&3);++n)write(0x16d26f,elements+(bytes&~3u)+n,0xba,1);}
    for(std::int32_t i=0;i<integer(capacity);++i)write(0x16d289,elements+std::uint32_t(i)*stride,0,2);
    fill(0x16d2ad,read(p+0x48),(capacity+31)>>5);
  }
  std::uint32_t allocate(std::uint32_t p)const{
    auto cursor=integer(read(p+0x34));const auto high=integer(read(p+0x38));std::int32_t index=-1;
    if(cursor<high){const auto bitmap=read(p+0x48);while(cursor<integer(read(p+0x38))){if(!(read(bitmap+std::uint32_t(cursor>>5)*4)&(1u<<(std::uint32_t(cursor)&31)))){index=cursor;break;}++cursor;}}
    if(index==-1){if(high>=integer(read(p+0x20))||high==-1)return 0xffffffffu;index=high;}
    const auto u=std::uint32_t(index),element=read(p+0x44)+u*read(p+0x24),word=read(p+0x48)+std::uint32_t(index>>5)*4;
    write(0x16d32e,word,read(word)|(1u<<(u&31)));write(0x16d33c,p+0x3c,read(p+0x3c)+1);write(0x16d33f,p+0x34,u+1);
    if(integer(read(p+0x38))<=index)write(0x16d344,p+0x38,u+1);
    const auto size=read(p+0x24);fill(0x16d353,element,size>>2);for(unsigned i=0;i<(size&3);++i)write(0x16d35a,element+(size&~3u)+i,0,1);
    write(0x16d360,element,read(p+0x40,2),2);write(0x16d363,p+0x40,read(p+0x40,2)+1,2);if(read(p+0x40,2)==0xffff)write(0x16d36e,p+0x40,0x8000,2);
    return (read(element,2)<<16)|u;
  }
public:
  Owner(const SoundLoopingBindings &bindings,const SoundLoopingServices &services,const SoundDriverStartupObserver &observer):b(bindings),service(services),observe(observer){}
  void construct(const SoundLoopingConstants &c)const{
    const auto allocator=read(b.allocator_pointer),method=read(read(allocator));std::uint32_t pool;
    if(method==c.game_allocator_method){const auto used=read(b.arena_used_pointer);pool=read(b.arena_base_pointer)+used;write(0x125e7f,b.arena_used_pointer,used+0x60cc);checksum(0x60cc);}
    else{if(!service.source_allocate)throw std::runtime_error("Original loop alternate allocator owner unavailable");pool=service.source_allocate(method,allocator,0x60cc);}
    if(pool){initialize_pool(pool,allocator,c.pool_name);write(0x18a233,pool+0x2a,read(pool+0x2a,1)|4,1);}
    const auto used=read(b.arena_used_pointer),base=read(b.arena_base_pointer);write(0x18a243,b.pool_pointer,pool);write(0x18a251,b.arena_used_pointer,used+0x244);
    checksum(0x244);write(0x18a272,b.state_pointer,base+used);
  }
  void class_construct()const{
    const auto used=read(b.arena_used_pointer),p=read(b.arena_base_pointer)+used;write(0x223976,b.arena_used_pointer,used+0x360);checksum(0x360);fill(0x22399f,p,216);write(0x2239a2,b.class_fades_pointer,p);
  }
  void class_reset()const{
    const auto p=read(b.class_fades_pointer);write(0x2239bb,b.class_fades_flag,1,1);for(unsigned i=0;i<54;++i){write(0x2239d0,p+i*16+4,0);write(0x2239d3,p+i*16,0);write(0x2239d6,p+i*16+8,0);}
  }
  void class_gain(const SoundLoopingConstants &c,std::string_view selector,std::uint32_t gain,std::uint32_t seconds)const{
    for(unsigned i=0;i<c.class_names.size();++i)if(!c.class_names[i].empty()&&c.class_names[i].find(selector)!=std::string::npos){
      const auto p=read(b.class_fades_pointer)+i*16;auto clamped=gain;if(scalar(c.minimum_db_bits)>scalar(gain))clamped=0xc2800000;else if(scalar(gain)>0.f)clamped=0;
      const auto duration=0.f>scalar(seconds)?0:seconds;write(0x223af3,p,clamped);write(0x223b05,p+8,duration);if(scalar(seconds)==0.f)write(0x223b0d,p+4,clamped);
    }
  }
  void engine_reset(const SoundLoopingConstants &c)const{
    const auto engine=read(b.engine_pointer),index=read(engine+0xc14);if(!read(b.engine_table+index*4))return;
    // Complete21B900 on the unchanged caller's literal zero and imported one.
    // Both inputs are exactly representable; no scheduler/device state enters.
    class_gain(c,c.empty_selector,c.minimum_db_bits,0);class_gain(c,c.empty_selector,0,0x40000000);class_gain(c,c.dialogue_selector,0,0);
  }
  void reset(const SoundLoopingConstants &c,const SoundDriverMasterGainConstants &gain,const SoundDriverFloatingPointContext &fp)const{
    const auto p=read(b.pool_pointer);if(p){write(0x18a2b5,p+0x29,1,1);reset_pool(p);const auto s=read(b.state_pointer);
      constexpr std::array<std::uint32_t,8> pcs{0x18a2cc,0x18a2ce,0x18a2d1,0x18a2d4,0x18a2d7,0x18a2da,0x18a2dd,0x18a2e0};for(unsigned i=0;i<8;++i)write(pcs[i],s+i*4,0xffffffffu);
      write(0x18a2e3,s+0x20,0);write(0x18a2e6,s+0x24,0);fill(0x18a316,s+0x28,64);fill(0x18a326,s+0x128,64,0xffffffffu);
      constexpr std::array<std::uint32_t,4> gains{0x18a32e,0x18a332,0x18a33a,0x18a342};for(unsigned i=0;i<4;++i)write(gains[i],s+0x228+i*4,c.one_bits);
      write(0x18a34a,s+0x238,8000);write(0x18a354,s+0x23c,0);
      source_sound_driver_master_gain({read(b.driver_pointer),b.master_output_globals},s+0x228,gain,fp,{service.memory},observe);
    }engine_reset(c);
  }
  std::uint32_t request(std::uint32_t definition,std::uint32_t object,std::uint16_t marker,std::uint32_t parameter)const{
    if(definition==0xffffffffu)return 0xffffffffu;
    const auto pool=read(b.pool_pointer),datum=allocate(pool);if(datum==0xffffffffu)return datum;const auto p=read(pool+0x44)+(datum&0xffff)*24;
    write(0x18a9e9,p+0xc,definition);write(0x18a9ee,p+2,3,1);write(0x18a9f2,p+4,0,2);write(0x18a9f6,p+5,read(p+5,1)|1,1);
    if(object!=0xffffffffu){write(0x18aa03,p+3,1,1);write(0x18aa07,p+0x14,parameter);write(0x18aa0f,p+0x10,object);write(0x18aa12,p+6,marker,2);}
    else{write(0x18aa19,p+6,0xffff,2);write(0x18aa1f,p+3,0,1);write(0x18aa22,p+0x10,0);write(0x18aa25,p+0x14,0xff,1);write(0x18aa29,p+0x16,0xffff,2);}return datum;
  }
};
}
void source_sound_looping_construct(const SoundLoopingBindings &b,const SoundLoopingConstants &c,const SoundLoopingServices &s,const SoundDriverStartupObserver &o){Owner(b,s,o).construct(c);}
void source_sound_looping_reset(const SoundLoopingBindings &b,const SoundLoopingConstants &c,const SoundDriverMasterGainConstants &m,const SoundDriverFloatingPointContext &fp,const SoundLoopingServices &s,const SoundDriverStartupObserver &o){Owner(b,s,o).reset(c,m,fp);}
void source_sound_looping_engine_reset(const SoundLoopingBindings &b,const SoundLoopingConstants &c,const SoundLoopingServices &s,const SoundDriverStartupObserver &o){Owner(b,s,o).engine_reset(c);}
void source_sound_looping_class_construct(const SoundLoopingBindings &b,const SoundLoopingServices &s,const SoundDriverStartupObserver &o){Owner(b,s,o).class_construct();}
void source_sound_looping_class_reset(const SoundLoopingBindings &b,const SoundLoopingServices &s,const SoundDriverStartupObserver &o){Owner(b,s,o).class_reset();}
std::uint32_t source_sound_looping_register(const SoundLoopingBindings &b,std::uint32_t d,std::uint32_t object,std::uint16_t marker,std::uint32_t parameter,const SoundLoopingServices &s,const SoundDriverStartupObserver &o){return Owner(b,s,o).request(d,object,marker,parameter);}
void source_sound_looping_class_gain(const SoundLoopingBindings &b,const SoundLoopingConstants &c,std::string_view selector,std::uint32_t gain,std::uint32_t duration,const SoundLoopingServices &s,const SoundDriverStartupObserver &o){Owner(b,s,o).class_gain(c,selector,gain,duration);}
} // namespace halo2
