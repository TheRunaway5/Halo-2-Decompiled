#include "halo2/native_tu15_physics_frame.hpp"
#include <array>
#include <bit>
#include <stdexcept>
#include <string>
#include <utility>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
using Storage=NativeTu15PhysicsFrameStorage;
using Fp=NativeTu15PhysicsFrameFp;
using Services=NativeTu15PhysicsFrameServices;
using Phase=NativeTu15PhysicsReleasePhase;
std::int32_t signed_word(std::uint32_t n){return std::bit_cast<std::int32_t>(n);}
std::int32_t next_index(std::int32_t n){return signed_word(std::bit_cast<std::uint32_t>(n)+1u);}
template<class F,class... Args> void owner(const F&f,const char*name,Args&&...args) {
  if(!f)throw std::runtime_error(std::string("Native TU1.5 physics frame requires ")+name);
  f(std::forward<Args>(args)...);
}
// Actual32B26C/32B2FE control-word encoding. These are floating-point policy
// fields, not a simulated CPU/register file. Reserved x87 bits normalize on
// the real host backend exactly as FLDCW/FNSTCW do in the original owner.
std::uint32_t control_policy(std::uint16_t cw) {
  std::uint32_t out{};
  for(const auto [from,to]:std::array<std::pair<unsigned,unsigned>,6>{{{1,16},{4,8},{8,4},{16,2},{32,1},{2,0x80000}}})
    if(cw&from)out|=to;
  out|=(cw&0xc00u)>>2u;
  if(!(cw&0x300u))out|=0x20000u;
  else if((cw&0x300u)==0x200u)out|=0x10000u;
  if(cw&0x1000u)out|=0x40000u;
  return out;
}
std::uint16_t control_word(std::uint32_t policy) {
  std::uint16_t out{};
  for(const auto [from,to]:std::array<std::pair<unsigned,unsigned>,6>{{{16,1},{8,4},{4,8},{2,16},{1,32},{0x80000,2}}})
    if(policy&from)out|=std::uint16_t(to);
  out|=std::uint16_t((policy&0x300u)<<2u);
  if(!(policy&0x30000u))out|=0x300u;
  else if((policy&0x30000u)==0x10000u)out|=0x200u;
  if(policy&0x40000u)out|=0x1000u;
  return out;
}
std::uint16_t normalized_control(std::uint16_t requested) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  std::uint16_t previous{},actual{};
  asm volatile("fnstcw %0":"=m"(previous));
  asm volatile("fldcw %1\n\tfnstcw %0\n\tfldcw %2":"=m"(actual):"m"(requested),"m"(previous):"memory");
  return actual;
#else
  (void)requested;throw std::runtime_error("Exact native TU1.5 physics control-word backend unavailable");
#endif
}
void set_control(Fp&fp,std::uint32_t value,std::uint32_t mask) {
  if(fp.status&~fp.control_word&0x3fu)
    throw std::runtime_error("Native TU1.5 physics pending unmasked x87 fault requires its platform owner");
  fp.control_word=normalized_control(control_word((control_policy(fp.control_word)&~mask)|(value&mask)));
}
std::uint32_t inverse_seconds(std::uint32_t one,std::uint32_t seconds,Fp&fp) {
#if defined(__i386__) || defined(__x86_64__)
  if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
    throw std::runtime_error("Native TU1.5 physics SSE exception environment requires its platform owner");
  struct Restore {unsigned value=_mm_getcsr();~Restore(){_mm_setcsr(value);}} restore;
  _mm_setcsr(fp.mxcsr);
  const auto a=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(one)));
  const auto b=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(seconds)));
  const auto result=_mm_div_ss(a,b);
  const auto bits=std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(result)));
  fp.mxcsr=_mm_getcsr();return bits;
#else
  (void)one;(void)seconds;(void)fp;
  throw std::runtime_error("Exact native TU1.5 physics scalar division backend unavailable");
#endif
}
void release(Storage&s,NativeTu15PhysicsBody body,Phase phase,Fp&fp,const Services&services) {
  const auto refs=std::uint16_t(s.references(body)-1u);
  s.references(body,refs,phase,false);
  // The completed store can notify an observer. The source explicitly reads
  // the live reference word again before deciding whether to destroy.
  if(s.references(body))return;
  if(s.protected_reference(body))s.references(body,1,phase,true);
  else owner(services.destroy_body,"body destruction",body,true,fp);
}
}

bool native_tu15_next_physics_body(Storage&s,NativeTu15PhysicsBodyIterator&i) {
  if(i.island==-1) {
    i.island=0;i.body=0;
    if(i.selection==NativeTu15PhysicsIslandSelection::Inactive)i.inactive=true;
  } else i.body=next_index(i.body);
  if(!i.inactive) {
    while(i.island<s.island_count(i.world,false)) {
      if(i.body<s.body_count(i))return true;
      i.island=next_index(i.island);i.body=0;
    }
    i.inactive=true;i.island=0;i.body=0;
  }
  if(i.selection!=NativeTu15PhysicsIslandSelection::Inactive &&
     i.selection!=NativeTu15PhysicsIslandSelection::Both)return false;
  while(i.island<s.island_count(i.world,true)) {
    if(i.body<s.body_count(i))return true;
    i.island=next_index(i.island);i.body=0;
  }
  return false;
}
void native_tu15_publish_physics_body(Storage&s,NativeTu15PhysicsBody body,Fp&fp,const Services&services) {
  std::uint32_t component{};
  const auto count=s.property_count(body);
  if(count>0) {
    const auto properties=s.properties(body);
    for(std::int32_t n=0;n<count;++n)
      if(s.property_key(properties,std::uint32_t(n))==0x2001u) {
        component=s.property_value(properties,std::uint32_t(n));break;
      }
  }
  const auto record=s.component(std::uint16_t(component));
  if(s.component_publishes(record))owner(services.publish_object,"object transform publication",s.component_object(record),fp);
}
void native_tu15_retire_physics_references(Storage&s,NativeTu15PhysicsManager manager,Fp&fp,const Services&services) {
  while(next_index(s.deferred_cursor(manager))<s.deferred_count(manager)) {
    const auto next=next_index(s.deferred_cursor(manager));
    const auto bodies=s.deferred_bodies(manager);
    s.deferred_cursor(manager,next,Phase::Drain,false);
    release(s,s.deferred_body(bodies,next),Phase::Drain,fp,services);
  }
  s.deferred_cursor(manager,-1,Phase::Drain,true);
  s.clear_deferred_count(manager);
}
void native_tu15_finish_physics_frame(Storage&s,Fp&fp,const Services&services) {
  const auto world=s.world();auto manager=s.manager();
  s.iteration_depth(std::uint16_t(s.iteration_depth()+1u),true);
  s.stepping(manager,false);
  NativeTu15PhysicsBodyIterator iterator{world};
  while(native_tu15_next_physics_body(s,iterator))
    native_tu15_publish_physics_body(s,s.body(iterator),fp,services);
  s.iteration_depth(std::uint16_t(s.iteration_depth()-1u),false);
  while(next_index(s.deferred_cursor(manager))<s.deferred_count(manager)) {
    const auto next=next_index(s.deferred_cursor(manager));
    const auto bodies=s.deferred_bodies(manager);
    s.deferred_cursor(manager,next,Phase::Frame,false);
    const auto body=s.deferred_body(bodies,next);
    release(s,body,Phase::Frame,fp,services);
    // The original queries this after destruction. Storage remains readable
    // through this deferred phase; do not cache the pre-destructor active bit.
    if(!s.active(body))native_tu15_publish_physics_body(s,body,fp,services);
    manager=s.manager();
  }
  native_tu15_retire_physics_references(s,manager,fp,services);
}
void native_tu15_step_physics_frame(Storage&s,Fp&fp,const Services&services) {
  if(!s.world().identity)return;
  const auto seconds=s.seconds_bits();
  s.clear_allocation_failure();s.stepping(s.manager(),true);
  if(s.debug_arena()) {
    owner(services.resize_debug_arena,"debug arena resize",2000000u,fp);
    owner(services.reset_debug_arena,"debug arena reset",fp);
  }
  const NativeTu15PhysicsStep step{seconds,inverse_seconds(s.one_bits(),seconds,fp)};
  owner(services.drain_commands,"pre-step command drain",fp);
  set_control(fp,0x9001fu,0x8001fu);
  const auto world=s.world();fp.mxcsr|=0x1f80u;
  owner(services.step_world,"world collision/integration",world,step,fp);
  fp.mxcsr&=~0x3fu;fp.status&=0x7f00u;
  set_control(fp,0x9001fu,0xfffffu);
  owner(services.post_step,"post-step actions",fp);
  owner(services.update_contacts,"post-step contact updates",fp);
  owner(services.drain_commands,"post-step command drain",fp);
  if(const auto arena=s.debug_arena())owner(services.publish_debug_arena,"debug arena publication",arena,fp);
  native_tu15_finish_physics_frame(s,fp,services);
}

NativeTu15PhysicsFrameMemory::NativeTu15PhysicsFrameMemory(SourcePhysicsContactMemory&m,SourcePhysicsContactRecordObserver o)
  :memory_(m),observer_(std::move(o)){}
std::uint32_t NativeTu15PhysicsFrameMemory::read(std::uint32_t p,unsigned n)const {
  std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(n));std::uint32_t value{};
  for(unsigned i=0;i<n;++i)value|=std::to_integer<unsigned>(b[i])<<(i*8u);return value;
}
void NativeTu15PhysicsFrameMemory::write(std::uint32_t site,std::uint32_t p,unsigned n,std::uint32_t value) {
  std::array<std::byte,4>b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(value>>(i*8u));
  SourcePhysicsContactRecordStore store{site,p,{},{}};
  if(observer_){store.before.resize(n);memory_.read(p,store.before);store.written.assign(b.begin(),b.begin()+n);}
  memory_.write(p,std::span<const std::byte>(b).first(n));if(observer_)observer_(store);
}
NativeTu15PhysicsWorld NativeTu15PhysicsFrameMemory::world()const{return{read(0x52be48u)};}
NativeTu15PhysicsManager NativeTu15PhysicsFrameMemory::manager()const{return{read(0x52d880u)};}
std::uint32_t NativeTu15PhysicsFrameMemory::seconds_bits()const{return read(read(0x51df54u)+4u);}
std::uint32_t NativeTu15PhysicsFrameMemory::one_bits()const{return read(0x46a0d0u);}
std::uint32_t NativeTu15PhysicsFrameMemory::debug_arena()const{return read(0x48b85cu);}
void NativeTu15PhysicsFrameMemory::clear_allocation_failure(){write(0x2f30f4u,0x46e468u,1,0);}
void NativeTu15PhysicsFrameMemory::stepping(NativeTu15PhysicsManager p,bool value){write(value?0x1cc903u:0x1cc9e6u,p.identity+0x18u,1,value);}
std::uint16_t NativeTu15PhysicsFrameMemory::iteration_depth()const{return std::uint16_t(read(0x52d888u,2));}
void NativeTu15PhysicsFrameMemory::iteration_depth(std::uint16_t value,bool entering){write(entering?0x1cc9dbu:0x1cca48u,0x52d888u,2,value);}
std::int32_t NativeTu15PhysicsFrameMemory::island_count(NativeTu15PhysicsWorld world,bool inactive)const{return signed_word(read(world.identity+(inactive?0x18u:0xcu)));}
std::uint32_t NativeTu15PhysicsFrameMemory::island(const NativeTu15PhysicsBodyIterator&i)const {
  return read(read(i.world.identity+(i.inactive?0x14u:8u))+std::bit_cast<std::uint32_t>(i.island)*4u);
}
std::int32_t NativeTu15PhysicsFrameMemory::body_count(const NativeTu15PhysicsBodyIterator&i)const{return signed_word(read(island(i)+0x40u));}
NativeTu15PhysicsBody NativeTu15PhysicsFrameMemory::body(const NativeTu15PhysicsBodyIterator&i)const{return{read(read(island(i)+0x3cu)+std::bit_cast<std::uint32_t>(i.body)*4u)};}
std::int32_t NativeTu15PhysicsFrameMemory::property_count(NativeTu15PhysicsBody b)const{return signed_word(read(b.identity+0x34u));}
NativeTu15PhysicsProperties NativeTu15PhysicsFrameMemory::properties(NativeTu15PhysicsBody b)const{return{read(b.identity+0x30u)};}
std::uint32_t NativeTu15PhysicsFrameMemory::property_key(NativeTu15PhysicsProperties p,std::uint32_t n)const{return read(p.identity+n*8u);}
std::uint32_t NativeTu15PhysicsFrameMemory::property_value(NativeTu15PhysicsProperties p,std::uint32_t n)const{return read(p.identity+n*8u+4u);}
NativeTu15PhysicsComponent NativeTu15PhysicsFrameMemory::component(std::uint16_t n)const{return{read(read(0x52be5cu)+0x44u)+std::uint32_t(n)*0xa0u};}
bool NativeTu15PhysicsFrameMemory::component_publishes(NativeTu15PhysicsComponent p)const{return(read(p.identity+4u)&0x20u)!=0;}
NativeTu15PhysicsObject NativeTu15PhysicsFrameMemory::component_object(NativeTu15PhysicsComponent p)const{return{read(p.identity+8u)};}
bool NativeTu15PhysicsFrameMemory::active(NativeTu15PhysicsBody b)const{const auto p=read(b.identity+0x44u);return p&&read(p+0x29u,1)!=0;}
std::uint16_t NativeTu15PhysicsFrameMemory::references(NativeTu15PhysicsBody b)const{return std::uint16_t(read(b.identity+6u,2));}
void NativeTu15PhysicsFrameMemory::references(NativeTu15PhysicsBody b,std::uint16_t n,Phase phase,bool reset) {
  write(phase==Phase::Frame?(reset?0x1cca87u:0x1cca6fu):(reset?0x282047u:0x28202fu),b.identity+6u,2,n);
}
bool NativeTu15PhysicsFrameMemory::protected_reference(NativeTu15PhysicsBody b)const {
  constexpr std::uint32_t first=0x80061000u;
  if(signed_word(b.identity)<signed_word(first))return false;
  const auto enabled=read(0x556588u,1),bytes=read(0x5565acu);
  const auto last=first+(enabled?bytes:0u);
  return signed_word(b.identity)<=signed_word(last);
}
std::int32_t NativeTu15PhysicsFrameMemory::deferred_cursor(NativeTu15PhysicsManager m)const{return signed_word(read(m.identity+0x14u));}
std::int32_t NativeTu15PhysicsFrameMemory::deferred_count(NativeTu15PhysicsManager m)const{return signed_word(read(m.identity+0xcu));}
NativeTu15PhysicsDeferredBodies NativeTu15PhysicsFrameMemory::deferred_bodies(NativeTu15PhysicsManager m)const{return{read(m.identity+8u)};}
NativeTu15PhysicsBody NativeTu15PhysicsFrameMemory::deferred_body(NativeTu15PhysicsDeferredBodies p,std::int32_t n)const{return{read(p.identity+std::bit_cast<std::uint32_t>(n)*4u)};}
void NativeTu15PhysicsFrameMemory::deferred_cursor(NativeTu15PhysicsManager m,std::int32_t n,Phase phase,bool reset) {
  write(reset?0x282063u:(phase==Phase::Frame?0x1cca67u:0x282027u),m.identity+0x14u,4,std::bit_cast<std::uint32_t>(n));
}
void NativeTu15PhysicsFrameMemory::clear_deferred_count(NativeTu15PhysicsManager m){write(0x28206au,m.identity+0xcu,4,0);}
} // namespace halo2
