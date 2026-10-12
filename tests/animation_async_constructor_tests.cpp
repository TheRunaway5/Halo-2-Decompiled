#include "halo2/animation_async_constructor.hpp"
#include "halo2/native_synchronization.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace halo2;
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
std::uint32_t hex(const std::string &s){return std::stoul(s,nullptr,16);}
std::vector<std::byte> bytes(const std::string &s){
  check(s.size()%2==0,"Odd fixture hex extent");std::vector<std::byte>b;
  for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(hex(s.substr(i,2))));
  return b;
}
struct Memory {
  std::map<std::uint32_t,std::vector<std::byte>> spans;
  std::span<std::byte> at(std::uint32_t p,std::size_t n){
    for(auto &[base,b]:spans)if(p>=base && std::uint64_t(p)-base+n<=b.size())
      return std::span(b).subspan(p-base,n);
    throw std::runtime_error("Missing bounded original async backing");
  }
  SourceAnimationResourceMemory services(){return{
    [this](std::uint32_t p,std::size_t n)->std::span<const std::byte>{return at(p,n);},
    [this](std::uint32_t p,std::span<const std::byte>b){auto out=at(p,b.size());std::copy(b.begin(),b.end(),out.begin());}};}
  std::uint32_t word(std::uint32_t p){auto b=at(p,4);std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[i]))<<(i*8);return v;}
};
struct Event {std::uint32_t pc,address,before,after;bool operator==(const Event&)const=default;};
struct Case {std::string name;Memory before,after;std::vector<Event>stores;};
void prefix(const Case &c,const SourceAnimationAsyncBindings &bindings){
  Memory m=c.before;SourceAnimationAsyncConstructionServices s{m.services(),bindings,{},{},{},{}};
  std::vector<Event>stores;bool stopped=false;
  try{construct_source_animation_async_queue(s,[&](const AnimationResourceStore&e){stores.push_back({e.instruction,e.address,e.before,e.after});});}
  catch(const std::runtime_error&e){stopped=std::string(e.what())=="Native XAPI mutex creation owner unavailable";}
  check(stopped,"Missing first real creation owner did not stop");
  check(stores==c.stores,"Complete original prefix store order/value differs");
  check(m.spans==c.after.spans,"Complete original prefix raw buffer differs");
}
void native_composition(const Case &c,const SourceAnimationAsyncBindings &b){
  Memory m=c.before;const auto initial_worker=m.word(b.worker_slot);
  // Genuinely owned raw native backing for the source-requested storage token;
  // this is not a recovered Xbox buffer initialization or a worker admission.
  m.spans.emplace(b.worker_storage,std::vector<std::byte>(0x8000));
  NativeSynchronization native({0x74000000u,0x74000100ull});auto platform=native.services();
  std::vector<NativeSynchronizationCreation>created;std::vector<Event>stores;
  SourceAnimationAsyncConstructionServices s{m.services(),b,{},{},{},{}};
  s.native_create_mutex=[&](std::uint32_t security,std::uint32_t owner,std::uint32_t name){
    check(security==0 && owner==0 && name==0,"Unexpected actual mutex request");
    auto r=platform.create_mutex(false);created.push_back(r);return r.handle;};
  s.native_create_semaphore=[&](std::uint32_t security,std::uint32_t initial,std::uint32_t maximum,std::uint32_t name){
    check(security==0 && initial==0 && maximum==150 && name==0,"Unexpected actual semaphore request");
    auto r=platform.create_semaphore(initial,maximum);created.push_back(r);return r.handle;};
  bool stopped=false;
  try{construct_source_animation_async_queue(s,[&](const AnimationResourceStore&e){stores.push_back({e.instruction,e.address,e.before,e.after});});}
  catch(const std::runtime_error&e){stopped=std::string(e.what())=="Native worker and source callback/TLS owner unavailable";}
  check(stopped && created.size()==3 && native.object_count()==3,"Actual native creation/worker stop differs");
  check(std::all_of(created.begin(),created.end(),[](const auto&r){return r.handle && !r.error;}),"Actual native synchronization creation failed");
  check(stores.size()==c.stores.size()+6 && std::equal(c.stores.begin(),c.stores.end(),stores.begin()),"Native provider changed source prefix");
  const std::array<std::uint32_t,6> pcs{0x122209,0x12221e,0x12223a,0x12223f,0x122249,0x122253};
  const std::array<std::uint32_t,6> addresses{b.free_mutex_slot,b.work_mutex_slot,b.semaphore_slot,b.worker_storage_descriptor,b.worker_storage_descriptor+4,b.worker_storage_descriptor+8};
  const std::array<std::uint32_t,6> values{created[0].handle,created[1].handle,created[2].handle,0,b.worker_storage,0x8000};
  for(unsigned i=0;i<6;++i){const auto&e=stores[c.stores.size()+i];check(e.pc==pcs[i] && e.address==addresses[i] && e.after==values[i],"Source store of actual native outcome differs");check(m.word(addresses[i])==values[i],"Actual native alias not retained");}
  check(m.word(b.worker_slot)==initial_worker,"Worker slot unexpectedly written before owner");
  for(unsigned i=0;i<2;++i){auto a=platform.wait(created[i].handle,0);check(a.state==NativeSynchronizationWaitState::Acquired && !a.error,"Retained mutex alias has no actual object");check(!platform.release_mutex(created[i].handle),"Actual mutex release failed");}
  auto empty=platform.wait(created[2].handle,0);check(empty.state==NativeSynchronizationWaitState::TimedOut && !empty.error,"Actual initial semaphore count differs");
  auto released=platform.release_semaphore(created[2].handle,1);check(released.previous_count==0 && !released.error,"Actual semaphore release differs");
  auto acquired=platform.wait(created[2].handle,0);check(acquired.state==NativeSynchronizationWaitState::Acquired && !acquired.error,"Retained semaphore alias has no actual permit");
  std::cout<<"native composition:3 actual aliases,6 original stores; missing worker/TLS/callback owner retained\n";
}
} // namespace
int main(int argc,char**argv){try{
  if(argc!=2)return 77;
  std::ifstream f(argv[1]);if(!f)return 77;
  SourceAnimationAsyncBindings bindings{};std::vector<Case>cases;std::string line;
  while(std::getline(f,line)){
    std::istringstream in(line);std::string kind;in>>kind;
    if(kind=="SOURCE")continue;
    if(kind=="BIND"){
      std::array<std::uint32_t*,10> fields{&bindings.nodes,&bindings.free_head,&bindings.work_head,&bindings.free_mutex_slot,&bindings.work_mutex_slot,&bindings.semaphore_slot,&bindings.worker_slot,&bindings.worker_storage_descriptor,&bindings.worker_storage,&bindings.worker_entry};
      for(auto p:fields){std::string v;in>>v;*p=hex(v);}
    }else if(kind=="CASE"){Case c;in>>c.name;cases.push_back(std::move(c));}
    else if(kind=="MEM" || kind=="AFTER"){std::string p,raw;in>>p>>raw;(kind=="MEM"?cases.back().before:cases.back().after).spans.emplace(hex(p),bytes(raw));}
    else if(kind=="STORE"){std::string pc,p,b,a;in>>pc>>p>>b>>a;cases.back().stores.push_back({hex(pc),hex(p),hex(b),hex(a)});}
    else check(kind=="END" || kind=="MISSING","Unknown source fixture token");
  }
  check(cases.size()==7,"Actual/explicit original prefix case count differs");
  for(const auto&c:cases)prefix(c,bindings);
  native_composition(cases.front(),bindings);
  Memory missing;SourceAnimationAsyncConstructionServices s{missing.services(),bindings,{},{},{},{}};bool stopped=false;
  try{construct_source_animation_async_queue(s);}catch(const std::runtime_error&){stopped=true;}
  check(stopped && missing.spans.empty(),"Missing real backing produced fields");
  std::cout<<"7 original complete-buffer prefixes/18921 ordered stores PASS; no completed source constructor or IO claim\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
