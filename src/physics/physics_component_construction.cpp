#include "halo2/physics_component_construction.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
class Backing {
  const SourcePhysicsComponentConstructionServices &services;
  ObjectPhysicsConstructionObserver observer;
public:
  Backing(const SourcePhysicsComponentConstructionServices &s,const ObjectPhysicsConstructionObserver &o):services(s),observer(o){}
  std::span<std::byte> memory(std::uint32_t address,unsigned size)const{
    if(!services.memory)throw std::runtime_error("Original component construction storage unavailable");
    const auto data=services.memory(address,size);
    if(data.size()!=size)throw std::runtime_error("Original component construction extent unavailable");
    return data;
  }
  std::uint32_t read(std::uint32_t address,unsigned size=4)const{
    const auto data=memory(address,size);std::uint32_t value=0;
    for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(data[i])<<(8*i);
    return value;
  }
  void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned size=4)const{
    const auto before=read(address,size);const auto data=memory(address,size);
    if(size<4)value&=(1u<<(8*size))-1;
    for(unsigned i=0;i<size;++i)data[i]=std::byte(value>>(8*i));
    if(observer)observer({pc,address,before,value,size});
  }
};
std::uint32_t negative_tick(std::int32_t tick,std::uint32_t scale,PhysicsComponentConstructionFloatingPointContext &context){
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
  if((context.x87_control_word&0x3f)!=0x3f||(context.mxcsr&0x1f80)!=0x1f80||(context.mxcsr&0xffff0000u))
    throw std::runtime_error("Component constructor unmasked floating exception ABI unavailable");
  struct Environment {
    std::fenv_t saved{};unsigned mxcsr;
    explicit Environment(const PhysicsComponentConstructionFloatingPointContext &ctx):mxcsr(_mm_getcsr()){
      if(std::fegetenv(&saved))throw std::runtime_error("Component constructor floating environment unavailable");
      asm volatile("fldcw %0"::"m"(ctx.x87_control_word):"memory");_mm_setcsr(ctx.mxcsr);
    }
    ~Environment(){std::fesetenv(&saved);_mm_setcsr(mxcsr);}
  }environment(context);
  std::uint32_t product;std::int32_t rounded;
  asm volatile("cvtsi2ssl %1, %%xmm0; mulss %2, %%xmm0; movd %%xmm0, %0"
      :"=r"(product):"r"(tick),"m"(scale):"xmm0","memory");
  asm volatile("flds %1; fistpl %0":"=m"(rounded):"m"(product):"st","memory");
  context.mxcsr=_mm_getcsr();
  return 0u-std::bit_cast<std::uint32_t>(rounded);
#else
  (void)tick;(void)scale;(void)context;
  throw std::runtime_error("Component constructor source floating backend unavailable");
#endif
}
}
void source_physics_component_initialize(const SourcePhysicsComponentConstructionState &state,
    std::uint32_t component,std::uint32_t object,const PhysicsComponentConstructionConstants &constants,
    PhysicsComponentConstructionFloatingPointContext &context,
    const SourcePhysicsComponentConstructionServices &services,const ObjectPhysicsConstructionObserver &observer){
  const Backing data(services,observer);
  data.write(0x1d3343,component+8,object);const auto clock=data.read(state.clock_slot);
  data.write(0x1d3352,component+4,0);data.write(0x1d3355,component+0xc,0xffffffffu);
  const auto tick=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(data.read(clock+2,2))));
  const auto initial=negative_tick(tick,constants.tick_scale_bits,context);
  data.write(0x1d337d,component+0x18,0xff,1);data.write(0x1d3380,component+0x19,0xff,1);
  data.write(0x1d3383,component+0x20,0xffffffffu);data.write(0x1d3386,component+0x14,0);
  data.write(0x1d338b,component+0x1a,0,1);data.write(0x1d338e,component+0x1b,0,1);data.write(0x1d3391,component+0x1c,0,1);
  data.write(0x1d3396,component+0x10,initial);
  data.write(0x1d3399,component+0x70,0);data.write(0x1d339c,component+0x74,0);
  data.write(0x1d33a4,component+0x78,0x80000000u);data.write(0x1d33a7,component+0x7c,0);
  data.write(0x1d33aa,component+0x80,0);data.write(0x1d33b0,component+0x84,0x80000000u);
  data.write(0x1d33b6,component+0x88,0);data.write(0x1d33bc,component+0x8c,0);
  data.write(0x1d33c2,component+0x90,0x80000000u);data.write(0x1d33c8,component+0x94,0);
  data.write(0x1d33ce,component+0x98,0);data.write(0x1d33d4,component+0x9c,0);
}
std::uint32_t source_physics_component_allocate(const SourcePhysicsComponentConstructionState &state,
    std::uint32_t object,const PhysicsComponentConstructionConstants &constants,
    PhysicsComponentConstructionFloatingPointContext &context,
    const SourcePhysicsComponentConstructionServices &services,const ObjectPhysicsConstructionObserver &observer){
  const Backing data(services,observer);const auto pool=data.read(state.component_pool_slot);
  const auto datum=source_raw_datum_allocate(pool,{services.memory},[&](const auto &store){
    if(observer)observer({store.instruction,store.address,store.before,store.after,store.bytes});
  });
  const auto component=data.read(pool+0x44)+(datum&0xffffu)*160;
  if(component)source_physics_component_initialize(state,component,object,constants,context,services,observer);
  const auto heads=data.read(data.read(state.object_pool_slot)+0x44);
  const auto body=data.read(heads+(object&0xffffu)*12+8);
  if(!(data.read(body+0xc0,1)&1)){
    data.write(0x1d2aef,body+0xc0,data.read(body+0xc0,2)|1,2);
    const auto counter=data.read(state.counter_slot);data.write(0x1d2afb,counter,data.read(counter)+1);
  }
  return datum;
}
} // namespace halo2
