#include "halo2/sound_driver_spatial.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 { namespace {
[[noreturn]] void unavailable(const char *message){throw std::runtime_error(message);}
float real(std::uint32_t value){return std::bit_cast<float>(value);}
std::uint32_t bits(float value){return std::bit_cast<std::uint32_t>(value);}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
struct FloatingPointScope {
  std::fenv_t environment;unsigned mxcsr;
  explicit FloatingPointScope(const SoundDriverFloatingPointContext &context):mxcsr(_mm_getcsr()){
    if((context.x87_control_word&0x3fu)!=0x3fu||(context.mxcsr&0x1f80u)!=0x1f80u||(context.mxcsr&0xffff0000u))
      unavailable("Unmasked source spatial-driver floating-point exception delivery unavailable");
    if(std::fegetenv(&environment)!=0)unavailable("Source spatial-driver floating-point environment unavailable");
    _mm_setcsr(context.mxcsr);asm volatile("fldcw %0"::"m"(context.x87_control_word):"memory");
  }
  ~FloatingPointScope(){std::fesetenv(&environment);_mm_setcsr(mxcsr);}
};
float add(float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float subtract(float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float multiply(float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));}
bool within(float a,float b,float epsilon){
  unsigned char result;
  asm volatile("flds %1\n\tfsubs %2\n\tfabs\n\tflds %3\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0"
    :"=qm"(result):"m"(a),"m"(b),"m"(epsilon):"cc","st","st(1)");
  return result;
}
// Original32909C low-word result. It first rounds to signed64 using the
// actual CW, then corrects toward zero with raw float/sign/carry operations.
// SSE signed32 conversion is not equivalent for nonfinite/out-of-range input.
std::uint32_t integer_low(long double value){
  std::int64_t integer;float original,difference;
  asm volatile("fldt %3\n\tfld %%st(0)\n\tfsts %1\n\tfistpq %0\n\tfildq %0\n\t.byte 0xde,0xe9\n\tfstps %2"
    :"=m"(integer),"=m"(original),"=m"(difference):"m"(value):"st","st(1)");
  const auto raw=std::bit_cast<std::uint64_t>(integer);
  const auto low=std::uint32_t(raw),high=std::uint32_t(raw>>32);
  if(!low&&!(high&0x7fffffffu))return low;
  auto correction=bits(difference);
  if(bits(original)&0x80000000u){
    correction^=0x80000000u;
    return low+std::uint32_t(std::uint64_t(correction)+0x7fffffffu>0xffffffffu);
  }
  return low-std::uint32_t(std::uint64_t(correction)+0x7fffffffu>0xffffffffu);
}
std::uint32_t frequency(float value,const SoundDriverSpatialConstants &c){
  if(real(c.frequency_min_bits)>value)return 0x8000u;
  if(value>real(c.frequency_max_bits))return 0;
  const auto period=real(c.frequency_sample_period_bits),pi=real(c.frequency_pi_bits),scale=real(c.frequency_log_scale_bits);
  float result;
  asm volatile("flds %1\n\tfmuls %2\n\tfmuls %3\n\tfsin\n\tfadd %%st(0),%%st\n\tfldln2\n\tfxch %%st(1)\n\tfyl2x\n\tfmuls %4\n\tfstps %0"
    :"=m"(result):"m"(value),"m"(period),"m"(pi),"m"(scale):"st","st(1)");
  return std::uint32_t(_mm_cvttss_si32(_mm_set_ss(result)))&0xffffu;
}
std::uint32_t filter_gain(float value,const SoundDriverSpatialConstants &c){
  if(value>real(c.filter_gain_max_bits))value=real(c.filter_gain_max_bits);
  const auto exponent=real(c.filter_gain_exponent_bits),scale=real(c.filter_gain_scale_bits);
  long double result;
  asm volatile("flds %1\n\tfmuls %2\n\tfldl2e\n\tfmulp\n\tfld %%st(0)\n\tfrndint\n\tfxch %%st(1)\n\tfsub %%st(1),%%st\n\tf2xm1\n\tfld1\n\tfaddp\n\tfscale\n\tfstp %%st(1)\n\tfmuls %3\n\tfstpt %0"
    :"=m"(result):"m"(value),"m"(exponent),"m"(scale):"st","st(1)");
  const auto converted=integer_low(result);return converted>0xffffu?0xffffu:converted;
}
class Driver {
  const SoundDriverSpatialBindings &bindings;const SoundDriverSpatialConstants &constants;
  const SoundDriverParameterConstants &parameters;const SoundDriverFloatingPointContext &fp;
  const SoundDriverSpatialServices &services;const SoundStartupObserver &observe;
  std::span<std::byte> memory(std::uint32_t address,std::size_t size)const{
    if(!services.memory)unavailable("Retained spatial driver memory unavailable");
    auto raw=services.memory(address,size);if(raw.size()!=size)unavailable("Retained spatial driver memory extent unavailable");return raw;
  }
  std::uint32_t read(std::uint32_t address,unsigned size=4)const{
    const auto raw=memory(address,size);std::uint32_t value=0;
    for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);
    return value;
  }
  float get(std::uint32_t address)const{return real(read(address));}
  void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned size=4)const{
    const auto before=read(address,size);auto raw=memory(address,size);
    for(unsigned i=0;i<size;++i)raw[i]=std::byte(value>>(8*i));
    if(observe)observe({pc,address,size,before,value&(size==4?0xffffffffu:((1u<<(8*size))-1))});
  }
  void platform(SoundDriverSpatialOperation op,std::uint32_t buffer,std::optional<std::uint32_t> apply,std::span<const std::uint32_t> words)const{
    if(!services.native_operation)unavailable("Required spatial SDK/platform operation unavailable");
    services.native_operation({op,buffer,apply,words});
  }
  std::int32_t volume(float value)const{
    if(value==real(constants.zero_bits))return -6400;
    const auto scale=real(constants.volume_log_scale_bits);long double output;
    asm volatile("flds %1\n\tfldlg2\n\tfxch %%st(1)\n\tfyl2x\n\tfmuls %2\n\tfstpt %0"
      :"=m"(output):"m"(value),"m"(scale):"st","st(1)");
    const auto v=std::bit_cast<std::int32_t>(integer_low(output));return v<-6400?-6400:v>0?0:v;
  }
  void environment(std::int32_t index,std::uint32_t db)const{
    const auto driver=read(bindings.driver_pointer_address),voice=driver+0x1a18+std::uint32_t(index)*0x3c;
    std::array<std::uint32_t,9> words{};
    words[0]=std::uint32_t(volume(get(driver+0x2abc)));words[1]=std::uint32_t(volume(get(driver+0x2ac0)));
    const auto normalized=real(source_sound_normalized_parameter(0xc2800000u,constants.zero_bits,db,1,parameters,fp));
    const auto c=real(constants.room_scale_bits),attenuation=subtract(c,multiply(normalized,c));
    const auto scaled=std::uint32_t(_mm_cvttss_si32(_mm_set_ss(attenuation)));
    const auto room=std::bit_cast<std::int32_t>(std::uint32_t(volume(get(driver+0x2ac4)))-scaled);
    const auto clamped=room<-10000?-10000:room>0?0:room;
    words[2]=words[3]=std::uint32_t(clamped);
    words[4]=words[6]=constants.zero_bits;words[8]=constants.quarter_bits;
    words[5]=std::uint32_t(volume(subtract(real(constants.one_bits),get(voice+0x30))));
    words[7]=std::uint32_t(volume(subtract(real(constants.one_bits),get(voice+0x2c))));
    platform(SoundDriverSpatialOperation::I3dl2Source,read(voice+0x34),1,words);
    const auto value=_mm_cvtss_f32(_mm_cvtsi32_ss(_mm_setzero_ps(),std::bit_cast<std::int32_t>(read(bindings.filter_frequency_address))));
    const std::array<std::uint32_t,6> filter{1,0,frequency(value,constants),filter_gain(get(bindings.filter_gain_address),constants),0,0};
    platform(SoundDriverSpatialOperation::Filter,read(voice+0x34),std::nullopt,filter);
  }
  float pin(float delta,float rate)const{
    const auto lower=subtract(real(constants.zero_bits),rate);
    if(lower>delta)return lower;
    return delta>rate?rate:delta;
  }
public:
  Driver(const SoundDriverSpatialBindings &b,const SoundDriverSpatialConstants &c,const SoundDriverParameterConstants &p,
         const SoundDriverFloatingPointContext &f,const SoundDriverSpatialServices &s,const SoundStartupObserver &o)
      :bindings(b),constants(c),parameters(p),fp(f),services(s),observe(o){}
  void run(std::int32_t index,const std::array<std::uint32_t,9> &request)const{
    const auto driver=read(bindings.driver_pointer_address),voice=driver+0x1a18+std::uint32_t(index)*0x3c;
    const bool force=!(read(voice+4,1)&2u)||!read(driver,1);const auto flag=request[0]&1u;
    const auto occlusion=real(request[4]),obstruction=real(request[5]);
    bool changed=force;
    for(unsigned i=0;i<3&&!changed;++i)changed=!within(real(request[i+1]),get(voice+8+4*i),real(constants.position_epsilon_bits));
    if(changed){
      const std::array<std::uint32_t,3> position{request[1],request[3],request[2]};
      platform(SoundDriverSpatialOperation::Position,read(voice+0x34),1,position);
      write(0x222fc8,voice+8,request[1]);write(0x222fcd,voice+0xc,request[2]);write(0x222fd3,voice+0x10,request[3]);
    }
    if(!force&&within(occlusion,get(voice+0x2c),real(constants.environment_epsilon_bits))&&
        within(obstruction,get(voice+0x30),real(constants.environment_epsilon_bits))&&(read(voice+4,1)&1u)==flag){
      write(0x2230d2,voice+4,read(voice+4,1)|2u,1);return;
    }
    const auto zero=real(constants.zero_bits);
    if(!force&&(read(voice+4,1)&1u)==flag&&!(zero>=real(request[8]))){
      const auto delta=pin(subtract(obstruction,get(voice+0x30)),real(request[8]));
      write(0x223055,voice+0x30,bits(add(get(voice+0x30),delta)));
    }else write(0x223065,voice+0x30,request[5]);
    if(!force&&(read(voice+4,1)&1u)==flag&&!(zero>=real(request[7]))){
      const auto delta=subtract(occlusion,get(voice+0x2c)),rate=real(request[7]),lower=subtract(zero,rate);
      const auto chosen=lower>delta?lower:delta>rate?rate:delta;
      write(lower>delta?0x223097:0x2230ab,voice+0x2c,bits(add(chosen,get(voice+0x2c))));
    }else write(0x2230b2,voice+0x2c,request[4]);
    write(0x2230c4,voice+4,(read(voice+4,1)&~1u)|flag,1);
    environment(index,request[6]);
    write(0x2230d2,voice+4,read(voice+4,1)|2u,1);
  }
};
#endif
} // namespace

void source_sound_driver_spatial_update(const SoundDriverSpatialBindings &b,std::int32_t index,
    const std::array<std::uint32_t,9> &request,const SoundDriverSpatialConstants &c,const SoundDriverParameterConstants &p,
    const SoundDriverFloatingPointContext &fp,const SoundDriverSpatialServices &s,const SoundStartupObserver &o){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);Driver(b,c,p,fp,s,o).run(index,request);
#else
  (void)b;(void)index;(void)request;(void)c;(void)p;(void)fp;(void)s;(void)o;
  unavailable("Original x87/SSE spatial driver backend unavailable");
#endif
}
std::uint32_t source_sound_filter_frequency(std::uint32_t value,const SoundDriverSpatialConstants &c,const SoundDriverFloatingPointContext &fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);return frequency(real(value),c);
#else
  (void)value;(void)c;(void)fp;unavailable("Original x87/SSE sound filter backend unavailable");
#endif
}
std::uint32_t source_sound_filter_gain(std::uint32_t value,const SoundDriverSpatialConstants &c,const SoundDriverFloatingPointContext &fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);return filter_gain(real(value),c);
#else
  (void)value;(void)c;(void)fp;unavailable("Original x87 sound filter backend unavailable");
#endif
}
} // namespace halo2
