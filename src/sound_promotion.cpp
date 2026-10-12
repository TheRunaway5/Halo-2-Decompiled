#include "halo2/sound_promotion.hpp"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
template<class T> const T &need(const T &f,const char *why) {
  if(!f)throw std::runtime_error(why);
  return f;
}
std::int32_t signed32(std::uint32_t x) {return std::bit_cast<std::int32_t>(x);}
float scalar(std::uint32_t x) {return std::bit_cast<float>(x);}
std::uint32_t bits(float x) {return std::bit_cast<std::uint32_t>(x);}
std::uint32_t truncate(float x) {
#if defined(__SSE__)
  return std::uint32_t(_mm_cvtt_ss2si(_mm_set_ss(x)));
#else
  if(!std::isfinite(x)||x>=2147483648.0f||x< -2147483648.0f)return 0x80000000u;
  return std::bit_cast<std::uint32_t>(std::int32_t(x));
#endif
}
class Owner {
  const SoundPromotionState &s;
  const SoundPromotionConstants &c;
  const SoundPromotionServices &service;
  const SoundStartupObserver &observe;
  std::span<std::byte> memory(std::uint32_t address,std::size_t size)const {
    auto b=need(service.memory,"Original promotion raw memory unavailable")(address,size);
    if(b.size()!=size)throw std::runtime_error("Original promotion raw memory extent unavailable");
    return b;
  }
public:
  Owner(const SoundPromotionState &state,const SoundPromotionConstants &constants,
        const SoundPromotionServices &services,const SoundStartupObserver &observer)
    :s(state),c(constants),service(services),observe(observer){}
  std::uint32_t read(std::uint32_t at,unsigned size=4)const {
    auto b=memory(at,size);std::uint32_t x=0;
    for(unsigned i=0;i<size;++i)x|=std::to_integer<std::uint32_t>(b[i])<<(i*8);
    return x;
  }
  void write(std::uint32_t pc,std::uint32_t at,std::uint32_t value,unsigned size=4)const {
    const auto before=read(at,size);auto b=memory(at,size);
    for(unsigned i=0;i<size;++i)b[i]=std::byte(value>>(i*8));
    if(observe)observe({pc,at,size,before,value});
  }
  float get(std::uint32_t at)const {return scalar(read(at));}
  void put(std::uint32_t pc,std::uint32_t at,float value)const {write(pc,at,bits(value));}
  std::int32_t promote(std::uint32_t sound,std::uint32_t output)const {
    const auto definition=read(s.tag_table+(sound&0xffff)*16+8);
    const auto index=std::bit_cast<std::int8_t>(std::uint8_t(read(definition+0xc,1)));
    if(index==-1) {write(0x12874e,output,0xffffffffu);return 0;}
    const auto row=read(s.current_ugh+0x4c)+std::uint32_t(std::int32_t(index)*0x1c);
    if(!row) {write(0x12874e,output,0xffffffffu);return 0;}
    if(signed32(read(row+0x18))<=signed32(read(s.system+0x84))) {
      write(0x12865e,row+0x10,0xffffffffu);write(0x128665,row+0x18,0);
    }
    const auto elapsed=read(s.system+0x84)-read(row+0x14);
    for(std::uint32_t i=0;signed32(i)<signed32(read(row+8));++i) {
      const auto at=read(row+0xc)+i*4,value=read(at)-elapsed;
      write(0x128693,at,value);write(0x128698,at,signed32(value)<0?0:value);
    }
    std::uint32_t selector=0;
    if(signed32(read(row))>0) {
      do {
        const auto at=read(row+0xc)+selector*4,rule=read(row+4)+selector*0x10;
        write(0x1286bd,at,read(at)+read(rule+0xc));
        const auto threshold=std::bit_cast<std::int16_t>(std::uint16_t(read(rule+2,2)));
        if(threshold<=0 || signed32(read(at))<signed32(read(rule+8)) ||
           signed32(selector)>=signed32(read(row)-1))break;
        write(0x1286d6,at,0);++selector;
      }while(signed32(selector)<signed32(read(row)));
    }
    write(0x1286ea,output,selector);
    const auto old=read(row+0x10);
    const auto suppressed=(signed32(selector)<=signed32(old)&&
      signed32(read(s.system+0x84))<signed32(read(row+0x18)))?1:0;
    if(signed32(selector)>=signed32(old)&&!suppressed) {
      write(0x128713,row+0x10,selector);
      const float clock=float(signed32(read(s.system+0x84)));
      const float interval=get(read(row+4)+selector*0x10+4)*scalar(c.milliseconds_per_second_bits);
      const float deadline=interval+clock;
      write(0x128737,row+0x18,truncate(deadline));
    }
    write(0x128742,row+0x14,read(s.system+0x84));
    return suppressed;
  }
  void clock()const {
    const auto tick=need(service.tick_milliseconds,"Original sound clock service unavailable")();
    const auto delta=tick-read(s.system+0x80);
    auto denominator=scalar(c.seconds_per_millisecond_bits);
    const float old_accumulator=get(s.system+0x1fc);
    write(0x128167,s.system+0x80,tick);
    const float converted=float(signed32(delta));
    const float seconds=converted*denominator;
    const float accumulator=old_accumulator+seconds;
    write(0x128184,s.system+0x84,read(s.system+0x84)+delta);
    put(0x128191,s.system+0x1fc,accumulator);
    const float other=get(s.system+0x204)+seconds;
    const float frame=converted*denominator;
    put(0x1281a9,s.system+0x1e0,frame);put(0x1281b1,s.system+0x204,other);
    if(!s.options || !read(s.options+0x1120,1) || read(s.options+8)!=1) {
      write(0x128307,s.system+0x1e4,c.one_bits);return;
    }
    if(read(s.options+0x1121,1)) {
      if(read(s.system+0x1e8)==0xffffffffu) {
        const float duration=get(s.system+0x1ec)*scalar(c.milliseconds_per_second_bits);
        write(0x128202,s.system+0x1e8,truncate(duration));
      }
      if(signed32(read(s.system+0x1e8))>0) {
        const auto remaining=read(s.system+0x1e8)-delta;
        write(0x12821e,s.system+0x1e8,signed32(remaining)<=0?0:remaining);
      }
      if(read(s.system+0x1e8))return;
      if(!(denominator>get(s.system+0x1f0)))denominator=get(s.system+0x1f0);
      float step=frame/denominator;
      const float target=0.0f-get(s.system+0x1e4),negative=0.0f-step;
      if(negative>target) {
        put(0x12826a,s.system+0x1e4,negative+get(s.system+0x1e4));return;
      }
      if(!(target>step))step=target;
      put(0x128285,s.system+0x1e4,step+get(s.system+0x1e4));return;
    }
    if(need(service.fade_inhibited,"Original12D600 fade control owner unavailable")())return;
    if(!(denominator>get(s.system+0x1f4)))denominator=get(s.system+0x1f4);
    const float target=scalar(c.one_bits)-get(s.system+0x1e4);
    float step=get(s.system+0x1e0)/denominator;
    const float negative=0.0f-step;
    if(negative>target)step=negative;
    else if(!(target>step))step=target;
    put(0x1282eb,s.system+0x1e4,get(s.system+0x1e4)+step);
    write(0x1282f3,s.system+0x1e8,0xffffffffu);
  }
};
}
std::int32_t source_sound_promote(const SoundPromotionState &s,std::uint32_t sound,
    std::uint32_t output,const SoundPromotionConstants &c,const SoundPromotionServices &services,
    const SoundStartupObserver &observer) {return Owner(s,c,services,observer).promote(sound,output);}
void source_sound_promotion_clock(const SoundPromotionState &s,const SoundPromotionConstants &c,
    const SoundPromotionServices &services,const SoundStartupObserver &observer) {Owner(s,c,services,observer).clock();}
} // namespace halo2
