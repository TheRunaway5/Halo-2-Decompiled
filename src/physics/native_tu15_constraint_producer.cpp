#include "halo2/native_tu15_constraint_producer.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
  SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
  Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
  std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
  void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n){
    SourcePhysicsContactRecordStore event{pc,p,{},{}};if(o_){event.before.resize(4);m_.read(p,event.before);}
    std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));m_.write(p,b);
    if(o_){event.written.assign(b.begin(),b.end());o_(event);}
  }
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  void float_word(std::uint32_t pc,std::uint32_t p){
    SourcePhysicsContactRecordStore event{pc,p,{},{}};if(o_){event.before.resize(4);m_.read(p,event.before);}
    alignas(16)std::array<std::byte,108>prior{};std::uint32_t n;
    asm volatile("fnsave %0\n\tfrstor %0":"=m"(prior)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    asm volatile("fstps %0":"=m"(n)::"memory","st");std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));
    try{m_.write(p,b);}catch(...){asm volatile("frstor %0"::"m"(prior):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}
    if(o_){event.written.assign(b.begin(),b.end());o_(event);}
  }
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
  alignas(16)std::array<std::byte,108>host_{};NativeTu15PhysicsFrameFp&fp_;unsigned host_mxcsr_;
public:
  Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
    if(depth>7||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 constraint producer requires its unsupported FP owner");
    host_mxcsr_=_mm_getcsr();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    std::array<std::byte,28>env{};std::memcpy(env.data(),host_.data(),28);const std::uint16_t empty=0xffff;
    std::memcpy(env.data(),&fp.control_word,2);std::memcpy(env.data()+4,&fp.status,2);std::memcpy(env.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(env):"memory");_mm_setcsr(fp.mxcsr);
  }
  ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);}
};
void push(std::uint32_t value){asm volatile("flds %0"::"m"(value):"memory","st");}
void multiply(std::uint32_t value){asm volatile("fmuls %0"::"m"(value):"memory","st");}
#endif
std::uint32_t emit(SourcePhysicsContactMemory&m,NativeTu15ConstraintSchemaPacket builder,unsigned kind,
  const SourcePhysicsContactRecordObserver&observe,NativeTu15ConstraintProducerStats*stats){
  constexpr std::array<std::uint32_t,3>owners{0x31f3a0u,0x31f410u,0x31f560u};
  if(stats)++stats->emissions.at(kind);const auto bytes=kind==2?32u:48u;
  return native_tu15_emit_custom_constraint_schema(m,bytes,builder,[=]{return NativeTu15PhysicsVirtualOwner{owners.at(kind)};},observe);
}
}
std::uint32_t native_tu15_constraint_axis_factors(SourcePhysicsContactMemory&memory,std::uint32_t owner){
  const SourcePhysicsContactRecordObserver none;Memory m(memory,none);const auto count=m.word(owner+0x34u);
  if(std::bit_cast<std::int32_t>(count)<=0)return 0;const auto first=m.word(owner+0x30u);
  for(std::uint32_t n=0;std::bit_cast<std::int32_t>(n)<std::bit_cast<std::int32_t>(count);++n)
    if(m.word(first+n*8u)==0x2003u)return m.word(first+n*8u+4u);
  return 0;
}
void native_tu15_begin_constraint_schemas(SourcePhysicsContactMemory&memory,NativeTu15ConstraintProducerInput input,
  NativeTu15ConstraintSchemaPacket builder,const SourcePhysicsContactRecordObserver&observe){
  Memory m(memory,observe);const auto first=m.word(input.identity);const auto second=m.word(input.identity+4u);
  const auto schema=m.word(builder.identity+4u);const auto jacobians=m.word(builder.identity);
  m.word(0x3296f4u,schema+12u,first);m.word(0x3296f9u,schema+16u,second);m.word(0x3296fcu,schema+4u,jacobians);
  m.word(0x3296ffu,schema,0x11801u);m.word(0x329705u,schema+8u,0);m.word(0x329708u,schema+20u,0);
  m.word(0x32970fu,builder.identity+4u,schema+24u);
}
void native_tu15_build_base_constraint(SourcePhysicsContactMemory&memory,NativeTu15ConstraintProducer object,
  NativeTu15ConstraintProducerInput input,NativeTu15ConstraintSchemaPacket builder,NativeTu15PhysicsFrameFp&fp,
  const NativeTu15ConstraintProducerServices&services,NativeTu15ConstraintProducerStats*stats){
  const SourcePhysicsContactRecordObserver none;Memory m(memory,none);const auto runtime=m.word(object.identity+0x90u);
  const auto count=m.word(object.identity+0x38u);const auto data=m.word(object.identity+0x34u);
  if(!services.build_base)throw std::runtime_error("Native TU1.5 constraint producer requires actual329720 base owner");
  if(stats)++stats->base_calls;services.build_base({data,count,runtime,object.identity+0x50u,input,builder},fp);
}
void native_tu15_build_axis_factor_constraint(SourcePhysicsContactMemory&memory,NativeTu15ConstraintProducer object,
  NativeTu15ConstraintProducerInput input,NativeTu15ConstraintSchemaPacket builder,NativeTu15PhysicsFrameFp&fp,
  unsigned depth,const NativeTu15ConstraintProducerServices&services,
  const SourcePhysicsContactRecordObserver&observe,NativeTu15ConstraintProducerStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(stats)++stats->axis_factor_calls;Memory m(memory,observe);const auto before=m.word(builder.identity);
  native_tu15_build_base_constraint(memory,object,input,builder,fp,services,stats);
  const auto properties=m.word(object.identity+12u);if(stats)++stats->property_lookups;
  const auto factors=native_tu15_constraint_axis_factors(memory,properties);if(!factors)return;
  const auto count=m.word(object.identity+0x38u);const auto after=m.word(builder.identity);const auto bytes=count*48u;
  if(after-before==bytes)return;const auto payload=emit(memory,builder,2,observe,stats);
  const auto scale=m.word(object.identity+0x44u);Environment environment(fp,depth);
  constexpr std::array<std::uint32_t,4>sites{0x31f7deu,0x31f7e7u,0x31f7f1u,0x31f7fbu};
  for(unsigned n=0;n<4;++n){push(scale);multiply(m.word(factors+n*4u));m.float_word(sites[n],payload+n*4u);}
  m.word(0x31f7feu,payload+16u,before+bytes);
#else
  (void)memory;(void)object;(void)input;(void)builder;(void)fp;(void)depth;(void)services;(void)observe;(void)stats;
  throw std::runtime_error("Exact native TU1.5 axis-factor constraint backend unavailable");
#endif
}
void native_tu15_build_scaled_accumulator_constraint(SourcePhysicsContactMemory&memory,NativeTu15ConstraintProducer object,
  NativeTu15ConstraintProducerInput input,NativeTu15ConstraintSchemaPacket builder,NativeTu15PhysicsFrameFp&fp,
  unsigned depth,const NativeTu15ConstraintProducerServices&services,
  const SourcePhysicsContactRecordObserver&observe,NativeTu15ConstraintProducerStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(stats)++stats->scaled_calls;Memory m(memory,observe);const auto first=m.word(input.identity),second=m.word(input.identity+4u);
  native_tu15_begin_constraint_schemas(memory,input,builder,observe);
  const auto first_scale=m.word(object.identity+0x40u),second_scale=m.word(object.identity+0x44u);
  const auto pre=emit(memory,builder,0,observe,stats);std::array<std::uint32_t,8>saved{};
  {
    Environment environment(fp,depth);push(first_scale);
    m.word(0x31f8b1u,pre+32u,first_scale);m.word(0x31f8b4u,pre+36u,second_scale);
    m.word(0x31f8b7u,pre+40u,first);m.word(0x31f8bau,pre+44u,second);
    multiply(m.word(first+48u));saved[0]=m.word(first+48u);saved[1]=m.word(first+52u);saved[2]=m.word(first+56u);
    saved[4]=m.word(second+48u);saved[3]=m.word(first+60u);saved[5]=m.word(second+52u);saved[7]=m.word(second+60u);saved[6]=m.word(second+56u);
    m.float_word(0x31f8ecu,first+48u);
    constexpr std::array<std::uint32_t,3>first_sites{0x31f901u,0x31f918u,0x31f922u};
    for(unsigned n=1;n<4;++n){push(first_scale);multiply(m.word(first+48u+n*4u));m.float_word(first_sites[n-1],first+48u+n*4u);}
    constexpr std::array<std::uint32_t,4>second_sites{0x31f92cu,0x31f936u,0x31f940u,0x31f94au};
    for(unsigned n=0;n<4;++n){push(second_scale);multiply(m.word(second+48u+n*4u));m.float_word(second_sites[n],second+48u+n*4u);}
  }
  native_tu15_build_base_constraint(memory,object,input,builder,fp,services,stats);
  const auto post=emit(memory,builder,1,observe,stats);
  constexpr std::array<std::uint32_t,8>payload_sites{0x31f96eu,0x31f974u,0x31f97bu,0x31f982u,0x31f989u,0x31f990u,0x31f997u,0x31f99eu};
  for(unsigned n=0;n<8;++n)m.word(payload_sites[n],post+n*4u,saved[n]);
  constexpr std::array<std::uint32_t,8>restore_sites{0x31f9a5u,0x31f9acu,0x31f9b3u,0x31f9bau,0x31f9c1u,0x31f9c8u,0x31f9cfu,0x31f9d2u};
  for(unsigned n=0;n<8;++n)m.word(restore_sites[n],(n<4?first:second)+48u+(n%4u)*4u,saved[n]);
  m.word(0x31f9d5u,post+44u,second);m.word(0x31f9d9u,post+40u,first);
#else
  (void)memory;(void)object;(void)input;(void)builder;(void)fp;(void)depth;(void)services;(void)observe;(void)stats;
  throw std::runtime_error("Exact native TU1.5 scaled-accumulator constraint backend unavailable");
#endif
}
} // namespace halo2
