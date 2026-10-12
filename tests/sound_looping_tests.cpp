#include "halo2/sound_looping.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
namespace {
std::uint32_t number(const std::string &s){return std::uint32_t(std::stoul(s,nullptr,16));}
void require(bool v,const char *s){if(!v)throw std::runtime_error(s);}
std::vector<std::string> fields(const std::string &s){std::istringstream f(s);std::string q;std::vector<std::string> r;while(std::getline(f,q,'\t'))r.push_back(q);return r;}
std::vector<std::byte> decode(const std::string &s){require(s.size()%2==0,"Literal source bytes");std::vector<std::byte> r(s.size()/2);for(std::size_t i=0;i<r.size();++i)r[i]=std::byte(number(s.substr(i*2,2)));return r;}
std::string text(const std::string &s){auto raw=decode(s);require(!raw.empty()&&raw.back()==std::byte{},"Literal terminated source string");return std::string(reinterpret_cast<const char*>(raw.data()),raw.size()-1);}
}
int main(int argc,char **argv)try{
 require(argc==2,"Usage: sound_looping_tests <fixtures.tsv>");std::ifstream f(argv[1]);require(bool(f),"Original looping fixtures unavailable");std::string line;require(bool(std::getline(f,line))&&line=="SOUND_LOOPING_NATIVE_V1","Fixture schema");
 halo2::SoundLoopingBindings bindings{};halo2::SoundLoopingConstants constants{};halo2::SoundDriverMasterGainConstants gain{};halo2::SoundLoopingServices services;
 std::map<std::uint32_t,std::vector<std::byte>> initial,actual,expected;std::vector<halo2::SoundDriverStartupStore> stores;std::vector<std::uint32_t> arguments;
 std::size_t calls=0,writes=0,bytes=0,banks=0,names=0,ordinal=0,stops=0,registrations=0;std::uint32_t entry=0,result=0;bool sequential=false,returned=false,active=false;std::string name,selector;
 auto memory=[&](auto &regions,std::uint32_t at,std::size_t n)->std::span<std::byte>{auto i=regions.upper_bound(at);if(i==regions.begin())throw std::runtime_error("Missing original loop address");--i;if(std::uint64_t(at)+n>std::uint64_t(i->first)+i->second.size())throw std::runtime_error("Missing original loop extent");return std::span<std::byte>(i->second).subspan(at-i->first,n);};
 services.memory=[&](std::uint32_t a,std::size_t n){return memory(actual,a,n);};
 while(std::getline(f,line)){
  const auto q=fields(line);if(q.empty())continue;
  if(q[0]=="BANK"){require(!active&&q.size()==16,"Retained source bindings");initial.clear();names=ordinal=0;sequential=q[3]=="1";bindings={number(q[4]),number(q[5]),number(q[6]),number(q[7]),number(q[8]),number(q[9]),number(q[10]),number(q[11]),number(q[12]),number(q[13]),number(q[14]),number(q[15])};}
  else if(q[0]=="K"){require(q.size()==7,"Literal immutable loop operands");constants.one_bits=number(q[1]);constants.minimum_db_bits=number(q[2]);constants.game_allocator_method=number(q[3]);constants.pool_name=text(q[4]);constants.empty_selector=text(q[5]);constants.dialogue_selector=text(q[6]);}
  else if(q[0]=="N"){require(q.size()==2&&names<54,"Literal class name");constants.class_names[names++]=text(q[1]);}
  else if(q[0]=="P"){require(q.size()==4,"Existing master gain operands");gain={number(q[1]),number(q[2]),number(q[3])};}
  else if(q[0]=="R"){require(q.size()==3,"Whole retained region");require(initial.emplace(number(q[1]),decode(q[2])).second,"Duplicate source region");}
  else if(q[0]=="C"){require(!active&&q.size()>=6&&names==54,"Original loop call");active=true;name=q[1];entry=number(q[2]);returned=q[3]=="1";result=number(q[4]);selector=q[5]=="-"?std::string{}:text(q[5]);arguments.clear();for(std::size_t i=6;i<q.size();++i)arguments.push_back(number(q[i]));if(!sequential||!ordinal)actual=initial;expected=actual;stores.clear();}
  else if(q[0]=="S"){require(active&&q.size()==6,"Original ordered store");stores.push_back({number(q[1]),number(q[2]),unsigned(std::stoul(q[3])),number(q[4]),number(q[5])});}
  else if(q[0]=="E"){
   require(active,"Original call end");active=false;for(const auto &s:stores){auto out=memory(expected,s.address,s.bytes);std::uint32_t before=0;for(unsigned i=0;i<s.bytes;++i)before|=std::to_integer<std::uint32_t>(out[i])<<(8*i);require(before==s.before,"Original store before bits");for(unsigned i=0;i<s.bytes;++i)out[i]=std::byte(s.after>>(8*i));}
   std::vector<halo2::SoundDriverStartupStore> observed;auto observer=[&](const auto &s){observed.push_back(s);};bool did_return=true;std::uint32_t got=0;
   try{
    if(entry==0x18a200)halo2::source_sound_looping_construct(bindings,constants,services,observer);
    else if(entry==0x18a2a0)halo2::source_sound_looping_reset(bindings,constants,gain,{0x027f,0x1f80},services,observer);
    else if(entry==0x223960)halo2::source_sound_looping_class_construct(bindings,services,observer);
    else if(entry==0x2239b0)halo2::source_sound_looping_class_reset(bindings,services,observer);
    else if(entry==0x18a790)halo2::source_sound_looping_engine_reset(bindings,constants,services,observer);
    else if(entry==0x18a9c0){require(arguments.size()==4,"Raw registration ABI");got=halo2::source_sound_looping_register(bindings,arguments[0],arguments[1],std::uint16_t(arguments[2]),arguments[3],services,observer);++registrations;}
    else if(entry==0x223a90){require(arguments.size()==3,"Raw class ABI");halo2::source_sound_looping_class_gain(bindings,constants,selector,arguments[1],arguments[2],services,observer);}
    else throw std::runtime_error("Unknown original owner");
   }catch(const std::runtime_error &error){did_return=false;if(returned)std::cerr<<"Unexpected source stop: "<<error.what()<<'\n';}
   if(did_return!=returned||got!=result||observed!=stores){std::cerr<<"Case "<<calls<<' '<<name<<" entry "<<std::hex<<entry<<" result "<<got<<'/'<<result<<std::dec<<" return "<<did_return<<'/'<<returned<<" writes "<<observed.size()<<'/'<<stores.size()<<'\n';for(std::size_t i=0;i<std::min(observed.size(),stores.size());++i)if(observed[i]!=stores[i]){const auto &a=observed[i],&e=stores[i];std::cerr<<"Store "<<i<<std::hex<<" observed "<<a.instruction<<' '<<a.address<<' '<<a.bytes<<' '<<a.before<<' '<<a.after<<" expected "<<e.instruction<<' '<<e.address<<' '<<e.bytes<<' '<<e.before<<' '<<e.after<<'\n';break;}throw std::runtime_error("Original looping owner mismatch");}
   for(const auto &[a,b]:expected){require(std::ranges::equal(b,memory(actual,a,b.size())),"Whole retained source buffer mismatch");bytes+=b.size();}++calls;++ordinal;writes+=observed.size();stops+=!did_return;
  }else if(q[0]=="ENDBANK"){require(!active,"Incomplete source bank");++banks;}
  else throw std::runtime_error("Unknown loop fixture row");
 }
 require(calls>2400&&registrations==2056&&stops==2&&banks==17,"Incomplete original loop owner suite");
 bool missing=false;try{halo2::source_sound_looping_register(bindings,0,0,0,0,{});}catch(const std::runtime_error&){missing=true;}require(missing,"Missing backing produced a registration");
 std::cout<<"PASS "<<calls<<" original controls, "<<registrations<<" registrations, "<<writes<<" ordered stores, "<<bytes<<" full bytes across "<<banks<<" banks; "<<stops<<" actual absent-driver stops, no allocation service callbacks or audio admission supplied.\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
