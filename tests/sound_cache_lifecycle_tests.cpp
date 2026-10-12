#include "halo2/sound_cache_lifecycle.hpp"
#include "halo2/native_sound_storage.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace halo2;
namespace {
void require(bool b,const char*s){if(!b)throw std::runtime_error(s);}
std::uint32_t number(const std::string&s){return std::stoul(s,nullptr,16);}
std::string hex(std::uint32_t v,unsigned n=4){std::ostringstream s;s<<std::hex<<std::setfill('0')<<std::setw(n*2)<<(v&(n==4?0xffffffffu:((1u<<(n*8))-1)));return s.str();}
std::vector<std::string> fields(const std::string&s,char sep='\t'){std::istringstream in(s);std::string v;std::vector<std::string>r;while(std::getline(in,v,sep))r.push_back(v);return r;}
std::vector<std::byte> decode(const std::string&s){require(s.size()%2==0,"Odd fixture bytes");std::vector<std::byte>b(s.size()/2);for(std::size_t k=0;k<b.size();++k)b[k]=std::byte(number(s.substr(k*2,2)));return b;}
struct Stage{std::string name;std::uint32_t entry;unsigned repeat;bool returned;std::vector<std::uint32_t>returns;std::map<std::uint32_t,std::vector<std::byte>>before,after;std::vector<std::string>events;};
struct Case{std::string name,control;std::uint32_t chunk;SoundCacheLifecycleBindings bindings;std::vector<Stage>stages;};
struct Counts{std::size_t stages=0,stores=0,bytes=0,aliases=0,clocks=0;};
Counts run(const Case &c){
 NativeSoundStorage storage({0x480d68,0xb000000,0xc000000});
 const auto&regions=c.stages.front().before;
 std::vector<std::pair<std::uint64_t,std::uint64_t>>ranges;
 for(const auto&[a,v]:regions)ranges.emplace_back(a&~4095u,(std::uint64_t(a)+v.size()+4095)&~std::uint64_t(4095));
 std::ranges::sort(ranges);std::uint64_t lo=0,hi=0;
 for(auto[a,z]:ranges){if(a<=hi){hi=std::max(hi,z);continue;}if(hi)storage.map_zeroed(std::uint32_t(lo),hi-lo);lo=a;hi=z;}if(hi)storage.map_zeroed(std::uint32_t(lo),hi-lo);
 auto memory=[&](std::uint32_t a,std::size_t n){auto it=regions.upper_bound(a);require(it!=regions.begin(),"Unowned cache lifecycle address");--it;require(std::uint64_t(a)+n<=std::uint64_t(it->first)+it->second.size(),"Unowned cache lifecycle extent");return storage.memory(a,n);};
 for(const auto&[a,v]:regions)std::ranges::copy(v,memory(a,v.size()).begin());
 auto read=[&](std::uint32_t a){auto v=memory(a,4);std::uint32_t out=0;for(unsigned k=0;k<4;++k)out|=std::to_integer<std::uint32_t>(v[k])<<(k*8);return out;};
 std::vector<std::string>actual;Counts counts;
 auto observe=[&](const SoundCacheStartupStore&q){++counts.stores;actual.push_back("STORE\t"+hex(q.instruction)+'\t'+hex(q.address)+'\t'+std::to_string(q.bytes)+'\t'+hex(q.before,q.bytes)+'\t'+hex(q.after,q.bytes));};
 SoundCacheLifecycleServices services;services.memory=memory;
 for(const auto&s:c.stages){
  for(const auto&[a,v]:s.before)require(std::ranges::equal(v,memory(a,v.size())),"Lifecycle phase before bytes differ");
  actual.clear();std::vector<std::uint32_t>ticks;
  for(const auto&e:s.events){auto q=fields(e);if(q[0]=="CALL"){require(q[1]=="native_tick_count","Unsupported original service");ticks.push_back(number(q[2]));}}
  std::size_t tick_index=0;services.tick_count=[&]{require(tick_index<ticks.size(),"Unobserved original clock request");auto value=ticks[tick_index++];++counts.clocks;actual.push_back("CALL\tnative_tick_count\t"+hex(value));return value;};
  require(s.returned,"Unclosed original fixture");
  for(unsigned k=0;k<s.repeat;++k){
   switch(s.entry){
    case 0x21a920:source_sound_cache_clock(c.bindings,services,observe);break;
    case 0x127290:source_sound_cache_audio_update_cold(c.bindings,services,observe);break;
    case 0x13e9a0:source_sound_cache_rebase(read(c.bindings.page_allocator_pointer),services,observe);break;
    case 0x21aa60:{auto result=source_sound_cache_alias(c.bindings,c.chunk,services,observe);require(result==s.returns.at(k),"Original physical alias differs");++counts.aliases;break;}
    case 0x21a800:source_sound_cache_drain(c.bindings,{0x21aaf0,0x21aab0},services,observe);break;
    default:throw std::runtime_error("Unknown original lifecycle entry");
   }
  }
  require(tick_index==ticks.size(),"Missed original clock request");
  if(actual!=s.events){std::size_t k=0;while(k<std::min(actual.size(),s.events.size())&&actual[k]==s.events[k])++k;std::cerr<<c.name<<' '<<s.name<<" event "<<k<<" actual "<<(k<actual.size()?actual[k]:"END")<<" expected "<<(k<s.events.size()?s.events[k]:"END")<<'\n';throw std::runtime_error("Lifecycle ordered events differ");}
  for(const auto&[a,v]:s.after){require(std::ranges::equal(v,memory(a,v.size())),"Lifecycle full output bytes differ");counts.bytes+=v.size();}++counts.stages;
 }
 return counts;
}
}
int main(int argc,char**argv)try{
 require(argc==2,"Usage: sound_cache_lifecycle_tests <fixtures.tsv>");std::ifstream in(argv[1]);require(bool(in),"Original fixtures unavailable");std::string line;std::getline(in,line);require(line=="SOUND_CACHE_LIFECYCLE_V1","Fixture schema");std::vector<Case>cases;Case c;Stage s;
 while(std::getline(in,line)){auto q=fields(line);if(q.empty())continue;
  if(q[0]=="CASE")c={q[1],q[2],number(q[3]),{number(q[4]),number(q[5]),number(q[6]),number(q[7]),number(q[8]),number(q[9]),number(q[10])},{}};
  else if(q[0]=="STAGE"){s={q[1],number(q[2]),unsigned(std::stoul(q[3])),q[4]=="true",{},{},{},{}};for(const auto&v:fields(q[5],','))s.returns.push_back(number(v));}
  else if(q[0]=="BEFORE"||q[0]=="AFTER")(q[0]=="BEFORE"?s.before:s.after).emplace(number(q[1]),decode(q[2]));
  else if(q[0]=="CALL"||q[0]=="STORE")s.events.push_back(line);
  else if(q[0]=="ENDSTAGE"){c.stages.push_back(std::move(s));s={};}
  else if(q[0]=="ENDCASE"){cases.push_back(std::move(c));c={};}
  else throw std::runtime_error("Unknown fixture row");
 }
 Counts total;for(const auto&c:cases){auto n=run(c);total.stages+=n.stages;total.stores+=n.stores;total.bytes+=n.bytes;total.aliases+=n.aliases;total.clocks+=n.clocks;}
 require(cases.size()==12&&total.stages==18&&total.aliases==768&&total.clocks==3,"Incomplete lifecycle suite");
 // Missing memory is rejected before any source flag or clock is invented.
 bool rejected=false;try{source_sound_cache_clock(cases.front().bindings,{});}catch(const std::runtime_error&){rejected=true;}require(rejected,"Missing memory silently admitted");
 std::cout<<"PASS "<<cases.size()<<" original lifecycle controls; "<<total.stages<<" full phases; "<<total.stores<<" ordered stores; "<<total.bytes<<" whole bytes; "<<total.aliases<<" physical aliases; "<<total.clocks<<" recorded real host clocks; no active audio/concurrent admission claim.\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
