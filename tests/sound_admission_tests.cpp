#include "halo2/sound_admission.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
std::uint32_t u(const std::string&s){return std::uint32_t(std::stoul(s,nullptr,16));}
void require(bool v,const char*s){if(!v)throw std::runtime_error(s);}
std::vector<std::string> fields(const std::string&s){std::vector<std::string>r;std::istringstream f(s);std::string q;while(std::getline(f,q,'\t'))r.push_back(q);return r;}
std::vector<std::byte> decode(const std::string&s){require(!(s.size()%2),"Literal bytes");std::vector<std::byte>b(s.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(u(s.substr(i*2,2)));return b;}
constexpr std::uint32_t request=0xa400000;
}
int main(int argc,char**argv)try{
 require(argc==2,"Usage: sound_admission_tests <fixtures.tsv>");std::ifstream f(argv[1]);require(bool(f),"Original admission fixtures unavailable");std::string line;require(bool(std::getline(f,line))&&line=="SOUND_ADMISSION_NATIVE_V1","Fixture schema");
 halo2::SoundAdmissionState state{};halo2::SoundAdmissionConstants constants{};halo2::SoundDriverParameterConstants parameters{};halo2::SoundAdmissionServices services;
 std::map<std::uint32_t,std::vector<std::byte>> initial,actual,expected;std::vector<halo2::SoundStartupStore> stores;
 std::uint32_t entry=0,sound=0,arg=0,result=0;bool reason=false,active=false;std::string name;
 std::size_t calls=0,writes=0,banks=0,bytes=0,cold=0,slices=0,queries=0;
 auto memory=[&](auto &regions,std::uint32_t at,std::size_t n)->std::span<std::byte>{auto i=regions.upper_bound(at);require(i!=regions.begin(),"Missing original address");--i;require(std::uint64_t(at)+n<=std::uint64_t(i->first)+i->second.size(),"Missing original extent");return std::span<std::byte>(i->second).subspan(at-i->first,n);};
 services.memory=[&](std::uint32_t at,std::size_t n){return memory(actual,at,n);};
 while(std::getline(f,line)){
  auto q=fields(line);if(q.empty())continue;
  if(q[0]=="K"){require(q.size()==13,"Twelve immutable words");constants={{u(q[1]),u(q[2]),u(q[3]),u(q[4]),u(q[5]),u(q[6]),u(q[7]),u(q[8]),u(q[9]),u(q[10]),u(q[11])},u(q[12])};}
  else if(q[0]=="P"){require(q.size()==17,"Sixteen existing driver words");parameters={u(q[1]),u(q[2]),u(q[3]),u(q[4]),u(q[5]),u(q[6]),u(q[7]),u(q[8]),u(q[9]),u(q[10]),u(q[11]),u(q[12]),u(q[13]),u(q[14]),u(q[15]),u(q[16])};}
  else if(q[0]=="M"){require(!active&&q.size()==12,"Retained source roots");initial.clear();state={u(q[3]),u(q[4]),u(q[5]),u(q[6]),u(q[7]),u(q[8]),u(q[9]),u(q[10]),u(q[11])};}
  else if(q[0]=="R"){require(q.size()==3,"Raw source region");require(initial.emplace(u(q[1]),decode(q[2])).second,"Duplicate region");}
  else if(q[0]=="C"){
   require(!active&&q.size()==8,"Original call");active=true;actual=initial;name=q[1];entry=u(q[2]);sound=u(q[3]);arg=u(q[4]);reason=q[5]=="1";result=u(q[6]);auto raw=decode(q[7]);require(raw.size()==160,"Full caller backing");std::ranges::copy(raw,memory(actual,request,160).begin());expected=actual;stores.clear();
  }else if(q[0]=="S"){require(active&&q.size()==6,"Original store");stores.push_back({u(q[1]),u(q[2]),unsigned(std::stoul(q[3])),u(q[4]),u(q[5])});}
  else if(q[0]=="E"){
   require(active,"Original end");active=false;for(const auto&s:stores){auto b=memory(expected,s.address,s.bytes);std::uint32_t before=0;for(unsigned n=0;n<s.bytes;++n)before|=std::to_integer<std::uint32_t>(b[n])<<(8*n);require(before==s.before,"Original before bits");for(unsigned n=0;n<s.bytes;++n)b[n]=std::byte(s.after>>(8*n));}
   std::vector<halo2::SoundStartupStore> observed;auto observer=[&](const auto&s){observed.push_back(s);};std::uint32_t got;
   if(entry==0x128380){got=halo2::source_sound_admit(state,sound,request,request+0x88,reason?request+0x8c:0,constants,parameters,{0x027f,0x1f80},services,observer);++cold;require(!got,"Unavailable source was admitted");}
   else if(entry==0x1283f4){got=halo2::source_sound_admission_skip_slice(state,sound,request+12,constants,{0x027f,0x1f80},services,observer);++slices;}
   else{got=halo2::source_sound_admission_query(state,entry,sound,request+12,arg,constants,parameters,{0x027f,0x1f80},services,observer);++queries;}
   if(got!=result||observed!=stores){std::cerr<<"Case "<<calls<<' '<<name<<" entry "<<std::hex<<entry<<" result "<<got<<'/'<<result<<std::dec<<" writes "<<observed.size()<<'/'<<stores.size()<<'\n';throw std::runtime_error("Original admission owner mismatch");}
   for(const auto&[a,b]:expected){require(std::ranges::equal(b,memory(actual,a,b.size())),"Complete source bank mismatch");bytes+=b.size();}++calls;writes+=observed.size();
  }else if(q[0]=="ENDMAP"){require(!active,"Incomplete bank");++banks;}
  else throw std::runtime_error("Unknown original fixture row");
 }
 require(calls==3864&&banks==46&&cold==276&&slices==138,"Incomplete original admission suite");
 bool unavailable=false;try{halo2::source_sound_admit(state,sound,request,request+0x88,request+0x8c,constants,parameters,{0x027f,0x1f80},{});}catch(const std::runtime_error&){unavailable=true;}require(unavailable,"Missing backing produced admission");
 std::cout<<"PASS "<<calls<<" original controls: "<<cold<<" genuine unavailable full owners, "<<slices<<" explicit RNG/chance slices, "<<queries<<" direct queries; "<<writes<<" ordered stores, "<<bytes<<" full bytes across "<<banks<<" image/map banks; no successful admission or playback claim.\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
