#include "halo2/sound_spatial.hpp"
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
std::int32_t signed_byte(std::uint32_t value){return std::bit_cast<std::int8_t>(std::uint8_t(value));}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
struct FloatingPointScope {
  std::fenv_t environment;unsigned mxcsr;
  explicit FloatingPointScope(const SoundDriverFloatingPointContext &context):mxcsr(_mm_getcsr()){
    if((context.x87_control_word&0x3fu)!=0x3fu||(context.mxcsr&0x1f80u)!=0x1f80u||(context.mxcsr&0xffff0000u))
      unavailable("Unmasked source spatial floating-point exception delivery unavailable");
    if(std::fegetenv(&environment)!=0)unavailable("Source spatial floating-point environment unavailable");
    _mm_setcsr(context.mxcsr);
    asm volatile("fldcw %0"::"m"(context.x87_control_word):"memory");
  }
  ~FloatingPointScope(){std::fesetenv(&environment);_mm_setcsr(mxcsr);}
};
float add(float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float subtract(float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float multiply(float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));}
float divide(float a,float b){return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(a),_mm_set_ss(b)));}

std::uint32_t gain_db_bits(std::uint32_t input_bits,const SoundSpatialConstants &constants){
  const auto input=real(input_bits);auto chosen=input;
  if(0.f>chosen)chosen=0.f;
  else if(chosen>real(constants.one_bits))chosen=real(constants.one_bits);
  float output=real(constants.minimum_gain_bits);
  if(input>0.f){
    float rounded;long double extended;const auto scale=real(constants.linear_db_scale_bits);
    asm volatile("flds %2\n\tfldlg2\n\tfxch %%st(1)\n\tfyl2x\n\tfmuls %3\n\tfsts %0\n\tfstpt %1"
        :"=m"(rounded),"=m"(extended):"m"(chosen),"m"(scale):"st","st(1)");
    if(!(static_cast<long double>(real(constants.minimum_gain_bits))>extended))output=rounded>0.f?0.f:rounded;
  }
  if(real(constants.minimum_gain_bits)>output)return 0xc2800000u;
  return bits(output>0.f?0.f:output);
}

class Spatial {
  const SoundSpatialState &state;const SoundSpatialConstants &constants;
  const SoundSpatialServices &services;const SoundStartupObserver &observe;
  std::span<std::byte> memory(std::uint32_t address,std::size_t size)const{
    if(!services.memory)unavailable("Retained source spatial memory unavailable");
    auto result=services.memory(address,size);
    if(result.size()!=size)unavailable("Retained source spatial memory extent unavailable");
    return result;
  }
  std::uint32_t read(std::uint32_t address,unsigned size=4)const{
    const auto raw=memory(address,size);std::uint32_t value=0;
    for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);
    return value;
  }
  void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned size=4)const{
    const auto before=read(address,size);auto raw=memory(address,size);
    for(unsigned i=0;i<size;++i)raw[i]=std::byte(value>>(8*i));
    if(observe)observe({pc,address,size,before,value&(size==4?0xffffffffu:((1u<<(8*size))-1))});
  }
  float get(std::uint32_t address)const{return real(read(address));}
  float pin(float value)const{
    if(0.f>value)return 0.f;
    return value>real(constants.one_bits)?real(constants.one_bits):value;
  }
  std::uint32_t sound_class(std::uint32_t source)const{
    const auto definition=read(state.tag_table+(read(source+0xc)&0xffffu)*16+8);
    const auto index=signed_byte(read(definition+2,1));
    const auto globals=read(state.current_matg+0xc0)?read(state.current_matg+0xc4):0;
    const auto classes=read(state.tag_table+(read(globals+4)&0xffffu)*16+8);
    return read(classes+4)+std::uint32_t(index*0x5c);
  }
  float gain_db(float input)const{return real(gain_db_bits(bits(input),constants));}
  // Original220100. Its orphaned byte is a private initialized caller local.
  bool shared_update(std::uint32_t source,std::uint32_t datum,bool &orphaned)const{
    const auto record=read(state.shared_voices+0x44)+(datum&0xffffu)*0x6c;
    orphaned=read(record+0x44)==0xffffffffu;
    const auto tick=read(state.system+0x80);
    if(read(record+0x44)==tick)return false;
    for(unsigned i=0;i<9;++i)write(0x220148,record+0x48+4*i,read(source+0x24+4*i));
    write(0x22014a,record+0x44,tick);
    return true;
  }
  void position(std::uint32_t location,std::int32_t listener,std::array<std::uint32_t,3> &point)const{
    const auto type=signed_byte(read(location+2,1));
    if(type!=0&&type!=1)return;
    point[0]=read(location+0xc);point[1]=read(location+0x10);
    point[2]=type==0?read(location+0x14):read(state.system+0xc0+std::uint32_t(listener*0x48));
  }
  void inverse_transform(std::uint32_t matrix,std::array<std::uint32_t,3> &point)const{
    const auto scale=get(matrix);
    if(scale==0.f){point={0,0,0};return;}
    auto x=subtract(real(point[0]),get(matrix+0x28));
    auto y=subtract(real(point[1]),get(matrix+0x2c));
    auto z=subtract(real(point[2]),get(matrix+0x30));
    if(scale!=real(constants.one_bits)){
      // Original143EB6 computes one reciprocal and then three products.
      const auto reciprocal=divide(real(constants.one_bits),scale);
      x=multiply(reciprocal,x);y=multiply(reciprocal,y);z=multiply(reciprocal,z);
    }
    for(unsigned i=0;i<3;++i){
      const auto row=matrix+4+i*12;
      const auto yz=add(multiply(get(row+8),z),multiply(get(row+4),y));
      point[i]=bits(add(yz,multiply(get(row),x)));
    }
  }
public:
  Spatial(const SoundSpatialState &s,const SoundSpatialConstants &c,
          const SoundSpatialServices &v,const SoundStartupObserver &o):state(s),constants(c),services(v),observe(o){}
  void run(std::int16_t index,std::uint32_t attenuation,std::span<std::byte,12> backing)const{
    const auto voice=state.voices+std::uint32_t(std::int32_t(index)*0x24);
    const auto source=read(state.sources+0x44)+(read(voice)&0xffffu)*0xbc;
    write(0x12af12,voice+0xb,read(voice+0xa,1),1);
    write(0x12af15,voice+0xa,0,1);
    const auto audible=signed_byte((read(source+0x1b,1)&15u)<<4)>>4;
    if(audible==0){
      if(!(read(source+0x18,2)&0x1000u))return;
      if(attenuation){
        const auto raw=multiply(get(attenuation+8),get(sound_class(source)+0x50));
        const auto value=pin(raw),gain=multiply(subtract(real(constants.one_bits),value),get(source+0x1c));
        write(0.f>raw?0x12b1a2:0x12b1c2,source+0x1c,bits(gain));
      }else if(read(source+0x46,2)!=read(state.current_local_index_address,2))write(0x12b1e1,source+0x1c,0);
      return;
    }
    if(audible<1||audible>2||!(read(source+6,1)&2u))return;
    const auto driver_index=std::bit_cast<std::int16_t>(std::uint16_t(read(voice+0xe,2)));
    if(driver_index==-1)return;
    bool orphaned=true;
    const auto shared=read(voice+4);
    if(shared!=0xffffffffu&&!shared_update(source,shared,orphaned))return;
    const auto listener_index=signed_byte(read(source+0xa1,1));
    const auto listener=state.system+0x88+std::uint32_t(listener_index*0x48);
    std::array<std::uint32_t,3> point{};
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<4;++j)point[i]|=std::to_integer<std::uint32_t>(backing[4*i+j])<<(8*j);
    if(audible==1){
      position(source+0x18,listener_index,point);inverse_transform(listener+8,point);
      for(unsigned i=0;i<3;++i)for(unsigned j=0;j<4;++j)backing[4*i+j]=std::byte(point[i]>>(8*j));
    }else for(unsigned i=0;i<3;++i)point[i]=read(source+0x24+4*i);
    if(orphaned){
      const auto flags=state.driver+0x1a18+std::uint32_t(std::int32_t(driver_index)*0x3c)+4;
      write(0x12afdf,flags,read(flags,1)&0xfdu,1);
    }
    std::array<std::uint32_t,9> request{};
    request[1]=point[0];request[2]=point[1];request[3]=point[2];
    const auto cls=sound_class(source);
    if(attenuation){
      const auto scale=get(cls+0x50);
      request[4]=bits(pin(multiply(get(attenuation+8),scale)));
      request[5]=bits(pin(multiply(get(attenuation),scale)));
      request[6]=bits(add(gain_db(subtract(real(constants.one_bits),multiply(get(attenuation+4),scale))),get(cls+0x10)));
      request[7]=bits(multiply(get(cls+0x58),get(state.system+0x1e0)));
      request[8]=bits(multiply(get(cls+0x54),get(state.system+0x1e0)));
    }else request[6]=read(cls+0x10);
    request[0]=std::uint32_t(read(listener+7,1)!=0)|2u;
    if(!services.driver_update)unavailable("Complete original spatial driver update unavailable");
    services.driver_update(driver_index,request);
  }
};
#endif
} // namespace

std::uint32_t source_sound_spatial_gain_db(std::uint32_t input,const SoundSpatialConstants &constants,
    const SoundDriverFloatingPointContext &context){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(context);return gain_db_bits(input,constants);
#else
  (void)input;(void)constants;(void)context;unavailable("Original x87 sound gain backend unavailable");
#endif
}

void source_sound_spatial_update(const SoundSpatialState &state,std::int16_t index,
    std::uint32_t attenuation,std::span<std::byte,12> position,
    const SoundSpatialConstants &constants,const SoundDriverFloatingPointContext &context,
    const SoundSpatialServices &services,const SoundStartupObserver &observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(context);Spatial(state,constants,services,observe).run(index,attenuation,position);
#else
  (void)state;(void)index;(void)attenuation;(void)position;(void)constants;(void)context;(void)services;(void)observe;
  unavailable("Original x87/SSE spatial sound backend unavailable");
#endif
}
} // namespace halo2
