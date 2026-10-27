#include "halo2/native_tu15_constraint_step_residuals.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
namespace {
using namespace halo2;
void check(bool value,const char*why){if(!value)throw std::runtime_error(why);}
struct Memory:SourcePhysicsContactMemory {
  std::map<std::uint32_t,std::vector<std::byte>>banks;
  auto at(std::uint32_t p,std::size_t bytes)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing constraint ABI bank");--i;check(std::uint64_t(p-i->first)+bytes<=i->second.size(),"Missing constraint ABI extent");return i;}
  void read(std::uint32_t p,std::span<std::byte>b)const override{const auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
  void write(std::uint32_t p,std::span<const std::byte>b)override{const auto i=at(p,b.size());std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
  void put(std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8u*i));write(p,b);}
  std::uint32_t get(std::uint32_t p,unsigned bytes=4)const{std::array<std::byte,4>b{};read(p,std::span(b).first(bytes));std::uint32_t value{};for(unsigned i=0;i<bytes;++i)value|=std::to_integer<unsigned>(b[i])<<(8u*i);return value;}
};
constexpr std::uint32_t base=0x1000000,island=base+0x100,step=base+0x200,solver=base+0x300,jacobian=base+0x400,workspace=base+0x500,
 allocator=base+0x600,bodies=base+0x700,constraints=base+0x800,gravity=base+0x900,body0=base+0x1000,body1=base+0x1100,
 motion0=base+0x2000,motion1=base+0x2200,arena=base+0x4000,extra=base+0x8000,packet=base+0xe000,
 setup_owner=0x230210,commit_owner=0x230220,builder_owner=0xf0000042;
Memory model(){Memory m;m.banks[base].resize(0x10000);m.banks[0x410000].resize(0x70000);m.banks[0x48c000].resize(0x2000);m.banks[0x491000].resize(0x1000);
  m.put(0x48d480,1);m.put(0x491350,workspace);m.put(workspace+8,arena);m.put(workspace+0xc,0x1000);m.put(0x48c938,allocator);m.put(allocator,0x45f4cc);
  m.put(allocator+8,extra);m.put(allocator+0xc,0x2000);m.put(allocator+0x14,0);m.put(0x45f4f0,0x2e10a0);m.put(0x45f4f4,0x2e1110);
  m.put(island+8,0x40);m.put(island+0xc,0x100);m.put(island+0x10,0x40);m.put(island+0x14,0x100);m.put(solver+0x20,0x3f000000);
  m.put(bodies,body0);m.put(bodies+4,body1);m.put(body0+0x3c,motion0);m.put(body1+0x3c,motion1);
  m.put(motion0,0x41f2e0);m.put(0x41f2ec,0x325410);m.put(0x41f2f0,0x3249e0);
  m.put(motion1,0x41f380);m.put(0x41f38c,setup_owner);m.put(0x41f390,commit_owner);
  for(auto motion:{motion0,motion1}){m.put(motion+0x2c,0x3f400000);m.put(motion+0x80,0x3f800000);m.put(motion+0x94,0x3f800000);m.put(motion+0xa8,0x3f800000);}
  m.put(motion0+0x3c,0x3f800000);m.put(motion0+0xc,0x3e800000);
  for(unsigned n=0;n<3;++n)m.put(0x41f1d0+n*4,0xffffffff);
  m.put(0x46a0e4,0x40400000);m.put(0x46a0cc,0x3f000000);
  m.put(step,0x3e000000);m.put(0x46a0d0,0x3f800000);m.put(0x46a0e8,0);m.put(gravity+8,std::bit_cast<std::uint32_t>(-9.8f));
  for(unsigned n=0;n<2;++n){const auto constraint=base+0xa00+n*0x100;m.put(constraints+n*4,constraint);m.put(constraint,0x41ed90);m.put(constraint+0xc,body0);m.put(constraint+0x10,body1);}
  m.put(0x41edb4,builder_owner);return m;
}
NativeTu15PhysicsConstraintInput input(){return{{step},solver,jacobian,{gravity},{island},{bodies},2,constraints,2};}
}
int main()try{
  for(bool spill:{false,true}){
    auto m=model();if(spill)m.put(workspace+0xc,0x10);const auto start=spill?extra:arena;
    NativeTu15PhysicsFrameFp fp;NativeTu15ConstraintBindingActivity activity;std::vector<std::uint32_t>calls;unsigned builders{};
    native_tu15_step_island_constraints_with_residuals(m,input(),fp,0,packet,[&](auto owner,auto args,auto receiver,unsigned pop,auto&context)->std::uint32_t{
      calls.push_back(owner);
      if(owner==setup_owner){check(receiver==motion1&&pop==12&&args.size()==3&&args[0]==step&&args[1]==gravity&&args[2]==start+0x100,"Other setup lost actual receiver/arguments/RET12");return args[2]+0x80;}
      if(owner==builder_owner){check(receiver==base+0xa00+builders*0x100&&pop==8&&args.size()==2&&args[0]==jacobian&&args[1]==packet,"Builder lost actual receiver/RET8/retained packet");check(m.get(jacobian)==start+0x80&&m.get(jacobian+4)==start+0x100,"Builder pair lost genuine accumulator offsets");
        if(!builders){m.put(packet,m.get(packet+8));m.put(packet+12,base+0xc000);}else check(m.get(packet)==base+0xc000&&m.get(packet+8)==0xffffffff,"Builder packet mutations were not recovered before rollover");m.put(packet+4,m.get(packet+4)+4);++builders;context.status|=1;return 0xabad1dea;
      }
      if(owner==0x3249e0||owner==commit_owner){const auto dynamic=owner==0x3249e0;check(receiver==(dynamic?motion0:motion1)&&pop==12&&args.size()==3&&args[0]==step&&args[1]==0x3f000000&&args[2]==start+(dynamic?0x80:0x100),"Commit lost actual returned accumulator/parameter/RET12");return args[2]+0x80;}
      if(owner==0x2e1110){check(spill&&receiver==allocator&&pop==8&&args.size()==2&&args[0]==extra&&args[1]==0x2e0,"Spilled workspace lost actual release lifetime/RET8");return 0xdeadbeef;}
      throw std::runtime_error("Unexpected constraint residual owner");
    },{},&activity);
    std::vector<std::uint32_t>expected{setup_owner,builder_owner,builder_owner,commit_owner};if(spill)expected.push_back(0x2e1110);
    check(calls==expected&&activity.residual_calls==calls.size()&&activity.solver.calls==1&&activity.solver.initializations==0&&activity.solver.iterations==0&&activity.native_dynamic_setups==1&&activity.native_motion_commits==1&&activity.native_motion_commit_kinds==std::array<std::uint64_t,3>{0,1,0},"Constraint binding failed native dynamic setup/actual dependency order");
    check(m.get(workspace+0x10,1)==0&&m.get(motion0+8)==0x80&&m.get(motion1+8)==0x100&&m.get(start+0x80+0xc,1)==0,"Constraint binding failed retained state and cleanup");
  }
  // Keep arbitrary-owner ABI transport distinct from the actual native
  // producers. The real zero-row inputs below invoke both native base/header
  // paths; scaled builders also create their genuine pre/post payloads.
  for(const auto native_owner:{0x31f770u,0x31f860u}){
    auto m=model();m.put(0x41edb4,native_owner);NativeTu15PhysicsFrameFp fp;
    NativeTu15ConstraintBindingActivity activity;std::vector<std::uint32_t>calls;
    native_tu15_step_island_constraints_with_residuals(m,input(),fp,0,packet,
      [&](auto owner,auto args,auto receiver,unsigned pop,auto&)->std::uint32_t{
        calls.push_back(owner);
        if(owner==setup_owner){check(receiver==motion1&&pop==12,"Native producer setup transport differs");return args[2]+0x80;}
        if(owner==commit_owner)return args[2]+0x80;
        throw std::runtime_error("Qualified native producer fell back to residual execution");
      },{},&activity);
    check(calls==std::vector<std::uint32_t>{setup_owner,commit_owner}&&activity.base.calls==2&&
      activity.base.rows==0&&activity.producers.base_calls==2,
      "Native producer binding bypassed genuine zero-row base construction");
    check(activity.producers.axis_factor_calls==2u*unsigned(native_owner==0x31f770u)&&
      activity.producers.scaled_calls==2u*unsigned(native_owner==0x31f860u),
      "Native producer binding selected the wrong original owner");
    if(native_owner==0x31f860u)check(activity.completed_producer_emissions==std::array<std::uint64_t,3>{2,2,0}&&
      activity.producers.emissions==activity.completed_producer_emissions,
      "Native producer completed-emission accounting differs from its direct native path");
  }
  for(const auto owner:{0x31f3a0u,0x31f410u,0x31f560u,0x230abcu}){
    auto m=model();NativeTu15PhysicsFrameFp fp;NativeTu15ConstraintBindingActivity activity;
    const auto schema=arena+0x298u,custom=schema+0x18u,payload=custom+8u;
    m.put(solver+0x1c,1);m.put(custom,0x400eu);m.put(custom+4,owner);
    m.put(payload+0x20,0x3f000000);m.put(payload+0x24,0x3f000000);
    m.put(payload+0x28,arena+0x80);m.put(payload+0x2c,arena+0x100);
    if(owner==0x31f560u)m.put(payload+0x10,arena+0x900);
    unsigned builders{};bool refused{};
    try{native_tu15_step_island_constraints_with_residuals(m,input(),fp,0,packet,
      [&](auto target,auto args,auto,unsigned,auto&)->std::uint32_t{
        if(target==setup_owner){m.put(args[2],2);return args[2]+0x80;}
        if(target==builder_owner){if(!builders++)m.put(args[1]+4,custom+0x40);return 0;}
        if(target==commit_owner)return args[2]+0x80;
        throw std::runtime_error("Native custom schema fell back to a residual owner");
      },{},&activity);
    }catch(const std::runtime_error&e){
      refused=std::string(e.what()).find("custom schema owner requires native/ABI qualification: 0x230abc")!=std::string::npos;
      if(!refused)throw;
    }
    const bool known=owner!=0x230abcu;
    check(refused!=known&&activity.solver.calls==1&&activity.solver.custom_schemas==1&&
      activity.custom_schemas.calls==unsigned(known)&&activity.custom_schema_refusals==unsigned(!known),
      "Native custom schema whitelist or unknown-owner refusal was bypassed");
    check(m.get(workspace+0x10,1)==unsigned(!known),
      "Custom schema failure fabricated cleanup or successful schema leaked workspace");
  }
  auto m=model();NativeTu15PhysicsFrameFp fp;bool failed{};
  try{native_tu15_step_island_constraints_with_residuals(m,input(),fp,0,packet,[&](auto owner,auto args,auto,unsigned,auto&context)->std::uint32_t{
    if(owner==setup_owner)return args[2]+0x80;
    check(owner==builder_owner,"Failure prefix reached unexpected owner");m.put(args[1]+4,0x12345678);context.mxcsr|=0x20;throw std::runtime_error("Actual builder stopped");
  });}catch(const std::runtime_error&e){failed=std::string(e.what())=="Actual builder stopped";}
  check(failed&&m.get(packet+4)==0x12345678&&(fp.mxcsr&0x20)&&m.get(workspace+0x10,1)==1,"Constraint failure lost actual partial packet/FP or fabricated cleanup");
  std::cout<<"Constraint ABI model passed: real owner arguments/RET sizes, native dynamic setup, mutable builder packet, native solver return, commits, workspace lifetime and fault prefix; independent original/live solver closure remains open\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
