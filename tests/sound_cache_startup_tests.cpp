#include "halo2/sound_cache_startup.hpp"
#include "halo2/sound_startup.hpp"
#include <algorithm>
#include <cerrno>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/mman.h>

using namespace halo2;
namespace {
void check(bool result,const std::string &message){if(!result)throw std::runtime_error(message);}
std::uint32_t number(const std::string &s){return std::uint32_t(std::stoul(s,nullptr,16));}
std::vector<std::byte> bytes(const std::string &hex){check(hex.size()%2==0,"odd hex");std::vector<std::byte> out(hex.size()/2);for(std::size_t i=0;i<out.size();++i)out[i]=std::byte(number(hex.substr(2*i,2)));return out;}
std::vector<std::uint32_t> fields(const SoundCacheStartupState &s){std::vector<std::uint32_t> out{s.general_allocator_identity,s.static_allocator_identity,s.entry_pool,s.page_allocator,s.physical_buffer,s.physical_arena_index};out.insert(out.end(),s.physical_lower_limits.begin(),s.physical_lower_limits.end());out.insert(out.end(),s.physical_upper_limits.begin(),s.physical_upper_limits.end());return out;}
struct Region{std::uint32_t address,requested;std::vector<std::byte> data;};
struct Stage{SoundCacheStartupState state;std::vector<std::byte> physical;std::vector<Region> regions;};
struct Allocation{std::uint32_t owner,requested,alias,rounded;};
struct Case{std::map<std::string,Stage> stages;std::vector<Allocation> allocations;std::vector<std::array<std::uint32_t,3>> protections;std::vector<SoundCacheStartupStore> stores;};
struct Backing {
  std::uint32_t alias;std::size_t extent;std::byte *data;
  Backing(std::uint32_t address,std::size_t size):alias(address),extent(size){auto p=mmap(nullptr,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);check(p!=MAP_FAILED,"actual anonymous mmap failed");data=static_cast<std::byte *>(p);}
  ~Backing(){if(data)munmap(data,extent);}
  Backing(const Backing&)=delete;Backing &operator=(const Backing&)=delete;
  Backing(Backing&& other)noexcept:alias(other.alias),extent(other.extent),data(other.data){other.data=nullptr;}
  std::span<std::byte> memory(std::uint32_t address,std::size_t size){if(address<alias||std::uint64_t(address-alias)+size>extent)return {};return {data+(address-alias),size};}
};
struct StartupStage {SoundStartupState state;std::vector<std::byte> data;};
std::vector<std::uint32_t> startup_fields(const SoundStartupState &s){return {s.references,s.arena_base,s.arena_used,s.arena_crc,s.allocator_identity,s.system,s.sources,s.static_voices,s.controllers,s.controller_root,s.looping,s.effects,s.shared_voices};}
struct PlatformBoundary {};
void startup_composition(const Case &fixture,const SoundCacheStartupConstants &cache_constants,const char *path){
 std::ifstream f(path);check(bool(f),"missing original startup fixtures");std::map<std::string,StartupStage> stages;SoundStartupConstants constants;std::array<std::uint32_t,3> clock{};std::string line;
 while(std::getline(f,line)){
  if(line.empty()||line[0]=='#')continue;
  std::vector<std::string> q;std::stringstream input(line);std::string v;while(std::getline(input,v,'\t'))q.push_back(v);
  if(q[0]=="constants"){constants.ambient_transition_bits=number(q[1]);auto b=bytes(q[2]);check(b.size()==16,"actual policy extent");std::copy(b.begin(),b.end(),constants.voice_policy.begin());}
  else if(q[0]=="controller"){for(unsigned i=0;i<3;++i)clock[i]=number(q[i+1]);}
  else if(q[0]=="stage"){check(q.size()==16,"startup stage extent");SoundStartupState s{number(q[2]),number(q[3]),number(q[4]),number(q[5]),number(q[6]),number(q[7]),number(q[8]),number(q[9]),number(q[10]),number(q[11]),number(q[12]),number(q[13]),number(q[14])};stages.emplace(q[1],StartupStage{s,bytes(q[15])});}
 }
 auto state=stages.at("entry").state;auto cache=fixture.stages.at("entry").state;
 check(state.allocator_identity==cache.static_allocator_identity,"actual static allocator identity composition");
 Backing arena(state.arena_base,stages.at("entry").data.size());std::copy(stages.at("entry").data.begin(),stages.at("entry").data.end(),arena.data);
 std::vector<Backing> raw,physical;for(const auto &p:fixture.protections)physical.emplace_back(p[0],p[1]);std::size_t allocation=0,protection=0;std::vector<unsigned> calls;
 auto memory=[&](std::uint32_t address,std::size_t extent)->std::span<std::byte>{auto a=arena.memory(address,extent);if(a.size()==extent)return a;for(auto &p:raw){auto b=p.memory(address,extent);if(b.size()==extent)return b;}throw std::runtime_error("composed actual storage unavailable");};
 SoundCacheStartupServices cache_services;
 cache_services.memory=memory;
 cache_services.raw_allocate=[&](std::uint32_t owner,std::uint32_t extent){check(allocation<fixture.allocations.size(),"composed extra raw allocation");const auto &q=fixture.allocations[allocation++];check(owner==q.owner&&extent==q.requested,"composed original allocation request");raw.emplace_back(q.alias,q.rounded);return q.alias;};
 cache_services.raw_protect=[&](std::uint32_t address,std::uint32_t extent,std::uint32_t mode){check(protection<fixture.protections.size(),"composed extra protection");check(std::array<std::uint32_t,3>{address,extent,mode}==fixture.protections[protection],"composed original protection request");check(mprotect(physical[protection++].data,extent,PROT_READ|PROT_WRITE)==0,"composed actual protection failed");};
 auto compare=[&](const std::string &phase){const auto &q=stages.at(phase);check(startup_fields(state)==startup_fields(q.state),"composed startup retained globals");check(std::equal(q.data.begin(),q.data.end(),arena.data),"composed complete startup arena");};
 SoundStartupServices services;services.memory=memory;
 services.cache_initialize=[&](SoundStartupState &owner){check(&owner==&state,"composed actual owner reference");compare("before_cache");source_sound_cache_startup(cache,cache_constants,cache_services);compare("after_cache");calls.push_back(0);};
 services.utc_seconds=[&]{calls.push_back(1);return clock[0];};services.crt_random=[&]{calls.push_back(2);return clock[1];};services.tick_milliseconds=[&]{calls.push_back(3);return clock[2];};
 services.native_platform_initialize=[&](std::span<const std::byte,16> policy)->bool{check(std::equal(policy.begin(),policy.end(),constants.voice_policy.begin()),"composed actual platform request");calls.push_back(4);compare("platform_request");throw PlatformBoundary{};};
 bool stopped=false;try{source_sound_startup(state,constants,services);}catch(const PlatformBoundary &){stopped=true;}
 check(stopped&&calls==std::vector<unsigned>({0,1,2,3,4}),"complete CPU owner composition order/boundary");compare("platform_request");
 check(fields(cache)==fields(fixture.stages.at("returned").state)&&allocation==2&&protection==1,"composed original cache outputs");
 const auto &expected=fixture.stages.at("returned").regions;check(raw.size()==expected.size(),"composed raw block count");for(std::size_t i=0;i<raw.size();++i)check(std::equal(expected[i].data.begin(),expected[i].data.end(),raw[i].data),"composed complete original cache buffers");
 std::cout<<"PASS genuine cache constructor wired through portable126B20 CPU owner; exact startup/cache arenas/globals; actual platform constructor boundary retained\n";
}
}
int main(int argc,char **argv){try{
 check(argc==2||argc==3,"usage: sound_cache_startup_tests original-cache-fixtures.tsv [original-startup-fixtures.tsv]");std::ifstream f(argv[1]);check(bool(f),"missing original source fixtures");
 SoundCacheStartupConstants constants;std::map<std::string,Case> cases;std::string line;
 while(std::getline(f,line)){
  if(line.empty()||line[0]=='#')continue;
  std::vector<std::string> q;std::stringstream in(line);std::string v;while(std::getline(in,v,'\t'))q.push_back(v);
  if(q[0]=="constants")constants={number(q[1]),number(q[2])};
  else if(q[0]=="case")cases.emplace(q[1],Case{});
  else if(q[0]=="stage"){
   check(q.size()==20,"stage extent");SoundCacheStartupState s{number(q[3]),number(q[4]),number(q[5]),number(q[6]),number(q[7]),number(q[8]),{}, {}};
   for(unsigned i=0;i<5;++i){s.physical_lower_limits[i]=number(q[9+i]);s.physical_upper_limits[i]=number(q[14+i]);}
   cases.at(q[1]).stages.emplace(q[2],Stage{s,bytes(q[19]),{}});
  }else if(q[0]=="region")cases.at(q[1]).stages.at(q[2]).regions.push_back({number(q[3]),std::uint32_t(std::stoul(q[4])),bytes(q[6])});
  else if(q[0]=="allocate")cases.at(q[1]).allocations.push_back({number(q[2]),std::uint32_t(std::stoul(q[3])),number(q[4]),std::uint32_t(std::stoul(q[5]))});
  else if(q[0]=="protect")cases.at(q[1]).protections.push_back({number(q[2]),number(q[3]),number(q[4])});
  else if(q[0]=="store")cases.at(q[1]).stores.push_back({number(q[2]),number(q[3]),unsigned(std::stoul(q[4])),number(q[5]),number(q[6])});
  else throw std::runtime_error("unknown fixture row");
 }
 std::size_t represented_stores=0,protections=0;
 for(const auto &[name,fixture]:cases){
  auto state=fixture.stages.at("entry").state;std::vector<Backing> raw;std::vector<SoundCacheStartupStore> actual;std::vector<unsigned> calls;std::size_t allocation=0,protection=0;
  std::vector<Backing> physical;for(const auto &p:fixture.protections)physical.emplace_back(p[0],p[1]);
  auto compare=[&](const std::string &phase){const auto &expected=fixture.stages.at(phase);check(fields(state)==fields(expected.state),name+" "+phase+" globals/physical arrays");check(raw.size()==expected.regions.size(),name+" "+phase+" block count");for(std::size_t i=0;i<raw.size();++i){check(raw[i].alias==expected.regions[i].address&&raw[i].extent==expected.regions[i].data.size(),"actual backing extent");check(std::equal(expected.regions[i].data.begin(),expected.regions[i].data.end(),raw[i].data),name+" "+phase+" complete source pool/page bytes");}};
  SoundCacheStartupServices services;
  services.memory=[&](std::uint32_t address,std::size_t extent)->std::span<std::byte>{for(auto &p:raw){auto q=p.memory(address,extent);if(q.size()==extent)return q;}throw std::runtime_error("actual raw backing unavailable");};
  services.raw_allocate=[&](std::uint32_t owner,std::uint32_t requested){check(allocation<fixture.allocations.size(),"extra allocation");const auto &q=fixture.allocations[allocation];check(owner==q.owner&&requested==q.requested,"original allocator identity/request");raw.emplace_back(q.alias,q.rounded);calls.push_back(0);++allocation;compare(allocation==1?"first_raw_allocation":"second_raw_allocation");return q.alias;};
  services.raw_protect=[&](std::uint32_t address,std::uint32_t extent,std::uint32_t mode){check(protection<fixture.protections.size(),"extra protection");check(std::array<std::uint32_t,3>{address,extent,mode}==fixture.protections[protection],"original protection request");compare("before_real_protection");check(mode==4,"unsupported original protection");auto &p=physical[protection];errno=0;check(mprotect(p.data,extent,PROT_READ|PROT_WRITE)==0,"actual mprotect failed");++protection;calls.push_back(1);};
  source_sound_cache_startup(state,constants,services,[&](const auto &q){actual.push_back(q);});compare("returned");
  check(allocation==2&&protection==fixture.protections.size(),"source service count");
  check(calls== (protection?std::vector<unsigned>{0,0,1}:std::vector<unsigned>{0,0}),"source allocation/protection order");
  std::set<std::uint32_t> represented;for(const auto &q:actual)if(q.instruction)represented.insert(q.instruction);
  auto filter=[&](const auto &rows){std::vector<SoundCacheStartupStore> out;for(const auto &q:rows)if(represented.contains(q.instruction))out.push_back(q);return out;};
  const auto native=filter(actual),original=filter(fixture.stores);
  if(native!=original){for(std::size_t i=0;i<std::min(native.size(),original.size());++i)if(native[i]!=original[i]){auto a=native[i],b=original[i];std::cerr<<name<<" store "<<i<<" native "<<std::hex<<a.instruction<<' '<<a.address<<' '<<a.bytes<<' '<<a.before<<' '<<a.after<<" original "<<b.instruction<<' '<<b.address<<' '<<b.bytes<<' '<<b.before<<' '<<b.after<<std::dec<<'\n';break;}throw std::runtime_error("original complete constructor ordered stores differ");}
  represented_stores+=native.size();protections+=protection;
  // A missing platform service must stop at the actual source request after
  // CPU construction/reservation; it cannot publish a fake final buffer.
  if(protection){raw.clear();allocation=0;state=fixture.stages.at("entry").state;services.raw_protect={};bool missing=false;try{source_sound_cache_startup(state,constants,services);}catch(const std::runtime_error&){missing=true;}check(missing,"missing protection did not stop");compare("before_real_protection");}
  state=fixture.stages.at("entry").state;bool missing=false;try{source_sound_cache_startup(state,constants,{});}catch(const std::runtime_error&){missing=true;}check(missing&&fields(state)==fields(fixture.stages.at("entry").state),"missing allocator changed state");
 }
 std::cout<<"PASS complete original21A6E0/13E8E0: "<<cases.size()<<" cases, "<<represented_stores<<" ordered represented stores, whole raw pool/page buffers, "<<protections<<" actual mmap/mprotect requests; no map/device/voice admission\n";
 if(argc==3)startup_composition(cases.at("original_arena"),constants,argv[2]);
 return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
