#include "halo2/sound_admission.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif
namespace halo2 { namespace {
template<class T>const T &need(const T &f,const char *s){if(!f)throw std::runtime_error(s);return f;}
float scalar(std::uint32_t x){return std::bit_cast<float>(x);}
std::uint32_t bits(float x){return std::bit_cast<std::uint32_t>(x);}
std::int32_t i16(std::uint32_t x){return std::bit_cast<std::int16_t>(std::uint16_t(x));}
std::int32_t i8(std::uint32_t x){return std::bit_cast<std::int8_t>(std::uint8_t(x));}
bool above(float a,float b){return a>b;}
bool below(float a,float b){return a<b||std::isunordered(a,b);}
bool below_equal(float a,float b){return a<=b||std::isunordered(a,b);}
class FloatingPointScope {
  unsigned short old_cw{};
#if defined(__SSE__)
  unsigned old_mxcsr{};
#endif
public:
  explicit FloatingPointScope(const SoundDriverFloatingPointContext &f){
#if defined(__i386__) || defined(__x86_64__)
    asm volatile("fnstcw %0":"=m"(old_cw));asm volatile("fldcw %0"::"m"(f.x87_control_word));
#else
    throw std::runtime_error("Original sound-create x87 instruction provider unavailable");
#endif
#if defined(__SSE__)
    old_mxcsr=_mm_getcsr();_mm_setcsr(f.mxcsr);
#endif
  }
  ~FloatingPointScope(){
#if defined(__i386__) || defined(__x86_64__)
    asm volatile("fldcw %0"::"m"(old_cw));
#endif
#if defined(__SSE__)
    _mm_setcsr(old_mxcsr);
#endif
  }
};
// These execute the actual x87 instruction chains. In particular, the
// intermediate extended values and source CW are not replaced by std::pow.
long double linear(float value,float first,float second){
  long double result{};
#if defined(__i386__) || defined(__x86_64__)
  asm volatile("flds %1; fmuls %2; fmuls %3; fldl2e; fmulp; fld %%st(0); frndint; fxch %%st(1); fsub %%st(1),%%st(0); f2xm1; fld1; faddp; fscale; fstp %%st(1); fstpt %0"
    :"=m"(result):"m"(value),"m"(first),"m"(second):"st","st(1)");
#endif
  return result;
}
long double logarithm(float value,float factor){
  long double result{};
#if defined(__i386__) || defined(__x86_64__)
  asm volatile("flds %1; fldlg2; fxch %%st(1); fyl2x; fmuls %2; fstpt %0"
    :"=m"(result):"m"(value),"m"(factor):"st","st(1)");
#endif
  return result;
}
long double multiply(long double a,long double b){
  long double out{};
#if defined(__i386__) || defined(__x86_64__)
  asm volatile("fldt %1; fldt %2; fmulp; fstpt %0":"=m"(out):"m"(a),"m"(b):"st","st(1)");
#endif
  return out;
}
long double add(long double a,long double b){
  long double out{};
#if defined(__i386__) || defined(__x86_64__)
  asm volatile("fldt %1; fldt %2; faddp; fstpt %0":"=m"(out):"m"(a),"m"(b):"st","st(1)");
#endif
  return out;
}
float rounded(long double x){float out{};
#if defined(__i386__) || defined(__x86_64__)
  asm volatile("fldt %1; fstps %0":"=m"(out):"m"(x):"st");
#endif
  return out;
}
long double square_root(long double x){long double out{};
#if defined(__i386__) || defined(__x86_64__)
  asm volatile("fldt %1; fsqrt; fstpt %0":"=m"(out):"m"(x):"st");
#endif
  return out;
}
class Owner {
  const SoundAdmissionState &s;const SoundCreateConstants &c;
  const SoundAdmissionConstants &a;const SoundDriverParameterConstants &parameters;
  const SoundDriverFloatingPointContext &fp;const SoundAdmissionServices &service;
  const SoundStartupObserver &observe;
  std::span<std::byte> memory(std::uint32_t a,std::size_t n)const{
    auto b=need(service.memory,"Original sound admission memory unavailable")(a,n);
    if(b.size()!=n)throw std::runtime_error("Original sound admission memory extent unavailable");
    return b;
  }
public:
  Owner(const SoundAdmissionState &state,const SoundAdmissionConstants &constants,
        const SoundDriverParameterConstants &parameters,const SoundDriverFloatingPointContext &fp,
        const SoundAdmissionServices &services,const SoundStartupObserver &o)
      :s(state),c(constants.shared),a(constants),parameters(parameters),fp(fp),service(services),observe(o){}
  std::uint32_t read(std::uint32_t a,unsigned n=4)const{auto b=memory(a,n);std::uint32_t x=0;for(unsigned i=0;i<n;++i)x|=std::to_integer<std::uint32_t>(b[i])<<(i*8);return x;}
  void write(std::uint32_t pc,std::uint32_t a,std::uint32_t v,unsigned n=4)const{
    const auto before=read(a,n);auto b=memory(a,n);for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(i*8));
    if(observe)observe({pc,a,n,before,v&(n==4?0xffffffffu:((1u<<(n*8))-1))});
  }
  float get(std::uint32_t a)const{return scalar(read(a));}
  std::uint32_t definition(std::uint32_t datum)const{return read(s.tag_table+(datum&0xffff)*16+8);}
  std::uint32_t cls(std::int32_t index)const{
    const auto globals=read(s.current_matg+0xc0)?read(s.current_matg+0xc4):0;
    return read(definition(read(globals+4))+4)+std::uint32_t(index*0x5c);
  }
  std::uint32_t cls_sound(std::uint32_t def)const{return cls(i8(read(def+2,1)));}
  std::uint32_t playback(std::uint32_t def)const{return read(s.current_ugh+4)+std::uint32_t(i16(read(def+6,2))*0x38);}
  long double gain_linear(float value)const{
    if(above(scalar(c.minimum_gain_bits),value))value=scalar(0xc2800000u);
    else if(above(value,scalar(c.upper_gain_bits)))value=0.0f;
    return linear(value,scalar(c.db_exponent_scale_bits),scalar(c.db_exponent_log_bits));
  }
  float gain_log(float input)const{
    float chosen=0.0f;
    if(!above(0.0f,input))chosen=above(input,scalar(c.one_bits))?scalar(c.one_bits):input;
    float output=scalar(c.minimum_gain_bits);
    if(above(input,0.0f)){
      const auto extended=logarithm(chosen,scalar(c.linear_db_scale_bits));
      const auto value=rounded(extended);
      if(!(static_cast<long double>(scalar(c.minimum_gain_bits))>extended))output=below_equal(value,0.0f)?value:0.0f;
    }
    if(above(scalar(c.minimum_gain_bits),output))return scalar(0xc2800000u);
    if(above(output,0.0f))return 0.0f;
    return output;
  }
  float class_gain(std::int32_t index)const{
    const auto fade=s.class_fades+std::uint32_t(index*16);float value=get(fade+4);
    if(read(fade+0xc,1)&1)return value;
    const auto row=cls(index);const auto next=read(s.system+0x200),old=read(s.system+0x1f8);
    const auto one=scalar(c.one_bits),epsilon=scalar(c.fade_epsilon_bits);
    if(next!=0xffffffffu&&old==next){
      const float delta=get(s.system+0x1fc)-get(s.system+0x204);const auto a=row+0x28+next*16;
      if(below_equal(delta,get(a+8)))return get(a)+value;
      if(below(get(a+4),epsilon))return value;
      const float duration=get(a+0xc);
      if(!below(duration,epsilon)&&above(duration+get(a+8),delta)){
        const float reciprocal=one/get(a+4);float v=(delta-get(a+8))*reciprocal;v=v*duration;
        const float elapsed=get(s.system+0x1fc)-delta;const float sum=elapsed+v;
        const float ratio=sum*reciprocal;const float divided=sum/get(a+4);
        const float fraction=above(one,ratio)?divided:one;
        const float amplitude=get(a)-0.0f;const float product=amplitude*fraction;
        return value+(product+0.0f);
      }
      const float elapsed=get(s.system+0x1fc)-delta,ratio=elapsed/get(a+4);
      const float fraction=below_equal(one,ratio)?one:ratio;
      const float amplitude=get(a)-0.0f,product=amplitude*fraction;
      return (product+0.0f)+value;
    }
    if(next!=0xffffffffu){const auto a=row+0x28+next*16;const float duration=get(a+0xc);
      if(!below(duration,epsilon)){
        const float start=get(a+8)/duration;const float progress=get(s.system+0x204)-start;
        float fraction=0.0f;if(!above(0.0f,progress))fraction=above(progress,one)?one:progress;
        const float product=(0.0f-get(a))*fraction;value=(product+get(a))+value;
      }
    }
    if(old==0xffffffffu)return value;
    const auto a=row+0x28+old*16;
    if(below(get(a+4),epsilon))return value;
    const float ratio=get(s.system+0x1fc)/get(a+4),fraction=below_equal(one,ratio)?one:ratio;
    const float product=(get(a)-0.0f)*fraction;return (product+0.0f)+value;
  }
  float volume(std::uint32_t def,float extra,float control)const{
    const auto row=read(s.current_ugh+0xc)+std::uint32_t(i8(read(def+0xb,1))*20),cl=cls_sound(def);
    const float lower=get(row)+get(cl+0x20),upper=get(row+4)+get(cl+0x24);
    const float low=rounded(gain_linear(lower));
    const auto difference=add(gain_linear(upper),-static_cast<long double>(low));
    const float selected=rounded(add(multiply(difference,control),low));
    const float logarithmic=gain_log(selected),faded=class_gain(i8(read(def+2,1)));
    return logarithmic+(faded+extra);
  }
  float distance(std::uint32_t location,std::uint32_t association)const{
    const auto type=i8((read(location+3,1)&15)<<4)>>4;if(!type)return 0.0f;
    if(type!=1){const long double x=get(location+0xc),y=get(location+0x10),z=get(location+0x14);
      return rounded(square_root(add(add(multiply(x,x),multiply(y,y)),multiply(z,z))));}
    const auto listener=s.system+0x88+association*0x48;const float z=get(listener+0x38)-get(location+0x14);
    if(i8(read(location+2,1))){float clamped=0.0f;if(!above(0.0f,z))clamped=below_equal(z,get(location+0x30))?z:get(location+0x30);
      if(clamped!=z||std::isunordered(clamped,z))return scalar(c.maximum_float_bits);
      const float y=get(location+0x10)-get(listener+0x34),x=get(location+0xc)-get(listener+0x30);
      return x*x+y*y;
    }
    const float y=get(listener+0x34)-get(location+0x10),x=get(listener+0x30)-get(location+0xc);
    const float first=z*z+y*y;return first+x*x;
  }

  bool codec(std::uint32_t def)const{return (read(def+5,1)==1&&read(def+4,1)!=2)||(read(def+4,1)==2&&read(s.alternate_codec_flag));}
  float bound(std::uint32_t sound,std::uint32_t location,bool upper)const{
    const auto flag=upper?8u:4u,offset=upper?0x38u:0x34u;
    if(read(location,1)&flag)return get(location+offset);
    const auto def=definition(sound);
    if(read(def+1,1)&flag)return get(cls_sound(def)+(upper?0x1cu:0x18u));
    return get(playback(def)+(upper?4u:0u));
  }
  bool supported(std::uint32_t sound)const{
    const auto def=definition(sound);return codec(def)&&read(def+10,1)&&
      read(read(s.current_ugh+0x24)+std::uint32_t(i16(read(def+8,2))*12)+10,2)!=0;
  }
  std::uint32_t first_local()const{for(unsigned n=0;n<4;++n)if(read(s.first_person_globals+12+n*4)!=0xffffffffu)return n;return 0xffffffffu;}
  bool occluded(std::uint32_t listener,std::uint32_t location,float ratio)const{
    if(listener==0xffffffffu)return true;
    if((read(location,1)&1)||((read(location+3,1)&15)!=1))return false;
    const auto at=s.system+0x88+listener*0x48;
    const auto cluster=std::uint16_t(read(location+0x2c,2));if(cluster==0xffff)return true;
    const auto from=std::uint16_t(read(at+4,2));if(from==0xffff)return true;
    if(read(at)==read(location+0x28)||from==cluster)return false;
    const float value=scalar(need(service.occlusion,"Original24BE30 sound scene occlusion unavailable")(s.current_scene,std::bit_cast<std::int16_t>(from),std::bit_cast<std::int16_t>(cluster)))*ratio;
    return below_equal(scalar(a.occlusion_threshold_bits),value);
  }
  std::uint32_t listener(std::uint32_t location,float range,std::uint32_t output)const{
    const auto selected=listener_value(location,range);
    if(output)write(0x129560,output,bits(selected.second));
    return selected.first;
  }
  float attenuation(std::uint32_t sound,std::uint32_t location,float distance)const{
    auto minimum=bound(sound,location,false);const float epsilon=scalar(c.fade_epsilon_bits);
    minimum=above(minimum,epsilon)?bound(sound,location,false):epsilon;
    const float maximum=bound(sound,location,true);
    return scalar(source_sound_distance_gain(bits(minimum),bits(maximum),bits(distance),parameters,fp));
  }
public:
  bool skip(std::uint32_t sound,std::uint32_t location)const{
    const auto def=definition(sound);const auto seed=read(s.local_rng+4)*0x19660du+0x3c6ef35fu;
    write(0x128408,s.local_rng+4,seed);
    const auto row=read(s.current_ugh+0xc)+std::uint32_t(i8(read(def+0xb,1))*20);
    const auto lower=static_cast<long double>(get(row+0xc));
    const auto threshold=multiply(add(multiply(add(static_cast<long double>(get(row+0x10)),-lower),get(location+4)),lower),get(playback(def)+8));
    const auto draw=multiply(seed>>16,scalar(c.rng_scale_bits));return draw>threshold;
  }
  bool admit(std::uint32_t sound,std::uint32_t request,std::uint32_t local_output,std::uint32_t reason_output)const{
    std::uint32_t reason=5;bool result=false;const auto location=request+12;
    if(read(s.system+0x78,1)&&read(s.system+0x79,1)&&read(s.system+0x7a,1)){
      const auto def=definition(sound);if(codec(def)){
        reason=1;if(skip(sound,location)){reason=4;if(supported(sound)){
          const float range=bound(sound,location,true);
          // The source uses a private stack local for its distance output.
          const auto selection=listener_value(location,range);
          write(0x1284a8,local_output,selection.first);
          if(selection.first!=0xffffffffu){
            const float normalized=attenuation(sound,location,selection.second);
            const float combined=volume(def,get(location+8),get(location+4));
            const float attenuation_db=gain_log(normalized),sum=combined+attenuation_db;
            const float total=get(playback(def)+0x10)+sum;
            if((read(location,2)&0x900)||above(total,scalar(c.minimum_gain_bits))){result=true;reason=0;}
          }
        }}
      }
    }
    if(reason_output)write(0x12852e,reason_output,reason);
    return result;
  }
  std::pair<std::uint32_t,float> listener_value(std::uint32_t location,float range)const{
    if((read(location+3,1)&15)!=1)return{first_local(),0.f};
    // Same complete129450 path, with the private stack output retained in a
    // host scalar rather than publishing an invented source address.
    float nearest=scalar(c.maximum_float_bits);std::uint32_t selected=0xffffffffu;
    for(unsigned n=0;n<4;++n)if(read(s.system+0x8e + n*0x48,1)){const auto value=distance(location,n);if(above(nearest,value)){nearest=value;selected=n;}}
    float value{},ratio{};
#if defined(__i386__) || defined(__x86_64__)
    asm volatile("flds %2; fsqrt; fsts %0; fdivs %3; fstps %1":"=m"(value),"=m"(ratio):"m"(nearest),"m"(range):"st");
#else
    throw std::runtime_error("Original sound listener x87 provider unavailable");
#endif
    if(occluded(selected,location,ratio))selected=0xffffffffu;
    if(!(read(location,2)&0x900)){const float squared=range*range;if(above(nearest,squared)){selected=0xffffffffu;value=scalar(c.maximum_float_bits);}}
    else if(selected==0xffffffffu)selected=first_local();
    return{selected,value};
  }
  std::uint32_t query(std::uint32_t entry,std::uint32_t sound,std::uint32_t location,std::uint32_t arg)const{
    switch(entry){
      case 0x128320:return supported(sound);
      case 0x126710:return bits(bound(sound,location,false));
      case 0x126760:return bits(bound(sound,location,true));
      case 0x129570:return bits(distance(location,arg));
      case 0x129450:return listener(location,scalar(arg),location+0x60);
      case 0x12c370:return bits(attenuation(sound,location,scalar(arg)));
      case 0x126930:return bits(volume(definition(sound),get(location+8),get(location+4)));
      case 0x21b900:return bits(gain_log(scalar(arg)));
      case 0x128760:return bits(class_gain(std::int16_t(arg)));
      default:throw std::runtime_error("Unmeasured sound admission query entry");
    }
  }
};
} // namespace
bool source_sound_admit(const SoundAdmissionState&s,std::uint32_t sound,std::uint32_t request,std::uint32_t output,std::uint32_t reason,const SoundAdmissionConstants&c,const SoundDriverParameterConstants&p,const SoundDriverFloatingPointContext&fp,const SoundAdmissionServices&v,const SoundStartupObserver&o){FloatingPointScope scope(fp);return Owner(s,c,p,fp,v,o).admit(sound,request,output,reason);}
std::uint32_t source_sound_admission_query(const SoundAdmissionState&s,std::uint32_t entry,std::uint32_t sound,std::uint32_t location,std::uint32_t argument,const SoundAdmissionConstants&c,const SoundDriverParameterConstants&p,const SoundDriverFloatingPointContext&fp,const SoundAdmissionServices&v,const SoundStartupObserver&o){FloatingPointScope scope(fp);return Owner(s,c,p,fp,v,o).query(entry,sound,location,argument);}
bool source_sound_admission_skip_slice(const SoundAdmissionState&s,std::uint32_t sound,std::uint32_t location,const SoundAdmissionConstants&c,const SoundDriverFloatingPointContext&fp,const SoundAdmissionServices&v,const SoundStartupObserver&o){FloatingPointScope scope(fp);return Owner(s,c,{},fp,v,o).skip(sound,location);}
} // namespace halo2
