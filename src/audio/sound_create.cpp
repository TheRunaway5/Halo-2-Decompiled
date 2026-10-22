#include "halo2/sound_create.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
template<class T>const T &need(const T &f,const char *s){if(!f)throw std::runtime_error(s);return f;}
float scalar(std::uint32_t x){return std::bit_cast<float>(x);}
std::uint32_t bits(float x){return std::bit_cast<std::uint32_t>(x);}
std::int32_t i32(std::uint32_t x){return std::bit_cast<std::int32_t>(x);}
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
std::uint32_t source_integer(long double x){std::uint32_t out{};
#if defined(__i386__) || defined(__x86_64__)
  // Complete32909C low-word result: source-CW FISTP64 followed by the
  // original float residual/sign/carry correction (no replacement CW).
  std::int64_t integer{};
  asm volatile("fldt %1; fistpq %0":"=m"(integer):"m"(x):"st");
  const auto raw=std::bit_cast<std::uint64_t>(integer);out=std::uint32_t(raw);
  if(out||((raw>>32)&0x7fffffffu)){
    auto residual=bits(rounded(add(x,-static_cast<long double>(integer))));
    if(bits(rounded(x))&0x80000000u){residual^=0x80000000u;out+=std::uint64_t(residual)+0x7fffffffu>0xffffffffu;}
    else out-=std::uint64_t(residual)+0x7fffffffu>0xffffffffu;
  }
#endif
  return out;
}
class Owner {
  const SoundCreateState &s;const SoundCreateConstants &c;
  const SoundCreateServices &service;const SoundStartupObserver &observe;
  std::span<std::byte> memory(std::uint32_t a,std::size_t n)const{
    auto b=need(service.memory,"Original sound creation memory unavailable")(a,n);
    if(b.size()!=n)throw std::runtime_error("Original sound creation memory extent unavailable");
    return b;
  }
public:
  Owner(const SoundCreateState &state,const SoundCreateConstants &constants,
        const SoundCreateServices &services,const SoundStartupObserver &o):s(state),c(constants),service(services),observe(o){}
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
  std::uint32_t pitch_range(std::uint32_t def,std::int32_t range)const{return read(s.current_ugh+0x24)+std::uint32_t((i16(read(def+8,2))+range)*12);}
  std::uint32_t playback(std::uint32_t def)const{return read(s.current_ugh+4)+std::uint32_t(i16(read(def+6,2))*0x38);}
  std::uint16_t random(std::uint32_t pc)const{const auto value=read(s.local_rng+4)*0x19660du+0x3c6ef35fu;write(pc,s.local_rng+4,value);return std::uint16_t(value>>16);}
  float gain(std::uint32_t def)const{
    const auto p=playback(def);const auto draw=rounded(multiply(random(0x1269d8),scalar(c.rng_scale_bits)));
    const float variance=get(p+0x14)-0.0f,product=variance*draw,offset=product+0.0f;
    return offset+get(p+0x10);
  }
  float pitch(std::uint32_t def)const{
    const auto p=playback(def);const long double upper=i16(read(p+0x1a,2)),lower=i16(read(p+0x18,2));
    const auto draw=multiply(random(0x126a7a),scalar(c.rng_scale_bits));
    return rounded(add(lower,multiply(upper-lower,draw)));
  }
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
  std::uint32_t allocate()const{
    const auto pool=s.sources;auto scan=read(pool+0x34);const auto high=read(pool+0x38);auto chosen=0xffffffffu;
    while(i32(scan)<i32(high)){
      const auto at=read(pool+0x48)+std::uint32_t((i32(scan)>>5)*4);
      if(!(read(at)&(1u<<(scan&31)))){if(scan!=0xffffffffu)chosen=scan;break;}
      ++scan;if(i32(scan)>=i32(read(pool+0x38)))break;
    }
    if(chosen==0xffffffffu){if(i32(high)>=i32(read(pool+0x20))||high==0xffffffffu)return 0xffffffffu;chosen=high;}
    const auto row=read(pool+0x44)+read(pool+0x24)*chosen,bitmap=read(pool+0x48)+std::uint32_t((i32(chosen)>>5)*4);
    write(0x16d32e,bitmap,read(bitmap)|(1u<<(chosen&31)));const auto count=read(pool+0x3c),extent=read(pool+0x38);
    write(0x16d33c,pool+0x3c,count+1);write(0x16d33f,pool+0x34,chosen+1);
    if(i32(extent)<=i32(chosen))write(0x16d344,pool+0x38,chosen+1);
    const auto stride=read(pool+0x24);unsigned n=0;for(;n<(stride&~3u);n+=4)write(0x16d353,row+n,0);
    for(;n<stride;++n)write(0x16d35a,row+n,0,1);
    const auto salt=read(pool+0x40,2);write(0x16d360,row,salt,2);write(0x16d363,pool+0x40,salt+1,2);
    if(read(pool+0x40,2)==0xffff)write(0x16d36e,pool+0x40,0x8000,2);
    return(salt<<16)|chosen;
  }
  void effects(std::uint32_t sound,std::uint32_t raw)const{
    const SoundSubmitState state{s.system,s.tag_table,s.current_matg,s.current_ugh,s.current_scenario,0,s.controllers,0};
    SoundSubmitServices sv;sv.memory=service.memory;
    const auto custom=source_sound_custom_playback(state,sound,(read(raw)&0x100)?read(raw+0x98):0xffffffffu,sv);
    const auto ordinary=source_sound_default_playback(state,sound,sv);std::uint32_t chosen=0;
    if(ordinary&&i32(read(ordinary+0x2c))>0)chosen=ordinary;
    if(custom&&i32(read(custom+0x2c))>0)chosen=custom;
    if(!chosen)return;
    const auto request=i32(read(chosen+0x2c))>0?read(chosen+0x30):0;const auto tag=read(request+4);
    if(tag==0xffffffffu)return;
    const auto def=definition(tag);
    if(i32(read(def+0x14))<=0)return;
    const auto first=read(def+0x18);const auto id=read(first+8);
    if(id==0x1300012d&&read(s.environment+6,2)!=0xffff){write(0x22484f,raw+0x8e,read(s.environment+6,2),2);write(0x224856,raw,read(raw)|0x40);}
    const auto effect=need(service.effect_identifier,"Original192F20 effect identifier owner unavailable")(id);
    if(effect!=0xffff){write(0x224867,raw+0x90,effect,1);write(0x22486d,raw,read(raw)|0x200);}
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
  std::uint32_t retain(std::uint32_t owner)const{
    if(!read(s.system+0x78,1)||!read(s.system+0x79,1)||!read(s.system+0x7a,1)||owner==0xffffffffu)return 0xffffffffu;
    const auto high=read(s.controllers+0x38),bitmap=read(s.controllers+0x48);
    for(std::uint32_t index=0;i32(index)>=0&&i32(index)<i32(high);++index){if(!(read(bitmap+(index>>5)*4)&(1u<<(index&31))))continue;
      const auto stored=read(s.controllers+0x44)+read(s.controllers+0x24)*index;const auto datum=(read(stored,2)<<16)|index;
      const auto row=read(s.controllers+0x44)+(datum&0xffff)*0x1c;if(read(row+4)!=owner)continue;
      const auto before=read(row+2,1),after=(((before+1)^before)&0x7f)^before;write(0x21bb97,row+2,after,1);return datum;
    }return 0xffffffffu;
  }
  std::int32_t range(std::uint32_t def,std::int32_t previous,float pitch)const{
    const auto count=i8(read(def+0xa,1));
    auto interval=[&](std::int32_t index){const auto r=pitch_range(def,index);return read(s.current_ugh+0x1c)+std::uint32_t(i16(read(r+2,2))*10);};
    auto clamped=[&](std::uint32_t at){const float lower=float(i16(read(at+6,2))),upper=float(i16(read(at+8,2)));
      if(above(lower,pitch))return lower;
      if(above(pitch,upper))return upper;
      return pitch;};
    if(previous!=-1&&previous<count){const auto r=pitch_range(def,previous),at=interval(previous);const float value=clamped(at);
      if(value==pitch&&!std::isunordered(value,pitch)&&i16(read(r+0xa,2))>0)return previous;}
    float best=scalar(c.maximum_float_bits);std::int32_t selected=-1;
    for(std::int32_t n=0;n<count;++n){if(i16(read(pitch_range(def,n)+0xa,2))<=0)continue;const auto at=interval(n);const float value=clamped(at);
      if(value==pitch&&!std::isunordered(value,pitch))return n;
      const float center=float(i16(read(at+4,2)));const float delta=above(pitch,center)?pitch-center:float(i16(read(at+2,2)))-pitch;
      if(above(best,delta)){selected=n;best=delta;}
    }return selected;
  }
  std::uint16_t permutation(std::uint32_t def,std::int32_t selected_range)const{
    const auto r=pitch_range(def,selected_range);const auto persistent=!(read(cls_sound(def)+0xa,1)&0x20);
    if(persistent&&i8(read(r+5,1))!=-1){const auto selected=std::uint16_t(i8(read(r+5,1)));write(0x21b46f,r+5,0xff,1);write(0x21b40f,r+4,selected,1);return selected;}
    const auto count=i16(read(r+0xa,2));
    // The source helper copies into a DWORD stack local. Larger authored
    // extents are not silently replaced by another selection algorithm.
    if(count<1||count>31)throw std::runtime_error("Original permutation stack-local domain unavailable");
    const auto bytes=unsigned(count+7)/8,bank=read(s.current_ugh+0x3c)+(read(r+6,2)>>3);
    std::uint32_t used=0;if(persistent&&count>1)used=read(bank,bytes);
    auto last=persistent?i8(read(r+4,1)):-1;std::uint32_t selected=0;int tries=0;
    for(;;){selected=(std::uint32_t(random(0x21b281))*std::uint32_t(count))>>16;const auto mask=(1u<<(unsigned(count)&31))-1;
      if(!(~used&mask)){used=0;if(count>1&&last!=-1)used=1u<<(unsigned(last)&31);last=-1;}
      const auto bit=1u<<(selected&31);if(used&bit)continue;used|=bit;
      const auto p=read(s.current_ugh+0x2c)+std::uint32_t(i16(read(r+8,2))+i16(selected))*16;
      const float skip=float(read(p+2,2))*scalar(c.rng_scale_bits);
      const auto draw=multiply(random(0x21b349),scalar(c.rng_scale_bits));
      if(draw>=static_cast<long double>(skip)||++tries==count*2)break;
    }
    if(persistent){if(count>1){unsigned offset=0;for(;offset<(bytes&~3u);offset+=4)write(0x21b6e8,bank+offset,used>>(offset*8));
      for(;offset<bytes;++offset)write(0x21b6ef,bank+offset,used>>(offset*8),1);}
      write(0x21b40f,r+4,selected,1);}
    return std::uint16_t(selected);
  }
  std::uint32_t chunk(std::uint32_t source)const{
    const auto r=pitch_range(definition(read(source+0xc)),i8(read(source+0x9c,1)));
    const auto p=read(s.current_ugh+0x2c)+std::uint32_t(i16(read(r+8,2))+i8(read(source+0x9d,1)))*16;
    return read(s.current_ugh+0x44)+std::uint32_t(i16(read(p+0xc,2))+i16(read(source+0x9e,2)))*12;
  }
  void select(std::uint32_t source,std::uint32_t sound,std::uint32_t range,std::uint32_t permutation)const{
    if(read(source+4,1)&2){const auto old=chunk(source),handle=read(old+8);
      const auto entry=read(s.cache_pool+0x44)+(handle&0xffff)*16;write(0x12812d,entry+4,read(entry+4,1)-1,1);
      write(0x128130,source+4,read(source+4,1)&~2u,1);}
    write(0x129e25,source+0xc,sound);write(0x129e2c,source+0x9c,range,1);
    write(0x129e37,source+0x9d,permutation,1);write(0x129e3d,source+0x9e,0,2);
  }
  void load(std::uint32_t source)const{
    const auto selected=chunk(source);const auto result=need(service.cache_request,"Original21A960 cache resource owner unavailable")(read(source+0xc),selected,2);
    if(result&3){if(!(read(source+4,1)&2)){write(0x12803a,source+4,read(source+4,1)|2,1);
      need(service.cache_request,"Original21A960 cache resource owner unavailable")(0xffffffffu,chunk(source),4);}}
  }
  std::uint32_t create(std::uint32_t sound,std::uint32_t association,std::uint32_t raw,std::uint32_t promotion)const{
    memory(raw,160);const auto def=definition(sound);const float extra=(read(raw,1)&0x80)?get(raw+0x94):scalar(c.default_extra_gain_bits);
    const float random_gain=extra+gain(def);const float adjusted=volume(def,get(raw+0x14),get(raw+0x10));
    if(below_equal(adjusted+random_gain,scalar(c.minimum_gain_bits)))return 0xffffffffu;
    effects(sound,raw);const auto datum=allocate();if(datum==0xffffffffu)return datum;
    const auto source=read(s.sources+0x44)+(datum&0xffff)*0xbc;const float squared=distance(raw+0xc,association);
    const auto delay=source_integer(multiply(square_root(squared),get(s.delay_scale_operand)));
    write(0x12783d,source+2,0,1);write(0x127841,source+0xac,0xffff,2);write(0x12784e,source+0xa1,association,1);
    write(0x12785d,source+6,read(raw+4,2),2);write(0x127866,source+0x90,bits(pitch(def)));
    write(0x127870,source+0x94,bits(random_gain));write(0x127878,source+0x98,read(s.local_rng+4));
    write(0x12788b,source+4,(read(raw,1)&2)?read(raw+8):0,2);write(0x12789d,source+0x10,(read(raw,1)&8)?read(raw+0x50):0xffffffffu);
    for(unsigned n=0;n<17;++n)write(0x1278aa,source+0x18+n*4,read(raw+0xc+n*4));
    if(read(def,1)&8)write(0x1278b5,source+0x18,read(source+0x18,1)|1,1);
    if(read(cls_sound(def)+0xa,1)&4)write(0x1278ce,source+0x18,read(source+0x18,1)|0x80,1);
    write(0x1278e8,source+0xa2,std::uint32_t((read(raw,1)&0x40)?i8(read(raw+0x8e,2)):-1),2);
    write(0x127903,source+0xa4,(read(raw)&0x200)?read(raw+0x90,1):0xff,1);
    write(0x12791c,source+8,(read(raw)&0x100)?read(raw+0x98):0xffffffffu);
    if(read(raw,1)&0x10){const auto controller=retain(read(raw+0x54));write(0x127930,source+0xa8,controller);
      if(controller!=0xffffffffu)need(service.enable_controller,"Original21BA00/21C070 controller owner unavailable")(controller);
    }else write(0x12794a,source+0xa8,0xffffffffu);
    write(0x127954,source+3,0xff,1);
    if(read(raw,1)&0x20){write(0x127961,source+0x14,read(raw+0x58));const auto size=read(raw+0x8c,2)>48?48:read(raw+0x8c,2);unsigned n=0;
      for(;n<(size&~3u);n+=4)write(0x127984,source+0x5c+n,read(raw+0x5c+n));
      for(;n<size;++n)write(0x12798b,source+0x5c+n,read(raw+0x5c+n,1),1);
    }else write(0x127997,source+0x14,0);
    std::uint32_t selected_range,selected_permutation;
    if(read(raw)&0x400){selected_range=read(raw+0x9c,2);selected_permutation=read(raw+0x9e,2);}
    else{
      if(promotion!=0xffffffffu){const auto index=i8(read(def+0xc,1));if(index==-1)selected_range=0;
        else{const auto row=read(s.current_ugh+0x4c)+std::uint32_t(index*0x1c);const auto value=i16(read(read(row+4)+promotion*16,2));selected_range=value==-1?0:std::uint32_t(value);}}
      else{const auto row=read(s.current_ugh+0xc)+std::uint32_t(i8(read(def+0xb,1))*20);const float lower=float(i16(read(row+8,2))),upper=float(i16(read(row+0xa,2)));
        float value=(upper-lower)*get(raw+0x10);value=value+get(source+0x90);value=value+lower;selected_range=std::uint32_t(range(def,-1,value));}
      selected_permutation=permutation(def,i16(selected_range));
    }
    select(source,sound,selected_range,selected_permutation);write(0x127a31,source+0xa0,promotion,1);
    write(0x127a3e,source+0xb8,0);write(0x127a44,source+0xb4,0);
    if(i32(delay)>100&&!(read(source+0x18,2)&0x900)){write(0x127a6c,source+0x8c,read(s.system+0x84)+delay);write(0x127a72,source+4,read(source+4,1)|1,1);}
    else write(0x127a97,source+0x8c,read(s.system+0x84));
    load(source);return datum;
  }
};
}
std::uint32_t source_sound_create(const SoundCreateState &s,std::uint32_t sound,std::uint32_t association,
    std::uint32_t raw,std::uint32_t promotion,const SoundCreateConstants &c,const SoundDriverFloatingPointContext &f,
    const SoundCreateServices &services,const SoundStartupObserver &observer){FloatingPointScope scope(f);return Owner(s,c,services,observer).create(sound,association,raw,promotion);}
} // namespace halo2
