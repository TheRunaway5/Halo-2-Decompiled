#include "halo2/sound_map_activation.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace halo2;
namespace {
void check(bool value,const std::string &text){if(!value)throw std::runtime_error(text);}
std::uint32_t number(const std::string &s){return std::uint32_t(std::stoul(s,nullptr,16));}
std::vector<std::byte> bytes(const std::string &s){check(s.size()%2==0,"odd byte input");std::vector<std::byte> b(s.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(number(s.substr(2*i,2)));return b;}
std::array<std::uint32_t,8> fields(const SoundMapActivationState &s){return {s.current_matg_address,s.sound_system_address,s.current_ugh_address,s.driver_address,s.environment_address,s.options_address,s.runtime_environment_parameters_address,s.queue_budget};}
struct Region {std::uint32_t address;bool mutable_backing;std::vector<std::byte> data;};
struct MemoryBoundary {std::uint32_t address;std::size_t extent;};
struct Case {std::map<std::string,SoundMapActivationState> states;std::vector<Region> regions;std::map<std::uint16_t,std::uint32_t> tags;std::vector<std::uint32_t> clocks;std::vector<SoundMapActivationStore> stores;std::vector<std::byte> output;std::uint32_t stop_pc,stop_address;std::size_t stop_extent;};
struct World {
 std::vector<Region> regions;
 std::span<std::byte> memory(std::uint32_t address,std::size_t extent,bool write=false){
  for(auto &r:regions)if(address>=r.address&&std::uint64_t(address-r.address)+extent<=r.data.size()){
   check(!write||r.mutable_backing,"attempted readonly source metadata mutation");return {r.data.data()+(address-r.address),extent};
  }
  throw MemoryBoundary{address,extent};
 }
};
}
int main(int argc,char **argv){try{
 check(argc==2,"usage: sound_map_activation_tests original-fixtures.tsv");std::ifstream f(argv[1]);check(bool(f),"source fixture unavailable");std::string line;SoundMapActivationConstants constants{};std::map<std::string,Case> cases;
 while(std::getline(f,line)){
  if(line.empty()||line[0]=='#')continue;
  std::vector<std::string> q;std::stringstream in(line);std::string v;while(std::getline(in,v,'\t'))q.push_back(v);
  if(q[0]=="constants")constants={number(q[1]),number(q[2])};
  else if(q[0]=="case")cases.emplace(q[1],Case{});
  else if(q[0]=="state"){
   check(q.size()==11,"state field count");cases.at(q[1]).states.emplace(q[2],SoundMapActivationState{number(q[3]),number(q[4]),number(q[5]),number(q[6]),number(q[7]),number(q[8]),number(q[9]),number(q[10])});
  }else if(q[0]=="region")cases.at(q[1]).regions.push_back({number(q[2]),q[3]=="1",bytes(q[4])});
  else if(q[0]=="output")cases.at(q[1]).output=bytes(q[2]);
  else if(q[0]=="tag")check(cases.at(q[1]).tags.emplace(std::uint16_t(number(q[2])),number(q[3])).second,"duplicate literal tag index");
  else if(q[0]=="clock")cases.at(q[1]).clocks.push_back(number(q[2]));
  else if(q[0]=="store")cases.at(q[1]).stores.push_back({number(q[2]),number(q[3]),unsigned(std::stoul(q[4])),number(q[5]),number(q[6])});
  else if(q[0]=="stop"){auto &c=cases.at(q[1]);c.stop_pc=number(q[2]);c.stop_address=number(q[3]);c.stop_extent=std::stoul(q[4]);}
  else throw std::runtime_error("unknown source row");
 }
 std::size_t total=0,clock_count=0;
 for(const auto &[name,c]:cases){
  auto state=c.states.at("entry");World world{c.regions};std::vector<SoundMapActivationStore> stores;std::size_t clocks=0;unsigned platform_requests=0;
  SoundMapActivationServices services;
  services.read=[&](std::uint32_t a,std::size_t n)->std::span<const std::byte>{return world.memory(a,n);};
  services.writable=[&](std::uint32_t a,std::size_t n){return world.memory(a,n,true);};
  services.tag_address=[&](std::uint16_t index){auto q=c.tags.find(index);check(q!=c.tags.end(),"literal tag-table input unavailable");return q->second;};
  services.tick_milliseconds=[&]{check(clocks<c.clocks.size(),"extra source clock request");return c.clocks[clocks++];};
  // These negative providers never supply a driver or callback completion.
  auto unavailable=[&]{++platform_requests;throw std::runtime_error("unavailable native routing/effect owner");};
  services.native_plugin_release=[&](auto){unavailable();};services.native_plugin_initialize=[&](const auto&){unavailable();};services.native_plugin_environment=[&](const auto&){unavailable();};services.native_set_effect_data=[&](const auto&){unavailable();};
  bool stopped=false;
  try{source_sound_map_activation(state,constants,services,[&](const auto &s){stores.push_back(s);});}
  catch(const MemoryBoundary &b){stopped=true;check(b.address==c.stop_address&&b.extent==c.stop_extent,"wrong original unavailable backing boundary");}
  check(stopped&&platform_requests==0,"source driver stop bypassed or callback success supplied");
  check(fields(state)==fields(c.states.at("stopped")),"original retained globals differ");
  check(stores==c.stores,name+" ordered original prefix stores differ");
  const auto data=world.memory(state.sound_system_address,c.output.size());check(std::equal(c.output.begin(),c.output.end(),data.begin()),name+" whole source startup arena differs");
  check(clocks==c.clocks.size(),"source clock count differs");
  check(world.memory(state.sound_system_address+0x78,1)[0]==std::byte{0},"unavailable driver initialization mutated");
  check(state.driver_address==0,"source driver identity supplied");
  total+=stores.size();clock_count+=clocks;
  // Missing clock must leave exactly the source-produced three preclock writes,
  // without using+7A as a completed activation flag.
  if(!c.clocks.empty()){
   state=c.states.at("entry");world.regions=c.regions;stores.clear();services.tick_milliseconds={};bool missing=false;
   try{source_sound_map_activation(state,constants,services,[&](const auto &s){stores.push_back(s);});}catch(const std::runtime_error&){missing=true;}
   check(missing&&stores.size()==3&&std::equal(stores.begin(),stores.end(),c.stores.begin()),"missing clock invented completion or changed preclock order");
   check(world.memory(state.sound_system_address+0x7a,1)[0]==std::byte{1}&&world.memory(state.sound_system_address+0x78,1)[0]==std::byte{0},"partial flags must remain distinct");
  }
 }
 std::cout<<"PASS original126DE0 prefix: "<<cases.size()<<" cases, "<<total<<" exact ordered stores, "<<clock_count<<" recorded real clock inputs, complete source arenas/globals and actual driver/metadata stops; no map/plugin/effect/voice admission\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
