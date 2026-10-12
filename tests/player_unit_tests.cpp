#include "halo2/source_player_unit.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
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
  const Raw &bytes(const Json &reference){
    const auto key=reference.at("raw_blob_sha256").get<std::string>();
    if(const auto found=decoded.find(key);found!=decoded.end())return found->second;
    const auto hex=data.at("raw_blobs").at(key).get<std::string>();Raw result(hex.size()/2);
    for(unsigned i=0;i<result.size();++i)result[i]=std::byte(std::stoul(hex.substr(2*i,2),nullptr,16));
    return decoded.emplace(key,std::move(result)).first->second;
  }
};
void run(Fixture &fixture,const Json &version,const Json &record,bool leave_missing=false){
  std::map<std::uint32_t,Raw> regions;
  for(const auto &region:record.at("before"))regions.emplace(number(region.at("address")),fixture.bytes(region.at("raw_hex")));
  SourcePlayerUnitServices s;
  s.memory=[&](std::uint32_t address,std::size_t size)->std::span<std::byte>{
    auto at=regions.upper_bound(address);check(at!=regions.begin(),"Missing original player-unit backing");--at;
    const auto offset=std::uint64_t(address)-at->first;
    check(offset+size<=at->second.size(),"Missing original player-unit extent");
    return std::span(at->second).subspan(std::size_t(offset),size);
  };
  const auto omitted=record.at("omit").is_null()?std::string():record.at("omit").get<std::string>();
  Json events=Json::array();
  const auto event=[&](const char *name,std::initializer_list<std::uint32_t> args){
    events.push_back(Json{{"name",name},{"arguments",args}});
    if(omitted==name)throw std::runtime_error("Declared original service stop");
  };
  s.bind_view=[&](auto user,auto unit){event("bind_view",{std::uint32_t(user),unit});};
  s.bind_control=[&](auto user,auto unit,auto representation){event("bind_control",{std::uint32_t(user),unit,std::uint32_t(representation)});};
  s.reset_local_state=[&](auto user){event("reset_local_state",{std::uint32_t(user)});};
  s.refresh_user=[&](auto user){event("refresh_user",{std::uint32_t(user)});};
  s.mark_network_dirty=[&](auto entity,auto flags){event("mark_network_dirty",{entity,flags});};
  s.activate_unit=[&](auto unit,bool active){event("activate_unit",{unit,std::uint32_t(active)});};
  s.parameter_bits=[&](auto player,auto parameter){event("parameter_bits",{player,std::uint32_t(parameter)});return version.at("parameter_bits").get<std::uint32_t>();};
  s.forget_view_object=[&](auto object){event("forget_view_object",{object});};
  if(leave_missing){
    if(omitted=="bind_view")s.bind_view={};else if(omitted=="bind_control")s.bind_control={};
    else if(omitted=="reset_local_state")s.reset_local_state={};else if(omitted=="refresh_user")s.refresh_user={};
    else if(omitted=="mark_network_dirty")s.mark_network_dirty={};else if(omitted=="activate_unit")s.activate_unit={};
    else if(omitted=="parameter_bits")s.parameter_bits={};else if(omitted=="forget_view_object")s.forget_view_object={};
    else throw std::runtime_error("Missing-service check has no original stop");
  }
  const auto &j=version.at("bindings");
  SourcePlayerUnitBindings b{j.at("players_slot"),j.at("player_globals_slot"),j.at("objects_slot"),
      j.at("game_globals_slot"),j.at("engine_globals_slot"),j.at("engine_table"),
      version.at("retail").get<bool>()?SourcePlayerUnitVersion::Retail:SourcePlayerUnitVersion::TitleUpdate};
  std::vector<SourcePlayerUnitStore> writes;bool complete=true;
  auto observe=[&](const auto &store){writes.push_back(store);};
  try{
    if(record.at("kind")=="bind")bind_source_player_unit(b,record.at("datum"),record.at("new_unit"),s,observe);
    else release_source_object_player_references(b,record.at("datum"),s,observe);
  }catch(const std::runtime_error &){if(omitted.empty())throw;complete=false;}
  check(complete==record.at("complete").get<bool>(),"Original completion/stop differs");
  for(const auto &region:record.at("after"))check(regions.at(number(region.at("address")))==fixture.bytes(region.at("raw_hex")),
      "Complete original player-unit bank differs");
  std::vector<SourcePlayerUnitStore> expected;
  for(const auto &store:record.at("writes"))expected.push_back({number(store.at("address")),store.at("before"),store.at("after"),store.at("size")});
  check(writes==expected,"Original player-unit write order/width differs");
  Json expected_events=record.at("events");
  if(leave_missing)expected_events.erase(expected_events.size()-1);
  check(events==expected_events,"Original reached service order/arguments differ");
}
} // namespace
int main(int argc,char **argv){
  try{
    check(argc==2,"usage: player_unit_tests original.json");
    std::ifstream input(argv[1]);check(bool(input),"Missing original player-unit fixture");Json data;input>>data;
    Fixture fixture{data,{}};unsigned cases=0,missing=0;
    for(const auto &[image,version]:data.at("versions").items())for(const auto &record:version.at("records")){
      try{
        run(fixture,version,record);
        if(!record.at("complete").get<bool>()){run(fixture,version,record,true);++missing;}
      }catch(const std::exception &e){throw std::runtime_error(image+" "+record.at("label").get<std::string>()+": "+e.what());}
      ++cases;
    }
    check(cases==339 && missing==24,"Original player-unit coverage changed; revalidate scope");
    bool stopped=false;
    try{bind_source_player_unit({0,0,0,0,0,0,SourcePlayerUnitVersion::Retail},0,0,{});}
    catch(const std::runtime_error &){stopped=true;}
    check(stopped,"Missing retained player backing must stop");
    std::cout<<"PASS "<<cases<<" complete/partial original player-unit records, ordered writes and service calls; "
        <<missing<<" absent-service checks; missing backing stops\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
