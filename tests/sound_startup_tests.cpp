#include "halo2/sound_startup.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace halo2;
namespace {
struct Stage {SoundStartupState state;std::vector<std::byte> arena;};
void check(bool result,const std::string &message) {if(!result)throw std::runtime_error(message);}
std::uint32_t number(const std::string &s) {return std::uint32_t(std::stoul(s,nullptr,16));}
std::vector<std::byte> bytes(const std::string &hex) {
  check(hex.size()%2==0,"odd hex");std::vector<std::byte> out(hex.size()/2);
  for(std::size_t i=0;i<out.size();++i)out[i]=std::byte(number(hex.substr(2*i,2)));
  return out;
}
std::vector<std::uint32_t> fields(const SoundStartupState &s) {
  return {s.references,s.arena_base,s.arena_used,s.arena_crc,s.allocator_identity,s.system,
    s.sources,s.static_voices,s.controllers,s.controller_root,s.looping,s.effects,s.shared_voices};
}
struct PlatformBoundary {};
}
int main(int argc,char **argv) {
 try {
  check(argc==2,"usage: sound_startup_tests original-fixtures.tsv");std::ifstream f(argv[1]);check(bool(f),"missing source fixtures");
  SoundStartupConstants constants;std::array<std::uint32_t,3> clock{};
  std::map<std::string,Stage> stages;std::map<std::string,std::vector<SoundStartupStore>> source_stores;
  std::string line;
  while(std::getline(f,line)) {
    if(line.empty()||line[0]=='#')continue;
    std::vector<std::string> q;std::stringstream input(line);std::string v;while(std::getline(input,v,'\t'))q.push_back(v);
    if(q[0]=="constants") {
      constants.ambient_transition_bits=number(q[1]);const auto p=bytes(q[2]);check(p.size()==16,"policy extent");std::copy(p.begin(),p.end(),constants.voice_policy.begin());
    }else if(q[0]=="controller") {for(unsigned i=0;i<3;++i)clock[i]=number(q[i+1]);}
    else if(q[0]=="stage") {
      check(q.size()==16,"stage fields");SoundStartupState s{number(q[2]),number(q[3]),number(q[4]),number(q[5]),number(q[6]),number(q[7]),number(q[8]),number(q[9]),number(q[10]),number(q[11]),number(q[12]),number(q[13]),number(q[14])};stages.emplace(q[1],Stage{s,bytes(q[15])});
    }else if(q[0]=="store")source_stores[q[1]].push_back({number(q[2]),number(q[3]),unsigned(std::stoul(q[4])),number(q[5]),number(q[6])});
    else throw std::runtime_error("unknown fixture row");
  }
  const auto &initial=stages.at("entry");auto state=initial.state;auto arena=initial.arena;
  std::vector<SoundStartupStore> actual;std::vector<unsigned> calls;
  SoundStartupServices services;
  services.memory=[&](std::uint32_t address,std::size_t extent)->std::span<std::byte>{
    check(address>=state.arena_base && std::uint64_t(address-state.arena_base)+extent<=arena.size(),"missing real arena backing");
    return std::span(arena).subspan(address-state.arena_base,extent);
  };
  auto compare=[&](const std::string &name){const auto &expected=stages.at(name);check(fields(state)==fields(expected.state),name+" globals");check(arena==expected.arena,name+" complete raw arena");};
  services.cache_initialize=[&](SoundStartupState &owner){check(&owner==&state,"retained owner identity");calls.push_back(0);compare("before_cache");check(fields(stages.at("before_cache").state)==fields(stages.at("after_cache").state) && stages.at("before_cache").arena==stages.at("after_cache").arena,"original cache dependency has unexpected effects on startup-owned state");};
  services.utc_seconds=[&]{calls.push_back(1);return clock[0];};
  services.crt_random=[&]{calls.push_back(2);return clock[1];};
  services.tick_milliseconds=[&]{calls.push_back(3);return clock[2];};
  services.native_platform_initialize=[&](std::span<const std::byte,16> policy)->bool{
    calls.push_back(4);check(std::equal(policy.begin(),policy.end(),constants.voice_policy.begin()),"exact original platform policy request");compare("platform_request");throw PlatformBoundary{};
  };
  auto observe=[&](const SoundStartupStore &store){actual.push_back(store);};
  bool stopped=false;try{source_sound_startup(state,constants,services,observe);}catch(const PlatformBoundary &){stopped=true;}
  check(stopped,"unsupported platform boundary must not return");compare("platform_request");
  check(calls==std::vector<unsigned>({0,1,2,3,4}),"source constructor/cache/time/CRT/tick/platform call order");
  std::set<std::uint32_t> represented;for(const auto &q:actual)if(q.instruction)represented.insert(q.instruction);
  auto filtered=[&](const auto &rows){std::vector<SoundStartupStore> out;for(const auto &q:rows)if(represented.contains(q.instruction))out.push_back(q);return out;};
  const auto native_prefix=filtered(actual),original_prefix=filtered(source_stores.at("prefix"));
  if(native_prefix!=original_prefix) {
    for(std::size_t i=0;i<std::min(native_prefix.size(),original_prefix.size());++i)if(native_prefix[i]!=original_prefix[i]) {
      const auto a=native_prefix[i],b=original_prefix[i];std::cerr<<"store mismatch "<<i<<" native "<<std::hex<<a.instruction<<' '<<a.address<<' '<<a.bytes<<' '<<a.before<<' '<<a.after<<" original "<<b.instruction<<' '<<b.address<<' '<<b.bytes<<' '<<b.before<<' '<<b.after<<std::dec<<'\n';break;
    }
    throw std::runtime_error("original CPU prefix ordered stores: native "+std::to_string(native_prefix.size())+" original "+std::to_string(original_prefix.size()));
  }
  actual.clear();source_sound_startup_commit_fragment(state,constants,services,observe);compare("commit_fragment");
  check(actual==source_stores.at("commit"),"complete original commit-fragment ordered stores");
  // Missing cache is unavailable and retains the actual before-cache phase;
  // it cannot become successful startup, false platform return or ref increment.
  state=initial.state;arena=initial.arena;services.cache_initialize={};bool missing=false;
  try{source_sound_startup(state,constants,services);}catch(const std::runtime_error &){missing=true;}
  check(missing,"missing cache owner did not stop");compare("before_cache");
  // Already-retained references take126D3C without reading any provider or
  // implying source+78 or map+7A. This is a declared native branch control.
  state.references=1;const auto before=arena;source_sound_startup(state,constants,{});
  check(state.references==2 && arena==before,"repeated-reference source branch");
  std::cout<<"PASS original CPU startup prefix/full arena, "<<native_prefix.size()<<" ordered represented stores; "<<source_stores.at("commit").size()<<" complete commit stores; real platform boundary retained; no admitted original playback\n";
  return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
