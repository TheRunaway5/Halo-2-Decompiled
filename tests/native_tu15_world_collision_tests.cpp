#include "halo2/native_tu15_world_collision_listeners.hpp"
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
 constexpr std::uint32_t base=0x1000000,w=base+0x100,gravity=base+0x800,step=base+0x900,array=base+0xa00,alternate=base+0xb00,a=base+0xc00,b=base+0xd00,replacement=base+0xe00,profile=base+0x1000;
 Memory m;m.banks[base].resize(0x4000);m.banks[0x48c974].resize(8);m.put(w+0xcc,gravity);m.put(step,0x3d088889);m.put(step+4,0x41f00000);m.put(w+0x17c,4);m.put(w+0x180,0x3e800000);m.put(w+8,array);m.put(w+0xc,2);m.put(array,a);m.put(array+4,b);m.put(w+0x24,1);m.put(w+0xc4,0x11111111);m.put(0x48c974,profile);m.put(0x48c978,profile+0x100);
 NativeTu15PhysicsFrameFp fp;NativeTu15WorldCollisionServices s;NativeTu15WorldCollisionStats stats;std::vector<std::string>order;unsigned drains{},collides{};
 s.timestamp_counter=[](){return __rdtsc();};
 s.drain_operations=[&](auto world,auto&){check(world.identity==w,"Model operation world differs");order.push_back("operations");++drains;check(m.get(w+0x12c,1)==(drains==1?1u:0u),"Collision flag lifetime differs during deferred operations");m.put(w+0x24,0);if(drains==1)m.put(w+0xc4,0x22222222);};
 s.collide_island=[&](auto island,auto dispatcher,auto configuration,auto&){++collides;order.push_back("island");check(island.identity==a&&dispatcher.identity==0x22222222&&configuration.identity==w+0x140,"Collision dispatcher/configuration or forward island differs");check(m.get(w+0x12d,1)==1,"Island collision critical flag missing");m.put(w+8,alternate);m.put(alternate,replacement);m.put(w+0xac,1);};
 s.notify_island_listeners=[&](auto world,auto island,auto packet,auto&){order.push_back("island-listeners");check(world.identity==w&&island.identity==replacement&&packet.identity==step,"Island listener must reread actual array/member after collision");check(m.get(w+0x12d,1)==0,"Island listener critical flag must clear");m.put(w+0xc,1);m.put(w+0x24,1);m.put(w+0xf4,1);m.put(w+0x100,1);};
 s.drain_deletes=[&](auto,auto&){order.push_back("deletes");m.put(w+0xf4,0);m.put(w+0x24,1);};
 s.drain_updates=[&](auto,auto&){order.push_back("updates");check(m.get(w+0x24)==1,"Pending operations must survive through update callback");m.put(w+0x100,0);};
 s.notify_world_listeners=[&](auto world,auto packet,auto&){order.push_back("world-listeners");check(world.identity==w&&packet.identity==step&&m.get(w+0x12c,1)==0&&m.get(w+0x12d,1)==0,"World listener phase/arguments differ");};
 native_tu15_collide_physics_world(m,{w},{step},fp,0,s,{},&stats);
 check(order==std::vector<std::string>{"operations","island","island-listeners","operations","deletes","updates","operations","world-listeners"},"Mutable service continuation order differs");
 check(collides==1&&drains==3&&stats.worlds==1&&stats.islands==1&&stats.operation_drains==3&&stats.delete_drains==1&&stats.update_drains==1&&stats.island_notifications==1&&stats.world_notifications==1,"Native collision activity differs");
 check(m.get(0x48c974)==profile+60,"Profile record cursor differs");const std::array labels{0x41d9d4u,0x41d9bcu,0x41c394u,0x41d9acu,0x41cabcu};for(unsigned i=0;i<labels.size();++i)check(m.get(profile+i*12)==labels[i],"Actual timestamp profile label order differs");
 // These models exercise genuine caller mutations, not original callback
 // outcomes. The physical oracle separately checks all closed source paths.
 for(bool island_mode:{false,true}){
  Memory listener;listener.banks[base].resize(0x4000);const auto offset=island_mode?0xa8u:0x90u,count=island_mode?0xacu:0x94u;
  const auto cohort=base+0x2000,changed=base+0x2100,l0=base+0x2200,l1=base+0x2210,l2=base+0x2220,table=base+0x2300;
  listener.put(w+offset,cohort);listener.put(w+count,3);for(unsigned i=0;i<3;++i){listener.put(cohort+i*4,l0+i*0x10);listener.put(l0+i*0x10,table);listener.put(changed+i*4,l0+i*0x10);}listener.put(table+4,0x12345678);
  std::vector<std::uint32_t>called;auto callback=[&](auto receiver,auto owner,auto subject,auto packet,auto&){called.push_back(receiver.identity);check(owner.identity==0x12345678&&subject.identity==(island_mode?a:w)&&packet.identity==step,"Listener real owner/subject differs");if(receiver.identity==l2){listener.put(w+offset,changed);listener.put(changed,0);} };
  NativeTu15WorldCollisionListenerServices listeners;if(island_mode)listeners.notify_island=callback;else listeners.notify=callback;
  if(island_mode)native_tu15_notify_island_collision_listeners(listener,{w},{a},{step},fp,listeners);else native_tu15_notify_world_collision_listeners(listener,{w},{step},fp,listeners);
  check(called==std::vector<std::uint32_t>{l2,l1},"Mutable listener reverse cohort differs");check(listener.get(w+count)==2&&listener.get(changed)==l1&&listener.get(changed+4)==l2,"Mutable listener null compaction differs");
 }
 std::cout<<"Mutable collision scheduling, phase flags, real timestamp profiling and both listener-cohort continuation models passed\n";
}
void oracle(const std::filesystem::path&path){
 check(std::filesystem::exists(path),"Required original collision fixture missing");std::ifstream input(path);const auto f=J::parse(input);
 check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("required_backend")=="unchanged-tu15-world-collision-physical-operands-stores-full-fp-v1","Required collision original authority absent");check(f.at("body_sites")==J::array({157,46,49,1}),"Original collision/listener closure changed");
 std::size_t count{},writes{},read_count{},faults{},complete{},prefixes{};
 for(const auto&c:f.at("cases")){
  const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>(),name+" physical original not verified");Memory original;for(const auto&b:c.at("banks"))check(original.banks.emplace(b.at("address"),bytes(b.at("before"))).second,"Duplicate collision bank");original.fault=c.value("fault",J{});
  for(bool read_pass:{false,true}){
   auto m=original;m.log_reads=read_pass;const bool fault_case=!m.fault.is_null();m.log_writes=fault_case&&!read_pass;std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observer;if(!read_pass&&!fault_case)observer=[&](const auto&s){stores.push_back(s);};
   NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};NativeTu15WorldCollisionServices services;NativeTu15WorldCollisionListenerServices listeners;
   services.drain_operations=[](auto world,auto&){throw Stop{"operations",0x319750,{world.identity}};};services.drain_deletes=[](auto world,auto&){throw Stop{"deletes",0x319cd0,{world.identity}};};services.drain_updates=[](auto world,auto&){throw Stop{"updates",0x316e10,{world.identity}};};
   services.collide_island=[](auto island,auto dispatcher,auto configuration,auto&){throw Stop{"island",0x321e50,{island.identity,dispatcher.identity,configuration.identity}};};
   listeners.notify=[](auto listener,auto owner,auto world,auto step,auto&){throw Stop{"world_listener",owner.identity,{listener.identity,owner.identity,world.identity,step.identity}};};listeners.notify_island=[](auto listener,auto owner,auto island,auto step,auto&){throw Stop{"island_listener",owner.identity,{listener.identity,owner.identity,island.identity,step.identity}};};
   services.notify_world_listeners=[&](auto world,auto step,auto&state){native_tu15_notify_world_collision_listeners(m,world,step,state,listeners,observer);};services.notify_island_listeners=[&](auto world,auto island,auto step,auto&state){native_tu15_notify_island_collision_listeners(m,world,island,step,state,listeners,observer);};
   const NativeTu15PhysicsWorld world{c.at("world")};const NativeTu15PhysicsStepPacket step{c.at("step")};bool failed{},stopped{};Stop stop{};const auto cw=host_control(),sw=host_status();const auto mxcsr=_mm_getcsr();
   try{const auto entry=c.at("entry").get<std::uint32_t>();if(entry==0x319d50)native_tu15_collide_physics_world(m,world,step,fp,0,services,observer);else if(entry==0x3228c0)native_tu15_notify_world_collision_listeners(m,world,step,fp,listeners,observer);else if(entry==0x3229e0)native_tu15_notify_island_collision_listeners(m,world,{c.at("island")},step,fp,listeners,observer);else throw std::runtime_error("Unknown original collision root");}catch(const Fault&){failed=true;}catch(const Stop&s){stopped=true;stop=s;}
   check(failed==fault_case,name+" memory-fault prefix completion differs");if(!fault_case){const auto&boundary=c.at("boundary");check(stopped==!boundary.is_null(),name+" actual dependency boundary completion differs");if(stopped)check(stop.kind==boundary.at("kind").get<std::string>()&&stop.owner==boundary.at("owner")&&stop.arguments==boundary.at("arguments").get<std::vector<std::uint32_t>>(),name+" actual dependency boundary/arguments differ");}
   check(host_control()==cw&&host_status()==sw&&_mm_getcsr()==mxcsr,name+" leaked host FP context");
   if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" original collision read order differs at "+std::to_string(n)+" native "+(n<m.reads.size()?m.reads[n].dump():"end")+" original "+(n<expected.size()?expected[n].dump():"end"));}read_count+=m.reads.size();}
   else{if(fault_case)stores=m.raw_stores;const auto&w=c.at("writes");check(stores.size()==w.size(),name+" ordered collision store count differs "+std::to_string(stores.size())+"/"+std::to_string(w.size()));for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=w[n];check((fault_case||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" ordered collision store differs at "+std::to_string(n));}writes+=stores.size();}
   for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" retained collision bytes differ");const auto&end=c.at("after_fp");check(fp.control_word==end[0]&&fp.status==end[1]&&fp.mxcsr==end[2],name+" complete collision FP differs "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+"/"+end.dump());
  }++count;faults+=!c.value("fault",J{}).is_null();if(c.value("fault",J{}).is_null()){complete+=c.at("boundary").is_null();prefixes+=!c.at("boundary").is_null();}
 }check(count>0,"Empty physical original collision fixture");std::cout<<count<<" physical original collision/listener cases, "<<complete<<" complete, "<<prefixes<<" actual owner stops, "<<writes<<" ordered stores, "<<read_count<<" reads and "<<faults<<" memory-fault prefixes passed\n";
}
}
int main(int argc,char**argv){try{if(argc==3&&std::string(argv[1])=="--oracle-only")oracle(argv[2]);else{models();if(argc==2)oracle(argv[1]);else check(argc==1,"Supply optional required original collision fixture");}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
