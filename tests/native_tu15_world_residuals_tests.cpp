#include "halo2/native_tu15_world_integration_residuals.hpp"
#include "halo2/native_tu15_array_allocator.hpp"
#include "halo2/native_tu15_pair_consumer_input_observer.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <optional>
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
  payload0=body0+12u,payload1=body1+12u,pair_dispatcher=world+0xe000u,pair_filter=world+0xf000u,
  motion0=world+0x3000,motion1=world+0x3200,motion2=world+0x3400,motion3=world+0x3600,
  action=world+0x4000,otherForce=0x230020,otherPose=0x230030,actionOwner=0x230010,destroy=0x230040,policyOwner=0x230050,arrayAllocate=0x230060,arrayRelease=0x230070,worldCollisionListener=0x230080,islandCollisionListener=0x230090,constraintBuilder=0x2300a0,shapeBounds=0x2300b0,dispatcherSweep=0x2300c0,agentProcess=0x2300d0,agentFinish=0x2300e0,nestedBounds=0x2300f0,islandStepListener=0x230100,worldStepListener=0x230110,lateWorldStepListener=0x230120,queueDestroy=0x230130,
  retired0=world+0x4400,retired1=world+0x4500,retiredReplacement=world+0x4600,
  queuedAction=world+0x7800,queuedDelete=world+0x7900,queuedReplacement=world+0x7a00,
  queuedUpdate=world+0x7b00,queuedUpdateBody=world+0x7c00,
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
  m.put(island0+0x74,world+0x6d00);m.put(island0+0x78,2);m.put(world+0x6d00,body0+0x1cu);m.put(world+0x6d04,body2+0x1cu);m.put(world+0x6d08,body3+0x1cu);m.put(world+0x6d0c,body1+0x1cu);
  m.put(island1+0x5c,2);m.put(island1+0x58,world+0x4200);
  m.put(island0+0x68,world+0x7100);m.put(island0+0x6c,1);m.put(island0+0x70,0x80000004u);
  m.put(world+0x138u,pair_dispatcher);m.put(world+0xd0u,pair_filter-8u);m.put(pair_filter,world+0xf180u);m.put(world+0xf180u,0x2e9a00u);
  m.put(pair_filter+0x14u+4u,1u<<2u);m.put(pair_filter+0x14u+8u,1u<<1u);
  for(unsigned n=0;n<4;++n){m.put(body0+n*0x100u+0x20u,0x1000u+n);m.put(body0+n*0x100u+0x24u,1);m.put(body0+n*0x100u+0x28u,n+1u);}
  for(auto island:{island0,island1,island2}){m.put(island+0x1cu,world+0xf100u);}
  m.put(world+0xf104u,0x320ef0u);m.put(world+0xf108u,0x320260u);
  m.put(world+0x7100,payload0+16u);m.put(world+0x7104,payload1+16u);m.put(world+0x7108,world+0x7000);
  m.put(world+0x7000,world+0x7040);m.put(world+0x7050,agentProcess);m.put(world+0x7008,world+0x7080);
  m.put(world+0x7080,world+0x70c0);m.put(world+0x70cc,agentFinish);
  m.put(world+0x90,world+0x5f00);m.put(world+0x94,2);m.put(world+0x5f04,world+0x5f20);m.put(world+0x5f20,world+0x5f40);m.put(world+0x5f44,worldCollisionListener);
  m.put(world+0xa8,world+0x5f60);m.put(world+0xac,1);m.put(world+0x5f60,world+0x5f80);m.put(world+0x5f80,world+0x5fa0);m.put(world+0x5fa4,islandCollisionListener);
  m.put(world+0x84,world+0x5e80);m.put(world+0x88,3);m.put(world+0x5e84,world+0x5ec0);
  m.put(world+0x5ec0,world+0x5ee0);m.put(world+0x5ee4,worldStepListener);
  m.put(world+0x5ea0,world+0x5eb0);m.put(world+0x5eb4,lateWorldStepListener);
  m.put(world+0x9c,world+0x5f90);m.put(world+0x5f90,world+0x5fc0);
  m.put(world+0x5fc0,world+0x5fe0);m.put(world+0x5fe4,islandStepListener);
  m.put(world+0xcc,world+0x5d00);m.put(world+0xc4,world+0x5d40);
  m.put(world+0x24,0); /* This callback fixture does not own a consistent merge pair. */m.put(world+0x10c,1);m.put(world+0xa0,1);m.put(world+0x23d,1,1);m.put(world+0x23c,0,1); /* Opaque callback fixture has inconsistent empty-island lifetimes; actual regroup is proved separately. */m.put(world+0xf4,1);m.put(world+0x100,1);
  m.put(world+0x20,world+0x7d00);m.put(world+0x7d00,body0);m.put(world+0x7d04,body1);
  m.put(body0+0x44,island0);m.put(body1+0x44,island1);
  m.put(world+0x108,world+0x7d40);m.put(world+0x7d40,queuedAction);m.put(queuedAction+6,2,2);
  m.put(world+0xf0,world+0x7d80);m.put(world+0x7d80,queuedDelete);m.put(queuedDelete+0x44,island1);m.put(queuedDelete+6,8,2);
  m.put(queuedReplacement+6,1,2);m.put(queuedReplacement,world+0x7dc0);m.put(world+0x7dc0,queueDestroy);
  m.put(world+0xfc,world+0x7e00);m.put(world+0x7e00,queuedUpdate);m.put(queuedUpdate+6,2,2);
  m.put(queuedUpdate+0xc,queuedUpdateBody);m.put(queuedUpdateBody+0x44,island0);
  m.put(world+0x114,world+0x4300);m.put(world+0x118,2);m.put(world+0x4300,retired0);m.put(world+0x4304,retired1);
  m.put(retired0+0x7c,world+0x7e80);m.put(retired0+0x80,4);m.put(retired0+0x84,0x80000004);
  m.put(world+0x7e84,0x12345678);m.put(world+0x7e8c,0x87654321);
  m.put(retired0+6,9,2);m.put(retiredReplacement+6,2,2);m.put(retired1+6,1,2);m.put(retired1,world+0x5300);m.put(world+0x5300,destroy);
  m.put(world+0x17c,8);m.put(world+0x180,0x3e000000);m.put(0x46a0d0,0x3f800000);m.put(0x46a0e8,0);m.put(0x46a0e4,0x40400000);m.put(0x46a0cc,0x3f000000);
  for(unsigned lane=0;lane<3;++lane)m.put(0x41f1d0+lane*4,0xffffffff);
  for(auto motion:{motion0,motion2,motion3}){m.put(motion+0x3c,0x3f800000);m.put(motion+0xc,0x3e800000);m.put(motion+0x40,0x3f800000);}
  m.put(motion2+0xf0,0x3f800000);m.put(motion3+0xf0,0x3f800000);return m;}
SourcePhysicsContactRuntimeAbi initial(){SourcePhysicsContactRuntimeAbi a{};a.gp[4]=top;a.fs_base=0x60800000;a.x87_depth=2;a.x87[0]=1.25L;a.x87[1]=-4.L;a.x87_status=0x2800;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};return a;}
}
#include "helpers/native_world_convex_factory_prefix_tu15.hpp"
#include "helpers/native_world_island_merge_prefix_tu15.hpp"
#include "helpers/native_contact_process_binding_model_tu15.hpp"
int main(int argc,char**argv){try{native_contact_process_binding_model();if(argc==3&&std::string(argv[1])=="pair-wake-prefix")return native_world_convex_factory_prefix(argv[2],false,true);if(argc==3&&std::string(argv[1])=="gsk-prefix")return native_world_convex_factory_prefix(argv[2],true);if(argc==4&&std::string(argv[1])=="merge-prefix")return native_world_island_merge_prefix(argv[2],argv[3]);if(argc==2)return native_world_convex_factory_prefix(argv[1]);
  auto m=model();auto a=initial();NativeTu15PhysicsFrameFp fp{0x27f,0x2800,0x1f80};NativeTu15WorldIntegrationStats stats;std::uint64_t residuals{};std::vector<std::uint32_t>calls;std::uint32_t packet=top-0x10u;unsigned checks{};
  m.put(0x48c974,world+0x5600);m.put(0x48c978,world+0x5800);std::uint64_t timestamps{};
  bool corrupt_clock{},contact_motion_case{},completion_case{},delete_pair_case{},inactive_cleanup_case{};
  bool reversed_list_cleanup_case{};
  bool bv_cleanup_case{},bv_secondary_cleanup_case{};
  bool single_point_cleanup_case{},single_point_none_case{};
  bool phantom_callback_cleanup_case{},phantom_callback_leave_case{};
  unsigned inactive_cleanup_policy_refs=2;
  unsigned inactive_cleanup_agent_kind{};
  std::vector<SourcePhysicsContactRecordStore>pair_filter_stores;
  std::vector<SourcePhysicsContactRecordStore>bv_cleanup_stores;
  NativeTu15PairConsumerInputObserverActivity pair_input_activity;
  std::vector<NativeTu15PairConsumerInput>pair_inputs;
  std::optional<NativeTu15PairConsumerInputObserverBinding>pair_input_binding;
  pair_input_binding.emplace(m,[&](const auto&owner,const auto&input){
    check(&owner==&m&&input.phase==NativeTu15PairConsumerInputPhase::Entry&&
      input.source_call_site==0&&input.preserved_x87_depth==2&&
      input.scratch.temporary.identity==top-0x60u&&
      input.scratch.merge.first_end==top-0x80u&&input.scratch.merge.tail_decision==top-0x74u,
      "World native pair input observer lost actual memory, typed DATA, depth or unknown source identity");
    pair_inputs.push_back(input);
  },&pair_input_activity);
  const SourcePhysicsContactRecordObserver body_observer=[&](const auto&store){
    if(store.instruction==0x2f0db7u)bv_cleanup_stores.push_back(store);
    if(store.instruction==0x2e9a28u||store.instruction==0x2e9a49u)pair_filter_stores.push_back(store);
    if(store.instruction==0x3174d9u&&store.address==world+0x7e80u)
      m.put(world+0x4300,retiredReplacement);
  };
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
    else if(target==dispatcherSweep){
      const auto entries=m.get(sp+4u),first=m.get(sp+16u),second=m.get(sp+20u),count=m.get(sp+12u),current_island=m.get(0x56dfd8u);
      check(actual.gp[1]==world+0x5d40&&first>=sp+24u&&second>=sp+24u&&first+20u<=packet&&second+20u<=packet,"Broad-phase pair packets overlap callee frame/arguments");
      check(count==4||count==0,"Actual broad-phase body count ABI");
      m.put(world+0x23eu,current_island==island1||(contact_motion_case&&current_island==island0),1);
      if(count){
        for(unsigned n=0;n<count;++n)check(m.get(entries+n*4u)==body0+n*0x100u+0x1cu,"Sweep did not receive actual source broad-phase handles");
        const auto output=delete_pair_case?second:first;
        m.put(m.get(output),m.get(entries));m.put(m.get(output)+4u,m.get(entries+4u));m.put(output+4u,1);
      }
      pop=20;
    }
    else if(target==0x320ef0u||target==0x320260u){
      const auto handler=actual.gp[1],pair=m.get(sp+4u),header=top-0x60u;
      check(handler==island0+0x1cu&&m.get(pair)==payload0+16u&&m.get(pair+4u)==payload1+16u,
        "Native pair callback lost real handler/current broad-phase handles");
      check(header>sp+8u&&top-0x80u>sp+8u&&m.get(pair_dispatcher+0x24u)==handler,
        "Native pair DATA or active dispatcher slot overlaps callback arguments");
      check(m.get(island0+0x78u)==0&&m.get(m.get(top-0xc0u))==(delete_pair_case?body2:body0)+0x1cu,
        "Native pending pair append/growth/normalization was bypassed");
      std::array<std::byte,20>retained_header{},after_header{};std::array<std::byte,16>retained_merge{},after_merge{};
      m.read(header,retained_header);m.read(top-0x80u,retained_merge);
      std::array<std::byte,0x180>locals;locals.fill(std::byte{0xa5});m.write(sp-0x180u,locals);
      m.read(header,after_header);m.read(top-0x80u,after_merge);
      check(retained_header==after_header&&retained_merge==after_merge,"Pair callback private frame overwrote its live DATA");
      const auto cursor=m.get(handler+0x80u),end=m.get(handler+0x84u);
      check(cursor<end&&m.get(cursor)==m.get(pair)&&m.get(cursor+4u)==m.get(pair+4u),
        "Declared callback case requires the actual matching cached pair record");
      if(target==0x320ef0u){
        check(!delete_pair_case,"Deletion cohort invented an addition callback");
        const auto output=m.get(handler+0x88u);for(unsigned n=0;n<4;++n)m.put(output+n*4u,m.get(cursor+n*4u));
        m.put(handler+0x88u,output+16u);
      }else{check(delete_pair_case,"Confirmation cohort invented a deletion callback");m.put(handler+12u,1,1);}
      m.put(handler+0x80u,cursor+16u);pop=4;
    }
    else if(target==agentProcess||target==agentFinish){
      const auto output=m.get(sp+16),buffer=m.get(output);
      check(actual.gp[1]==(target==agentProcess?world+0x7000:world+0x7080)&&
        m.get(sp+4)==payload0&&m.get(sp+8)==payload1&&m.get(sp+12)==world+0x140,
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
        if(completion_case){
          for(unsigned lane=0;lane<8;++lane)m.put(m.get(buffer)+lane*4u,0x3f000000u+lane);
          m.put(m.get(buffer)+32u,2,2);
        }
        if(contact_motion_case)m.put(m.get(buffer)+12u,0xbf800000u);
      }else check(m.get(buffer+4)==1,"Native island finisher did not consume process callback stores");
      pop=16;
    }
    else if(target==0x230140u){
      const auto event=m.get(sp+4);
      check(completion_case&&actual.gp[1]==world+0x7f80u&&event==top-0x180u&&event>sp+8u&&
        m.get(event)==world+0x7080u&&m.get(event+4)==payload0&&m.get(event+8)==payload1&&m.get(event+16)==0,
        "Native contact event lost its actual caller-owned packet or world subject");
      std::array<std::byte,0x180>locals;locals.fill(std::byte{0xa5});m.write(sp-0x180u,locals);
      check(m.get(event)==world+0x7080u,"Contact callback private frame overwrote the live event");
      // The root cached both metadata owners before this actual callback.
      // Counts and arrays are intentionally published only during notification.
      m.put(payload0+0x20u,world+0x7400u);m.put(payload1+0x20u,world+0x7500u);
      m.put(payload0+12u,0);m.put(payload1+12u,0);m.put(payload0+24u,1);m.put(payload1+24u,1);
      m.put(world+0x7260u,world+0x7fc0u);m.put(world+0x7264u,2);
      pop=4;
    }
    else if(target==worldCollisionListener){check(actual.gp[1]==world+0x5f20&&m.get(sp+4)==world&&m.get(sp+8)==packet,"Actual custom collision world-listener/step ABI");check(m.get(world+0x12c,1)==0&&m.get(world+0x12d,1)==0,"Native collision failed to leave callback-safe phase");pop=8;}
    else if(target==islandCollisionListener){check(actual.gp[1]==world+0x5f80&&(m.get(sp+4)==island0||m.get(sp+4)==island1||m.get(sp+4)==island2)&&m.get(sp+8)==packet,"Actual custom collision island-listener/step ABI");check(m.get(world+0x12c,1)==1&&m.get(world+0x12d,1)==0,"Native collision island listener phase differs");pop=8;}
    else if(target==0x3190d0)throw std::runtime_error("General World fixture must not invoke an opaque island merge");
    else if(target==0x3198a0||target==0x316aa0){
      const auto result=m.get(sp+4);
      check(actual.gp[1]==world&&m.get(sp+8)==(target==0x3198a0?queuedDelete:queuedUpdate)&&
        result>=sp+12&&result<packet,"Native deferred queue lost its real callback arguments or result-byte lifetime");
      std::array<std::byte,0x180>locals;locals.fill(std::byte{0xa5});m.write(sp-0x180u,locals);
      m.put(result,target==0x3198a0,1);
      if(target==0x3198a0)m.put(world+0x7d80,queuedReplacement);pop=8;
    }
    else if(target==queueDestroy){
      check(actual.gp[1]==queuedReplacement&&m.get(sp+4)==1&&m.get(queuedReplacement+6,2)==0&&
        m.get(queuedDelete+6,2)==8,"Native deferred release used a stale cohort or invented destruction");pop=4;
    }
    else if(target==0x3187c0)throw std::runtime_error("Old regroup parent must not execute");
    else if(target==policyOwner){const auto out=m.get(sp+4);check(actual.gp[1]==world+0x5400&&m.get(sp+8)==body0,"Deactivation policy owner/body ABI");check(out>sp+12&&out<packet,"Deactivation policy result lifetime");m.put(out,0,1);m.put(world+0x5500,1,1);actual.gp[0]=world+0x5500;++checks;pop=8;}
    else if(target==arrayAllocate){
      const auto bytes=m.get(sp+4);check(actual.gp[1]==world+0x5e00&&(bytes==4||bytes==32)&&m.get(sp+8)==0x12,"Actual array allocation virtual ABI");
      actual.gp[0]=bytes==32?world+0x6e00:world+0x5c00;
      if(bytes==4&&inactive_cleanup_case){
        // Declared callback DATA becomes visible before the transition's
        // real notification. It cannot affect the earlier collision phase.
        m.put(island2+0x68u,world+0x9100u);m.put(island2+0x6cu,1);
        m.put(world+0x9100u,payload0+16u);m.put(world+0x9104u,payload1+16u);m.put(world+0x9108u,world+0x9900u);
        m.put(payload0+0x20u,world+0x9800u);m.put(world+0x9840u,1,1);
        m.put(world+0x9900u,world+0x9940u);m.put(world+0x9908u,world+0x9920u);
        m.put(world+0x9920u,world+0x9940u);m.put(world+0x9954u,0x31d400u);m.put(world+0x9950u,0x31d400u);
        m.put(island2+0x3cu,world+0x9300u);m.put(island2+0x40u,1);m.put(world+0x9300u,world+0x9400u);
        m.put(world+0x9454u,world+0x9500u);m.put(world+0x9500u,world+0x9600u);
        m.put(world+0x9506u,2,2);m.put(world+0x961cu,0x31d520u);
        if(inactive_cleanup_agent_kind&&inactive_cleanup_agent_kind<=2){
          const auto agent=world+0x9900u,manager=world+0x9920u;
          m.put(agent,world+0xd000u);m.put(world+0xd000u,0x2e4d60u);
          m.put(world+0xd014u,inactive_cleanup_agent_kind==1?0x2f3100u:0x2f7a20u);
          m.put(agent+0xcu,world+0xd300u);m.put(agent+0x10u,2);
          m.put(world+0xd100u,world+0xd200u);m.put(world+0xd200u,0x2e4d60u);
          m.put(world+0xd214u,0x31d400u);
          if(inactive_cleanup_agent_kind==1){m.put(world+0xd308u,world+0xd100u);m.put(world+0xd314u,0);}
          else{
            m.put(world+0xd300u,world+0xd100u);m.put(world+0xd304u,world+0xd140u);
            m.put(world+0xd214u,0x2f3100u);m.put(world+0xd10cu,world+0xd400u);m.put(world+0xd110u,2);
            m.put(world+0xd408u,world+0xd500u);m.put(world+0xd414u,0);
            m.put(world+0xd500u,world+0xd600u);m.put(world+0xd614u,0x31d400u);
            m.put(world+0xd140u,world+0xd240u);m.put(world+0xd254u,0x31d400u);
          }
          m.put(manager,world+0xd700u);m.put(world+0xd700u,0x2e4d60u);m.put(world+0xd710u,0x323220u);
        }
        if(inactive_cleanup_policy_refs==1){
          constexpr std::uint32_t pool=0x2000000u,policy=pool+0x4340u,table=pool+0x1000u;
          m.put(pool,table);m.put(table+0x14u,0x233420u);m.put(pool+0x2e8u,1);
          m.put(0x486064u,1);m.put(0x486068u,pool);m.put(world+0x5e54u,0x149e00u);
          m.put(world+0x9454u,policy);m.put(policy,world+0x9600u);m.put(policy+4u,0x60u,2);m.put(policy+6u,1,2);
          m.put(world+0x9600u,0x1d9c80u);
        }
        if(inactive_cleanup_agent_kind>=3){
          constexpr std::uint32_t pool=0x2000000u;
          m.put(pool+0xf0340u,1);m.put(pool+0x2d0u,1);
          m.put(pool+0x18u,pool+0x4340u);m.put(pool+0x1cu,pool+0x6300u);m.put(pool+0x20u,pool+0x43a0u);
          m.put(pool+0x68u,96);m.put(pool+0x6cu,256);m.put(pool+0x70u,32);
          m.put(pool+0xacu+256u,1,1);m.put(pool+0xacu+32u,2,1);
          NativeTu15PoolMemory storage(m,pool,{});
          const auto make=[&](std::uint32_t size,std::uint32_t table){
            const auto object=native_tu15_pool_allocate(storage,size);
            check(object!=0,"Native model allocator exhausted valid source pool");
            m.put(object,table);m.put(object+4u,size,2);return object;
          };
          const auto manager=world+0x9920u;
          m.put(manager,world+0xd700u);m.put(world+0xd700u,0x2e4d60u);
          m.put(world+0xd708u,0x2e4d60u);m.put(world+0xd710u,0x323220u);
          const auto cached=make(256,world+0xd800u);
          m.put(world+0xd800u,0x2fc540u);m.put(world+0xd814u,single_point_cleanup_case?0x2fbb10u:0x2f82d0u);
          m.put(cached+8u,manager);m.put(cached+0x18u,1);m.put(cached+0x4cu,13,2);
          if(single_point_cleanup_case)m.put(cached+0xcu,single_point_none_case?0xffffu:13u,2);
          if(phantom_callback_cleanup_case){
            m.put(world+0xd814u,0x2f7790u);
            m.put(cached+0xcu,world+0xdf00u);m.put(cached+0x10u,world+0xdf40u);
            m.put(cached+0x14u,world+0xde00u);m.put(cached+0x18u,world+0xde00u);
            m.put(cached+0x1cu,phantom_callback_leave_case?22u:0u);
            m.put(cached+0x20u,phantom_callback_leave_case?22u:0u);
            m.put(world+0xde00u,world+0xde40u);m.put(world+0xde08u,0);
            m.put(world+0xde6cu,0x1f3550u);
            m.put(world+0xdf18u,0);m.put(world+0xdf58u,0);
          }
          auto agent=cached;
          if(inactive_cleanup_agent_kind>=4){
            agent=make(32,world+0xd840u);m.put(world+0xd840u,0x2fc540u);
            m.put(world+0xd854u,bv_cleanup_case?0x2f0da0u:0x2f6c80u);m.put(agent+0xcu,cached);
            if(bv_cleanup_case){
              m.put(agent+0x10u,bv_secondary_cleanup_case?world+0xd140u:0);
              m.put(world+0xd140u,world+0xd240u);m.put(world+0xd254u,0x31d400u);
            }
          }
          if(inactive_cleanup_agent_kind==5){
            const auto records=make(32,world+0xd880u);m.put(world+0xd880u,0x3051a0u);
            m.put(world+0xd894u,0x2f3100u);m.put(records+0xcu,world+0xd400u);
            m.put(records+0x10u,1);m.put(records+0x14u,0x80000001u);m.put(world+0xd408u,agent);
            agent=make(32,world+0xd8c0u);m.put(world+0xd8c0u,reversed_list_cleanup_case?0x2f81b0u:0x2f8090u);
            m.put(world+0xd8d4u,0x2f7a20u);m.put(agent+0xcu,world+0xd300u);
            m.put(agent+0x10u,1);m.put(agent+0x14u,0x80000001u);m.put(world+0xd300u,records);
          }
          if(inactive_cleanup_agent_kind>=6){
            // Positive declared integration model: native pool owns the manager,
            // source borrowed collector arrays and two genuine listener lists.
            m.put(pool+0x74u,288);m.put(pool+0xacu+288u,3,1);
            const auto contact=make(288,world+0xdc00u),collector=contact+0x10u;
            m.put(world+0xdc00u,0x3232b0u);m.put(world+0xdc10u,0x323220u);
            m.put(contact+0xa4u,0);m.put(collector+0x98u,0x80000004u);
            m.put(collector+0x3cu,0x80000000u);m.put(collector+0x28u,0x80000008u);
            if(inactive_cleanup_agent_kind==7){
              const auto owned=make(32,world+0xde00u);
              m.put(collector+0x34u,owned);m.put(collector+0x3cu,1);
            }
            if(inactive_cleanup_agent_kind==8){
              m.put(pool+0xacu+28u,2,1);m.put(pool+0x78u,64);m.put(pool+0xacu+64u,4,1);
              const auto points=make(28,world+0xde00u),rows=make(256,world+0xde00u),bytes=make(64,world+0xde00u);
              m.put(collector+0x90u,points);m.put(collector+0x98u,1);
              m.put(collector+0x34u,rows);m.put(collector+0x3cu,8);
              m.put(collector+0x20u,bytes);m.put(collector+0x28u,64);
            }
            if(inactive_cleanup_agent_kind==9){
              for(const auto offset:{0x98u,0x3cu,0x28u})m.put(collector+offset,0);
              for(const auto offset:{0x90u,0x34u,0x20u})m.put(collector+offset,0);
            }
            for(unsigned n=0;n<2;++n){
              const auto body=world+0xd900u+n*0x100u,entries=world+0xdb00u+n*0x20u;
              m.put(collector+(n?0x10u:0xcu),body);m.put(body+6u,3,2);m.put(body+8u,world);
              m.put(body+0x88u,entries);m.put(body+0x8cu,1);m.put(body+0x90u,1);m.put(entries,collector);
            }
            m.put(world+0x114u,world+0xdd00u);m.put(world+0x118u,0);m.put(world+0x11cu,0x80000008u);
            // Native reference cleanup reaches the manager after agent cleanup.
            m.put(agent+8u,contact);m.put(world+0x9920u,world+0xd700u);
            m.put(world+0xd710u,0x31d400u);
            m.put(world+0x9108u,agent);
          }else{m.put(agent+8u,manager);m.put(world+0x9108u,agent);}
        }
      }
      if(bytes==32){
        // This model allocator owns a new backing chunk for the returned
        // block. Returning its pair allocation retains that backing chunk,
        // so the native monitor must derive growth from the real descriptor.
        m.put(world+0x6908u,actual.gp[0]);m.put(world+0x690cu,bytes);
        m.put(world+0x681cu,16);m.put(world+0x6820u,2);
      }
      pop=8;
    }
    else if(target==arrayRelease){check(actual.gp[1]==world+0x5e00&&m.get(sp+12)==0x12,"Actual array release receiver/class ABI");if(m.get(sp+4)==world+0x6e00)check(m.get(sp+8)==32,"Owned broad-phase pair release extent ABI");else{check(m.get(sp+4)==0&&m.get(sp+8)==0,"Actual inactive-array release ABI");check(m.get(world+0x14)==0&&m.get(world+0x1c)==0,"Array published replacement before releasing previous storage");}pop=12;}
    else if(target==islandStepListener){
      check(actual.gp[1]==world+0x5fc0&&(m.get(sp+4)==island0||m.get(sp+4)==island1||m.get(sp+4)==island2)&&m.get(sp+8)==packet,
        "Native post-integration island listener lost its actual receiver/island/step ABI");pop=8;
    }
    else if(target==constraintBuilder){
      const auto build=m.get(sp+8);
      check((actual.gp[1]==world+0x6400||actual.gp[1]==world+0x6500)&&m.get(sp+4)==world+0x190&&build>=sp+12,"Actual constraint builder receiver/retained-packet ABI");
      std::array<std::byte,16>retained{},after{};m.read(build,retained);
      std::array<std::byte,0x180>callee_locals;callee_locals.fill(std::byte{0xa5});m.write(sp-0x180u,callee_locals);
      m.read(build,after);check(retained==after,"Callee private stack overwrote retained caller packet");
      m.put(build+4,m.get(build+4)+4);m.put(world+0x17c,0);pop=8;
    }
    else if(target==actionOwner){check(actual.gp[1]==action&&m.get(sp+4)==world+0x150,"Action virtual ABI");pop=4;}
    else if(target==otherForce){check(actual.gp[1]==motion1&&m.get(sp+4)==world+0x150&&m.get(sp+8)==world+0xe0,"Other force virtual ABI");pop=8;}
    else if(target==otherPose){check(actual.gp[1]==motion1&&m.get(sp+4)==world+0x150,"Pose virtual ABI");pop=4;}
    else if(target==destroy){check(actual.gp[1]==retired1&&m.get(sp+4)==1&&m.get(retired1+6,2)==0,"Actual body destruction ABI");pop=4;}
    else if(target==worldStepListener){
      check(actual.gp[1]==world+0x5ec0&&m.get(sp+4)==world&&m.get(sp+8)==packet&&m.get(world+0x88)==3,
        "Native post-integration world listener changed callback/compaction order");
      m.put(world+0xc0,world+0x5ea0);pop=8;
    }
    else if(target==lateWorldStepListener){
      check(actual.gp[1]==world+0x5ea0&&m.get(sp+4)==world&&m.get(sp+8)==packet&&
        m.get(world+0x88)==1&&m.get(world+0x5e80)==world+0x5ec0,
        "Native final listener used a stale getter or ran before stable compaction");pop=8;
    }
    else throw std::runtime_error("Unexpected world residual target="+std::to_string(target));
    check(!packet||m.get(packet)==0x3d088889,"World retained step packet overwritten");actual.gp[4]+=4u+pop;
  };
  native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,body_observer,&stats,&residuals,[&]{return ++timestamps;});
  check(timestamps>0&&m.get(world+0x5604)==1,"World profiling bypassed its actual configured platform clock");
  const std::vector<std::uint32_t>expected{shapeBounds,shapeBounds,shapeBounds,shapeBounds,dispatcherSweep,arrayAllocate,arrayRelease,agentProcess,agentFinish,islandCollisionListener,dispatcherSweep,islandCollisionListener,dispatcherSweep,islandCollisionListener,0x3198a0,queueDestroy,0x316aa0,worldCollisionListener,arrayAllocate,arrayRelease,islandStepListener,constraintBuilder,constraintBuilder,islandStepListener,policyOwner,actionOwner,otherForce,otherPose,islandStepListener,destroy,worldStepListener,lateWorldStepListener};
  if(calls!=expected){std::cerr<<"Actual world residual order:";for(auto owner:calls)std::cerr<<' '<<std::hex<<owner;std::cerr<<std::dec<<'\n';}
  check(std::count(calls.begin(),calls.end(),0x320ef0u)==0,"Native paircreate unexpectedly executed old parent");check(calls==expected&&residuals==calls.size()&&checks==1,"Complete world native/residual phase order");
  std::map<std::uint32_t,std::uint64_t>expected_owners;
  for(auto owner:calls)++expected_owners[owner];
  check(stats.residual_owner_calls==expected_owners&&
    stats.shape_bounds_residual_owners==std::map<std::uint32_t,std::uint64_t>{{shapeBounds,4}},
    "Native world diagnostics lost or misidentified an actual residual owner");
  check(stats.island_regroup.attempts==0&&!stats.residual_owner_calls.contains(0x3187c0u),"Opaque callback fixture unexpectedly admitted regroup");
  check(stats.world_integrations==1&&stats.native_world_collision_calls==1&&stats.native_solver_calls==1&&stats.native_solver_initializations==0&&stats.native_constraint_calls==1&&stats.native_island_collision_calls==3&&stats.native_broad_phase_calls==3&&stats.native_dynamic_accumulator_calls==0&&stats.native_constraint_setup_calls==0&&stats.native_world_collision_listener_calls==1&&stats.native_island_collision_listener_calls==3&&stats.native_pose_calls==1&&stats.native_deactivation_calls==3&&stats.native_island_transition_calls==1&&stats.native_array_reserve_calls==1&&stats.native_array_growth_calls==1&&stats.native_array_pool_allocations==0&&stats.native_array_pool_releases==0&&stats.native_body_policy_calls==3&&stats.native_kinematic_pose_calls==1&&stats.native_fixed_pose_calls==1&&stats.native_prescribed_force_calls==2&&stats.island.native_force_calls==1&&stats.island.other_force_calls==3&&stats.island.constraint_calls==1&&stats.island.pose_calls==4&&stats.island.action_calls==1,"Direct native bodyforce composition not used");
  check(stats.native_pair_consumer_attempts==3&&stats.native_pair_consumer_calls==3&&
    stats.native_pair_consumer_nonempty_calls==1&&stats.native_pair_consumer_filter_calls==3&&
    stats.native_pair_consumer_create_calls==1&&stats.native_pair_consumer_delete_calls==0&&
    stats.native_pair_consumer_refusals==0&&stats.native_pair_consumer_unqualified_services==0&&
    !stats.residual_owner_calls.contains(0x321510u)&&!stats.residual_owner_calls.contains(0x2e9a00u)&&
    pair_filter_stores.size()==3&&pair_filter_stores[0].written[0]==std::byte{1}&&
    pair_filter_stores[1].written[0]==std::byte{}&&pair_filter_stores[2].written[0]==std::byte{}&&
    m.get(island0+0x6cu)==1&&m.get(world+0x7108u)==world+0x7000u&&
    m.get(pair_dispatcher+0x24u)==pair_dispatcher+0x100u,
    "World pair integration bypassed native filter/record flow, genuine callbacks or dispatcher restoration");
  check(pair_input_activity.attempts==3&&pair_input_activity.completed==3&&pair_input_activity.failures==0&&
    pair_inputs.size()==3&&pair_inputs[0].island.identity==island0&&
    pair_inputs[0].first.data==world+0x6e00u&&pair_inputs[0].first.count==3&&pair_inputs[0].second.count==0&&
    pair_inputs[1].island.identity==island1&&pair_inputs[1].first.count==0&&
    pair_inputs[2].island.identity==island2&&pair_inputs[2].first.count==0,
    "World native pair input observation changed input identity/counts or missed an actual boundary");
  pair_input_binding.reset();
  check(stats.body_references.calls==2&&stats.body_references.cohorts==6&&
    stats.body_references.moved_entries==2&&stats.body_references.reserves==0&&
    m.get(retired0+0x80)==2&&m.get(world+0x7e80)==0x12345678&&m.get(world+0x7e84)==0x87654321&&
    !stats.residual_owner_calls.contains(0x3174a0u),
    "Native retirement compaction lost source cohorts or used lifted execution");
  check(stats.deferred.operation_calls==0&&stats.deferred.action_calls==1&&
    stats.deferred.delete_calls==1&&stats.deferred.update_calls==1&&stats.deferred.merges==0&&
    stats.deferred.body_removals==1&&stats.deferred.update_removals==1&&
    stats.deferred.reference_releases==3&&stats.deferred.destructions==1&&
    m.get(queuedAction+6,2)==1&&m.get(queuedUpdate+6,2)==1&&m.get(world+0x24)==0&&
    m.get(world+0x10c)==0&&m.get(world+0xf4)==0&&m.get(world+0x100)==0&&m.get(0x56dfdc)==0,
    "Native deferred queues lost real work, reference lifetime or source count clears");
  for(auto owner:{0x319750u,0x316e90u,0x319cd0u,0x316e10u})
    check(!stats.residual_owner_calls.contains(owner),"World deferred queue still invoked lifted scheduling");
  check(stats.native_world_step_listener_calls==1&&stats.native_island_step_listener_calls==3&&
    !stats.residual_owner_calls.contains(0x322820u)&&!stats.residual_owner_calls.contains(0x322950u),
    "World post-integration listeners bypassed native cohorts or hid lifted scheduling");
  check(stats.island_agents.flag_clear_calls==2&&stats.island_agents.flag_set_calls==1&&
    stats.island_agents.process_calls==1&&stats.island_agents.finish_calls==1&&
    stats.island_agents.constructed_buffers==3&&stats.island_agents.destroyed_buffers==3,
    "World collision bypassed native agent scheduling or contact-buffer lifetime");
  check(m.get(motion0+0x70)==0x3d088889&&m.get(motion0+0x7c)==0x3f800000,"Native pose failed to advance actual retained motion");
  check(m.get(world+0x18)==1&&m.get(world+0xc)==2&&m.get(island2+0x24)==0&&m.get(world+0x5c00)==island2,"Native island transition failed to publish actual inactive state");
  check(m.get(0x56dfd8)==0&&m.get(0x56dfcc,1)==1&&m.get(island0+0xae,1)==1&&m.get(island1+0xae,1)==0&&m.get(island2+0xae,1)==0,
    "Native island collision allocation monitor lost the actual allocator chunk or marked empty islands");
  check(m.get(world+0x94)==1&&m.get(world+0x5f00)==world+0x5f20,"Native collision listener failed stable null compaction");
  check(m.get(island0+0x2c,1)==1,"Body policy failed to consume actual returned result or complete cohort");
  check(m.get(motion2+0x70)==0x3d088889&&m.get(motion3+0x70)==0&&m.get(motion2+0xf0)==0x3f800000&&m.get(motion3+0xf0)==0x3f800000,"Prescribed/fixed native motion used dynamic force or wrong pose");
  check(m.get(retired0+6,2)==9&&m.get(retiredReplacement+6,2)==1&&m.get(world+0x118)==0,"Post-detach array reread/reference lifetime");
  check(a.x87[0]==1.25L+calls.size()&&a.x87_status==fp.status&&a.floating_point->mxcsr==fp.mxcsr,"Final actual residual FP/x87 state lost");
  {
    m=model();a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;delete_pair_case=true;m.put(body2+0x20u,0);
    // The former opaque process-only agent does not define deletion owners.
    // This declared parent profile has no agent; genuine child cleanup is proved independently.
    m.put(world+0x7108u,0);
    const auto filter_begin=pair_filter_stores.size();
    native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,body_observer,&stats,&residuals,[&]{return ++timestamps;});
    check(stats.native_pair_consumer_calls==3&&stats.native_pair_consumer_nonempty_calls==1&&
      stats.native_pair_consumer_filter_calls==2&&stats.native_pair_consumer_create_calls==0&&
      stats.native_pair_consumer_delete_calls==1&&stats.native_pair_consumer_refusals==0&&
      pair_filter_stores.size()==filter_begin+2u&&m.get(island0+0x6cu)==0&&
      stats.island_agents.process_calls==0&&stats.island_agents.finish_calls==0&&
      stats.pair_deletion.attempts==1&&stats.pair_deletion.completed==1&&
      stats.pair_deletion.refusals==0&&stats.pair_deletion_scope.refused==0&&
      !stats.residual_owner_calls.contains(0x320260u)&&
      std::count(calls.begin(),calls.end(),0x320260u)==0&&
      !stats.residual_owner_calls.contains(0x321510u)&&!stats.residual_owner_calls.contains(0x2e9a00u),
      "World native pair deletion lost typed callback/record removal or silently kept a lifted parent");
    // Explicit unsupported-destructor control, not an authenticated object class.
    m=model();a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;
    m.put(body2+0x20u,0);m.put(world+0x7054u,0x2f3100u);m.put(world+0x7040u,0x2301a0u);
    bool refused=false;try{native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,body_observer,&stats,&residuals,[&]{return ++timestamps;});}
    catch(const std::runtime_error&error){refused=std::string(error.what()).find("2294176")!=std::string::npos;}
    check(refused&&stats.pair_deletion.attempts==1&&stats.pair_deletion.completed==0&&stats.pair_deletion.refusals==1&&
      stats.pair_deletion_scope.refused==1&&stats.pair_deletion_scope.owners.at(0x2301a0u)==1&&
      !stats.residual_owner_calls.contains(0x2301a0u)&&std::find(calls.begin(),calls.end(),0x2301a0u)==calls.end(),
      "Native pair deletion entered original transport for an unsupported nested owner");
    delete_pair_case=false;
  }
  {
    m=model();a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;
    constexpr std::uint32_t pool=0x2000000u,table=0x2001000u;
    m.banks[pool].resize(0x100000u);m.put(pool,table);m.put(table+0x10u,0x233250u);m.put(table+0x14u,0x233420u);
    m.put(pool+0x2c0u,0);m.put(pool+0x68u,0x2000u);
    m.put(pool+0xacu+32u,1,1);m.put(pool+0x6cu,32u);
    m.put(pool+0xacu+4u,2,1);m.put(pool+0x70u,16u);
    m.put(0x486064u,1);m.put(0x486068u,pool);
    m.put(world+0x5e50u,0x149da0u);m.put(world+0x5e54u,0x149e00u);
    m.put(world+0x5e64u,0x2e10a0u);m.put(world+0x5e68u,0x2e1110u);m.put(world+0x5e0cu,0);
    m.put(world+0x600cu,0); // Exhaust the separate constraint workspace too.
    native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,
      {stack,0x10000},invoke,body_observer,&stats,&residuals,[&]{return ++timestamps;});
    check(stats.native_stack_arena_allocations>0&&stats.native_stack_arena_allocations==stats.native_stack_arena_releases&&
      stats.native_stack_arena_refusals==0&&stats.native_array_pool_allocations>0&&stats.native_array_pool_releases>0&&
      stats.native_constraint_arena_allocations>0&&stats.native_constraint_arena_allocations==stats.native_constraint_arena_releases&&
      stats.native_constraint_arena_refusals==0&&
      !stats.residual_owner_calls.contains(0x2e10a0u)&&!stats.residual_owner_calls.contains(0x2e1110u),
      "World broad-phase/contact buffers bypassed native slow arenas or lost backing lifetime");
  }
  for(const auto profile:{std::pair{2u,0u},std::pair{1u,1u},std::pair{1u,2u},
      std::pair{1u,3u},std::pair{1u,4u},std::pair{1u,5u},std::pair{1u,6u},std::pair{1u,7u},std::pair{1u,8u},std::pair{1u,9u},std::pair{1u,10u},std::pair{1u,11u},std::pair{1u,12u},std::pair{1u,13u},std::pair{1u,14u},std::pair{1u,15u},std::pair{1u,16u}}){
    const auto policy_refs=profile.first;reversed_list_cleanup_case=profile.second==10u;
    bv_cleanup_case=profile.second==11u||profile.second==12u;bv_secondary_cleanup_case=profile.second==12u;
    bv_cleanup_stores.clear();
    single_point_cleanup_case=profile.second==13u||profile.second==14u;single_point_none_case=profile.second==14u;
    phantom_callback_cleanup_case=profile.second==15u||profile.second==16u;
    phantom_callback_leave_case=profile.second==16u;
    inactive_cleanup_agent_kind=reversed_list_cleanup_case?5u:bv_cleanup_case?4u:
      (single_point_cleanup_case||phantom_callback_cleanup_case)?3u:profile.second;
    m=model();a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;inactive_cleanup_case=true;
    inactive_cleanup_policy_refs=policy_refs;if(policy_refs==1)m.banks[0x2000000u].resize(0x100000u);
    m.put(world+0x78u,world+0x9c00u);m.put(world+0x7cu,1);m.put(world+0x9c00u,world+0x9c20u);
    m.put(world+0x9c20u,world+0x9c40u);m.put(world+0x9c48u,0x282ab0u);
    native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,
      {stack,0x10000},invoke,body_observer,&stats,&residuals,[&]{return ++timestamps;});
    check(stats.inactive_island_cleanup.calls==1&&stats.inactive_island_cleanup.invalidated_agents==1&&
      stats.inactive_island_cleanup.invalidated_managers==1&&stats.inactive_island_cleanup.cleared_pairs==1&&
      stats.inactive_island_cleanup.policy_queries==1&&stats.inactive_island_cleanup.policy_selections==1&&
      stats.inactive_island_policy.calls==1&&stats.inactive_island_policy.default_installs==1&&
      m.get(world+0x9108u)==0&&m.get(world+0x9454u)==0x48d3d8u&&
      (policy_refs==1?(stats.inactive_island_policy.gameplay_destructions==1&&
        stats.inactive_island_policy.storage_releases==1&&stats.inactive_island_policy.unconverted_destructions==0&&
        m.get(0x2000024u)==0x2004340u&&m.get(0x20002e8u)==0):m.get(world+0x9506u,2)==1)&&
      stats.native_inactive_island_cleanup_refusals==0&&!stats.residual_owner_calls.contains(0x282ab0u)&&
      !stats.residual_owner_calls.contains(0x31b730u)&&!stats.residual_owner_calls.contains(0x31d400u)&&
      !stats.residual_owner_calls.contains(0x1d9c80u),
      "World inactive island bypassed native pair cleanup/default policy or used an old parent");
    check(stats.native_contact_agent_record_cleanups==((inactive_cleanup_agent_kind==1u||inactive_cleanup_agent_kind==2u||inactive_cleanup_agent_kind==5u)?1u:0u)&&
      stats.native_contact_agent_pointer_cleanups==((inactive_cleanup_agent_kind==2u||inactive_cleanup_agent_kind==5u)?1u:0u)&&
      stats.native_contact_reference_disposals==(inactive_cleanup_agent_kind?1u:0u)&&
      stats.native_contact_agent_child_residuals==0&&stats.native_contact_agent_destructor_residuals==0&&
      stats.native_contact_agent_cleanup_refusals==0&&!stats.residual_owner_calls.contains(0x2f3100u)&&
      !stats.residual_owner_calls.contains(0x2f7a20u)&&!stats.residual_owner_calls.contains(0x323220u),
      "World recursive agent cleanup bypassed native parents or lost typed disposal");
    check(stats.native_contact_agent_cached_cleanups==((inactive_cleanup_agent_kind>=3u&&!phantom_callback_cleanup_case)?1u:0u)&&
      stats.native_contact_agent_wrapped_cleanups==(inactive_cleanup_agent_kind>=4u?1u:0u)&&
      stats.native_contact_agent_native_destructors==(inactive_cleanup_agent_kind==5u?4u:inactive_cleanup_agent_kind>=4u?2u:inactive_cleanup_agent_kind==3u?1u:0u)&&
      stats.native_contact_agent_point_residuals==0&&!stats.residual_owner_calls.contains(0x2f82d0u)&&
      !stats.residual_owner_calls.contains(0x2f6c80u)&&!stats.residual_owner_calls.contains(0x2fc540u)&&
      !stats.residual_owner_calls.contains(0x2f8090u)&&!stats.residual_owner_calls.contains(0x3051a0u),
      "World cached/wrapped cleanup bypassed native point loop, destructor or actual pool lifetime");
    // Final World detach drains the two deferred references after manager cleanup.
    if(inactive_cleanup_agent_kind>=6u)check(stats.native_contact_manager_cleanups==1&&
      stats.native_contact_collector_cleanups==1&&stats.native_contact_listener_unregistrations==2&&
      stats.native_contact_listener_deferred_references==2&&stats.native_contact_manager_service_residuals==0&&
      stats.native_contact_manager_cleanup_refusals==0&&m.get(world+0x118u)==0&&
      m.get(world+0xdb00u)==0&&m.get(world+0xdb20u)==0&&
      m.get(world+0xd906u,2)==2&&m.get(world+0xda06u,2)==2&&
      !stats.residual_owner_calls.contains(0x3232b0u),
      "World manager cleanup bypassed native collector/listener/reference/storage ownership");
    check(stats.reversed_list_agent_destructor.attempts==unsigned(reversed_list_cleanup_case)&&
      stats.reversed_list_agent_destructor.completed==unsigned(reversed_list_cleanup_case)&&
      stats.reversed_list_agent_destructor.refusals==0&&stats.reversed_list_agent_destructor.native_only.refused==0&&
      !stats.residual_owner_calls.contains(0x2f81b0u),"World reversed list scalar destructor bypassed native binding");
    check(!stats.residual_owner_calls.contains(0x2f0da0u)&&
      bv_cleanup_stores.size()==unsigned(bv_secondary_cleanup_case)&&
      (!bv_secondary_cleanup_case||(m.get(world+0xd140u)==world+0xd240u&&
        bv_cleanup_stores[0].written==std::vector<std::byte>(4,std::byte{}))),
      "World BV cleanup dispatched a residual, lost its clear store or disposed an unowned leaf");
    check(!stats.residual_owner_calls.contains(0x2fbb10u),
      "World single-point cleanup bypassed its native owner");
    check(!stats.residual_owner_calls.contains(0x2f7790u)&&!stats.residual_owner_calls.contains(0x1f3550u),
      "World phantom callback cleanup bypassed its native owner");
    reversed_list_cleanup_case=false;bv_cleanup_case=false;bv_secondary_cleanup_case=false;
    single_point_cleanup_case=false;single_point_none_case=false;
    phantom_callback_cleanup_case=false;phantom_callback_leave_case=false;
    inactive_cleanup_case=false;
  }
  for(const auto phantom_owner:{0x2e4d60u,0x315a70u,0x315100u}){
    m=model();a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;
    delete_pair_case=true;m.put(island0+0x78u,0);m.put(island0+0x6cu,0);
    m.put(body0+0x24u,2);m.put(body1+0x24u,2);
    m.put(body0+0x2cu,world+0x9800u);m.put(body1+0x2cu,world+0x9a00u);
    m.put(world+0x9800u,world+0x9c00u);m.put(world+0x9a00u,world+0x9c00u);
    m.put(world+0x9c20u,phantom_owner);
    if(phantom_owner==0x315a70u){
      m.put(world+0x98c0u,world+0x9e00u);m.put(world+0x98c4u,1);m.put(world+0x9e00u,payload1);
      m.put(world+0x9ac0u,world+0x9e20u);m.put(world+0x9ac4u,1);m.put(world+0x9e20u,payload0);
    }
    if(phantom_owner==0x315100u){
      m.put(world+0x9880u,world+0x9e00u);m.put(world+0x9884u,1);m.put(world+0x9e00u,payload1);
      m.put(world+0x9a80u,world+0x9e20u);m.put(world+0x9a84u,1);m.put(world+0x9e20u,payload0);
    }
    m.put(pair_dispatcher+(2u*8u+2u)*4u,island0+0x1cu);
    m.put(m.get(island0+0x1cu)+8u,0x320040u);
    native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,
      {stack,0x10000},invoke,body_observer,&stats,&residuals,[&]{return ++timestamps;});
    check(stats.native_pair_consumer_phantom_delete_calls==1&&stats.native_pair_consumer_delete_calls==1&&
      stats.native_pair_consumer_phantom_notifications==2&&
      stats.native_pair_consumer_phantom_noops==(phantom_owner==0x2e4d60u?2u:0u)&&
      stats.native_pair_consumer_phantom_overlap_remove_calls==(phantom_owner==0x315a70u?2u:0u)&&
      stats.native_pair_consumer_simple_phantom_overlap_remove_calls==(phantom_owner==0x315100u?2u:0u)&&
      (phantom_owner!=0x315100u||(m.get(world+0x9884u)==0&&m.get(world+0x9a84u)==0))&&
      stats.native_pair_consumer_phantom_unqualified_owners==0&&stats.native_pair_consumer_refusals==0&&
      stats.island_agents.process_calls==0&&!stats.residual_owner_calls.contains(0x320040u)&&
      !stats.residual_owner_calls.contains(0x2e4d60u),
      "World phantom deletion bypassed native parent/children or lost symmetric notifications");
    delete_pair_case=false;
  }
  for(const auto owner:{0x1d9b20u,0x1d9b70u})for(bool active:{false,true}){
    m=model();a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;
    m.put(world+0x5494u,owner);m.put(world+0x5460u,0x12340001u);m.put(world+0x5464u,0);
    m.put(0x4ecbd0u,world+0x9c00u);m.put(world+0x9c44u,world+0xa000u);
    m.put(world+0xa014u,world+0xa400u);m.put(world+0xa00fu,active?1:2,1);
    m.put(world+0xa4b4u,0xabcd0001u);m.put(0x52be5cu,world+0x9d00u);
    m.put(world+0x9d44u,world+0xb000u);m.put(world+0xb110u,world+0xc000u);
    m.put(world+0xa400u,0x55550001u);m.put(0x4f0414u,world+0xd000u);
    m.put(world+0xd018u,world+0xd100u);m.put(world+0xd2ecu,1);m.put(world+0xa78bu,0xfe,1);
    native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,body_observer,&stats,&residuals,[&]{return ++timestamps;});
    check(stats.gameplay_deactivation.calls==1&&stats.gameplay_deactivation.blocked==unsigned(active)&&
      stats.gameplay_deactivation.vehicle_tests==unsigned(active)&&
      stats.gameplay_deactivation.body_policies==unsigned(!active)&&checks==0&&
      !stats.residual_owner_calls.contains(0x1d9b20u)&&!stats.residual_owner_calls.contains(0x1d9b70u),
      "World sleep policy bypassed native eligibility or lost its real policy/output data");
  }
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
    native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,body_observer,&stats,&residuals,[&]{return ++timestamps;});
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
    native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,body_observer,&stats,&residuals,[&]{return ++timestamps;});
    check(stats.shape_bounds.transformed_calls==2&&stats.shape_bounds_residual_calls==5&&stats.compound_shape_bounds.compound_calls==1&&
      stats.compound_shape_bounds.child_calls==2&&
      std::count(calls.begin(),calls.end(),nestedBounds)==1,
      "Native compound bounds lost nested caller-owned packets or a genuine child");
  }
  {
    m=model();a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;
    contact_motion_case=true;
    m.put(world+0x254u,world+0x5c80u);m.put(world+0x24cu,0x3d800000u);
    m.put(world+0x150u,0x3d088889u);
    m.put(payload0+32u,body0);m.put(payload1+32u,body1);
    m.put(world+0x5074u,0x325810u);m.put(world+0x5174u,0x8fdb0u);
    m.put(motion0+0x7cu,0x3f800000u);m.put(motion1+0x7cu,0x3f800000u);
    m.put(motion0+0xc0u,0x3f800000u);
    std::vector<SourcePhysicsContactRecordStore>motion_stores;
    native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,
      [&](const auto&store){body_observer(store);if(store.instruction==0x325a50u)motion_stores.push_back(store);},
      &stats,&residuals,[&]{return ++timestamps;});
    check(stats.contact_advance.calls==1&&stats.cached_motion.calls>0&&
      stats.cached_motion.rewinds==stats.cached_motion.calls&&stats.cached_motion.advances==stats.cached_motion.calls&&
      stats.native_contact_fixed_motion_calls>0&&stats.contact_motion_residual_calls==0,
      "Native contact-time world binding bypassed cached/fixed motion or hid fallback");
    check(motion_stores.size()==stats.cached_motion.calls&&
      std::all_of(motion_stores.begin(),motion_stores.end(),[](const auto&s){return s.address==motion0+0x7cu;})&&
      !stats.residual_owner_calls.contains(0x325810u)&&!stats.residual_owner_calls.contains(0x8fdb0u),
      "Cached motion did not publish source fraction stores or invoked a lifted motion owner");
    check(a.x87_depth==2&&a.x87[0]==1.25L+calls.size()&&a.x87[1]==-4.L,
      "Native cached motion destroyed real preserved caller x87 values");
    contact_motion_case=false;
  }
  {
    m=model();a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;
    m.put(body0+12u,world+0x7f00u);m.put(world+0x7f00u,world+0x7f40u);
    m.put(world+0x7f58u,0x2e8d90u);m.put(world+0x7f0cu,0x3f000000u);
    std::vector<SourcePhysicsContactRecordStore>sphere_stores;
    native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,
      [&](const auto&store){body_observer(store);if(store.instruction==0x2e8dbau||store.instruction==0x2e8dc4u)sphere_stores.push_back(store);},
      &stats,&residuals,[&]{return ++timestamps;});
    check(stats.sphere_bounds.calls==1&&stats.shape_bounds_residual_calls==3&&sphere_stores.size()==2&&
      sphere_stores[1].address==sphere_stores[0].address+16u&&
      !stats.residual_owner_calls.contains(0x2e8d90u),
      "Native sphere bounds lost source stores or invoked its lifted owner");
    for(unsigned lane=0;lane<4;++lane){
      check(sphere_stores[0].written[lane*4+3]==std::byte{0xbf}&&
        sphere_stores[1].written[lane*4+3]==std::byte{0x3f},
        "Native sphere did not publish the original lower/upper center expansion");
    }
  }
  {
    m=model();a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;
    completion_case=true;
    m.put(world+0x70ccu,0x3230c0u);m.put(world+0x7088u,world);m.put(world+0x708cu,1);
    m.put(world+0x70b0u,world+0x7e40u);m.put(world+0x7e42u,0,1);m.put(world+0x70c4u,world+0x7f00u);
    m.put(payload0+0x20u,world+0x7200u);m.put(payload1+0x20u,world+0x7300u);
    m.put(world+0x735cu,7,2);m.put(world+0x725cu,3,2);
    m.put(world+0xb4u,world+0x7f40u);m.put(world+0xb8u,3);m.put(world+0x7f44u,world+0x7f80u);
    m.put(world+0x7f80u,world+0x7fa0u);m.put(world+0x7fa8u,0x230140u);
    m.put(world+0x7f40u,world+0x7600u);m.put(world+0x7600u,world+0x7640u);m.put(world+0x7648u,0x281e10u);
    for(unsigned n=0;n<2;++n){
      const auto entity=world+0x7400u+n*0x100u,properties=world+0x7700u+n*0x40u;
      m.put(entity+0x30u,properties);m.put(entity+0x34u,2);
      m.put(properties,0x2001u);m.put(properties+4u,n?0x22220002u:0x11110001u);
      m.put(properties+8u,0x2002u);m.put(properties+12u,n?7u:3u);
    }
    m.put(0x52be5cu,world+0x9000u);m.put(world+0x9044u,world+0x9100u);m.put(world+0x9100u+320u+4u,1,1);
    m.put(world+0x92c8u,world+0x9400u);m.put(world+0x92ccu,2);
    for(auto row:{world+0x9400u,world+0x9448u}){
      m.put(row,2,2);m.put(row+0x14u,0x11110001u);m.put(row+0x45u,7,1);m.put(row+0x46u,3,1);
    }
    // Declared native contact DATA, replacing a former opaque Body callback.
    m.put(world+0x70d4u,0x323040u);m.put(world+0x70d8u,0x323020u);
    m.put(world+0x70b0u,world+0xe000u);m.put(world+0xe000u+2u*4u,0u);
    m.put(world+0x7120u,world+0xe200u);
    m.put(world+0xe200u,0u);m.put(world+0x92b0u,world+0xe300u);
    m.put(world+0xe300u+7u*96u+0x40u,world+0xe800u);
    m.put(world+0xe83cu,world+0xe900u);m.put(world+0xe900u,world+0xea00u);
    m.put(world+0xea18u,0x31dd70u);m.put(world+0xe92cu,0x3f800000u);
    m.put(0x46a0d0u,0x3f800000u);m.put(0x46a0dcu,0xbf800000u);
    m.put(0x46a240u,0x3f800000u);m.put(0x46a238u,0u);
    m.put(0x51df54u,world+0xeb00u);m.put(world+0xeb08u,100u);
    m.put(world+0x7fc4u,world+0x7fd0u);m.put(world+0x7fd0u,world+0x7fe0u);m.put(world+0x7fe8u,0x2e4d60u);
    m.put(world+0x7360u,world+0x7ff0u);m.put(world+0x7364u,1);m.put(world+0x7ff0u,world+0x7fd0u);
    native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,body_observer,&stats,&residuals,[&]{return ++timestamps;});
    check(stats.contact_completion.calls==1&&stats.contact_completion.copied_contacts==1&&
      stats.contact_completion.world_notifications==1&&stats.contact_completion.body_notifications==2&&
      stats.contact_completion.custom_callbacks==2&&stats.contact_completion.null_entries_removed==2&&
      m.get(world+0x708cu)==4&&m.get(world+0xb8u)==2&&m.get(world+0x7264u)==1&&
      m.get(world+0x7f00u)==0x3f000000u&&m.get(world+0x7f1cu)==0x3f000007u&&
      !stats.residual_owner_calls.contains(0x3230c0u)&&
      std::count(calls.begin(),calls.end(),0x230140u)==1,
      "Native contact completion lost source data, late callback counts or cached body owners");
    check(stats.contact_entities.calls==1&&stats.contact_entities.property_queries==4&&
      stats.contact_entities.dispatch_calls==1&&stats.contact_entities.disabled_components==0&&
      stats.contact_body.completed==2&&stats.contact_body.refusals==0&&stats.contact_body.native_only.refused==0&&!stats.residual_owner_calls.contains(0x281e10u)&&
      !stats.residual_owner_calls.contains(0x1d8a20u)&&!stats.residual_owner_calls.contains(0x1d77d0u)&&
      stats.contact_processor.calls==1&&stats.contact_processor.contacts==1&&
      stats.contact_processor.candidate_rows==2&&stats.contact_processor.matched_rows==2&&stats.contact_processor.updates==2,
      "Native contact notification bypassed typed entity selection or hid original parent fallback");
    check(a.x87_depth==2&&a.x87[0]==1.25L+calls.size()&&a.x87[1]==-4.L,
      "Contact completion destroyed the genuine callback FP context");
    completion_case=false;
  }
  for(bool corrupt:{false,true}){
    m=model();m.put(0x48d480,0);m.put(0x47c1b0,world+0x6f00);
    a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;corrupt_clock=corrupt;
    bool rejected{};
    try{native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,body_observer,&stats,&residuals,[&]{return ++timestamps;});}
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
    try{native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,body_observer,&stats,&residuals,[&]{return ++timestamps;},
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
  {
    m=model();a=initial();fp={0x27f,0x2800,0x1f80};stats={};residuals=0;calls.clear();checks=0;
    const auto broad=world+0x5d40u,nodes=world+0xc000u;
    m.put(world+0x6c14u,0x2ea380u);
    for(unsigned lane=0;lane<4;++lane){m.put(broad+0x10u+4u*lane,0);m.put(broad+0x20u+4u*lane,0);m.put(broad+0x30u+4u*lane,0x3f800000u);m.put(0x41c680u+4u*lane,0x47800000u);m.put(0x41c690u+4u*lane,0);m.put(0x41c6a0u+4u*lane,0x477ffc00u);}
    m.put(broad+0x40u,nodes);m.put(broad+0x78u,world+0xc800u);
    constexpr std::array<unsigned,3>low{8,0,2},high{10,4,6};
    for(unsigned axis=0;axis<3;++axis){const auto edges=world+0xc100u+axis*0x100u;m.put(broad+std::array<unsigned,3>{0x4c,0x58,0x64}[axis],edges);m.put(edges,0);m.put(edges+9u*4u,0xffffu);
      for(unsigned n=0;n<4;++n){m.put(edges+(n+1u)*4u,(n+1u)<<16u);m.put(edges+(n+5u)*4u,((n+1u)<<16u)|1u);m.put(nodes+(n+1u)*16u+low[axis],n+1u,2);m.put(nodes+(n+1u)*16u+high[axis],n+5u,2);m.put(nodes+(n+1u)*16u+12u,body0+n*0x100u+0x1cu);m.put(body0+n*0x100u+0x1cu,n+1u);}}
    struct SweepPrefixStop{};bool stopped=false;
    NativeTu15PairConsumerInputObserverBinding boundary(m,[&](const auto&,const auto&input){if(input.phase==NativeTu15PairConsumerInputPhase::Entry)throw SweepPrefixStop{};});
    try{native_tu15_step_physics_world_with_residuals(m,{world},{0x3d088889,0x41f00000},fp,a,0x60300000,{stack,0x10000},invoke,body_observer,&stats,&residuals,[&]{return ++timestamps;});}
    catch(const SweepPrefixStop&){stopped=true;}
    check(stopped&&stats.broad_phase_edges.attempts==1&&stats.broad_phase_edges.completed==1&&stats.broad_phase_edges.refusals==0&&stats.broad_phase_edges.edges.objects==4&&stats.broad_phase_edges.edges.array_growths==0&&std::count(calls.begin(),calls.end(),dispatcherSweep)==0&&!stats.residual_owner_calls.contains(0x2ea380u),"World sweep bypassed semantic2EA380 or supplied an old result");
  }
  std::cout<<"World residual model: exact residual owner/phase order and ready-zero clock/logger transport, retained packets/outbyte, native dynamic/kinematic/fixed motion, policy, island-transition and native array-growth/collision scheduling/listener/native-constraint/island-collision/broad-phase composition, live x87/FP and failure prefixes passed; full original/live composition remains open\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
