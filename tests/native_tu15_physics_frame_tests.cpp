#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>
#include <xmmintrin.h>
namespace {
using namespace halo2;
using J=nlohmann::json;
void check(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
std::vector<std::byte> bytes(const std::string&s){
  check(s.size()%2==0,"Invalid byte string");std::vector<std::byte> out;
  for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;
}
std::string hex(std::span<const std::byte>b){
  const char*d="0123456789abcdef";std::string s;for(auto b:b){auto n=std::to_integer<unsigned>(b);s+=d[n>>4];s+=d[n&15];}return s;
}
struct Memory:SourcePhysicsContactMemory {
  std::map<std::uint32_t,std::vector<std::byte>> banks;
  std::function<void(std::uint32_t,std::size_t)> before_read;
  std::size_t write_serial{};
  auto bank(std::uint32_t p)const {
    auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing physics bank");--i;
    check(std::uint64_t(p-i->first)<i->second.size(),"Missing physics bank extent");return i;
  }
  void read(std::uint32_t p,std::span<std::byte>b)const override{if(before_read)before_read(p,b.size());for(auto&v:b){auto i=bank(p);v=i->second[p++-i->first];}}
  void write(std::uint32_t p,std::span<const std::byte>b)override{for(auto v:b){auto i=bank(p);banks.at(i->first)[p++-i->first]=v;}++write_serial;}
  void put(std::uint32_t p,std::uint32_t n,unsigned size=4){std::array<std::byte,4>b{};for(unsigned i=0;i<size;++i)b[i]=std::byte(n>>(i*8u));write(p,std::span(b).first(size));}
  std::uint32_t get(std::uint32_t p,unsigned size=4)const{std::array<std::byte,4>b{};read(p,std::span(b).first(size));std::uint32_t n{};for(unsigned i=0;i<size;++i)n|=std::to_integer<unsigned>(b[i])<<(i*8u);return n;}
};
constexpr std::uint32_t world=0x1000000,manager=world+0x100,islands=world+0x200,island=world+0x300,
  bodies=world+0x400,body=world+0x500,properties=world+0x600,pool=world+0x700,components=world+0x800,
  queue=world+0x900,clock=world+0xa00;
Memory model(){
  Memory m;m.banks[0x400000].resize(0x160000);m.banks[world].resize(0x4000);m.banks[0x80061000].resize(0x1000);
  m.put(0x52be48,world);m.put(0x52d880,manager);m.put(0x51df54,clock);m.put(clock+4,0x3d088889);
  m.put(0x46a0d0,0x3f800000);m.put(manager+0x14,0xffffffff);m.put(manager+8,queue);
  m.put(0x52be5c,pool);m.put(pool+0x44,components);m.put(world+8,islands);m.put(islands,island);
  m.put(island+0x3c,bodies);m.put(bodies,body);m.put(body+0x30,properties);m.put(body+0x34,1);
  m.put(properties,0x2001);m.put(properties+4,0x12340001);m.put(components+0xa0+4,0x20);
  m.put(components+0xa0+8,0x12345678);return m;
}
struct Stop {std::string owner;std::uint32_t identity{};NativeTu15PhysicsStep step;};
void models(){
  { // Null world must neither touch other data nor require FP/backend services.
    Memory m;m.banks[0x52be48].resize(4);NativeTu15PhysicsFrameMemory storage(m);NativeTu15PhysicsFrameFp fp{0,0xffff,0xffffffff};
    native_tu15_step_physics_frame(storage,fp,{});check(fp.control_word==0&&fp.status==0xffff&&fp.mxcsr==0xffffffff,"Null world changed FP state");
  }
  { // Callback order, FP changes, live manager reload and post-destructor query.
    auto m=model();m.put(0x48b85c,0x42);m.put(world+0xc,1);m.put(island+0x40,1);m.put(manager+0xc,1);
    m.put(queue,body);m.put(body+6,1,2);m.put(body+0x44,island);m.put(island+0x29,1,1);m.put(0x52d888,0xffff,2);
    const auto other=manager+0xb00;m.put(other+8,queue+0x40);m.put(other+0xc,1);m.put(other+0x14,0xffffffff);
    m.put(queue+0x40,body+0x1000);m.put(body+0x1000+6,2,2);
    NativeTu15PhysicsFrameMemory storage(m);NativeTu15PhysicsFrameFp fp{0x0f7f,0x7f3f,0x5fbf};
    std::vector<std::string> events;unsigned publications=0,drains=0;NativeTu15PhysicsFrameServices s;
    s.resize_debug_arena=[&](auto n,auto&){check(n==2000000,"Debug arena size");events.push_back("resize");};
    s.reset_debug_arena=[&](auto&){events.push_back("reset");};
    s.drain_commands=[&](auto&){events.push_back("drain");++drains;};
    s.step_world=[&](auto w,const auto&t,auto&f){check(w.identity==world&&t.seconds_bits==0x3d088889,"World step input");check((f.control_word&0xf3f)==0xf3f,"Pre-step control policy");events.push_back("world");f.mxcsr|=0x3f;f.status=0xffff;};
    s.post_step=[&](auto&f){check((f.control_word&0xf3f)==0x23f&&f.status==0x7f00&&(f.mxcsr&0x3f)==0,"Post-step FP reset");events.push_back("post");};
    s.update_contacts=[&](auto&){events.push_back("contacts");};
    s.publish_debug_arena=[&](auto a,auto&){check(a==0x42,"Debug owner");events.push_back("debug");};
    s.publish_object=[&](auto o,auto&){check(o.datum==0x12345678,"Publication handle");++publications;events.push_back("publish");};
    s.destroy_body=[&](auto b,bool deleting,auto&){check(b.identity==body&&deleting&&m.get(body+6,2)==0,"Destruction reference phase");events.push_back("destroy");m.put(island+0x29,0,1);m.put(0x52d880,other);};
    const auto host_mxcsr=_mm_getcsr();native_tu15_step_physics_frame(storage,fp,s);check(_mm_getcsr()==host_mxcsr,"Leaked host MXCSR");
    check(events==std::vector<std::string>{"resize","reset","drain","world","post","contacts","drain","debug","publish","destroy","publish"},"Frame phase order");
    check(publications==2&&drains==2&&m.get(0x52d888,2)==0xffff,"Iteration depth/wrap");
    check(m.get(other+0xc)==0&&m.get(other+0x14)==0xffffffff&&m.get(body+0x1000+6,2)==1,"Manager reload and final drain");
    check(m.get(manager+0xc)==1&&m.get(manager+0x18,1)==0,"Captured manager semantics");
  }
  { // Missing residual owner stops after real preceding stores, never succeeds.
    auto m=model();m.put(0x46e468,0x91,1);NativeTu15PhysicsFrameMemory storage(m);NativeTu15PhysicsFrameFp fp;
    bool failed=false;try{native_tu15_step_physics_frame(storage,fp,{});}catch(const std::runtime_error&e){failed=std::string(e.what()).find("command drain")!=std::string::npos;}
    check(failed&&m.get(0x46e468,1)==0&&m.get(manager+0x18,1)==1,"Missing owner prefix");
  }
  { // Protected references restore one; zero underflows to65535 without delete.
    auto m=model();m.put(manager+0xc,2);m.put(queue,0x80061000);m.put(queue+4,body);m.put(0x80061006,1,2);m.put(body+6,0,2);
    m.put(0x556588,0,1);NativeTu15PhysicsFrameMemory storage(m);NativeTu15PhysicsFrameFp fp;
    native_tu15_retire_physics_references(storage,{manager},fp,{});
    check(m.get(0x80061006,2)==1&&m.get(body+6,2)==0xffff&&m.get(manager+0xc)==0,"Reference edge cases");
  }
  { // Original1CD5D0 reads keys first; only1CD60E reads a matched value.
    auto m=model();m.put(body+0x34,2);m.put(properties,0x1000);m.put(properties+8,0x2001);m.put(properties+12,1);
    unsigned property_arrays{},component_arrays{};
    m.before_read=[&](auto p,auto){
      check(p!=properties+4,"Fetched unused property value");
      if(p==body+0x30)++property_arrays;
      if(p==pool+0x44)++component_arrays;
      if(p==properties)m.put(body+0x30,0xdead0000); // Source caches the actual array.
    };
    NativeTu15PhysicsFrameMemory storage(m);NativeTu15PhysicsFrameFp fp;bool published{};NativeTu15PhysicsFrameServices s;
    s.publish_object=[&](auto o,auto&){check(o.datum==0x12345678,"Cached component publication");published=true;};
    native_tu15_publish_physics_body(storage,{body},fp,s);
    check(published&&property_arrays==1&&component_arrays==1,"Publication source access phases");
  }
  { // Queue pointer failure precedes the source cursor store282027.
    auto m=model();m.put(manager+0xc,1);m.put(queue,body);m.put(body+6,2,2);
    m.before_read=[&](auto p,auto){if(p==manager+8)throw std::runtime_error("actual queue read fault");};
    NativeTu15PhysicsFrameMemory storage(m);NativeTu15PhysicsFrameFp fp;bool failed{};
    try{native_tu15_retire_physics_references(storage,{manager},fp,{});}catch(const std::runtime_error&e){failed=std::string(e.what())=="actual queue read fault";}
    m.before_read={};check(failed&&m.get(manager+0x14)==0xffffffff&&m.get(body+6,2)==2,"Queue read fault crossed cursor store");
  }
  { // The source re-reads the live reference word after its observed store.
    auto m=model();m.put(manager+0xc,1);m.put(queue,body);m.put(body+6,1,2);
    NativeTu15PhysicsFrameMemory storage(m,[&](const auto&s){if(s.instruction==0x28202f)m.put(body+6,2,2);});
    NativeTu15PhysicsFrameFp fp;native_tu15_retire_physics_references(storage,{manager},fp,{});
    check(m.get(body+6,2)==2&&m.get(manager+0xc)==0,"Reference decision ignored its source re-read");
  }
  { // The array identity is captured before a cursor-store observer runs.
    auto m=model();m.put(manager+0xc,1);m.put(queue,body);m.put(body+6,2,2);
    m.put(queue+0x20,body+0x1000);m.put(body+0x1000+6,2,2);
    NativeTu15PhysicsFrameMemory storage(m,[&](const auto&s){if(s.instruction==0x282027)m.put(manager+8,queue+0x20);});
    NativeTu15PhysicsFrameFp fp;native_tu15_retire_physics_references(storage,{manager},fp,{});
    check(m.get(body+6,2)==1&&m.get(body+0x1000+6,2)==2,"Queue array was reloaded after source cursor publication");
  }
  std::cout<<"native frame model regressions passed; residual closure and multiplayer qualification remain open\n";
}
void residuals(){
  auto m=model();constexpr std::uint32_t stack=0x60000000,top=stack+0x4000;
  m.banks[stack].resize(0x5000);m.put(top,0x60300000);m.put(0x48b85c,0x77);
  m.put(world+0xc,1);m.put(island+0x40,1);m.put(manager+0xc,1);m.put(queue,body);m.put(body+6,1,2);
  m.put(body+0x44,island);m.put(island+0x29,1,1);const auto vtable=body+0x1800,destructor=0x282110u;
  m.put(body,vtable);m.put(vtable,destructor);
  SourcePhysicsContactRuntimeAbi a{};a.gp[4]=top;a.fs_base=0x60800000;a.x87_depth=2;
  a.x87[0]=1.25L;a.x87[1]=-3.5L;a.x87_status=0x2800;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
  NativeTu15PhysicsFrameFp fp{0x27f,0x2800,0x1f80};NativeTu15PhysicsFrameExecutionStats stats;
  std::vector<std::uint32_t> calls;std::uint32_t saved_step{};unsigned publications{};
  native_tu15_step_physics_frame_with_residuals(m,fp,a,{stack,0x4000},[&](auto entry,auto&actual,const auto&){
    check(actual.fs_base==0x60800000&&actual.x87_depth==2&&actual.x87[1]==-3.5L,"Residual live context lost");
    check(actual.x87[0]==(calls.empty()?1.25L:2.5L),"Residual live x87 return did not survive");
    check(m.get(actual.gp[4])==0x60300000&&actual.gp[4]<top,"Residual actual continuation/frame");
    check(actual.floating_point.has_value(),"Residual FP absent");calls.push_back(entry);actual.x87[0]=2.5L;
    unsigned pop=0;
    if(entry==0x2e2990)check(m.get(actual.gp[4]+4)==2000000,"Residual debug size ABI");
    else if(entry==0x2e2b30)check(actual.gp[1]==0x77,"Residual debug owner ABI");
    else if(entry==0x31b1a0){
      check(actual.gp[1]==world,"Residual world receiver ABI");saved_step=m.get(actual.gp[4]+4);
      check(saved_step>actual.gp[4]+8&&m.get(saved_step)==0x3d088889&&m.get(saved_step+4)==0x41efffff,"Residual real step packet ABI");
      actual.floating_point->mxcsr|=0x3f;actual.x87_status=0xabff;pop=4;
    }else if(entry==0x282c10){check(actual.x87_status==0x2b00&&(actual.floating_point->mxcsr&0x3f)==0&&actual.floating_point->x87_control_word==0x27f,"Residual source FP clear phase");}
    else if(entry==0xbda50){check(actual.gp[0]==0x12345678,"Residual publication EAX ABI");++publications;}
    else if(entry==destructor){check(actual.gp[1]==body&&m.get(actual.gp[4]+4)==1&&m.get(body+6,2)==0,"Residual actual destructor ABI");m.put(island+0x29,0,1);pop=4;}
    check(!saved_step||m.get(saved_step)==0x3d088889,"Residual retained step lifetime");
    actual.gp[4]+=4+pop;
  },{},&stats);
  check(calls==std::vector<std::uint32_t>{0x2e2990,0x2e2910,0x1496f0,0x31b1a0,0x282c10,0x1cce00,0x1496f0,0x2e2b30,0xbda50,destructor,0xbda50},"Residual complete dependency order");
  check(stats.frame_calls==1&&stats.residual_calls==calls.size()&&publications==2,"Residual native/mixed counters");
  check(a.x87_depth==2&&a.x87[0]==2.5L&&a.x87_status==fp.status&&a.floating_point->mxcsr==fp.mxcsr,"Residual final live FP result");
  // A throwing real residual keeps its mutated FP and all preceding stores.
  m=model();m.banks[stack].resize(0x5000);m.put(top,0x60300000);a.gp[4]=top;fp={};bool failed=false;
  try{native_tu15_step_physics_frame_with_residuals(m,fp,a,{stack,0x4000},[&](auto entry,auto&actual,const auto&){
    check(entry==0x1496f0,"Residual failure boundary");actual.floating_point->mxcsr|=0x20;actual.x87_status=0x2801;throw std::runtime_error("actual missing owner");});}
  catch(const std::runtime_error&e){failed=std::string(e.what())=="actual missing owner";}
  check(failed&&fp.mxcsr==0x1fa0&&fp.status==0x2801&&m.get(manager+0x18,1)==1,"Residual failure lost real prefix/FP");
  std::cout<<"native frame residual ABI/lifetime/FP model regressions passed; adapters still depend on residual modules\n";
}
void oracle(const std::filesystem::path&path){
  check(std::filesystem::exists(path),"Required original fixture missing");std::ifstream input(path);const auto fixture=J::parse(input);
  check(fixture.at("image")=="English TU1.5"&&fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Original fixture authority mismatch");
  check(fixture.at("root_instruction_sites")==137,"Original root CFG changed");std::size_t cases=0,total_stores=0;
  check(fixture.at("required_backend")=="unchanged-tu15-unicorn-writes+physical-i386-banks-boundary-fp-v1","Required independent original backend missing");
  for(const auto&c:fixture.at("cases")){
    const auto name=c.at("name").get<std::string>();Memory m;
    check(c.at("physical_verified").get<bool>()&&c.at("physical_original_instructions").get<unsigned>()>0,"Original physical reference not verified");
    for(const auto&b:c.at("banks"))check(m.banks.emplace(b.at("address"),bytes(b.at("before"))).second,"Duplicate fixture bank");
    std::vector<SourcePhysicsContactRecordStore> stores;NativeTu15PhysicsFrameMemory storage(m,[&](const auto&s){stores.push_back(s);});
    NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};NativeTu15PhysicsFrameServices s;
    // This exact recursion-guarded1496F0 path returns without stores or FP work.
    // The oracle executes its unchanged original instructions. No other helper
    // result is replayed or supplied by this declared prefix comparison.
    if(c.value("guarded_command_drain",false))s.drain_commands=[&](auto&){check(m.get(0x48b873,1)!=0,"Command drain guard missing");};
    s.step_world=[](auto w,const auto&t,auto&){throw Stop{"world",w.identity,t};};
    s.publish_object=[](auto o,auto&){throw Stop{"publication",o.datum,{}};};
    s.destroy_body=[](auto b,bool,auto&){throw Stop{"destruction",b.identity,{}};};
    Stop stop{};bool stopped=false;
    try{if(c.at("mode")=="tail")native_tu15_finish_physics_frame(storage,fp,s);else native_tu15_step_physics_frame(storage,fp,s);}
    catch(const Stop&e){stopped=true;stop=e;}
    check(stopped==!c.at("boundary").is_null(),name+" completion differs");
    if(stopped){const auto&b=c.at("boundary");check(stop.owner==b.at("owner").get<std::string>()&&stop.identity==b.at("identity"),name+" dependency boundary differs");if(stop.owner=="world")check(stop.step.seconds_bits==b.at("step")[0]&&stop.step.inverse_seconds_bits==b.at("step")[1],name+" world step words differ");}
    const auto&expected=c.at("writes");check(stores.size()==expected.size(),name+" persistent store count differs "+std::to_string(stores.size())+"/"+std::to_string(expected.size()));
    for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=expected[n];check(a.instruction==b.at("instruction")&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" persistent store differs at "+std::to_string(n));}
    for(const auto&b:c.at("banks")){const auto p=b.at("address").get<std::uint32_t>();check(hex(m.banks.at(p))==b.at("after").get<std::string>(),name+" retained bytes differ");}
    const auto&expected_fp=c.at("after_fp");
    check(fp.control_word==expected_fp[0]&&fp.status==expected_fp[1]&&fp.mxcsr==expected_fp[2],name+" complete FP context differs native "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original "+expected_fp.dump());
    // Observe source data accesses without a store collector's diagnostic
    // before-byte reads. Native kernels must not eagerly fetch unused fields
    // or move array reads across observable stores.
    Memory read_memory;
    for(const auto&b:c.at("banks"))read_memory.banks.emplace(b.at("address"),bytes(b.at("before")));
    J native_reads=J::array();
    read_memory.before_read=[&](auto p,auto n){native_reads.push_back({{"address",p},{"size",n},{"stores_before",read_memory.write_serial}});};
    NativeTu15PhysicsFrameMemory read_storage(read_memory);
    NativeTu15PhysicsFrameFp read_fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
    auto read_services=s;
    if(c.value("guarded_command_drain",false))read_services.drain_commands=[&](auto&){check(read_memory.get(0x48b873,1)!=0,"Command drain guard missing");};
    try{if(c.at("mode")=="tail")native_tu15_finish_physics_frame(read_storage,read_fp,read_services);else native_tu15_step_physics_frame(read_storage,read_fp,read_services);}catch(const Stop&){}
    const auto&original_reads=c.at("reads");
    check(native_reads.size()==original_reads.size(),name+" source read count differs "+std::to_string(native_reads.size())+"/"+std::to_string(original_reads.size()));
    for(std::size_t n=0;n<native_reads.size();++n)
      check(native_reads[n].at("address")==original_reads[n].at("address")&&native_reads[n].at("size")==original_reads[n].at("size")&&native_reads[n].at("stores_before")==original_reads[n].at("stores_before"),
          name+" source read differs at "+std::to_string(n)+" native "+native_reads[n].dump()+" original "+original_reads[n].dump());
    ++cases;total_stores+=stores.size();
  }
  check(cases>0,"Empty original fixture");std::cout<<cases<<" independent TU1.5 declared prefix/tail cases, "<<total_stores<<" ordered persistent stores passed; full residual closure remains unqualified\n";
}
}
int main(int argc,char**argv){try{if(argc==3&&std::string(argv[1])=="--oracle-only")oracle(argv[2]);else{models();residuals();if(argc==3&&std::string(argv[1])=="--oracle-fixture")oracle(argv[2]);else check(argc==1,"Expected optional --oracle-fixture PATH");}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
