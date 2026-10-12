#include "halo2/animation_residency_capacity.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <vector>
namespace {
using Bytes=std::vector<std::byte>;
using Memory=std::map<std::uint32_t,Bytes>;
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::uint32_t number(std::istringstream &s){std::uint32_t v;check(bool(s>>std::hex>>v),"Incomplete capacity fixture");return v;}
Bytes hex(std::string s){check(s.size()%2==0,"Odd capacity literal");Bytes r;for(std::size_t i=0;i<s.size();i+=2)r.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return r;}
struct Event{
  std::string kind;std::array<std::uint32_t,4> words;
  friend bool operator==(const Event&,const Event&)=default;
};
struct Case{
 std::uint32_t entry;std::vector<std::uint32_t> args;
 Memory before,after,at_protect;std::vector<Event> events;
};
std::vector<Case> fixture(const char *path){
 std::ifstream in(path);if(!in)return {};
 std::vector<Case> rows;Case *c=nullptr;std::string line;
 while(std::getline(in,line)){
  std::istringstream s(line);std::string key;s>>key;
  if(key.empty()||key[0]=='#')continue;
  if(key=="CASE"){
   rows.emplace_back();c=&rows.back();(void)number(s);c->entry=number(s);
   const auto n=c->entry==0x124980 ? 1u : 2u;
   for(unsigned i=0;i<n;++i)c->args.push_back(number(s));
  }else{
   check(c!=nullptr,"Capacity row outside case");
   if(key=="BEFORE"||key=="AFTER"||key=="AT_PROTECT"){
    const auto address=number(s);std::string raw;s>>raw;
    auto &m=key=="BEFORE"?c->before:key=="AFTER"?c->after:c->at_protect;
    check(m.emplace(address,hex(raw)).second,"Duplicate capacity backing");
   }else if(key=="EVENT"){
    Event e{};s>>e.kind;
    for(unsigned i=0;i<(e.kind=="STORE"?4u:3u);++i)e.words[i]=number(s);
    c->events.push_back(e);
   }else check(key=="ENDCASE","Unknown capacity fixture row");
  }
 }
 check(rows.size()==2,"Unexpected original capacity cases");return rows;
}
std::span<std::byte> bytes(Memory &m,std::uint32_t p,std::size_t n){
 auto it=m.upper_bound(p);check(it!=m.begin(),"Missing capacity backing");--it;
 const auto offset=std::uint64_t(p)-it->first;
 check(offset<=it->second.size()&&n<=it->second.size()-offset,"Escaped capacity backing");
 return std::span(it->second).subspan(static_cast<std::size_t>(offset),n);
}
struct Backing{
 void *p=nullptr;std::size_t n=0;
 explicit Backing(std::size_t size):n(size){
  p=mmap(nullptr,n,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
  check(p!=MAP_FAILED,"Genuine host backing allocation failed");
 }
 ~Backing(){if(p&&p!=MAP_FAILED)munmap(p,n);}
 Backing(const Backing&)=delete;Backing &operator=(const Backing&)=delete;
};
halo2::SourceAnimationCapacityServices services(Memory &m){
 halo2::SourceAnimationCapacityServices s;
 s.bindings={0x4eb124,0x4ed9f0,0x4ed9fc,0x4eda10,0x4eb128,0x4eb12c,0x4eb120};
 s.memory.read=[&](std::uint32_t p,std::size_t n)->std::span<const std::byte>{return bytes(m,p,n);};
 s.memory.write=[&](std::uint32_t p,std::span<const std::byte> b){auto out=bytes(m,p,b.size());std::copy(b.begin(),b.end(),out.begin());};
 return s;
}
void run(const Case &c){
 auto memory=c.before;auto s=services(memory);std::vector<Event> events;
 std::size_t protections=0;std::vector<std::unique_ptr<Backing>> owned;
 s.protect=[&](std::uint32_t base,std::uint32_t n,std::uint32_t protection){
  check(memory==c.at_protect,"Original complete pre-protection buffers differ");
  check(n>0&&protection==4,"Original protection request differs");
  auto actual=std::make_unique<Backing>(n);
  auto *data=static_cast<std::byte*>(actual->p);
  check(std::all_of(data,data+n,[](std::byte b){return b==std::byte{0};}),"Host anonymous backing differs");
  check(mprotect(actual->p,n,PROT_READ|PROT_WRITE|PROT_EXEC)==0,"Actual host protection failed");
  check(std::all_of(data,data+n,[](std::byte b){return b==std::byte{0};}),"Protection changed backing contents");
  events.push_back({"PROTECT",{base,n,protection,0}});++protections;owned.push_back(std::move(actual));
 };
 auto observer=[&](const halo2::AnimationResourceStore &q){events.push_back({"STORE",{q.instruction,q.address,q.before,q.after}});};
 if(c.entry==0x124980)halo2::configure_source_animation_capacity(std::bit_cast<std::int32_t>(c.args[0]),s,observer);
 else halo2::resize_source_animation_cache(c.args[0],c.args[1],s,observer);
 check(memory==c.after,"Original complete retained resource buffers differ");
 check(events==c.events,"Original capacity stores/protection order differ");
 check(protections==(c.entry==0x124980?1u:0u),"Original protection count differs");
 // Retirement is deliberately unavailable: the real source pool is empty.
 check(!s.retire,"Unexpected retirement substitute");
}
template<class F>void rejects(F f){bool stopped=false;try{f();}catch(const std::runtime_error&){stopped=true;}check(stopped,"Unavailable capacity owner did not stop");}
void missing(const Case &c){
 auto memory=c.before;auto s=services(memory);
 rejects([&]{halo2::configure_source_animation_capacity(std::bit_cast<std::int32_t>(c.args[0]),s);});
 check(memory==c.at_protect,"Missing protection stop lost preceding source stores");
 memory=c.before;s=services(memory);s.memory.read={};rejects([&]{halo2::configure_source_animation_capacity(1,s);});check(memory==c.before,"Missing read mutated retained fields");
 s=services(memory);s.memory.read=[&](std::uint32_t p,std::size_t n)->std::span<const std::byte>{return bytes(memory,p,n).first(n-1);};
 rejects([&]{halo2::configure_source_animation_capacity(1,s);});check(memory==c.before,"Truncated backing mutated retained fields");
}
}
int main(int argc,char **argv){
 try{
  if(argc!=2)return 2;
  auto rows=fixture(argv[1]);if(rows.empty())return 77;
  for(const auto &c:rows)run(c);
  auto it=std::find_if(rows.begin(),rows.end(),[](const Case&c){return c.entry==0x124980;});check(it!=rows.end(),"Missing actual capacity callback");missing(*it);
  std::size_t stores=0,protects=0;for(const auto &c:rows)for(const auto &e:c.events){stores+=e.kind=="STORE";protects+=e.kind=="PROTECT";}
  std::cout<<"actual source capacity cases="<<rows.size()<<" stores="<<stores<<" protections="<<protects<<" completeBuffers=PASS missingOwners=PASS\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
