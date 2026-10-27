#include "halo2/native_tu15_world_integration_residuals.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
namespace {
using namespace halo2;
void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Memory:SourcePhysicsContactMemory{
  std::map<std::uint32_t,std::vector<std::byte>>banks;
  auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing residual model bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing residual model extent");return i;}
  void read(std::uint32_t p,std::span<std::byte>b)const override{auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
  void write(std::uint32_t p,std::span<const std::byte>b)override{auto i=at(p,b.size());std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
  void put(std::uint32_t p,std::uint32_t n,unsigned size=4){std::array<std::byte,4>b{};for(unsigned i=0;i<size;++i)b[i]=std::byte(n>>(i*8u));write(p,std::span(b).first(size));}
  std::uint32_t get(std::uint32_t p,unsigned size=4)const{std::array<std::byte,4>b{};read(p,std::span(b).first(size));std::uint32_t n{};for(unsigned i=0;i<size;++i)n|=std::to_integer<unsigned>(b[i])<<(i*8u);return n;}
};
constexpr std::uint32_t world=0x1000000,island0=world+0x1000,island1=world+0x1100,island2=world+0x1200,
  bodies=world+0x2000,body0=world+0x2100,body1=world+0x2200,body2=world+0x2300,body3=world+0x2400,
  motion0=world+0x3000,motion1=world+0x3200,motion2=world+0x3400,motion3=world+0x3600,
  action=world+0x4000,otherForce=0x230020,otherPose=0x230030,actionOwner=0x230010,destroy=0x230040,policyOwner=0x230050,arrayAllocate=0x230060,arrayRelease=0x230070,worldCollisionListener=0x230080,islandCollisionListener=0x230090,constraintBuilder=0x2300a0,shapeBounds=0x2300b0,dispatcherSweep=0x2300c0,agentProcess=0x2300d0,agentFinish=0x2300e0,nestedBounds=0x2300f0,
  retired0=world+0x4400,retired1=world+0x4500,retiredReplacement=world+0x4600,
  stack=0x60000000,top=stack+0x10000;
Memory model(){Memory m;m.banks[world].resize(0x10000);m.banks[0x400000].resize(0x160000);m.banks[stack].resize(0x10000);m.banks[0x56d000].resize(0x1000);
  m.put(world+8,world+0x900);m.put(world+0x900,island0);m.put(world+0x904,island1);m.put(world+0x908,island2);m.put(world+0xc,3);
  for(auto island:{island0,island1,island2}){m.put(island+0x20,world);m.put(island+0x3c,bodies);}
  m.put(island0+0x40,4);m.put(bodies,body0);m.put(bodies+4,body1);m.put(bodies+8,body2);m.put(bodies+12,body3);
  m.put(body0+0x3c,motion0);m.put(body1+0x3c,motion1);m.put(body2+0x3c,motion2);m.put(body3+0x3c,motion3);
  m.put(island0+0x24,0);m.put(island1+0x24,1);m.put(island2+0x24,2);
  m.put(motion0,world+0x5000);m.put(world+0x5008,0x3252d0);m.put(world+0x5004,0x325170);m.put(motion1,world+0x5100);m.put(world+0x5108,otherForce);m.put(world+0x5104,otherPose);
  m.put(motion2,world+0x5140);m.put(world+0x5144,0x323730);m.put(world+0x5148,0x8fdb0);
  m.put(motion3,world+0x5180);m.put(world+0x5184,0x2e4d60);m.put(world+0x5188,0x8fdb0);
  for(auto motion:{motion0,motion1,motion2,motion3}){m.put(motion+0x80,0x3f800000);m.put(motion+0x94,0x3f800000);m.put(motion+0xa8,0x3f800000);m.put(motion+0x2c,0x3f800000);}
  m.put(island0+0x4c,world+0x4020);m.put(island0+0x50,1);m.put(world+0x4020,action);m.put(action,world+0x5200);m.put(world+0x5214,actionOwner);
  m.put(island2+0x2c,5,1);
  m.put(0x48c938,world+0x5e00);m.put(world+0x5e00,world+0x5e40);m.put(world+0x5e50,arrayAllocate);m.put(world+0x5e54,arrayRelease);
  m.put(body0+0x54,world+0x5400);m.put(world+0x5400,world+0x5480);m.put(world+0x5494,policyOwner);
  for(auto body:{body1,body2,body3})m.put(body+0x54,world+0x5a00);
  m.put(world+0x5a00,world+0x5b00);m.put(world+0x5b14,0x31e5c0);
  m.put(world+0x5a08,0x3f800000);m.put(world+0x5a50,0x40000000);m.put(world+0x5a54,0x40000000);
  m.put(0x486070,world+0x6800);m.put(world+0x6818,world+0x6900);m.put(world+0x681c,8);m.put(world+0x6820,1);
  m.put(0x48d480,1);m.put(0x491350,world+0x6000);m.put(world+0x6008,world+0x8000);m.put(world+0x600c,0x1000);
  for(unsigned n=0;n<2;++n){const auto constraint=world+0x6400+n*0x100;m.put(world+0x4200+n*4,constraint);m.put(constraint,world+0x6600);m.put(constraint+0xc,body0);m.put(constraint+0x10,body1);}
  m.put(world+0x6624,constraintBuilder);
  m.put(world+0x5e08,world+0xa000);m.put(world+0x5e0c,0x1000);m.put(world+0x5e14,world+0xb000);
  m.put(world+0x238,2);m.put(world+0x5d40,world+0x6c00);m.put(world+0x6c14,dispatcherSweep);m.put(world+0x6c18,shapeBounds);
  for(unsigned n=0;n<4;++n){m.put(body0+n*0x100+0xc,world+0x6a00+n*0x20);m.put(body0+n*0x100+0x14,motion0+n*0x200);m.put(world+0x6a00+n*0x20,world+0x6c00);}
  m.put(island0+0x74,world+0x6d00);m.put(island0+0x78,2);m.put(world+0x6d00,0x12345678);m.put(world+0x6d04,0x87654321);m.put(world+0x6d08,0x11223344);m.put(world+0x6d0c,0x55667788);
  m.put(island1+0x5c,2);m.put(island1+0x58,world+0x4200);
  m.put(island0+0x68,world+0x7100);m.put(island0+0x6c,1);
  m.put(world+0x7100,body0+16);m.put(world+0x7104,body1+16);m.put(world+0x7108,world+0x7000);
  m.put(world+0x7000,world+0x7040);m.put(world+0x7050,agentProcess);m.put(world+0x7008,world+0x7080);
  m.put(world+0x7080,world+0x70c0);m.put(world+0x70cc,agentFinish);
  m.put(world+0x90,world+0x5f00);m.put(world+0x94,2);m.put(world+0x5f04,world+0x5f20);m.put(world+0x5f20,world+0x5f40);m.put(world+0x5f44,worldCollisionListener);
  m.put(world+0xa8,world+0x5f60);m.put(world+0xac,1);m.put(world+0x5f60,world+0x5f80);m.put(world+0x5f80,world+0x5fa0);m.put(world+0x5fa4,islandCollisionListener);
  m.put(world+0xcc,world+0x5d00);m.put(world+0xc4,world+0x5d40);
  m.put(world+0x24,1);m.put(world+0x10c,1);m.put(world+0xa0,1);m.put(world+0x23d,1,1);m.put(world+0x23c,1,1);m.put(world+0xf4,1);m.put(world+0x100,1);
  m.put(world+0x114,world+0x4300);m.put(world+0x118,2);m.put(world+0x4300,retired0);m.put(world+0x4304,retired1);
  m.put(retired0+6,9,2);m.put(retiredReplacement+6,2,2);m.put(retired1+6,1,2);m.put(retired1,world+0x5300);m.put(world+0x5300,destroy);
  m.put(world+0x17c,8);m.put(world+0x180,0x3e000000);m.put(0x46a0d0,0x3f800000);m.put(0x46a0e8,0);m.put(0x46a0e4,0x40400000);m.put(0x46a0cc,0x3f000000);
  for(unsigned lane=0;lane<3;++lane)m.put(0x41f1d0+lane*4,0xffffffff);
  for(auto motion:{motion0,motion2,motion3}){m.put(motion+0x3c,0x3f800000);m.put(motion+0xc,0x3e800000);m.put(motion+0x40,0x3f800000);}
  m.put(motion2+0xf0,0x3f800000);m.put(motion3+0xf0,0x3f800000);return m;}
SourcePhysicsContactRuntimeAbi initial(){SourcePhysicsContactRuntimeAbi a{};a.gp[4]=top;a.fs_base=0x60800000;a.x87_depth=2;a.x87[0]=1.25L;a.x87[1]=-4.L;a.x87_status=0x2800;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};return a;}
}
int main(){try{
  auto m=model();auto a=initial();NativeTu15PhysicsFrameFp fp{0x27f,0x2800,0x1f80};NativeTu15WorldIntegrationStats stats;std::uint64_t residuals{};std::vector<std::uint32_t>calls;std::uint32_t packet=top-0x10u;unsigned checks{};
  m.put(0x48c974,world+0x5600);m.put(0x48c978,world+0x5800);std::uint64_t timestamps{};
  bool corrupt_clock{};
  auto invoke=[&](auto target,auto&actual,const SourcePhysicsContactRecordObserver&){
    const auto sp=actual.gp[4];check(m.get(sp)==0x60300000&&actual.fs_base==0x60800000,"World actual continuation/TLS ABI");
    if(target==0x37c5c0u){
      if(corrupt_clock)actual.x87[0]+=1.L;
      actual.gp[0]=0;actual.gp[4]+=4u;return;
    }
    if(target==0x2e3850u){
      check(m.get(sp+4)==0x41f510u,"Solver failure logger lost actual message/cdecl ABI");
      throw std::runtime_error("actual solver logger stopped");
    }
    check(actual.x87_depth==2&&actual.x87[1]==-4.L&&actual.x87[0]==1.25L+calls.size(),"World residual live x87 context lost");actual.x87[0]+=1.L;
    check(actual.floating_point.has_value(),"World residual FP policy missing");calls.push_back(target);unsigned pop=0;
    if(target==shapeBounds){check(actual.gp[1]>=world+0x6a00&&actual.gp[1]<world+0x6a80&&m.get(sp+8)==0&&m.get(sp+12)>=world+0xa020,"Actual broad-phase shape/margin/output ABI");const auto index=(actual.gp[1]-world-0x6a00)/0x20;check(m.get(sp+4)==motion0+index*0x200+0x20,"Actual broad-phase shape transform ABI");pop=12;}
    else if(target==nestedBounds){
      const auto transform=m.get(sp+4);
      check(actual.gp[1]==world+0x7700&&m.get(sp+8)==0&&transform>=sp+20&&transform+128<=packet,
        "Nested native shape lost actual child receiver or caller-owned transforms");
      check(m.get(transform+48)==0x41000000u&&m.get(transform+52)==0x41200000u&&m.get(transform+56)==0x41400000u,
        "Nested native shape reused a live parent matrix or composed the wrong inputs");
      check(m.get(transform+64+48)==0x40e00000u&&m.get(transform+64+52)==0x41000000u,
        "Native nested composition overwrote its retained parent matrix");
      std::array<std::byte,128>before{},after{};m.read(transform,before);
      std::array<std::byte,0x180>locals;locals.fill(std::byte{0xa5});m.write(sp-0x180u,locals);m.read(transform,after);
      check(before==after,"Residual child private frame overwrote native transform packets");pop=12;
    }
    else if(target==dispatcherSweep){const auto first=m.get(sp+16),second=m.get(sp+20),count=m.get(sp+12);check(actual.gp[1]==world+0x5d40&&first>=sp+24&&second>=sp+24&&first+20<=packet&&second+20<=packet,"Broad-phase pair packets overlap callee frame/arguments");check(count==4||count==0,"Actual broad-phase body count ABI");if(count){m.put(m.get(first),body0);m.put(m.get(first)+4,body1);m.put(first+4,1);}pop=20;}
    else if(target==0x321510){check(m.get(0x56dfd8)==actual.gp[1]&&m.get(sp+16)==0,"Actual broad-phase pair consumer island/count ABI");if(actual.gp[1]==island0){check(m.get(sp+4)==world+0x6e00&&m.get(sp+8)==3&&m.get(world+0x6e08)==0x12345678&&m.get(world+0x6e10)==0x11223344&&m.get(island0+0x78)==0,"Pending pair append/growth/reread was lost");}else check(m.get(sp+8)==0,"Empty island acquired fabricated pairs");m.put(world+0x6904,m.get(world+0x6904)+1);m.put(world+0x23e,actual.gp[1]==island1,1);pop=16;}
    else if(target==agentProcess||target==agentFinish){
      const auto output=m.get(sp+16),buffer=m.get(output);
      check(actual.gp[1]==(target==agentProcess?world+0x7000:world+0x7080)&&
        m.get(sp+4)==body0&&m.get(sp+8)==body1&&m.get(sp+12)==world+0x140,
        "Native island agent lost actual receiver/body/configuration ABI");
      check(output>=sp+20&&buffer>=sp+20&&buffer+20<packet,
        "Native island agent packets overlap actual callback frame/arguments");
      check(m.get(0x56dfd8)==island0&&m.get(buffer+8)==0x80000040,
        "Native island agent lost actual current island or borrowed buffer lifetime");
      if(target==agentProcess){
        check(m.get(buffer+4)==0,"Native island agent did not clear real contact count");
        std::array<std::byte,0x180>locals;locals.fill(std::byte{0xa5});m.write(sp-0x180u,locals);
        check(m.get(output)==buffer,"Agent private frame overwrote caller-owned output holder");
        m.put(buffer+4,1);
      }else check(m.get(buffer+4)==1,"Native island finisher did not consume process callback stores");
      pop=16;
    }
    else if(target==worldCollisionListener){check(actual.gp[1]==world+0x5f20&&m.get(sp+4)==world&&m.get(sp+8)==packet,"Actual custom collision world-listener/step ABI");check(m.get(world+0x12c,1)==0&&m.get(world+0x12d,1)==0,"Native collision failed to leave callback-safe phase");pop=8;}
    else if(target==islandCollisionListener){check(actual.gp[1]==world+0x5f80&&(m.get(sp+4)==island0||m.get(sp+4)==island1||m.get(sp+4)==island2)&&m.get(sp+8)==packet,"Actual custom collision island-listener/step ABI");check(m.get(world+0x12c,1)==1&&m.get(world+0x12d,1)==0,"Native collision island listener phase differs");pop=8;}
    else if(target==0x319750||target==0x316e90||target==0x3187c0||target==0x319cd0||target==0x316e10)check(actual.gp[1]==world,"World ECX service ABI");
    else if(target==policyOwner){const auto out=m.get(sp+4);check(actual.gp[1]==world+0x5400&&m.get(sp+8)==body0,"Deactivation policy owner/body ABI");check(out>sp+12&&out<packet,"Deactivation policy result lifetime");m.put(out,0,1);m.put(world+0x5500,1,1);actual.gp[0]=world+0x5500;++checks;pop=8;}
    else if(target==arrayAllocate){const auto bytes=m.get(sp+4);check(actual.gp[1]==world+0x5e00&&(bytes==4||bytes==32)&&m.get(sp+8)==0x12,"Actual array allocation virtual ABI");actual.gp[0]=bytes==32?world+0x6e00:world+0x5c00;pop=8;}
    else if(target==arrayRelease){check(actual.gp[1]==world+0x5e00&&m.get(sp+12)==0x12,"Actual array release receiver/class ABI");if(m.get(sp+4)==world+0x6e00)check(m.get(sp+8)==32,"Owned broad-phase pair release extent ABI");else{check(m.get(sp+4)==0&&m.get(sp+8)==0,"Actual inactive-array release ABI");check(m.get(world+0x14)==0&&m.get(world+0x1c)==0,"Array published replacement before releasing previous storage");}pop=12;}
    else if(target==0x322950){check(m.get(sp+4)==world&&m.get(sp+12)==packet,"Island listener cdecl ABI");}
    else if(target==constraintBuilder){
      const auto build=m.get(sp+8);
      check((actual.gp[1]==world+0x6400||actual.gp[1]==world+0x6500)&&m.get(sp+4)==world+0x190&&build>=sp+12,"Actual constraint builder receiver/retained-packet ABI");
      std::array<std::byte,0x180>callee_locals;callee_locals.fill(std::byte{0xa5});m.write(sp-0x180u,callee_locals);
      check(m.get(build+4)==world+0x8098||m.get(build+4)==world+0x809c,"Callee private stack overwrote retained caller packet");
      m.put(build+4,m.get(build+4)+4);m.put(world+0x17c,0);pop=8;
    }
    else if(target==actionOwner){check(actual.gp[1]==action&&m.get(sp+4)==world+0x150,"Action virtual ABI");pop=4;}
    else if(target==otherForce){check(actual.gp[1]==motion1&&m.get(sp+4)==world+0x150&&m.get(sp+8)==world+0xe0,"Other force virtual ABI");pop=8;}
    else if(target==otherPose){check(actual.gp[1]==motion1&&m.get(sp+4)==world+0x150,"Pose virtual ABI");pop=4;}
    else if(target==0x3174a0){if(actual.gp[1]==retired0)m.put(world+0x4300,retiredReplacement);else check(actual.gp[1]==retired1,"Detach body ABI");}
    else if(target==destroy){check(actual.gp[1]==retired1&&m.get(sp+4)==1&&m.get(retired1+6,2)==0,"Actual body destruction ABI");pop=4;}
    else if(target==0x322820)check(m.get(sp+4)==world&&m.get(sp+8)==packet,"World listener cdecl ABI");
    else throw std::runtime_error("Unexpected world residual target");
    check(!packet||m.get(packet)==0x3d088889,"World retained step packet overwritten");actual.gp[4]+=4u+pop;
  };
  native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,{},&stats,&residuals,[&]{return ++timestamps;});
  check(timestamps>0&&m.get(world+0x5604)==1,"World profiling bypassed its actual configured platform clock");
  const std::vector<std::uint32_t>expected{0x319750,shapeBounds,shapeBounds,shapeBounds,shapeBounds,dispatcherSweep,arrayAllocate,0x321510,arrayRelease,agentProcess,agentFinish,islandCollisionListener,dispatcherSweep,0x321510,islandCollisionListener,dispatcherSweep,0x321510,islandCollisionListener,0x319750,0x319cd0,0x316e10,0x319750,worldCollisionListener,0x319750,0x316e90,arrayAllocate,arrayRelease,0x322950,constraintBuilder,constraintBuilder,0x322950,policyOwner,actionOwner,otherForce,otherPose,0x322950,0x3187c0,0x319cd0,0x316e10,0x3174a0,0x3174a0,destroy,0x322820};
  check(calls==expected&&residuals==calls.size()&&checks==1,"Complete world native/residual phase order");
  std::map<std::uint32_t,std::uint64_t>expected_owners;
  for(auto owner:calls)++expected_owners[owner];
  check(stats.residual_owner_calls==expected_owners&&
    stats.shape_bounds_residual_owners==std::map<std::uint32_t,std::uint64_t>{{shapeBounds,4}},
    "Native world diagnostics lost or misidentified an actual residual owner");
  check(stats.world_integrations==1&&stats.native_world_collision_calls==1&&stats.native_solver_calls==1&&stats.native_solver_initializations==0&&stats.native_constraint_calls==1&&stats.native_island_collision_calls==3&&stats.native_broad_phase_calls==3&&stats.native_dynamic_accumulator_calls==0&&stats.native_constraint_setup_calls==0&&stats.native_world_collision_listener_calls==1&&stats.native_island_collision_listener_calls==3&&stats.native_pose_calls==1&&stats.native_deactivation_calls==3&&stats.native_island_transition_calls==1&&stats.native_array_reserve_calls==1&&stats.native_array_growth_calls==1&&stats.native_array_pool_allocations==0&&stats.native_array_pool_releases==0&&stats.native_body_policy_calls==3&&stats.native_kinematic_pose_calls==1&&stats.native_fixed_pose_calls==1&&stats.native_prescribed_force_calls==2&&stats.island.native_force_calls==1&&stats.island.other_force_calls==3&&stats.island.constraint_calls==1&&stats.island.pose_calls==4&&stats.island.action_calls==1,"Direct native bodyforce composition not used");
  check(stats.island_agents.flag_clear_calls==2&&stats.island_agents.flag_set_calls==1&&
    stats.island_agents.process_calls==1&&stats.island_agents.finish_calls==1&&
    stats.island_agents.constructed_buffers==3&&stats.island_agents.destroyed_buffers==3,
    "World collision bypassed native agent scheduling or contact-buffer lifetime");
  check(m.get(motion0+0x70)==0x3d088889&&m.get(motion0+0x7c)==0x3f800000,"Native pose failed to advance actual retained motion");
  check(m.get(world+0x18)==1&&m.get(world+0xc)==2&&m.get(island2+0x24)==0&&m.get(world+0x5c00)==island2,"Native island transition failed to publish actual inactive state");
  check(m.get(0x56dfd8)==0&&m.get(0x56dfcc,1)==1&&m.get(island0+0xae,1)==1&&m.get(island1+0xae,1)==1&&m.get(island2+0xae,1)==1,"Native island collision allocation-monitor lifetime differs");
  check(m.get(world+0x94)==1&&m.get(world+0x5f00)==world+0x5f20,"Native collision listener failed stable null compaction");
  check(m.get(island0+0x2c,1)==1,"Body policy failed to consume actual returned result or complete cohort");
  check(m.get(motion2+0x70)==0x3d088889&&m.get(motion3+0x70)==0&&m.get(motion2+0xf0)==0x3f800000&&m.get(motion3+0xf0)==0x3f800000,"Prescribed/fixed native motion used dynamic force or wrong pose");
  check(m.get(retired0+6,2)==9&&m.get(retiredReplacement+6,2)==1&&m.get(world+0x118)==0,"Post-detach array reread/reference lifetime");
  check(a.x87[0]==1.25L+calls.size()&&a.x87_status==fp.status&&a.floating_point->mxcsr==fp.mxcsr,"Final actual residual FP/x87 state lost");
  {
    m=model();a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;
    // Two native transform parents feed a real service boundary. Its private
    // frame is deliberately poisoned, while both retained matrices stay live.
    m.put(body0+12u,world+0x7400u);
    for(unsigned k=0;k<2;++k){
      const auto shape=world+0x7400u+k*0x100u;m.put(shape,shape+0x80u);m.put(shape+0x98u,0x2e8870u);
      m.put(shape+12u,k?world+0x7700u:world+0x7500u);
      for(unsigned col=0;col<4;++col)for(unsigned lane=0;lane<4;++lane)
        m.put(shape+16u+col*16u+lane*4u,col==lane?0x3f800000u:0u);
    }
    m.put(world+0x7400u+64u,0x40e00000u);m.put(world+0x7400u+68u,0x41000000u);m.put(world+0x7400u+72u,0x41100000u);
    m.put(world+0x7500u+64u,0x3f800000u);m.put(world+0x7500u+68u,0x40000000u);m.put(world+0x7500u+72u,0x40400000u);
    m.put(world+0x7700u,world+0x7780u);m.put(world+0x7798u,nestedBounds);
    for(unsigned col=0;col<4;++col)for(unsigned lane=0;lane<4;++lane)
      m.put(motion0+32u+col*16u+lane*4u,col==lane?0x3f800000u:0u);
    native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,{},&stats,&residuals,[&]{return ++timestamps;});
    check(stats.shape_bounds.transformed_calls==2&&stats.shape_bounds_residual_calls==4&&
      std::count(calls.begin(),calls.end(),nestedBounds)==1,
      "World broad phase bypassed native nested shape dispatch or hid a residual child");
  }
  {
    m=model();a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;
    // Two native transform parents feed a real service boundary. Its private
    // frame is deliberately poisoned, while both retained matrices stay live.
    m.put(body0+12u,world+0x7200u);m.put(world+0x7200u,world+0x7280u);
    m.put(world+0x7298u,0x306170u);m.put(world+0x720cu,world+0x7300u);m.put(world+0x7210u,2);
    m.put(world+0x7300u,world+0x7400u);m.put(world+0x7308u,world+0x6a00u);
    for(unsigned k=0;k<2;++k){
      const auto shape=world+0x7400u+k*0x100u;m.put(shape,shape+0x80u);m.put(shape+0x98u,0x2e8870u);
      m.put(shape+12u,k?world+0x7700u:world+0x7500u);
      for(unsigned col=0;col<4;++col)for(unsigned lane=0;lane<4;++lane)
        m.put(shape+16u+col*16u+lane*4u,col==lane?0x3f800000u:0u);
    }
    m.put(world+0x7400u+64u,0x40e00000u);m.put(world+0x7400u+68u,0x41000000u);m.put(world+0x7400u+72u,0x41100000u);
    m.put(world+0x7500u+64u,0x3f800000u);m.put(world+0x7500u+68u,0x40000000u);m.put(world+0x7500u+72u,0x40400000u);
    m.put(world+0x7700u,world+0x7780u);m.put(world+0x7798u,nestedBounds);
    for(unsigned col=0;col<4;++col)for(unsigned lane=0;lane<4;++lane)
      m.put(motion0+32u+col*16u+lane*4u,col==lane?0x3f800000u:0u);
    native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,{},&stats,&residuals,[&]{return ++timestamps;});
    check(stats.shape_bounds.transformed_calls==2&&stats.shape_bounds_residual_calls==5&&stats.compound_shape_bounds.compound_calls==1&&
      stats.compound_shape_bounds.child_calls==2&&
      std::count(calls.begin(),calls.end(),nestedBounds)==1,
      "Native compound bounds lost nested caller-owned packets or a genuine child");
  }
  for(bool corrupt:{false,true}){
    m=model();m.put(0x48d480,0);m.put(0x47c1b0,world+0x6f00);
    a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;corrupt_clock=corrupt;
    bool rejected{};
    try{native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,{},&stats,&residuals,[&]{return ++timestamps;});}
    catch(const std::runtime_error&e){rejected=std::string(e.what())==(corrupt?
      "Native TU1.5 solver dependency changed a preserved live x87 value":"actual solver logger stopped");}
    check(rejected&&stats.native_solver_initializations==1&&m.get(0x48d480,1)==0,
      "Native initializer transport fabricated ready or failed to reject changed live x87 bits");
    check(a.x87[0]==1.25L+calls.size()+(corrupt?1.L:0.L)&&a.x87[1]==-4.L,
      "Solver dependency replaced actual preserved or corrupted live values");
  }
  {
    m=model();m.put(0x48d480,0);m.put(0x47c1b0,world+0x6f00);
    a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;corrupt_clock=false;
    unsigned filetime_reads{};bool stopped{};
    try{native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,{},&stats,&residuals,[&]{return ++timestamps;},
      [&]{++filetime_reads;return std::uint64_t{116444736000000000};});}
    catch(const std::runtime_error&e){stopped=std::string(e.what())=="actual solver logger stopped";}
    check(stopped&&filetime_reads==1&&stats.native_solver_clock_calls==1&&stats.native_solver_initializations==1&&m.get(0x48d480,1)==0,
      "Native solver clock bypassed its actual FILETIME input or invented initialization success");
    check(a.x87[0]==1.25L+calls.size()&&a.x87[1]==-4.L,
      "Native integer solver clock changed preserved live x87 values");
  }
  m=model();a=initial();fp={0x27f,0x2800,0x1f80};bool failed=false;
  try{native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},[](auto,auto&state,const auto&){state.x87_status=0x2801;state.floating_point->mxcsr=0x1fa0;throw std::runtime_error("actual collision owner missing");});}
  catch(const std::runtime_error&e){failed=std::string(e.what())=="actual collision owner missing";}
  check(failed&&fp.status==0x2801&&fp.mxcsr==0x1fa0&&m.get(world+0x12c,1)==1,"World residual failure fabricated progress/lost actual FP");
  std::cout<<"World residual model: all44 residual ABI phases and ready-zero clock/logger transport, retained packets/outbyte, native dynamic/kinematic/fixed motion, policy, island-transition and native array-growth/collision scheduling/listener/native-constraint/island-collision/broad-phase composition, live x87/FP and failure prefixes passed; full original/live composition remains open\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
