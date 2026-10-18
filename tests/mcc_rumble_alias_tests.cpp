#include "halo2/mcc_script_layout.hpp"
#include "halo2/source_script_rumble_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_campaign_runtime_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/simulation.hpp"
#include <iostream>
#include <set>
namespace {
using namespace halo2;
void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
void put(SourceRuntimeMemory &m,unsigned a,unsigned v,unsigned count=4){std::array<std::byte,4>b{};for(unsigned i=0;i<count;++i)b[i]=std::byte(v>>(i*8));m.write(a,std::span(b).first(count));}
unsigned get(const SourceRuntimeMemory&m,unsigned a){return Bytes(m.view(a,4)).u32(0);}
void row(std::span<std::byte>out,unsigned index,unsigned salt,unsigned opcode,unsigned type,unsigned flags,unsigned next,unsigned offset,unsigned value){unsigned a=index*20;const auto store=[&](unsigned at,unsigned v,unsigned count=4){for(unsigned n=0;n<count;++n)out[at+n]=std::byte(v>>(n*8));};store(a,salt,2);store(a+2,opcode,2);store(a+4,type,2);store(a+6,flags,2);store(a+8,next);store(a+12,offset);store(a+16,value);}
const std::string name("player_effect_set_max_vibration\0",32);
std::vector<std::byte> syntax(){std::vector<std::byte> n(80);row(n,0,0x9000,0x295,4,8,0xffffffff,0xabcdef01,0x90010001);row(n,1,0x9001,2,2,9,0x90020002,0,0);row(n,2,0x9002,6,6,9,0x90030003,30,0x3f800000);row(n,3,0x9003,6,6,9,0xffffffff,30,0x3f800000);return n;}
ScriptProgram program(const std::vector<std::byte>&n){ScriptProgram p;p.strings=name;const Bytes b(n);for(unsigned a=0;a<n.size();a+=20)p.nodes.push_back({b.u16(a),b.u16(a+2),b.u16(a+4),b.u16(a+6),b.u32(a+8),b.u32(a+12),b.u32(a+16)});p.scripts.push_back({"sample",0,4,0x90000000});return p;}
void effect_bank(SourceRuntimeMemory&m,unsigned owner){m.mount(0x50ebf8,std::vector<std::byte>(4),SourceRuntimeMemory::Access::ReadWrite,"effect pointer fixture");m.mount(owner,std::vector<std::byte>(0x22c,std::byte{0x5a}),SourceRuntimeMemory::Access::ReadWrite,"effect constructor extent fixture");put(m,0x50ebf8,owner);}
void cpp_test(){auto n=syntax();const auto old=n;const auto names=std::as_bytes(std::span(name));const auto converted=project_mcc_script_layout(n,names,{});require(converted.native_ready()&&Bytes(converted.nodes).u16(2)==0x28d,"Verified vibration call not converted to original rumble");for(unsigned i=0;i<n.size();++i)if(i!=2&&i!=3)require(n[i]==converted.nodes[i],"Alias changed salted identities or other AST bytes");require(n==old,"Alias modified donor AST");
 auto bad=n;bad[6]=std::byte{10};std::vector<std::byte>defs(0x296*40);std::copy(names.begin(),names.end(),defs.begin()+0x295*40);const auto stat=project_mcc_script_layout(bad,names,defs);require(stat.nodes==bad&&stat.static_calls==1,"Alias changed static script ordinal");
 bad=n;bad[48]=std::byte{0xff};bad[49]=std::byte{0xff};bad[50]=std::byte{0xff};bad[51]=std::byte{0xff};require(!project_mcc_script_layout(bad,names,{}).native_ready(),"Alias admitted wrong arity");
 SourceRuntimeMemory memory;effect_bank(memory,0x90008000);unsigned writes=0;const auto writer=[&](unsigned a,std::span<const std::byte>b){require(a==get(memory,0x50ebf8)+0x220&&b.size()==8,"Setter did not publish exact original two words");++writes;memory.write(a,b);};const auto setter=[&](float a,float b){source_script_max_rumble_tu15(memory,writer,a,b);};
 auto p=program(converted.nodes);std::string seen;ScriptVm vm(p,[&](const auto&callee,const auto&args){seen=callee;require(callee=="player_effect_set_max_rumble"&&args.size()==2,"Retained donor name bypassed projected dispatch");setter(std::bit_cast<float>(args[0].bits),std::bit_cast<float>(args[1].bits));return ScriptValue{};});vm.start("sample");const auto saved=vm.snapshot();vm.tick(0);require(vm.diagnostics().empty()&&writes==1&&get(memory,0x90008220)==0x3f800000&&get(memory,0x90008224)==0x3f800000,"C++ alias did not reach original effect state");
 SourceRuntimeMemory replacement;effect_bank(replacement,0x90009000);memory.swap_backing(replacement);vm.restore(saved);vm.tick(0);require(writes==2&&get(memory,0x90009220)==0x3f800000,"Restored setter retained old effect pointer");
 const auto cp=memory.checkpoint();setter(std::bit_cast<float>(0x80000000u),std::bit_cast<float>(0x7fc12345u));require(get(memory,0x90009220)==0x80000000&&get(memory,0x90009224)==0x7fc12345,"Setter changed exact REAL words");memory.restore(cp);vm.restore(saved);vm.tick(0);require(get(memory,0x90009220)==0x3f800000,"Checkpoint setter did not resolve fresh state");
 LevelContent content;content.player.datum=1;content.player.height=1;content.player.radius=.2f;content.player.vitality={100,100,2,.1f,0};content.collision={{{-100,-100,0},{100,-100,0},{100,100,0},0,0},{{-100,-100,0},{100,100,0},{-100,100,0},0,0}};content.scripts=p;Simulation simulation(content,0,Spawn{}, {},{},{},{},{},{},{},{},setter);simulation.start_level();simulation.step({});require(simulation.scripts().diagnostics().empty()&&get(memory,0x90009220)==0x3f800000,"Simulation typed setter not wired");
 bool failed=false;put(memory,0x50ebf8,0);try{setter(1,1);}catch(const FormatError&){failed=true;}require(failed,"Absent original effect owner synthesized");
}
void actual_fixture(const char*path){Cache c(path);const auto*t=c.find_tag(c.scenario_tag());const Bytes r(c.read_tag_bytes(t->address,0x240));const auto bank=[&](unsigned o,unsigned stride){return r.u32(o)?c.read_tag_bytes(r.u32(o+4),std::size_t(r.u32(o))*stride):std::span<const std::byte>{};};const auto raw=bank(0x238,20);const auto before=std::vector<std::byte>(raw.begin(),raw.end());const auto converted=project_mcc_script_layout(raw,bank(0x1b0,1),bank(0x1b8,40),{true});const unsigned at=0x216e*20;require(c.name()=="05b_deltatowers"&&Bytes(raw).u32(at)==0x029584e2&&Bytes(raw).u32(at+16)==0x84e3216f&&Bytes(converted.nodes).u16(at+2)==0x28d,"Actual DeltaTowers cam_shake fixture differs");for(unsigned i=0;i<20;++i)if(i!=2&&i!=3)require(raw[at+i]==converted.nodes[at+i],"Actual alias altered full salt or node fields");require(std::ranges::equal(raw,before),"Actual cache changed");std::cout<<"Actual DeltaTowers alias fixture passed\n";}
void native_test(){
 constexpr unsigned nodes=0x90004000,thread=0x90003000,frame=thread+0x40,stack=0x60008000,
     pool=0x90002000,syntax_pool=0x90002100,effect=0x90008000,output=0x90007000;
 for(const auto words:{std::array{0x3f800000u,0x3f800000u},std::array{0x80000000u,0x3e800000u},
     std::array{0x7fc12345u,0xff800000u}}){
  SourceRuntimeMemory m;
  for(const auto [a,n]:{std::pair(0x90000000u,0x10000u),std::pair(0x60000000u,0x10000u),
      std::pair(0x505e5cu,0x14u),std::pair(0x50ebf8u,4u),std::pair(0x481634u,4u),
      std::pair(0x459d58u,20u),std::pair(0x502248u,0x3c10u),std::pair(0x4f3218u,4u)})
    m.mount(a,std::vector<std::byte>(n),SourceRuntimeMemory::Access::ReadWrite,"explicit original HS invocation fixture");
  put(m,0x505e5c,pool);put(m,0x505e60,1);put(m,0x505e6c,syntax_pool);put(m,pool+0x44,thread);
  put(m,syntax_pool+0x20,4);put(m,syntax_pool+0x24,20);put(m,syntax_pool+0x44,nodes);
  put(m,0x50ebf8,effect);put(m,0x481634,0x459d58);
  const std::array descriptor{4u,0x2b4a40u,0u,0x00060002u,6u};
  for(unsigned i=0;i<descriptor.size();++i)put(m,0x459d58+i*4,descriptor[i]);
  auto raw=syntax();const auto projected=project_mcc_script_layout(raw,std::as_bytes(std::span(name)),{});
  m.write(nodes,projected.nodes);put(m,nodes+56,words[0]);put(m,nodes+76,words[1]);
  put(m,thread+2,1,1);put(m,thread+3,1,1);put(m,thread+0x10,frame);
  put(m,frame,thread+0x18);put(m,frame+4,0x90000000);put(m,thread+0x20,output);
  // Explicit caller scratch extents for original212010's argument allocator.
  put(m,thread+0xc,0x100,2);put(m,frame+0xc,0,2);put(m,stack,0xfeedcafe);
  SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x12345678,0x23456789,0x3456789a,0x456789ab,
      stack,0x56789abc,0x6789abcd,0xa0010000};abi.eflags=0x202;const auto before=abi;
  SourcePhysicsWorldRuntimeServices services;std::set<unsigned>seen;
  services.instruction=[&](unsigned pc){seen.insert(pc);};
  services.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &observe){
    require(call.target==0x212010,"Rumble evaluator required an unverified owner");
    source_run_ai_frame_tu15(bank,call.target,state,{0x37f,0x1f80},services,observe);
  };
  source_run_campaign_runtime_tu15(m,0x212470,abi,{0x37f,0x1f80},services);
  abi=before;
  std::vector<std::pair<unsigned,unsigned>> stores;
  source_run_campaign_runtime_tu15(m,0x211b10,abi,{0x37f,0x1f80},services,[&](const auto &store){
    if(store.address==effect+0x220||store.address==effect+0x224)
      stores.emplace_back(store.instruction,Bytes(store.written).u32(0));
  });
  require(stores==std::vector<std::pair<unsigned,unsigned>>{{0x2b4a75,words[0]},{0x2b4a7b,words[1]}} &&
      get(m,effect+0x220)==words[0]&&get(m,effect+0x224)==words[1],
      "Original evaluator's actual two stores differ");
  require(seen.contains(0x212010)&&seen.contains(0x211da0)&&seen.contains(0x2b4a40)&&
      get(m,thread+0x10)==thread+0x18&&get(m,output)==0&&abi.gp[4]==stack+4,
      "Original argument/completion path or RET12 did not run");
  for(unsigned r:{3u,5u,6u,7u})require(abi.gp[r]==before.gp[r],"Rumble wrapper changed nonvolatile state");
  source_script_max_rumble_tu15(m,[&](unsigned a,auto b){m.write(a,b);},
      std::bit_cast<float>(words[0]),std::bit_cast<float>(words[1]));
  require(get(m,effect+0x220)==stores[0].second&&get(m,effect+0x224)==stores[1].second,
      "C++ checked setter differs from original evaluator stores");
 }
}
}
int main(int argc,char**argv)try{cpp_test();native_test();if(argc>1)actual_fixture(argv[1]);std::cout<<"MCC rumble alias, original dispatch/completion, exact stores, checkpoint and Simulation tests passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
