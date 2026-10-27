#include "halo2/native_tu15_island_collision.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
#include <x86intrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
std::vector<std::byte>bytes(const std::string&s){check(s.size()%2==0,"Invalid collision fixture bytes");std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
struct Fault{};struct Stop{std::string kind;std::uint32_t owner;std::vector<std::uint32_t>arguments;};
struct Memory:SourcePhysicsContactMemory{
 std::map<std::uint32_t,std::vector<std::byte>>banks;bool log_reads{},log_writes{};mutable J reads=J::array();J fault;mutable unsigned fault_count{};std::vector<SourcePhysicsContactRecordStore>raw_stores;
 auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing collision bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing collision extent");return i;}
 void read(std::uint32_t p,std::span<std::byte>b)const override{if(!fault.is_null()&&fault.value("access","read")=="read"&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};if(log_reads)reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
 void write(std::uint32_t p,std::span<const std::byte>b)override{if(!fault.is_null()&&fault.value("access","read")=="write"&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};auto i=at(p,b.size());if(log_writes)raw_stores.push_back({0,p,std::vector<std::byte>(i->second.begin()+p-i->first,i->second.begin()+p-i->first+b.size()),std::vector<std::byte>(b.begin(),b.end())});std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
 void put(std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8u*i));write(p,b);}
 std::uint32_t get(std::uint32_t p,unsigned n=4)const{std::array<std::byte,4>b{};read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(b[i])<<(8u*i);return v;}
};
std::uint16_t host_status(){std::uint16_t n;asm volatile("fnstsw %0":"=a"(n));return n;}
std::uint16_t host_control(){std::uint16_t n;asm volatile("fnstcw %0":"=m"(n));return n;}
void models(){
 constexpr std::uint32_t base=0x1000000,island=base+0x100,old_world=base+0x300,new_world=base+0x700,dispatcher=base+0x800,configuration=base+0x900,heap_a=base+0x1000,heap_b=base+0x1100,table_a=base+0x2000,table_b=base+0x2100;
 for(bool flag_set:{false,true}){
  Memory m;m.banks[base].resize(0x4000);m.banks[0x486000].resize(0x1000);m.banks[0x56df00].resize(0x100);m.put(0x486070,heap_a);m.put(heap_a+0x18,table_a);m.put(heap_a+0x1c,8);m.put(heap_a+0x20,1);m.put(table_a+4,5);m.put(heap_b+0x18,table_b);m.put(heap_b+0x1c,8);m.put(heap_b+0x20,1);m.put(table_b+4,11);m.put(island+0x20,old_world);m.put(old_world+0x23e,flag_set?0:1);m.put(new_world+0x23e,flag_set?0x80:0);
  NativeTu15IslandCollisionServices services;std::vector<std::string>order;NativeTu15PhysicsFrameFp fp;
  services.first_phase=[&](auto subject,auto dispatch,auto config,auto&){order.push_back("first");check(subject.identity==island&&dispatch.identity==dispatcher&&config.identity==configuration,"First island phase actual arguments differ");check(m.get(0x56dfd8)==island&&m.get(0x56dfd4)==5,"Monitor must publish subject and actual baseline before first phase");m.put(island+0x20,new_world);m.put(0x486070,heap_b);};
  auto phase=[&](auto subject,auto config,auto&state){order.push_back(flag_set?"set":"clear");check(subject.identity==island&&config.identity==configuration&&m.get(0x56dfd8)==island,"Selected island phase retained arguments/lifetime differ");m.put(table_b+4,17);state.mxcsr|=1u;};
  if(flag_set)services.flag_set_phase=phase;else services.flag_clear_phase=phase;
  native_tu15_collide_physics_island(m,{island},{dispatcher},{configuration},fp,services);
  check(order==std::vector<std::string>{"first",flag_set?"set":"clear"},"Island must reread actual world/flag23E after first phase");check(m.get(0x56dfd8)==0&&m.get(island+0xae,1)==1&&m.get(0x56dfcc,1)==1,"Monitor must clear subject and publish actual changed allocation flags");check(fp.mxcsr&1u,"Actual phase numeric state must propagate");
 }
 // Missing required owners preserve the preceding source publications.
 Memory m;m.banks[base].resize(0x4000);m.banks[0x486000].resize(0x1000);m.banks[0x56df00].resize(0x100);NativeTu15PhysicsFrameFp fp;bool stopped{};
 try{native_tu15_collide_physics_island(m,{island},{dispatcher},{configuration},fp,{});}catch(const std::runtime_error&){stopped=true;}
 check(stopped&&m.get(0x56dfd8)==island&&m.get(0x56dfd4)==0,"Required phase stop must not invent monitor cleanup");
 std::cout<<"Actual allocator/world rereads, neutral flag23E branch, monitor lifetime and required-owner continuation models passed\n";
}
void oracle(const std::filesystem::path&path){
 check(std::filesystem::exists(path),"Required original island collision fixture missing");std::ifstream input(path);const auto f=J::parse(input);
 check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("required_backend")=="unchanged-tu15-island-collision-physical-operands-stores-return-full-fp-v1","Required original island collision authority absent");check(f.at("body_sites")==J::array({32,5,9,36}),"Original island collision/monitor roots changed");
 std::size_t count{},writes{},read_count{},faults{},complete{},prefixes{},totals{};
 for(const auto&c:f.at("cases")){
  const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>(),name+" physical original not verified");Memory original;for(const auto&b:c.at("banks"))check(original.banks.emplace(b.at("address"),bytes(b.at("before"))).second,"Duplicate island collision bank");original.fault=c.value("fault",J{});
  for(bool read_pass:{false,true}){
   auto m=original;m.log_reads=read_pass;const bool fault_case=!m.fault.is_null();m.log_writes=fault_case&&!read_pass;std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observer;if(!read_pass&&!fault_case)observer=[&](const auto&s){stores.push_back(s);};
   NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};NativeTu15IslandCollisionServices services;
   services.first_phase=[](auto island,auto dispatcher,auto configuration,auto&){throw Stop{"first",0x321700,{island.identity,dispatcher.identity,configuration.identity}};};
   services.flag_set_phase=[](auto island,auto configuration,auto&){throw Stop{"set",0x321a70,{island.identity,configuration.identity}};};services.flag_clear_phase=[](auto island,auto configuration,auto&){throw Stop{"clear",0x321d20,{island.identity,configuration.identity}};};
   bool failed{},stopped{};Stop stop{};std::uint32_t total{};const auto cw=host_control(),sw=host_status();const auto mxcsr=_mm_getcsr();const auto entry=c.at("entry").get<std::uint32_t>();
   try{if(entry==0x149b90)total=native_tu15_island_allocated_bytes(m);else if(entry==0x282b70)native_tu15_begin_island_collision_monitor(m,{c.at("island")},observer);else if(entry==0x282b90)native_tu15_end_island_collision_monitor(m,{c.at("island")},observer);else if(entry==0x321e50)native_tu15_collide_physics_island(m,{c.at("island")},{c.at("dispatcher")},{c.at("configuration")},fp,services,observer);else throw std::runtime_error("Unknown original island collision root");}catch(const Fault&){failed=true;}catch(const Stop&s){stopped=true;stop=s;}
   check(failed==fault_case,name+" memory-fault prefix completion differs");if(!fault_case){const auto&boundary=c.at("boundary");check(stopped==!boundary.is_null(),name+" actual owner boundary completion differs");if(stopped)check(stop.owner==boundary.at("owner")&&stop.arguments==boundary.at("arguments").get<std::vector<std::uint32_t>>(),name+" actual owner arguments differ");if(entry==0x149b90)check(total==c.at("return_gp")[0],name+" actual allocation total differs");}
   check(host_control()==cw&&host_status()==sw&&_mm_getcsr()==mxcsr,name+" leaked host FP context");
   if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" original island collision read order differs at "+std::to_string(n)+" native "+(n<m.reads.size()?m.reads[n].dump():"end")+" original "+(n<expected.size()?expected[n].dump():"end"));}read_count+=m.reads.size();}
   else{if(fault_case)stores=m.raw_stores;const auto&w=c.at("writes");check(stores.size()==w.size(),name+" island collision store count differs");for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=w[n];check((fault_case||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" island collision store differs at "+std::to_string(n));}writes+=stores.size();}
   for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" retained island collision bytes differ");const auto&end=c.at("after_fp");check(fp.control_word==end[0]&&fp.status==end[1]&&fp.mxcsr==end[2],name+" complete island collision FP differs");
  }++count;faults+=!c.value("fault",J{}).is_null();if(c.value("fault",J{}).is_null()){complete+=c.at("boundary").is_null();prefixes+=!c.at("boundary").is_null();totals+=c.at("entry")==0x149b90;}
 }check(count>0,"Empty original island collision fixture");std::cout<<count<<" original island collision/monitor cases, "<<complete<<" complete, "<<prefixes<<" actual owner stops, "<<totals<<" exact returned allocation totals, "<<writes<<" stores, "<<read_count<<" reads and "<<faults<<" fault prefixes passed\n";
}
}
int main(int argc,char**argv){try{if(argc==3&&std::string(argv[1])=="--oracle-only")oracle(argv[2]);else{models();if(argc==2)oracle(argv[1]);else check(argc==1,"Supply optional original island collision fixture");}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
