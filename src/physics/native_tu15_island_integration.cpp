#include "halo2/native_tu15_island_integration.hpp"
#include <bit>
#include <stdexcept>
#include <string>
#include <utility>
namespace halo2 {
namespace {
template<class F,class...Args>void require(const F&f,const char*name,Args&&...args){if(!f)throw std::runtime_error(std::string("Native TU1.5 island requires ")+name);f(std::forward<Args>(args)...);}
std::uint32_t profile(NativeTu15IslandIntegrationStorage&s,std::uint32_t cursor,bool beginning,const NativeTu15IslandIntegrationServices&services){
  if(cursor>=s.profile_limit())return cursor;
  s.profile_label(cursor,beginning);
  if(!services.timestamp_counter)throw std::runtime_error("Native TU1.5 island requires actual platform profiling timestamp");
  const auto ticks=std::uint32_t(services.timestamp_counter());s.profile_counter(cursor,ticks,beginning);s.profile_cursor(cursor+12u,beginning);return cursor+12u;
}
}
void native_tu15_integrate_physics_island(NativeTu15IslandIntegrationStorage&s,
  NativeTu15PhysicsIsland island,NativeTu15PhysicsStepConfiguration configuration,NativeTu15PhysicsFrameFp&fp,
  unsigned depth,const NativeTu15IslandIntegrationServices&services,NativeTu15IslandIntegrationStats*stats){
  if(stats)++stats->islands;
  auto cursor=s.actions(island).identity;const auto end=cursor+s.action_count(island)*4u;
  const NativeTu15PhysicsStepPacket step{configuration.identity+0x10u};
  while(cursor!=end){const auto action=s.action(cursor);const auto owner=s.action_owner(action);
    if(stats)++stats->action_calls;require(services.update_action,"action update",action,owner,step,fp);cursor+=4u;}
  const auto count=s.constraint_count(island);
  if(count){
    NativeTu15PhysicsConstraintInput input;input.constraint_count=count;input.constraints=s.constraints(island);
    input.body_count=s.body_count(island);input.bodies=s.bodies(island);const auto world=s.world(island);
    input.island=island;input.gravity={world.identity+0xe0u};input.step=step;
    input.solver_configuration=configuration.identity+0x20u;input.jacobian_context=configuration.identity+0x50u;
    if(stats)++stats->constraint_calls;require(services.solve_constraints,"constraint solver",input,fp);return;
  }
  cursor=profile(s,s.profile_cursor(),true,services);
  // With no bodies the original retains ECX from its first profile decision;
  // it does not re-read the global cursor before the closing marker.
  auto index=s.body_count(island)-1u;
  if(std::bit_cast<std::int32_t>(index)>=0){
    do{
      const auto bodies=s.bodies(island);const auto body=s.body(bodies,index);const auto world=s.world(island);
      const auto motion=s.motion(body);const auto owner=s.force_owner(motion);const NativeTu15PhysicsGravity gravity{world.identity+0xe0u};
      if(owner.identity==0x3252d0u){if(stats)++stats->native_force_calls;native_tu15_apply_motion_forces(s.motion_forces(motion,step,gravity),fp,depth);}
      else{if(stats)++stats->other_force_calls;require(services.apply_other_motion_forces,"unconverted motion force kind",motion,owner,step,gravity,fp);}
      // Re-read the pose virtual owner after force work; a residual force
      // owner can change the motion's retained type before this phase.
      const auto pose=s.pose_owner(motion);if(stats)++stats->pose_calls;
      require(services.integrate_pose,"motion pose integration",motion,pose,step,fp);
      --index;
    }while(std::bit_cast<std::int32_t>(index)>=0);
    cursor=s.profile_cursor();
  }
  profile(s,cursor,false,services);
}
NativeTu15IslandIntegrationMemory::NativeTu15IslandIntegrationMemory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
std::uint32_t NativeTu15IslandIntegrationMemory::read(std::uint32_t p)const{std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t n{};for(unsigned i=0;i<4;++i)n|=std::to_integer<unsigned>(b[i])<<(i*8u);return n;}
void NativeTu15IslandIntegrationMemory::write(std::uint32_t site,std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(n>>(i*8u));SourcePhysicsContactRecordStore store{site,p,{},{}};if(observer_){store.before.resize(4);memory_.read(p,store.before);store.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(store);}
NativeTu15PhysicsArray NativeTu15IslandIntegrationMemory::actions(NativeTu15PhysicsIsland p)const{return{read(p.identity+0x4c)};}
std::uint32_t NativeTu15IslandIntegrationMemory::action_count(NativeTu15PhysicsIsland p)const{return read(p.identity+0x50);}
NativeTu15PhysicsAction NativeTu15IslandIntegrationMemory::action(std::uint32_t cursor)const{return{read(cursor)};}
NativeTu15PhysicsVirtualOwner NativeTu15IslandIntegrationMemory::action_owner(NativeTu15PhysicsAction p)const{return{read(read(p.identity)+0x14)};}
std::uint32_t NativeTu15IslandIntegrationMemory::constraint_count(NativeTu15PhysicsIsland p)const{return read(p.identity+0x5c);}
std::uint32_t NativeTu15IslandIntegrationMemory::constraints(NativeTu15PhysicsIsland p)const{return read(p.identity+0x58);}
std::uint32_t NativeTu15IslandIntegrationMemory::body_count(NativeTu15PhysicsIsland p)const{return read(p.identity+0x40);}
NativeTu15PhysicsArray NativeTu15IslandIntegrationMemory::bodies(NativeTu15PhysicsIsland p)const{return{read(p.identity+0x3c)};}
NativeTu15PhysicsBody NativeTu15IslandIntegrationMemory::body(NativeTu15PhysicsArray p,std::uint32_t n)const{return{read(p.identity+n*4u)};}
NativeTu15PhysicsWorld NativeTu15IslandIntegrationMemory::world(NativeTu15PhysicsIsland p)const{return{read(p.identity+0x20)};}
NativeTu15PhysicsMotion NativeTu15IslandIntegrationMemory::motion(NativeTu15PhysicsBody p)const{return{read(p.identity+0x3c)};}
NativeTu15PhysicsVirtualOwner NativeTu15IslandIntegrationMemory::force_owner(NativeTu15PhysicsMotion p)const{return{read(read(p.identity)+8)};}
NativeTu15PhysicsVirtualOwner NativeTu15IslandIntegrationMemory::pose_owner(NativeTu15PhysicsMotion p)const{return{read(read(p.identity)+4)};}
NativeTu15MotionForceStorage& NativeTu15IslandIntegrationMemory::motion_forces(NativeTu15PhysicsMotion m,NativeTu15PhysicsStepPacket step,NativeTu15PhysicsGravity gravity){force_.emplace(memory_,m.identity,step.identity,gravity.identity,observer_);return*force_;}
std::uint32_t NativeTu15IslandIntegrationMemory::profile_cursor()const{return read(0x48c974);}
std::uint32_t NativeTu15IslandIntegrationMemory::profile_limit()const{return read(0x48c978);}
void NativeTu15IslandIntegrationMemory::profile_label(std::uint32_t cursor,bool beginning){write(beginning?0x320bb8:0x320c15,cursor,beginning?0x41f068:0x41c394);}
void NativeTu15IslandIntegrationMemory::profile_counter(std::uint32_t cursor,std::uint32_t ticks,bool beginning){write(beginning?0x320bc8:0x320c27,cursor+4,ticks);}
void NativeTu15IslandIntegrationMemory::profile_cursor(std::uint32_t value,bool beginning){write(beginning?0x320bce:0x320c2e,0x48c974,value);}
} // namespace halo2
