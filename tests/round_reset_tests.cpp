#include "halo2/source_round_reset.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

using namespace halo2;
using Json=nlohmann::json;
namespace {
using Raw=std::vector<std::byte>;
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::uint32_t number(const Json &value){return std::uint32_t(std::stoul(value.get<std::string>(),nullptr,16));}
struct Fixture {
  const Json &data;std::map<std::string,Raw> decoded;
  const Raw &bytes(const Json &ref){
    const auto key=ref.at("raw_blob_sha256").get<std::string>();
    if(auto found=decoded.find(key);found!=decoded.end())return found->second;
    const auto hex=data.at("raw_blobs").at(key).get<std::string>();Raw result(hex.size()/2);
    for(unsigned i=0;i<result.size();++i)result[i]=std::byte(std::stoul(hex.substr(i*2,2),nullptr,16));
    return decoded.emplace(key,std::move(result)).first->second;
  }
};
void run(Fixture &f,const Json &version,const Json &record,bool absent=false){
  std::map<std::uint32_t,Raw> regions;
  for(const auto &r:record.at("before"))regions.emplace(number(r.at("address")),f.bytes(r.at("raw_hex")));
  SourceRoundResetServices s;
  s.memory=[&](std::uint32_t address,std::size_t size)->std::span<std::byte>{
    auto at=regions.upper_bound(address);check(at!=regions.begin(),"Missing original reset backing");--at;
    const auto offset=std::uint64_t(address)-at->first;
    check(offset+size<=at->second.size(),"Missing original reset extent");
    return std::span(at->second).subspan(std::size_t(offset),size);
  };
  const auto &j=version.at("bindings");
  SourceRoundResetBindings b{j.at("game_globals_slot"),j.at("players_slot"),j.at("objects_slot"),
      j.at("tags_slot"),j.at("mask_slot"),j.at("engine_globals_slot"),j.at("engine_table")};
  const auto omitted=record.at("omit").is_null()?std::string():record.at("omit").get<std::string>();
  std::vector<SourceRoundResetStore> writes;Json events=Json::array();
  auto read=[&](std::uint32_t p,unsigned n=4){auto raw=s.memory(p,n);std::uint32_t value=0;
    for(unsigned i=0;i<n;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);return value;};
  auto write=[&](std::uint32_t p,std::uint32_t value,unsigned n=4){
    const auto before=read(p,n);auto raw=s.memory(p,n);
    for(unsigned i=0;i<n;++i)raw[i]=std::byte(value>>(8*i));
    writes.push_back({p,before,value&(n==4?0xffffffffu:(1u<<(8*n))-1),n});
  };
  const auto event=[&](const char *name,std::initializer_list<std::uint32_t> args){
    events.push_back(Json{{"name",name},{"arguments",args}});
    if(omitted==name)throw std::runtime_error("Declared original service boundary");
  };
  // Explicit fixture mutations, shared with the declared original API leaves.
  // These callbacks do not implement actual native world/object teardown.
  constexpr std::uint32_t P=0x1100000,O=0x1110000,B=0x1120000,L=0x1180000;
  std::function<void(std::uint32_t)> retire=[&](std::uint32_t datum){
    for(unsigned i=0;i<16;++i){const auto h=O+0x4c+i*12;
      if(read(h,2)&&read(B+i*0x400+0x14)==datum)retire((read(h,2)<<16)|i);}
    const auto h=O+0x4c+(datum&65535u)*12;write(h,0,2);write(h+8,0);
    for(unsigned i=0;i<16;++i){const auto p=P+0x4c+i*0x21c;
      if(read(p+0x2c)==datum)write(p+0x2c,0xffffffffu);}
  };
  s.deactivate_object=[&](auto datum,bool flag){event("deactivate_object",{datum,std::uint32_t(flag)});};
  s.delete_object=[&](auto datum,bool flag){event("delete_object",{datum,std::uint32_t(flag)});retire(datum);};
  s.reset_decals=[&]{event("reset_decals",{});if(record.at("relocate_mask").get<bool>())write(b.mask_slot,L);};
  s.reset_contrails=[&]{event("reset_contrails",{});};s.reset_effects=[&]{event("reset_effects",{});};
  s.reset_registry=[&]{event("reset_registry",{});};s.reset_lights=[&]{event("reset_lights",{});};
  s.reset_sound=[&]{event("reset_sound",{});};s.place_scenario=[&]{event("place_scenario",{});};
  s.reset_scenario=[&](auto value){event("reset_scenario",{value});};
  s.scenario_callback=[&](auto value){event("scenario_callback",{value});};
  if(absent){
    if(omitted=="deactivate_object")s.deactivate_object={};else if(omitted=="delete_object")s.delete_object={};
    else if(omitted=="reset_decals")s.reset_decals={};else if(omitted=="reset_contrails")s.reset_contrails={};
    else if(omitted=="reset_effects")s.reset_effects={};else if(omitted=="reset_registry")s.reset_registry={};
    else if(omitted=="reset_lights")s.reset_lights={};else if(omitted=="reset_sound")s.reset_sound={};
    else if(omitted=="reset_scenario")s.reset_scenario={};else if(omitted=="scenario_callback")s.scenario_callback={};
    else if(omitted=="place_scenario")s.place_scenario={};else throw std::runtime_error("No original missing-service stop");
  }
  bool complete=true;
  try{source_round_reset(b,s,[&](const auto &store){writes.push_back(store);});}
  catch(const std::runtime_error &){if(omitted.empty())throw;complete=false;}
  check(complete==record.at("complete").get<bool>(),"Original round-reset completion differs");
  for(const auto &r:record.at("after"))check(regions.at(number(r.at("address")))==f.bytes(r.at("raw_hex")),
      "Complete original reset bank differs");
  std::vector<SourceRoundResetStore> expected;
  for(const auto &w:record.at("writes"))expected.push_back({number(w.at("address")),w.at("before"),w.at("after"),w.at("size")});
  check(writes==expected,"Original round-reset write order/width differs");
  Json expected_events=record.at("events");if(absent)expected_events.erase(expected_events.size()-1);
  check(events==expected_events,"Original round-reset service arguments/order differ");
}
} // namespace
int main(int argc,char **argv){
  try{
    check(argc==2,"usage: round_reset_tests original.json");std::ifstream input(argv[1]);
    check(bool(input),"Missing original round-reset fixture");Json data;input>>data;Fixture fixture{data,{}};
    unsigned cases=0,missing=0;
    for(const auto &[image,v]:data.at("versions").items())for(const auto &r:v.at("records")){
      try{run(fixture,v,r);if(!r.at("complete").get<bool>()){run(fixture,v,r,true);++missing;}}
      catch(const std::exception &e){throw std::runtime_error(image+" "+r.at("label").get<std::string>()+": "+e.what());}
      ++cases;
    }
    check(cases==228&&missing==33,"Original reset coverage changed; revalidate scope");
    bool stopped=false;try{source_round_reset({0,0,0,0,0,0,0},{});}catch(const std::runtime_error &){stopped=true;}
    check(stopped,"Missing reset memory must stop");
    std::cout<<"PASS "<<cases<<" complete/partial original reset records, ordered writes and services; "
        <<missing<<" absent-service checks; missing backing stops\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
