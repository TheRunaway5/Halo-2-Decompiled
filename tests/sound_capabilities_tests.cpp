#include "halo2/sound_capabilities.hpp"
#include "halo2/native_sound_storage.hpp"
#include "halo2/native_synchronization.hpp"
#include <algorithm>
#include <bit>
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
std::uint32_t num(const std::string&s){return std::stoul(s,nullptr,16);}
std::vector<std::string>fields(const std::string&s){std::istringstream in(s);std::string v;std::vector<std::string>r;while(std::getline(in,v,'\t'))r.push_back(v);return r;}
std::vector<std::byte>decode(const std::string&s){require(s.size()%2==0,"Odd original bytes");std::vector<std::byte>b(s.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(num(s.substr(2*i,2)));return b;}
std::string hex(std::uint32_t v,unsigned n=4){std::ostringstream s;for(unsigned i=0;i<n;++i)s<<std::hex<<std::setfill('0')<<std::setw(2)<<((v>>(8*i))&255);return s.str();}
struct Allocation{std::uint32_t alias,bytes,capacity,tag;};
struct Case{std::string name,op;std::uint32_t ecx,result;std::vector<std::uint32_t>args;std::map<std::uint32_t,std::vector<std::byte>>before,after;std::vector<Allocation>allocations;std::vector<std::string>stores,locks;};
struct Count{std::size_t stores=0,bytes=0,allocations=0,locks=0,negative=0;};
Count run(const Case&c,const SoundCapabilitiesBindings&bindings,const SoundCapabilitiesConstructorConstants&constants,const std::map<std::uint32_t,std::vector<std::byte>>&immutable,std::uint8_t recorded_irql){
 const std::uint32_t allocator=0xA11A0001,first=c.allocations.empty()?0xE000000:c.allocations.front().alias;
 NativeSoundStorage storage({allocator,first,0xF000000});require(storage.page_size()==4096,"Measured native allocation granularity differs");
 std::vector<std::pair<std::uint64_t,std::uint64_t>>ranges;for(const auto&[a,v]:c.before)ranges.emplace_back(a&~4095u,(std::uint64_t(a)+v.size()+4095)&~std::uint64_t(4095));std::ranges::sort(ranges);
 std::uint64_t lo=0,hi=0;for(auto[a,z]:ranges){if(a<=hi){hi=std::max(hi,z);continue;}if(hi)storage.map_zeroed(std::uint32_t(lo),hi-lo);lo=a;hi=z;}if(hi)storage.map_zeroed(std::uint32_t(lo),hi-lo);
 std::map<std::uint32_t,std::size_t>extents;for(const auto&[a,v]:c.before){std::ranges::copy(v,storage.memory(a,v.size()).begin());extents.emplace(a,v.size());}
 auto literal=immutable;
 auto memory=[&](std::uint32_t a,std::size_t n)->std::span<std::byte>{
  auto k=extents.upper_bound(a);if(k!=extents.begin()){--k;if(std::uint64_t(a)+n<=std::uint64_t(k->first)+k->second)return storage.memory(a,n);}
  auto i=literal.upper_bound(a);if(i==literal.begin()){std::cerr<<c.name<<" missing source address "<<std::hex<<a<<" bytes "<<n<<'\n';throw std::runtime_error("Source accounting address unavailable");}--i;require(std::uint64_t(a)+n<=std::uint64_t(i->first)+i->second.size(),"Source accounting extent unavailable");return std::span(i->second).subspan(a-i->first,n);
 };
 NativeSynchronization synchronization({0xF100000,0xF100100});const auto sync=synchronization.services();const auto mutex=sync.create_mutex(true);require(mutex.handle&&!mutex.error,"Real recursive mutex unavailable");
 std::vector<std::string>stores,locks;std::map<std::uint32_t,std::uint32_t>capacities;std::size_t allocation_index=0;Count count;
 SoundCapabilitiesServices services;services.memory=memory;
 // Recorded whole compatibility KPCR input, never a liveXbox/SDL default.
 // All native mutex results come from the genuine retained provider.
 services.current_irql=[&](){return recorded_irql;};
 const auto raw=storage.cache_services();
 services.raw_pool_allocate=[&](auto bytes,auto tag){require(allocation_index<c.allocations.size(),"Unmeasured raw allocation");const auto&q=c.allocations[allocation_index++];require(bytes==q.bytes&&tag==q.tag,"Original raw allocation request differs");const auto alias=raw.raw_allocate(allocator,bytes);require(alias==q.alias,"Actual raw allocation alias differs");const auto capacity=std::uint32_t((std::uint64_t(bytes)+storage.page_size()-1)&~std::uint64_t(storage.page_size()-1));require(capacity==q.capacity&&storage.memory(alias,capacity).size()==capacity,"Actual backing capacity differs");capacities.emplace(alias,capacity);extents.emplace(alias,capacity);return alias;};
 services.raw_pool_capacity=[&](auto a){return capacities.at(a);};
 // Instruction/address columns use normal hexadecimal, while before/after
 // columns retain original literal little-endian bytes.
 auto value_hex=[](std::uint32_t v){std::ostringstream s;s<<std::hex<<std::setfill('0')<<std::setw(8)<<v;return s.str();};
 auto observe=[&](const SoundCapabilitiesStore&q){stores.push_back("STORE\t"+value_hex(q.instruction)+'\t'+value_hex(q.address)+'\t'+std::to_string(q.bytes)+'\t'+hex(q.before,q.bytes)+'\t'+hex(q.after,q.bytes));};
 // Lock strings likewise use ordinary address notation.
 services.enter_critical_section=[&](auto a){require(a==bindings.critical_section,"Wrong source lock identity");require(sync.wait(mutex.handle,0xffffffffu).state==NativeSynchronizationWaitState::Acquired,"Real SDK mutex acquisition failed");locks.push_back("LOCK\t277\t"+value_hex(a));};
 services.leave_critical_section=[&](auto a){require(a==bindings.critical_section,"Wrong source unlock identity");require(!sync.release_mutex(mutex.handle),"Real SDK mutex release failed");locks.push_back("LOCK\t294\t"+value_hex(a));};
 std::uint32_t result;
 if(c.op=="caps"){require(c.args.size()==2,"Caps ABI");result=std::bit_cast<std::uint32_t>(source_sound_capabilities(c.args[0],c.args[1],bindings,services,observe));}
 else if(c.op=="allocate"){require(c.args.size()==1,"Allocate ABI");result=source_sound_sdk_allocate(0x626f5344,c.args[0],1,bindings,services,observe);}
 else if(c.op=="apu"){require(c.args.size()==1,"APU ABI");result=source_sound_apu_construct(c.ecx,c.args[0],bindings,constants,services,observe);}
 else if(c.op=="sge"){require(c.args.size()==1,"SGE ABI");result=std::bit_cast<std::uint32_t>(source_sound_sge_initialize(c.ecx,c.args[0],bindings,services,observe));}
 else throw std::runtime_error("Unknown original accounting owner");
 require(!sync.release_mutex(mutex.handle),"Initial actual lock ownership lost");
 require(result==c.result,"Original capability/constructor return differs");require(locks==c.locks,"Original guard request sequence differs");
 if(stores!=c.stores){std::size_t i=0;while(i<stores.size()&&i<c.stores.size()&&stores[i]==c.stores[i])++i;std::cerr<<c.name<<" first store "<<i<<" native="<<(i<stores.size()?stores[i]:"END")<<" original="<<(i<c.stores.size()?c.stores[i]:"END")<<'\n';throw std::runtime_error("Original ordered SDK stores differ");}
 for(const auto&[a,v]:c.after){if(!std::ranges::equal(v,memory(a,v.size()))){std::cerr<<c.name<<" region "<<value_hex(a)<<'\n';throw std::runtime_error("Original whole accounting output differs");}count.bytes+=v.size();}
 require(allocation_index==c.allocations.size(),"Original raw request missing");count.stores=stores.size();count.locks=locks.size();count.allocations=allocation_index;count.negative=result==0x80004005u;
 // Required real dependencies must fail closed before original capability
 // output. This is a native availability control, not a source success fixture.
 if(c.op=="caps"){auto missing=services;missing.current_irql={};bool stopped=false;try{source_sound_capabilities(c.args[0],c.args[1],bindings,missing);}catch(const std::runtime_error&){stopped=true;}require(stopped,"Missing actual IRQL admitted");}
 return count;
}
}
int main(int argc,char**argv)try{
 require(argc==2,"Usage: sound_capabilities_tests <fixtures.tsv>");std::ifstream in(argv[1]);require(bool(in),"Original accounting fixtures unavailable");std::string line;std::getline(in,line);require(line=="SOUND_CAPABILITIES_V1","Fixture schema");
 SoundCapabilitiesBindings b{};SoundCapabilitiesConstructorConstants k{};std::map<std::uint32_t,std::vector<std::byte>>immutable;std::vector<Case>cases;Case c;std::uint8_t irql=255;bool have_irql=false;
 while(std::getline(in,line)){const auto q=fields(line);if(q.empty())continue;
  if(q[0]=="BINDINGS"){require(q.size()==11,"Bindings extent");b={num(q[1]),num(q[2]),num(q[3]),num(q[4]),num(q[5]),num(q[6]),num(q[7]),num(q[8]),num(q[9]),num(q[10])};}
  else if(q[0]=="CONSTANTS"){require(q.size()==7,"Constants extent");k={num(q[1]),num(q[2]),num(q[3]),num(q[4]),num(q[5]),num(q[6])};}
  else if(q[0]=="IMMUTABLE")immutable.emplace(num(q[1]),decode(q[2]));
  else if(q[0]=="IRQL"){irql=std::uint8_t(num(q[1]));have_irql=true;}
  else if(q[0]=="CASE"){c={};c.name=q[1];c.op=q[2];c.ecx=num(q[3]);c.result=num(q[4]);for(std::size_t i=5;i<q.size();++i)c.args.push_back(num(q[i]));}
  else if(q[0]=="BEFORE"||q[0]=="AFTER")(q[0]=="BEFORE"?c.before:c.after).emplace(num(q[1]),decode(q[2]));
  else if(q[0]=="ALLOC")c.allocations.push_back({num(q[1]),std::uint32_t(std::stoul(q[2])),std::uint32_t(std::stoul(q[3])),num(q[4])});
  else if(q[0]=="STORE")c.stores.push_back(line);
  else if(q[0]=="LOCK")c.locks.push_back(line);
  else if(q[0]=="ENDCASE")cases.push_back(std::move(c));
  else throw std::runtime_error("Unknown original row");
 }
 require(have_irql,"Recorded thread input missing");Count total;for(const auto&c:cases){const auto n=run(c,b,k,immutable,irql);total.stores+=n.stores;total.bytes+=n.bytes;total.allocations+=n.allocations;total.locks+=n.locks;total.negative+=n.negative;}
 require(cases.size()==10&&total.allocations==2&&total.locks==14&&total.negative==3,"Incomplete original accounting suite");
 std::cout<<"PASS "<<cases.size()<<" original accounting/constructor controls; "<<total.stores<<" ordered SDK stores; "<<total.bytes<<" whole bytes; "<<total.allocations<<" actual OS allocations; "<<total.locks<<" real recursive synchronization requests; 27 complete u5 bodies separately paired; no device/startup/admission result supplied.\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
