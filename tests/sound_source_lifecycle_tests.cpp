#include "halo2/sound_source_lifecycle.hpp"
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
void check(bool ok,const std::string &message){if(!ok)throw std::runtime_error(message);}
using Raw=std::vector<std::byte>;
struct Write {
  std::uint32_t address;unsigned size;std::uint32_t before,after;
  friend bool operator==(const Write &,const Write &)=default;
};
std::uint32_t number(const Json &v){return std::uint32_t(std::stoul(v.get<std::string>(),nullptr,16));}
struct Fixture {
  const Json &data;std::map<std::string,Raw> blobs;
  const Raw &bytes(const Json &reference){
    const auto key=reference.at("raw_blob_sha256").get<std::string>();
    if(const auto found=blobs.find(key);found!=blobs.end())return found->second;
    const auto hex=data.at("raw_blobs").at(key).get<std::string>();Raw out(hex.size()/2);
    for(unsigned i=0;i<out.size();++i)out[i]=std::byte(std::stoul(hex.substr(2*i,2),nullptr,16));
    return blobs.emplace(key,std::move(out)).first->second;
  }
};
void run(Fixture &fixture,const Json &version,const Json &record){
  std::map<std::uint32_t,Raw> pages;
  for(const auto &region:record.at("before"))pages.emplace(number(region.at("address")),fixture.bytes(region.at("raw_hex")));
  const auto &b=version.at("bindings");
  SoundSourceLifecycleBindings bindings{b.at("system"),b.at("sources"),b.at("static_voices"),b.at("controllers"),b.at("effects"),
    b.at("tag_table"),b.at("ugh"),b.at("cache_pool"),b.at("shared_voices"),b.at("driver")};
  SoundSourceLifecycleServices services;
  services.memory=[&](std::uint32_t address,std::size_t size)->std::span<std::byte>{
    auto at=pages.upper_bound(address);if(at==pages.begin())throw std::runtime_error("Missing original raw backing");--at;
    const auto offset=std::uint64_t(address)-at->first;
    if(offset+size>at->second.size())throw std::runtime_error("Missing original raw extent");
    return std::span(at->second).subspan(std::size_t(offset),size);
  };
  // No source callback or platform result is supplied. The original fixture
  // retains those stops rather than inventing successful device/voice work.
  std::vector<Write> writes;bool stopped=false;
  auto observe=[&](const SoundStartupStore &w){writes.push_back({w.address,w.bytes,w.before,w.after});};
  try{
    if(record.at("input").value("direct",false))source_sound_stop(bindings,number(record.at("source")),
      record.at("input").value("reason",10u),services,observe);
    else source_sound_round_reset(bindings,services,observe);
  }catch(const std::runtime_error &){stopped=true;}
  check(stopped==(record.at("outcome")=="stopped_original_instruction"),"Original completion/stop differs");
  for(const auto &region:record.at("after"))check(pages.at(number(region.at("address")))==fixture.bytes(region.at("raw_hex")),
    "Complete original raw output differs");
  std::vector<Write> expected;
  for(const auto &w:record.at("writes"))expected.push_back({number(w.at("address")),w.at("size"),w.at("before"),w.at("after")});
  if(writes!=expected){
    for(unsigned i=0;i<std::min(writes.size(),expected.size());++i)if(writes[i]!=expected[i]){
      const auto a=writes[i],b=expected[i];std::cerr<<"write "<<i<<" native "<<std::hex<<a.address<<' '<<a.size<<' '<<a.before<<' '<<a.after
        <<" original "<<b.address<<' '<<b.size<<' '<<b.before<<' '<<b.after<<std::dec<<'\n';break;
    }
    throw std::runtime_error("Original write order differs: native "+std::to_string(writes.size())+" original "+std::to_string(expected.size()));
  }
}
}
int main(int argc,char **argv){
  try{
    check(argc==2,"usage: sound_source_lifecycle_tests original.json");std::ifstream input(argv[1]);check(bool(input),"Missing original fixture");
    Json data;input>>data;Fixture fixture{data,{}};unsigned cases=0,stops=0;
    for(const auto &[image,version]:data.at("versions").items())for(const auto &record:version.at("records")){
      try{run(fixture,version,record);}catch(const std::exception &e){throw std::runtime_error(image+" case "+std::to_string(cases%version.at("records").size())+": "+e.what());}
      ++cases;stops+=record.at("outcome")=="stopped_original_instruction";
    }
    check(cases==225 && stops==9,"Original fixture coverage changed; revalidate boundaries");
    std::cout<<"PASS "<<cases<<" whole original cleanup records/full buffers/ordered writes, including "<<stops<<" retained dependency stops\n";
    return 0;
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
