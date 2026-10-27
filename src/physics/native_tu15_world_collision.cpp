#include "halo2/native_tu15_world_collision.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#include <utility>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
  SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&observe_;
public:
  Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),observe_(o){}
  std::uint32_t word(std::uint32_t p)const {
    std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t v{};
    for(unsigned i=0;i<4;++i)v|=std::to_integer<unsigned>(b[i])<<(8u*i);return v;
  }
  void write(std::uint32_t site,std::uint32_t p,std::uint32_t value,unsigned bytes=4){
    std::array<std::byte,4>b{};for(unsigned i=0;i<bytes;++i)b[i]=std::byte(value>>(8u*i));
    SourcePhysicsContactRecordStore event{site,p,{},{}};
    if(observe_){event.before.resize(bytes);m_.read(p,event.before);event.written.assign(b.begin(),b.begin()+bytes);}
    m_.write(p,std::span<const std::byte>(b).first(bytes));if(observe_)observe_(event);
  }
};
template<class F,class...A>void require(const F&f,const char*why,A&&...args){if(!f)throw std::runtime_error(why);f(std::forward<A>(args)...);}
bool positive(std::uint32_t n){return std::bit_cast<std::int32_t>(n)>0;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class NumericEnvironment {
  alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};unsigned mxcsr_{};NativeTu15PhysicsFrameFp&fp_;
public:
  NumericEnvironment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
    if(depth>7||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
      throw std::runtime_error("Native TU1.5 world collision requires unsupported numeric fault owner");
    mxcsr_=_mm_getcsr();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    std::memcpy(native_.data(),host_.data(),28);const std::uint16_t tags=0xffff;
    std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&tags,2);
    asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
  }
  ~NumericEnvironment(){asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(mxcsr_);}
};
void load_float(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
void load_integer(std::uint32_t bits){asm volatile("fildl %0"::"m"(bits):"memory","st");}
std::uint32_t multiply(std::uint32_t bits){std::uint32_t out;asm volatile("fmuls %1\n\tfstps %0":"=m"(out):"m"(bits):"memory","st");return out;}
#endif
void prepare_step(Memory&m,std::uint32_t world,std::uint32_t step,NativeTu15PhysicsFrameFp&fp,unsigned depth){
  const auto configuration=m.word(world+0xccu);
  m.write(0x319d8fu,world+0x150u,m.word(step));
  m.write(0x319d98u,world+0x154u,m.word(step+4u));
  constexpr std::array<std::uint32_t,4>sites{0x319da8u,0x319dadu,0x319db3u,0x319db9u};
  for(unsigned i=0;i<4;++i)m.write(sites[i],world+0x140u+i*4u,m.word(configuration+i*4u));
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  NumericEnvironment environment(fp,depth);
  // The source loads seconds before the interval. Paired-NaN operand order
  // differs from the integration phase and must not be reassociated.
  load_float(m.word(step));m.write(0x319dc5u,world+0x170u,multiply(m.word(world+0x180u)));
  const auto scaled=m.word(world+0x170u);
  load_integer(m.word(world+0x17cu));m.write(0x319ddau,world+0x174u,multiply(m.word(step+4u)));
  m.write(0x319de0u,world+0x198u,scaled);m.write(0x319decu,world+0x19cu,m.word(world+0x174u));
  load_integer(m.word(world+0x17cu));m.write(0x319dfeu,world+0x1a0u,multiply(m.word(world+0x170u)));
  load_float(m.word(world+0x180u));m.write(0x319e10u,world+0x1a4u,multiply(m.word(world+0x174u)));
#else
  (void)fp;(void)depth;throw std::runtime_error("Exact native TU1.5 world collision numeric backend unavailable");
#endif
}
enum class Profile {begin,island_begin,island_end,world_begin,end};
void profile(Memory&m,std::uint32_t cursor,std::uint32_t limit,Profile phase,const NativeTu15WorldCollisionServices&s){
  if(cursor>=limit)return;
  constexpr std::array<std::uint32_t,5>labels{0x41d9d4u,0x41d9bcu,0x41c394u,0x41d9acu,0x41cabcu};
  constexpr std::array<std::uint32_t,5>label_sites{0x319d66u,0x319e6bu,0x319eb5u,0x319f37u,0x319f70u};
  constexpr std::array<std::uint32_t,5>counter_sites{0x319d76u,0x319e7bu,0x319ec5u,0x319f47u,0x319f80u};
  constexpr std::array<std::uint32_t,5>cursor_sites{0x319d7cu,0x319e81u,0x319ecbu,0x319f4du,0x319f86u};
  const auto i=unsigned(phase);m.write(label_sites[i],cursor,labels[i]);
  if(!s.timestamp_counter)throw std::runtime_error("Native TU1.5 world collision requires actual platform timestamp");
  m.write(counter_sites[i],cursor+4u,std::uint32_t(s.timestamp_counter()));m.write(cursor_sites[i],0x48c974u,cursor+12u);
}
void profile(Memory&m,Profile phase,const NativeTu15WorldCollisionServices&s){const auto cursor=m.word(0x48c974u),limit=m.word(0x48c978u);profile(m,cursor,limit,phase,s);}
}
void native_tu15_collide_physics_world(SourcePhysicsContactMemory&memory,NativeTu15PhysicsWorld world,
  NativeTu15PhysicsStepPacket step,NativeTu15PhysicsFrameFp&fp,unsigned depth,
  const NativeTu15WorldCollisionServices&s,const SourcePhysicsContactRecordObserver&observe,NativeTu15WorldCollisionStats*stats){
  Memory m(memory,observe);if(stats)++stats->worlds;
  const auto limit=m.word(0x48c978u),cursor=m.word(0x48c974u);profile(m,cursor,limit,Profile::begin,s);
  const auto w=world.identity;prepare_step(m,w,step.identity,fp,depth);
  auto operations=m.word(w+0x24u);m.write(0x319e1bu,w+0x12cu,1,1);
  const auto drain=[&]{if(stats)++stats->operation_drains;require(s.drain_operations,"Native TU1.5 world collision requires actual operation drain",world,fp);};
  if(positive(operations))drain();
  const auto initial_count=m.word(w+0xcu);m.write(0x319e32u,w+0x12du,1,1);
  if(positive(initial_count)){
    std::uint32_t index{};
    do{
      const NativeTu15CollisionDispatcher dispatcher{m.word(w+0xc4u)};
      const NativeTu15PhysicsIsland island{m.word(m.word(w+8u)+index*4u)};
      if(stats)++stats->islands;
      require(s.collide_island,"Native TU1.5 world collision requires actual island collision",island,dispatcher,NativeTu15PhysicsStepConfiguration{w+0x140u},fp);
      if(m.word(w+0xacu)){
        profile(m,Profile::island_begin,s);const auto array=m.word(w+8u);
        m.write(0x319e8au,w+0x12du,0,1);const NativeTu15PhysicsIsland current{m.word(array+index*4u)};
        if(stats)++stats->island_notifications;
        require(s.notify_island_listeners,"Native TU1.5 world collision requires actual island listeners",world,current,step,fp);
        m.write(0x319e9cu,w+0x12du,1,1);profile(m,Profile::island_end,s);
      }
      const auto count=m.word(w+0xcu);++index;
      if(std::bit_cast<std::int32_t>(index)>=std::bit_cast<std::int32_t>(count))break;
    }while(true);
  }
  operations=m.word(w+0x24u);m.write(0x319ee2u,w+0x12du,0,1);m.write(0x319ee9u,w+0x12cu,0,1);
  if(positive(operations))drain();
  if(m.word(w+0xf4u)){if(stats)++stats->delete_drains;require(s.drain_deletes,"Native TU1.5 world collision requires actual pending deletes",world,fp);}
  if(m.word(w+0x100u)){if(stats)++stats->update_drains;require(s.drain_updates,"Native TU1.5 world collision requires actual pending updates",world,fp);}
  if(positive(m.word(w+0x24u)))drain();
  profile(m,Profile::world_begin,s);if(stats)++stats->world_notifications;
  require(s.notify_world_listeners,"Native TU1.5 world collision requires actual world listeners",world,step,fp);
  profile(m,Profile::end,s);
}
} // namespace halo2
