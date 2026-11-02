#include "halo2/native_tu15_look_controller.hpp"
#include <cstring>
#include <stdexcept>
#include <type_traits>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
 SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
 Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
 std::uint32_t word(std::uint32_t p,unsigned size=4)const{std::array<std::byte,4>b{};m_.read(p,std::span(b).first(size));std::uint32_t n{};for(unsigned k=0;k<size;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
 std::uint64_t wide(std::uint32_t p)const{std::array<std::byte,8>b{};m_.read(p,b);std::uint64_t n{};for(unsigned k=0;k<8;++k)n|=std::uint64_t(std::to_integer<unsigned>(b[k]))<<(k*8u);return n;}
 void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o_){e.before.resize(4);m_.read(p,e.before);}m_.write(p,b);if(o_){e.written.assign(b.begin(),b.end());o_(e);}}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 void pop(std::uint32_t pc,std::uint32_t p){std::uint32_t n;asm volatile("fstps %0":"=m"(n)::"memory","st");word(pc,p,n);}
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;bool active_{};
    void validate()const{if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 look controller requires its unsupported FP exception owner");}
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),previous_(_mm_getcsr()){
        if(depth>7)throw std::runtime_error("Native TU1.5 look controller requires its overfull x87 owner");validate();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(previous_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){validate();const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class F>decltype(auto)dependency(F&&f){publish();try{if constexpr(std::is_void_v<std::invoke_result_t<F>>){f();resume();return;}else{auto result=f();resume();return result;}}catch(...){resume();throw;}}
};

using F=__m128;
F scalar(std::uint32_t bits){F x{};std::memcpy(&x,&bits,4);return x;}
std::uint32_t bits(F x){std::uint32_t n;std::memcpy(&n,&x,4);return n;}
F add(F a,F b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F sub(F a,F b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
F mul(F a,F b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool above(F a,F b){unsigned char n;asm volatile("comiss %2,%1;seta %0":"=qm"(n):"x"(a),"x"(b):"cc","memory");return n;}
bool below(F a,F b){unsigned char n;asm volatile("comiss %2,%1;setb %0":"=qm"(n):"x"(a),"x"(b):"cc","memory");return n;}
bool zero(F x){unsigned char equal,ordered;F z{};asm volatile("ucomiss %3,%2;sete %0;setnp %1":"=qm"(equal),"=qm"(ordered):"x"(x),"x"(z):"cc","memory");return equal&&ordered;}
void push(std::uint32_t n){asm volatile("flds %0"::"m"(n):"memory","st");}
void push_wide(std::uint64_t n){asm volatile("fldl %0"::"m"(n):"memory","st");}
#endif
}
void native_tu15_update_look_controller(SourcePhysicsContactMemory&memory,
 std::uint32_t index,std::uint32_t yaw_delta,std::uint32_t pitch_delta,
 NativeTu15LookControllerScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
 const NativeTu15LookControllerServices&services,const SourcePhysicsContactRecordObserver&observe,
 NativeTu15LookControllerStats*stats){
 if(stats)++stats->calls;
 try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  // The source's velocity norm reaches five temporary x87 entries.
  if(depth>3)throw std::runtime_error("Native look controller requires its source x87 stack-capacity owner");
  Memory m(memory,observe);Environment environment(fp,depth);const auto p=scratch.identity;
  const auto read=[&](std::uint32_t at){return scalar(m.word(at));};
  const auto store=[&](std::uint32_t pc,std::uint32_t at,F value){m.word(pc,at,bits(value));};
  const auto controls=m.word(0x4f9b54u),record=controls+index*0x94u+0x14u;
  const auto global_min=read(0x46a588u);
  const auto globals=m.word(0x4ecc1cu),level_definition=m.word(globals+0xf4u);
  store(0x18a32fu,p+0x14u,global_min);
  const auto global_max=read(0x46a58cu);
  m.word(0x18a341u,p+0x28u,level_definition);store(0x18a345u,p+0x10u,global_max);
  environment.dependency([&]{native_tu15_build_look_context(memory,index,p+0x2cu,fp,
   services.context,observe,stats?&stats->context:nullptr);});
  auto yaw=read(record+0x10u);const auto seat_word=std::uint16_t(m.word(p+0x30u,2));
  yaw=add(yaw,scalar(yaw_delta));const auto objects=m.word(0x4ecbd0u);F z{};
  store(0x18a370u,record+0x10u,yaw);F two_pi{};
  if(seat_word!=0xffffu){
   const auto object=m.word(p+0x2cu),rows=m.word(objects+0x44u);
   const auto data=m.word(rows+(object&0xffffu)*12u+8u),definition=m.word(data);
   const auto tags=m.word(0x4f0414u),tag=m.word(tags+(definition&0xffffu)*16u+8u);
   const auto seats=m.word(tag+0x1ccu);
   const auto seat=seats+std::uint32_t(std::int32_t(std::int16_t(seat_word)))*0xb0u;
   const auto minimum=read(seat+0x88u);
   bool constrained=!zero(minimum);
   if(!constrained)constrained=!zero(read(seat+0x8cu));
   if(constrained){
    const auto marker=m.word(seat+8u);
    if(!services.marker)throw std::runtime_error("Native look controller requires wholeBAD60 marker computation");
    if(stats)++stats->marker_requests;
    environment.dependency([&]{services.marker(object,marker,p+0x44u,fp);});
    push(m.word(p+0x84u));push(m.word(p+0x80u));
    const auto authored_min=read(seat+0x88u);asm volatile("fpatan":::"memory","st","st(1)");
    const auto authored_max=read(seat+0x8cu),pi=read(0x5564c8u),authored_two_pi=read(0x5564c4u);
    m.pop(0x18a41fu,p+0x1cu);const auto base=read(p+0x1cu);
    const auto low=add(authored_min,base),high=add(authored_max,base);
    auto range=sub(high,low);if(!below(range,pi))range=sub(range,authored_two_pi);
    const auto minus_pi=sub(z,pi);if(!below(minus_pi,range))range=add(range,authored_two_pi);
    auto to_high=sub(high,read(record+0x10u));const bool high_wrap=!below(to_high,pi);
    store(0x18a45cu,p+0x1cu,range);store(0x18a462u,p+0x18u,to_high);
    if(high_wrap){to_high=sub(to_high,authored_two_pi);store(0x18a46eu,p+0x18u,to_high);}
    if(!below(minus_pi,to_high)){to_high=add(to_high,authored_two_pi);store(0x18a47du,p+0x18u,to_high);}
    auto from_low=sub(read(record+0x10u),low);const bool low_wrap=!below(from_low,pi);
    store(0x18a48fu,p+0x0cu,from_low);
    if(low_wrap){from_low=sub(from_low,authored_two_pi);store(0x18a49bu,p+0x0cu,from_low);}
    if(!below(minus_pi,from_low)){from_low=add(from_low,authored_two_pi);store(0x18a4aau,p+0x0cu,from_low);}
    two_pi=read(0x44923cu);const bool negative=above(z,range);auto shifted_range=add(range,two_pi);
    if(!negative)shifted_range=read(p+0x1cu);
    const bool high_inside=!below(to_high,z)&&above(shifted_range,to_high);
    const bool low_inside=high_inside?false:(!below(from_low,z)&&above(shifted_range,from_low));
    if(!high_inside&&!low_inside){
     push(m.word(p+0x0cu));asm volatile("fabs":::"memory","st");
     push(m.word(p+0x18u));asm volatile("fabs;fxch %%st(1);fxch %%st(1)":::"memory","st","st(1)");
     unsigned char nearer_low;asm volatile("fcomip %%st(1),%%st;fstp %%st(0);seta %0":"=qm"(nearer_low)::"cc","memory","st","st(1)");
     store(nearer_low?0x18a4f7u:0x18a4feu,record+0x10u,nearer_low?low:high);
    }
   }else two_pi=read(0x44923cu);
  }else two_pi=read(0x44923cu);
  if(above(z,read(record+0x10u))){auto value=read(record+0x10u);do{value=add(value,two_pi);}while(above(z,value));store(0x18a53fu,record+0x10u,value);}
  auto current_yaw=read(record+0x10u);
  if(above(current_yaw,two_pi)){do{current_yaw=sub(current_yaw,two_pi);}while(above(current_yaw,two_pi));store(0x18a560u,record+0x10u,current_yaw);}
  const auto constraints=m.word(p+0x34u);
  if(constraints){
   const auto object=m.word(p+0x2cu);auto maximum=read(constraints+0x10u),level=read(constraints+8u);
   const auto rows=m.word(objects+0x44u),body=m.word(rows+(object&0xffffu)*12u+8u);
   const bool maximum_zero=zero(maximum);store(0x18a595u,p+0x0cu,level);
   bool authored=!maximum_zero;if(!authored)authored=!zero(read(constraints+0x0cu));
   if(authored){
    const auto seated=std::uint16_t(m.word(p+0x30u,2))!=0xffffu;
    store(0x18a5bau,p+0x14u,read(constraints+0x0cu));store(0x18a5c5u,p+0x10u,read(constraints+0x10u));
    bool standing=false;
    if(seated){const auto up_z=read(body+0x84u),threshold=read(0x46a16cu);standing=above(up_z,threshold);}
    if(standing){
     push(m.word(record+0x10u));push_wide(m.wide(0x46a0f0u));
     asm volatile("fcos;fld %%st(1);fcos;fmul %%st(1)":::"memory","st","st(1)");m.pop(0x18a5fau,p+0x1cu);
     asm volatile("fxch %%st(1);fsin;fmul %%st(1)":::"memory","st","st(1)");m.pop(0x18a604u,p+0x20u);
     asm volatile("fstp %%st(0)":::"memory","st");push_wide(m.wide(0x46a0f0u));asm volatile("fsin":::"memory","st");m.pop(0x18a612u,p+0x24u);
     if(!services.vector_angle)throw std::runtime_error("Native look controller requires whole11F7E0 angle computation");
     if(stats)++stats->angle_calls;
     const auto angle=environment.dependency([&]{return services.vector_angle(body+0x7cu,p+0x1cu,fp,depth);});
     const auto tilt=sub(read(0x41c4ccu),scalar(angle));
     const auto minimum=sub(read(p+0x14u),tilt);store(0x18a634u,p+0x14u,minimum);
     maximum=sub(read(p+0x10u),tilt);store(0x18a644u,p+0x10u,maximum);
     level=sub(read(p+0x0cu),tilt);store(0x18a654u,p+0x0cu,level);
    }
    const auto low_bound=read(0x46a588u);auto minimum=read(p+0x14u);const bool minimum_low=above(low_bound,minimum);const auto high_bound=read(0x46a58cu);
    if(minimum_low)store(0x18a67bu,p+0x14u,low_bound);else if(above(minimum,high_bound))store(0x18a688u,p+0x14u,high_bound);
    maximum=read(p+0x10u);if(above(low_bound,maximum))store(0x18a699u,p+0x10u,low_bound);else if(above(maximum,high_bound))store(0x18a6a6u,p+0x10u,high_bound);
   }
   bool leveling=!zero(level);if(!leveling)leveling=m.word(record+0x30u,1)!=0;
   if(leveling){
    push(m.word(record+0x14u));const auto time=m.word(0x51df54u),level_bits=m.word(p+0x0cu);
    asm volatile("fsubs %0"::"m"(level_bits):"memory","st");const bool nonzero_level=!zero(level);
    const auto difference=sub(level,read(record+0x14u));asm volatile("fabs":::"memory","st");const auto scale=m.word(0x46a7d4u);asm volatile("fmuls %0"::"m"(scale):"memory","st");m.pop(0x18a6e1u,p+0x0cu);
    push(m.word(body+0x90u));push(m.word(body+0x8cu));push(m.word(body+0x88u));
    asm volatile("fld %%st(0);fmul %%st(1);fld %%st(2);fmul %%st(3);faddp;fld %%st(3);fmul %%st(4);faddp;fsqrt;fstp %%st(3);fstp %%st(0);fstp %%st(0)":::"memory","st","st(1)","st(2)","st(3)","st(4)");
    const auto seconds=m.word(time+4u);asm volatile("fmuls %0"::"m"(seconds):"memory","st");m.pop(0x18a712u,p+0x18u);
    F step;if(nonzero_level){const auto speed=read(p+0x18u),rate=read(p+0x0cu);step=mul(speed,rate);step=mul(step,read(0x46a7f0u));}
    else{const auto definition=m.word(p+0x28u);const auto scale=read(definition+0x5cu),speed=read(p+0x18u);step=mul(scale,speed);step=mul(step,read(p+0x0cu));}
    const auto negative=sub(z,step);auto change=step;if(above(negative,difference))change=negative;else if(!above(difference,step))change=difference;
    store(0x18a75eu,record+0x14u,add(change,read(record+0x14u)));
   }
  }
  const auto limit_minimum=read(p+0x14u),current_minimum=read(record+0x80u);
  auto minimum_delta=sub(limit_minimum,current_minimum);const auto negative_rate=read(0x46a98cu);const bool minimum_below=above(negative_rate,minimum_delta);const auto rate=read(0x46a988u);
  if(minimum_below)minimum_delta=negative_rate;else if(above(minimum_delta,rate))minimum_delta=rate;
  const auto next_minimum=add(read(record+0x80u),minimum_delta);auto maximum=read(p+0x10u);store(0x18a7a5u,record+0x80u,next_minimum);
  auto maximum_delta=sub(maximum,read(record+0x84u));const bool maximum_below=above(negative_rate,maximum_delta);F increment=negative_rate;
  if(!maximum_below){const bool maximum_above=above(maximum_delta,rate);increment=rate;if(!maximum_above)increment=maximum_delta;}
  store(0x18a7cdu,record+0x84u,add(increment,read(record+0x84u)));
  auto pitch=add(read(record+0x14u),scalar(pitch_delta));const auto minimum=read(record+0x80u);const bool lower=above(minimum,pitch);store(0x18a7eeu,record+0x14u,pitch);
  if(lower)store(0x18a7f8u,record+0x14u,minimum);
  else{if(above(pitch,read(record+0x84u)))pitch=read(record+0x84u);store(0x18a81au,record+0x14u,pitch);}
  if(stats)++stats->completed;
#else
  (void)memory;(void)index;(void)yaw_delta;(void)pitch_delta;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;
  throw std::runtime_error("Exact native look controller FP backend unavailable");
#endif
 }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
