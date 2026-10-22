#include "halo2/sound_driver_master_gain.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
float scalar(std::uint32_t value){return std::bit_cast<float>(value);}
std::uint32_t bits(float value){return std::bit_cast<std::uint32_t>(value);}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class FloatingPointScope {
  std::fenv_t saved;
  unsigned mxcsr;
public:
  explicit FloatingPointScope(const SoundDriverFloatingPointContext &context)
      :mxcsr(_mm_getcsr()) {
    if((context.x87_control_word&0x3f)!=0x3f ||
       (context.mxcsr&0x1f80)!=0x1f80 || (context.mxcsr&0xffff0000))
      throw std::runtime_error("Original master gain unmasked exception delivery unavailable");
    if(std::fegetenv(&saved)!=0)throw std::runtime_error("Original master gain FP context unavailable");
    _mm_setcsr(context.mxcsr);
    asm volatile("fldcw %0"::"m"(context.x87_control_word):"memory");
  }
  ~FloatingPointScope(){std::fesetenv(&saved);_mm_setcsr(mxcsr);}
};
long double logarithm(float value,float scale) {
  long double result;
  asm volatile("flds %1; fldlg2; fxch %%st(1); fyl2x; fmuls %2; fstpt %0"
      :"=m"(result):"m"(value),"m"(scale):"st","st(1)");
  return result;
}
float rounded(long double value) {
  float result;
  asm volatile("fldt %1; fstps %0":"=m"(result):"m"(value):"st");
  return result;
}
long double difference(long double value,std::int64_t integer) {
  long double result;
  // Original FILD64 is exact before the source-CW residual subtraction.
  asm volatile("fldt %1; fildq %2; fsubrp; fstpt %0"
      :"=m"(result):"m"(value),"m"(integer):"st","st(1)");
  return result;
}
std::uint32_t source_integer(long double value) {
  std::int64_t integer;
  asm volatile("fldt %1; fistpq %0":"=m"(integer):"m"(value):"st");
  const auto raw=std::bit_cast<std::uint64_t>(integer);
  auto result=std::uint32_t(raw);
  if(result||((raw>>32)&0x7fffffffu)) {
    auto residual=bits(rounded(difference(value,integer)));
    if(bits(rounded(value))&0x80000000u) {
      residual^=0x80000000u;
      result+=std::uint64_t(residual)+0x7fffffffu>0xffffffffu;
    } else result-=std::uint64_t(residual)+0x7fffffffu>0xffffffffu;
  }
  return result;
}
#endif
class Owner {
  const SoundDriverMasterGainState &state;
  const SoundDriverMasterGainConstants &constants;
  const SoundDriverMasterGainServices &services;
  const SoundDriverStartupObserver &observe;
  std::span<std::byte> memory(std::uint32_t address,std::size_t bytes) const {
    if(!services.memory)throw std::runtime_error("Original master gain backing unavailable");
    auto result=services.memory(address,bytes);
    if(result.size()!=bytes)throw std::runtime_error("Original master gain backing extent unavailable");
    return result;
  }
  std::uint32_t read(std::uint32_t address,unsigned bytes=4) const {
    const auto data=memory(address,bytes);std::uint32_t value=0;
    for(unsigned i=0;i<bytes;++i)value|=std::to_integer<std::uint32_t>(data[i])<<(8*i);
    return value;
  }
  void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned bytes=4) const {
    const auto before=read(address,bytes);auto data=memory(address,bytes);
    if(bytes<4)value&=(1u<<(8*bytes))-1;
    for(unsigned i=0;i<bytes;++i)data[i]=std::byte(value>>(8*i));
    if(observe)observe({pc,address,bytes,before,value});
  }
  std::uint32_t clamp(std::uint32_t value) const {
    if(0.f>scalar(value))return 0;
    if(scalar(value)>scalar(constants.one_bits))return constants.one_bits;
    return value;
  }
  std::uint32_t volume(std::uint32_t value,bool third) const {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(scalar(value)==0.f)return 0xffffe700u;
    auto result=std::bit_cast<std::int32_t>(source_integer(logarithm(scalar(value),scalar(constants.logarithm_scale_bits))));
    if(result< -6400)return 0xffffe700u;
    if(result>0)result=0;
    if(third&&result< -10000)result=-10000;
    return std::bit_cast<std::uint32_t>(result);
#else
    (void)value;(void)third;
    throw std::runtime_error("Original master gain x87/CRT backend unavailable");
#endif
  }
public:
  Owner(const SoundDriverMasterGainState &s,const SoundDriverMasterGainConstants &c,
        const SoundDriverMasterGainServices &p,const SoundDriverStartupObserver &o)
      :state(s),constants(c),services(p),observe(o) {}
  void apply(std::uint32_t input) const {
    const auto first=clamp(read(input));
    write(0x22310c,state.driver+0x2abc,first);
    write(0x223131,state.driver+0x2ac0,clamp(read(input+4)));
    write(0x223156,state.driver+0x2ac4,clamp(read(input+8)));
    write(0x223176,state.driver+0x2ac8,clamp(read(input+12)));
    write(0x223184,state.driver+0x2acc,read(input+16));
    write(0x223194,state.driver+0x2ad0,read(input+20));
    write(0x2231d5,state.output_globals+4,volume(first,false));
    write(0x22322c,state.output_globals+8,volume(read(state.driver+0x2ac0),false));
    write(0x223231,state.output_globals+0x14,0);
    write(0x223239,state.output_globals+0x1c,0);
    write(0x223241,state.output_globals+0x24,constants.quarter_bits);
    const auto third=volume(read(state.driver+0x2ac4),true);
    write(0x2232a1,state.output_globals+0xc,third);
    write(0x2232a6,state.output_globals+0x10,third);
    write(0x2232b1,state.output_globals+0x28,read(state.driver+0x2acc));
    write(0x2232c1,state.output_globals+0x2c,read(state.driver+0x2ad0));
    auto count=std::bit_cast<std::int16_t>(std::uint16_t(read(state.driver+6,2)));
    for(std::int32_t i=0;i<count;++i) {
      const auto slot=state.driver+0x1a1c+std::uint32_t(i)*0x3c;
      write(0x2232d5,slot,read(slot,1)&0xfdu,1);
      count=std::bit_cast<std::int16_t>(std::uint16_t(read(state.driver+6,2)));
    }
    write(0x2232e4,state.output_globals,1,1);
  }
};
}
void source_sound_driver_master_gain(const SoundDriverMasterGainState &s,
    std::uint32_t input,const SoundDriverMasterGainConstants &c,
    const SoundDriverFloatingPointContext &context,const SoundDriverMasterGainServices &p,
    const SoundDriverStartupObserver &o) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(context);Owner(s,c,p,o).apply(input);
#else
  (void)s;(void)input;(void)c;(void)context;(void)p;(void)o;
  throw std::runtime_error("Original master gain x87/SSE backend unavailable");
#endif
}
} // namespace halo2
