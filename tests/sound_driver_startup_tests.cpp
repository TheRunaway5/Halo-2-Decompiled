#include "halo2/sound_driver_startup.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/mman.h>
using namespace halo2;
namespace {
void check(bool v,const std::string &s){if(!v)throw std::runtime_error(s);}
std::uint32_t number(const std::string &s){return std::uint32_t(std::stoul(s,nullptr,16));}
std::vector<std::byte> bytes(const std::string &s){check(s.size()%2==0,"odd hex");std::vector<std::byte> b(s.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(number(s.substr(2*i,2)));return b;}
std::array<std::uint32_t,11> fields(const SoundDriverStartupState &s){return {s.arena_base,s.arena_used,s.arena_crc,s.driver_address,s.environment_address,s.runtime_environment_parameters_address,s.hardware_queue_address,s.hardware_queue_flag,s.queue_4865dc,s.queue_budget,s.queue_4865ec};}
struct Case {std::string image;std::array<std::byte,16> policy;std::map<std::string,SoundDriverStartupState> states;std::vector<std::byte> input,output;std::vector<SoundDriverStartupStore> stores;std::array<std::uint32_t,3> request;};
struct PlatformBoundary {};
struct Storage {
 std::uint32_t alias;std::size_t extent;std::byte *data;
 Storage(std::uint32_t a,std::span<const std::byte> b):alias(a),extent(b.size()){auto p=mmap(nullptr,extent,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);check(p!=MAP_FAILED,"actual host mmap failed");data=static_cast<std::byte*>(p);std::copy(b.begin(),b.end(),data);}
 ~Storage(){munmap(data,extent);}
 std::span<std::byte> memory(std::uint32_t a,std::size_t n){if(a<alias||std::uint64_t(a-alias)+n>extent)throw std::runtime_error("actual source raw span unavailable");return {data+(a-alias),n};}
};
}
int main(int argc,char **argv){try{
 check(argc==2,"usage: sound_driver_startup_tests original-fixtures.tsv");std::ifstream f(argv[1]);check(bool(f),"source fixtures unavailable");std::map<std::string,SoundDriverStartupConstants> constants;std::map<std::string,Case> cases;std::string line;
 while(std::getline(f,line)){
  if(line.empty()||line[0]=='#')continue;
  std::vector<std::string> q;std::stringstream input(line);std::string v;while(std::getline(input,v,'\t'))q.push_back(v);
  if(q[0]=="constants"){SoundDriverStartupConstants c{};c.one_bits=number(q[2]);auto forward=bytes(q[3]),up=bytes(q[4]);check(forward.size()==12&&up.size()==12,"immutable source vector extent");std::copy(forward.begin(),forward.end(),c.basis_forward.begin());std::copy(up.begin(),up.end(),c.basis_up.begin());c.routing_plugin_identity=number(q[5]);constants.emplace(q[1],c);}
  else if(q[0]=="case"){Case c{};c.image=q[2];auto b=bytes(q[3]);check(b.size()==16,"source policy extent");std::copy(b.begin(),b.end(),c.policy.begin());cases.emplace(q[1],c);}
  else if(q[0]=="state"){check(q.size()==14,"state field extent");cases.at(q[1]).states.emplace(q[2],SoundDriverStartupState{number(q[3]),number(q[4]),number(q[5]),number(q[6]),number(q[7]),number(q[8]),number(q[9]),std::uint8_t(number(q[10])),number(q[11]),number(q[12]),number(q[13])});}
  else if(q[0]=="input")cases.at(q[1]).input=bytes(q[2]);
  else if(q[0]=="output")cases.at(q[1]).output=bytes(q[2]);
  else if(q[0]=="store")cases.at(q[1]).stores.push_back({number(q[2]),number(q[3]),unsigned(std::stoul(q[4])),number(q[5]),number(q[6])});
  else if(q[0]=="request")cases.at(q[1]).request={number(q[2]),number(q[3]),number(q[4])};
  else throw std::runtime_error("unknown source row");
 }
 std::size_t total=0;
 for(const auto &[name,fixture]:cases){auto state=fixture.states.at("before");Storage storage(state.arena_base,fixture.input);std::vector<SoundDriverStartupStore> stores;unsigned requests=0;SoundDriverStartupServices services;
  services.memory=[&](std::uint32_t a,std::size_t n){return storage.memory(a,n);};
  services.native_device_create=[&](const SoundDriverDeviceCreateRequest &r)->std::int32_t{++requests;check(std::array<std::uint32_t,3>{r.guid_address,r.output_address,r.outer_identity}==fixture.request,name+" original device request differs");check(fields(state)==fields(fixture.states.at("after")),name+" complete original globals differ");check(std::equal(fixture.output.begin(),fixture.output.end(),storage.data),name+" entire source arena differs");throw PlatformBoundary{};};
  bool stopped=false;try{source_sound_driver_startup(state,fixture.policy,constants.at(fixture.image),services,[&](const auto &q){stores.push_back(q);});}catch(const PlatformBoundary &){stopped=true;}
  check(stopped&&requests==1,"original device boundary bypassed");check(stores==fixture.stores,name+" all original ordered writes differ");total+=stores.size();
  check(storage.memory(state.driver_address,1)[0]==std::byte{0},"driver initialized before native device");
  check(std::all_of(storage.memory(state.driver_address+0x2ab0,4).begin(),storage.memory(state.driver_address+0x2ab0,4).end(),[](auto b){return b==std::byte{0};}),"device identity fabricated");
  // Unavailable native constructor produces no chosen HRESULT/identity. The
  // exact already-constructed source buffers and globals are retained.
  state=fixture.states.at("before");std::copy(fixture.input.begin(),fixture.input.end(),storage.data);stores.clear();services.native_device_create={};bool missing=false;
  try{source_sound_driver_startup(state,fixture.policy,constants.at(fixture.image),services,[&](const auto &q){stores.push_back(q);});}catch(const std::runtime_error&){missing=true;}
  check(missing&&fields(state)==fields(fixture.states.at("after"))&&stores==fixture.stores,"missing native constructor invented a source result");check(std::equal(fixture.output.begin(),fixture.output.end(),storage.data),"missing provider altered original prefix bytes");
 }
 std::cout<<"PASS original2205C0/224EC0 allocation prefixes: "<<cases.size()<<" cases, "<<total<<" exact ordered stores, complete arenas/globals, all four alignment residues, real mmap backing and exact unavailable device requests; no saved-setting/caps/DSP/stream/plugin/admission return\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
