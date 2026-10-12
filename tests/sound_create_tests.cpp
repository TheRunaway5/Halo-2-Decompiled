#include "halo2/sound_create.hpp"
#include "halo2/native_sound_storage.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
std::uint32_t u(const std::string&s){return std::uint32_t(std::stoul(s,nullptr,16));}
void require(bool b,const char*s){if(!b)throw std::runtime_error(s);}
std::vector<std::string> fields(const std::string&s){std::vector<std::string>r;std::istringstream f(s);std::string q;while(std::getline(f,q,'\t'))r.push_back(q);return r;}
std::vector<std::byte> decode(const std::string&s){require(!(s.size()%2),"Literal bytes");std::vector<std::byte>b(s.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(u(s.substr(i*2,2)));return b;}
struct Boundary{};
}
int main(int argc,char**argv)try{
 require(argc==2,"Usage: sound_create_tests <fixtures.tsv>");std::ifstream f(argv[1]);require(bool(f),"Original creation fixtures unavailable");
 std::string line;require(bool(std::getline(f,line))&&line=="SOUND_CREATE_NATIVE_V1","Original fixture schema");
 halo2::SoundCreateState state{};halo2::SoundCreateConstants constants{};halo2::SoundCreateServices services;
 std::map<std::uint32_t,std::vector<std::byte>> initial,expected;std::unique_ptr<halo2::NativeSoundStorage> storage;
 std::vector<halo2::SoundStartupStore> stores;std::uint32_t sound=0,association=0,promotion=0,cache_sound=0,chunk=0,flags=0;
 std::string scenario;std::size_t calls=0,writes=0,maps=0,bytes=0,cache_calls=0;bool active=false,initialized=false;
 auto initialize=[&]{
  storage=std::make_unique<halo2::NativeSoundStorage>(halo2::NativeSoundStorageConfiguration{0x480d68,0xb000000,0xc000000});
  std::vector<std::pair<std::uint64_t,std::uint64_t>> ranges;
  for(const auto&[a,b]:initial)ranges.emplace_back(a&~std::uint32_t(4095),(std::uint64_t(a)+b.size()+4095)&~std::uint64_t(4095));
  std::ranges::sort(ranges);std::uint64_t start=0,end=0;
  for(const auto&[a,b]:ranges){if(a<=end){end=std::max(end,b);continue;}if(end)storage->map_zeroed(std::uint32_t(start),std::size_t(end-start));start=a;end=b;}
  if(end)storage->map_zeroed(std::uint32_t(start),std::size_t(end-start));
  services.memory=[&](std::uint32_t at,std::size_t n){auto it=initial.upper_bound(at);require(it!=initial.begin(),"Unprovided source address");--it;require(std::uint64_t(at)+n<=std::uint64_t(it->first)+it->second.size(),"Unprovided source bytes");return storage->memory(at,n);};
  services.cache_request=[&](std::uint32_t s,std::uint32_t c,std::uint32_t f)->std::uint32_t{require(s==cache_sound&&c==chunk&&f==flags,"Original cache request mismatch");++cache_calls;throw Boundary{};};
  initialized=true;
 };
 while(std::getline(f,line)){
  const auto q=fields(line);if(q.empty())continue;
  if(q[0]=="K"){require(q.size()==12,"Constant words");constants={u(q[1]),u(q[2]),u(q[3]),u(q[4]),u(q[5]),u(q[6]),u(q[7]),u(q[8]),u(q[9]),u(q[10]),u(q[11])};}
  else if(q[0]=="M"){require(!active&&q.size()==15,"Original map roots");initial.clear();storage.reset();initialized=false;state={u(q[3]),u(q[4]),u(q[5]),u(q[6]),u(q[7]),u(q[8]),u(q[9]),u(q[10]),u(q[11]),u(q[12]),u(q[13]),u(q[14])};}
  else if(q[0]=="R"){require(!initialized&&q.size()==3,"Source region");require(initial.emplace(u(q[1]),decode(q[2])).second,"Overlapping source identity");}
  else if(q[0]=="C"){
   require(!active&&q.size()==9,"Source call");if(!initialized)initialize();active=true;expected=initial;
   for(const auto&[a,b]:initial)std::ranges::copy(b,storage->memory(a,b.size()).begin());
   scenario=q[1];sound=u(q[2]);association=u(q[3]);promotion=u(q[4]);auto raw=decode(q[5]);require(raw.size()==160,"Full raw ABI");
   expected.at(0xa400000)=raw;std::ranges::copy(raw,storage->memory(0xa400000,160).begin());cache_sound=u(q[6]);chunk=u(q[7]);flags=u(q[8]);stores.clear();
  }else if(q[0]=="S"){require(active&&q.size()==6,"Original store");stores.push_back({u(q[1]),u(q[2]),unsigned(std::stoul(q[3])),u(q[4]),u(q[5])});}
  else if(q[0]=="E"){
   require(active,"Original end");active=false;
   for(const auto&s:stores){auto it=expected.upper_bound(s.address);require(it!=expected.begin(),"Store region");--it;auto&a=it->first;auto&b=it->second;require(std::uint64_t(s.address)+s.bytes<=std::uint64_t(a)+b.size(),"Store extent");const auto off=s.address-a;std::uint32_t before=0;for(unsigned n=0;n<s.bytes;++n)before|=std::to_integer<std::uint32_t>(b[off+n])<<(n*8);require(before==s.before,"Original before bits");for(unsigned n=0;n<s.bytes;++n)b[off+n]=std::byte(s.after>>(n*8));}
   std::vector<halo2::SoundStartupStore> actual;bool stopped=false;
   try{halo2::source_sound_create(state,sound,association,0xa400000,promotion,constants,{0x027f,0x1f80},services,[&](const auto&s){actual.push_back(s);});}
   catch(const Boundary&){stopped=true;}
   if(actual!=stores||!stopped){std::cerr<<"Case "<<calls<<' '<<scenario<<" stores "<<actual.size()<<'/'<<stores.size()<<" stopped "<<stopped<<'\n';for(std::size_t n=0;n<std::min(actual.size(),stores.size());++n)if(actual[n]!=stores[n]){std::cerr<<"first "<<n<<" pc "<<std::hex<<actual[n].instruction<<'/'<<stores[n].instruction<<" address "<<actual[n].address<<'/'<<stores[n].address<<" bits "<<actual[n].after<<'/'<<stores[n].after<<std::dec<<'\n';break;}throw std::runtime_error("Original source creation prefix mismatch");}
   for(const auto&[a,b]:expected){require(std::ranges::equal(b,storage->memory(a,b.size())),"Full creation region mismatch");bytes+=b.size();}
   ++calls;writes+=actual.size();
  }else if(q[0]=="ENDMAP"){require(!active&&initialized,"Incomplete map");++maps;}
  else throw std::runtime_error("Unknown original row");
 }
 require(calls==138&&maps==46&&cache_calls==calls,"Incomplete source creation suite");
 bool unavailable=false;try{halo2::source_sound_create(state,sound,association,0xa400000,promotion,constants,{0x027f,0x1f80},{});}catch(const std::runtime_error&){unavailable=true;}
 require(unavailable,"Missing memory supplied a result");
 std::cout<<"PASS "<<calls<<" original creation prefixes, "<<writes<<" ordered stores, "<<maps<<" image/map banks, "<<bytes<<" full region bytes, "<<cache_calls<<" exact cache requests stopped without results; no admission/voice/playback claim.\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
