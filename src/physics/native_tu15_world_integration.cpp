#include "halo2/native_tu15_world_integration.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
using Storage=NativeTu15WorldIntegrationStorage;using Fp=NativeTu15PhysicsFrameFp;
using Services=NativeTu15WorldIntegrationServices;using Profile=NativeTu15WorldProfile;
template<class F,class...Args>void require(const F&f,const char*name,Args&&...args){if(!f)throw std::runtime_error(std::string("Native TU1.5 world integration requires ")+name);f(std::forward<Args>(args)...);}
// Each four-stage scaling expression is native x87 arithmetic. This scope
// preserves the host's unrelated physical FP state and captures actual source
// policy/status without representing a guest register file or dispatch loop.
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class NumericEnvironment {
  alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};unsigned mxcsr_{};Fp&fp_;
public:
  NumericEnvironment(Fp&fp,unsigned depth):fp_(fp){
    if(depth>7||(fp.control_word&0x3f)!=0x3f||(fp.status&0x8080)||(fp.mxcsr&0x1f80)!=0x1f80||(fp.mxcsr&0xffff0000))throw std::runtime_error("Native TU1.5 world numeric platform fault owner unavailable");
    mxcsr_=_mm_getcsr();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    std::memcpy(native_.data(),host_.data(),28);const std::uint16_t tags=0xffff;
    std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&tags,2);
    asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
  }
  ~NumericEnvironment(){asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(mxcsr_);}
};
void load(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
void load(std::int32_t n){asm volatile("fildl %0"::"m"(n):"memory","st");}
std::uint32_t multiply(std::uint32_t bits){std::uint32_t out;asm volatile("fmuls %1\n\tfstps %0":"=m"(out):"m"(bits):"memory","st");return out;}
#endif
void step_parameters(Storage&s,NativeTu15PhysicsWorld world,NativeTu15PhysicsStepPacket step,Fp&fp,unsigned depth){
  s.step_output(world,NativeTu15WorldStepOutput::Seconds,s.seconds(step));
  s.step_output(world,NativeTu15WorldStepOutput::Inverse,s.inverse_seconds(step));
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  NumericEnvironment environment(fp,depth);
  load(s.scalar(world,NativeTu15WorldScalar::SolverInterval));const auto scaled=multiply(s.seconds(step));
  s.step_output(world,NativeTu15WorldStepOutput::ScaledSeconds,scaled);
  const auto cached_scaled=s.scalar(world,NativeTu15WorldScalar::ScaledSeconds);
  load(s.solver_iterations(world));s.step_output(world,NativeTu15WorldStepOutput::ScaledInverse,multiply(s.inverse_seconds(step)));
  s.step_output(world,NativeTu15WorldStepOutput::JacobianSeconds,cached_scaled);
  s.step_output(world,NativeTu15WorldStepOutput::JacobianInverse,s.scalar(world,NativeTu15WorldScalar::ScaledInverse));
  load(s.solver_iterations(world));s.step_output(world,NativeTu15WorldStepOutput::IterationSeconds,multiply(s.scalar(world,NativeTu15WorldScalar::ScaledSeconds)));
  load(s.scalar(world,NativeTu15WorldScalar::SolverInterval));s.step_output(world,NativeTu15WorldStepOutput::IterationInverse,multiply(s.scalar(world,NativeTu15WorldScalar::ScaledInverse)));
#else
  (void)fp;(void)depth;throw std::runtime_error("Exact native TU1.5 world numeric backend unavailable");
#endif
}
std::uint32_t profile(Storage&s,std::uint32_t cursor,Profile phase,const Services&services){
  if(cursor>=s.profile_limit())return cursor;s.profile_label(cursor,phase);
  if(!services.timestamp_counter)throw std::runtime_error("Native TU1.5 world requires actual platform profiling timestamp");
  s.profile_counter(cursor,std::uint32_t(services.timestamp_counter()),phase);s.profile_cursor(cursor+12u,phase);return cursor+12u;
}
}
void native_tu15_integrate_physics_world(Storage&s,NativeTu15PhysicsWorld world,NativeTu15PhysicsStepPacket step,Fp&fp,
  unsigned depth,const Services&services,NativeTu15WorldIntegrationStats*stats){
  if(stats)++stats->world_integrations;
  // Original319F90 reads limit before cursor at this initial marker.
  const auto initial_limit=s.profile_limit(),initial_cursor=s.profile_cursor();
  if(initial_cursor<initial_limit){s.profile_label(initial_cursor,Profile::Begin);if(!services.timestamp_counter)throw std::runtime_error("Native TU1.5 world requires actual platform profiling timestamp");s.profile_counter(initial_cursor,std::uint32_t(services.timestamp_counter()),Profile::Begin);s.profile_cursor(initial_cursor+12u,Profile::Begin);}
  step_parameters(s,world,step,fp,depth);
  // Original31D400 is an authenticated bare RET, with no state/results.
  if(std::bit_cast<std::int32_t>(s.count(world,NativeTu15WorldCounter::Operations))>0)require(services.drain_operations,"operation drain",world,fp);
  if(s.count(world,NativeTu15WorldCounter::AddedActions))require(services.add_actions,"pending actions",world,fp);
  auto index=s.count(world,NativeTu15WorldCounter::ActiveIslands)-1u;s.clear_added_actions(world);s.integrating(world,true);
  while(std::bit_cast<std::int32_t>(index)>=0){
    const auto check_deactivation=s.flag(world,NativeTu15WorldFlag::CheckDeactivation);const auto island=s.island(world,index);bool deactivate=false;
    if(check_deactivation){if(!services.check_deactivation)throw std::runtime_error("Native TU1.5 world requires island deactivation test");deactivate=services.check_deactivation(island,step,fp);}
    if(deactivate)require(services.deactivate_island,"island deactivation",world,island,fp);
    else{native_tu15_integrate_physics_island(s.islands(),island,{world.identity+0x140u},fp,depth,services.island,stats?&stats->island:nullptr);s.clear_island_activation(island);}
    if(s.has_island_listeners(world)){
      profile(s,s.profile_cursor(),Profile::IslandListeners,services);require(services.notify_island_listeners,"island integration listeners",world,island,step,fp);
      profile(s,s.profile_cursor(),Profile::IslandEnd,services);
    }
    --index;
  }
  if(s.flag(world,NativeTu15WorldFlag::RegroupIslands))require(services.regroup_islands,"island regrouping",world,fp);
  const auto pending_deletes=s.count(world,NativeTu15WorldCounter::PendingDeletes);s.integrating(world,false);
  if(pending_deletes)require(services.drain_deletes,"pending body deletion",world,fp);
  if(s.count(world,NativeTu15WorldCounter::PendingUpdates))require(services.drain_updates,"pending body updates",world,fp);
  for(std::uint32_t n=0;std::bit_cast<std::int32_t>(n)<std::bit_cast<std::int32_t>(s.count(world,NativeTu15WorldCounter::RetiredBodies));++n){
    const auto body=s.retired_body(world,n);require(services.detach_body,"body world detach",body,fp);
    const auto retained=s.retired_body(world,n);s.references(retained,std::uint16_t(s.references(retained)-1u),false);
    if(!s.references(retained)){if(s.protected_reference(retained))s.references(retained,1,true);else require(services.destroy_body,"body destruction",retained,fp);}
  }
  s.clear_retired_bodies(world);
  auto cursor=s.profile_cursor();if(cursor<s.profile_limit()){
    s.profile_label(cursor,Profile::BeforeEndListeners);if(!services.timestamp_counter)throw std::runtime_error("Native TU1.5 world requires actual platform profiling timestamp");s.profile_counter(cursor,std::uint32_t(services.timestamp_counter()),Profile::BeforeEndListeners);
    const auto limit=s.profile_limit();cursor+=12u;s.profile_cursor(cursor,Profile::BeforeEndListeners);
    if(cursor<limit){s.profile_label(cursor,Profile::EndListeners);s.profile_counter(cursor,std::uint32_t(services.timestamp_counter()),Profile::EndListeners);s.profile_cursor(cursor+12u,Profile::EndListeners);}
  }
  require(services.notify_world_listeners,"world integration listeners",world,step,fp);profile(s,s.profile_cursor(),Profile::End,services);
}
NativeTu15WorldIntegrationMemory::NativeTu15WorldIntegrationMemory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o),islands_(m,o){}
std::uint32_t NativeTu15WorldIntegrationMemory::read(std::uint32_t p,unsigned size)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(size));std::uint32_t n{};for(unsigned i=0;i<size;++i)n|=std::to_integer<unsigned>(b[i])<<(i*8u);return n;}
void NativeTu15WorldIntegrationMemory::write(std::uint32_t site,std::uint32_t p,std::uint32_t n,unsigned size){std::array<std::byte,4>b{};for(unsigned i=0;i<size;++i)b[i]=std::byte(n>>(i*8u));SourcePhysicsContactRecordStore store{site,p,{},{}};if(observer_){store.before.resize(size);memory_.read(p,store.before);store.written.assign(b.begin(),b.begin()+size);}memory_.write(p,std::span<const std::byte>(b).first(size));if(observer_)observer_(store);}
NativeTu15IslandIntegrationStorage&NativeTu15WorldIntegrationMemory::islands(){return islands_;}
std::uint32_t NativeTu15WorldIntegrationMemory::count(NativeTu15PhysicsWorld p,NativeTu15WorldCounter c)const{constexpr std::array<std::uint32_t,6>offsets{0x24,0x10c,0xc,0xf4,0x100,0x118};return read(p.identity+offsets.at(unsigned(c)));}
bool NativeTu15WorldIntegrationMemory::flag(NativeTu15PhysicsWorld p,NativeTu15WorldFlag f)const{constexpr std::array<std::uint32_t,3>offsets{0x23d,0x23c,0x12c};return read(p.identity+offsets.at(unsigned(f)),1)!=0;}
void NativeTu15WorldIntegrationMemory::clear_added_actions(NativeTu15PhysicsWorld p){write(0x31a05a,p.identity+0x10c,0);}
void NativeTu15WorldIntegrationMemory::integrating(NativeTu15PhysicsWorld p,bool active){write(active?0x31a064:0x31a139,p.identity+0x12c,active,1);}
NativeTu15PhysicsIsland NativeTu15WorldIntegrationMemory::island(NativeTu15PhysicsWorld p,std::uint32_t n)const{return{read(read(p.identity+8)+n*4u)};}
void NativeTu15WorldIntegrationMemory::clear_island_activation(NativeTu15PhysicsIsland p){write(0x31a0ab,p.identity+0x30,0,1);}
bool NativeTu15WorldIntegrationMemory::has_island_listeners(NativeTu15PhysicsWorld p)const{return read(p.identity+0xa0)!=0;}
NativeTu15PhysicsBody NativeTu15WorldIntegrationMemory::retired_body(NativeTu15PhysicsWorld p,std::uint32_t n)const{return{read(read(p.identity+0x114)+n*4u)};}
std::uint16_t NativeTu15WorldIntegrationMemory::references(NativeTu15PhysicsBody p)const{return std::uint16_t(read(p.identity+6,2));}
void NativeTu15WorldIntegrationMemory::references(NativeTu15PhysicsBody p,std::uint16_t n,bool reset){write(reset?0x31a19f:0x31a187,p.identity+6,n,2);}
bool NativeTu15WorldIntegrationMemory::protected_reference(NativeTu15PhysicsBody p)const{constexpr std::uint32_t first=0x80061000;if(std::bit_cast<std::int32_t>(p.identity)<std::bit_cast<std::int32_t>(first))return false;const auto enabled=read(0x556588,1),size=read(0x5565ac);return std::bit_cast<std::int32_t>(p.identity)<=std::bit_cast<std::int32_t>(first+(enabled?size:0u));}
void NativeTu15WorldIntegrationMemory::clear_retired_bodies(NativeTu15PhysicsWorld p){write(0x31a1ba,p.identity+0x118,0);}
std::uint32_t NativeTu15WorldIntegrationMemory::seconds(NativeTu15PhysicsStepPacket p)const{return read(p.identity);}
std::uint32_t NativeTu15WorldIntegrationMemory::inverse_seconds(NativeTu15PhysicsStepPacket p)const{return read(p.identity+4);}
std::uint32_t NativeTu15WorldIntegrationMemory::scalar(NativeTu15PhysicsWorld p,NativeTu15WorldScalar f)const{constexpr std::array<std::uint32_t,3>offsets{0x180,0x170,0x174};return read(p.identity+offsets.at(unsigned(f)));}
std::int32_t NativeTu15WorldIntegrationMemory::solver_iterations(NativeTu15PhysicsWorld p)const{return std::bit_cast<std::int32_t>(read(p.identity+0x17c));}
void NativeTu15WorldIntegrationMemory::step_output(NativeTu15PhysicsWorld p,NativeTu15WorldStepOutput f,std::uint32_t n){constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,8>fields{{{0x319fc8,0x150},{0x319fd1,0x154},{0x319fdf,0x170},{0x319ff4,0x174},{0x319ffa,0x198},{0x31a006,0x19c},{0x31a018,0x1a0},{0x31a02a,0x1a4}}};const auto[site,offset]=fields.at(unsigned(f));write(site,p.identity+offset,n);}
std::uint32_t NativeTu15WorldIntegrationMemory::profile_cursor()const{return read(0x48c974);}
std::uint32_t NativeTu15WorldIntegrationMemory::profile_limit()const{return read(0x48c978);}
void NativeTu15WorldIntegrationMemory::profile_label(std::uint32_t cursor,NativeTu15WorldProfile p){constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,6>fields{{{0x319fa6,0x41da10},{0x31a0c7,0x41d9fc},{0x31a0fd,0x41c394},{0x31a1d4,0x41cabc},{0x31a1f9,0x41d9e8},{0x31a230,0x41c394}}};const auto[site,label]=fields.at(unsigned(p));write(site,cursor,label);}
void NativeTu15WorldIntegrationMemory::profile_counter(std::uint32_t cursor,std::uint32_t ticks,NativeTu15WorldProfile p){constexpr std::array<std::uint32_t,6>sites{0x319fb6,0x31a0d7,0x31a10d,0x31a1e4,0x31a209,0x31a240};write(sites.at(unsigned(p)),cursor+4,ticks);}
void NativeTu15WorldIntegrationMemory::profile_cursor(std::uint32_t cursor,NativeTu15WorldProfile p){constexpr std::array<std::uint32_t,6>sites{0x319fbc,0x31a0dd,0x31a113,0x31a1f1,0x31a20f,0x31a246};write(sites.at(unsigned(p)),0x48c974,cursor);}
} // namespace halo2
