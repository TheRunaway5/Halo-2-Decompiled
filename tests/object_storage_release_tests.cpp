#include "halo2/object_storage_release.hpp"
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
void run(Fixture &fixture,const Json &version,const Json &record){
  std::map<std::uint32_t,Raw> regions;
  for(const auto &region:record.at("before"))regions.emplace(number(region.at("address")),fixture.bytes(region.at("raw_hex")));
  SourceObjectStorageReleaseServices services;
  services.memory=[&](std::uint32_t address,std::size_t size)->std::span<std::byte>{
    auto at=regions.upper_bound(address);check(at!=regions.begin(),"Missing original release backing");--at;
    const auto offset=std::uint64_t(address)-at->first;
    check(offset+size<=at->second.size(),"Missing original release extent");
    return std::span(at->second).subspan(std::size_t(offset),size);
  };
  const auto &b=version.at("bindings");
  SourceObjectStorageReleaseBindings bindings{b.at("object_pool_slot"),b.at("body_heap_slot")};
  std::vector<SourceObjectStorageReleaseStore> writes;
  release_source_object_storage(bindings,record.at("datum"),services,
      [&](const auto &store){writes.push_back(store);});
  for(const auto &region:record.at("after"))check(regions.at(number(region.at("address")))==fixture.bytes(region.at("raw_hex")),
      "Complete original object storage differs");
  std::vector<SourceObjectStorageReleaseStore> expected;
  for(const auto &store:record.at("writes"))expected.push_back({number(store.at("address")),store.at("before"),store.at("after"),store.at("size")});
  if(writes!=expected){
    for(unsigned i=0;i<std::min(writes.size(),expected.size());++i)if(writes[i]!=expected[i]){
      const auto a=writes[i],b=expected[i];std::cerr<<"write "<<i<<" native "<<std::hex<<a.address<<' '<<a.bytes<<' '<<a.before<<' '<<a.after
        <<" original "<<b.address<<' '<<b.bytes<<' '<<b.before<<' '<<b.after<<std::dec<<'\n';break;
    }
    std::cerr<<"native writes "<<writes.size()<<" original writes "<<expected.size()<<'\n';
  }
  check(writes==expected,"Original storage write order/width differs");
}
} // namespace
int main(int argc,char **argv){
  try{
    check(argc==2,"usage: object_storage_release_tests original.json");
    std::ifstream input(argv[1]);check(bool(input),"Missing original release fixture");Json data;input>>data;
    Fixture fixture{data,{}};unsigned cases=0;
    for(const auto &[image,version]:data.at("versions").items())for(const auto &record:version.at("records")){
      try{run(fixture,version,record);}catch(const std::exception &e){throw std::runtime_error(image+" source record "+
          std::to_string(record.at("source_record").get<unsigned>())+(record.value("header_only",false)?" header-only":" body")+
          (record.at("debug_poison").get<bool>()?" poison": "")+": "+e.what());}
      ++cases;
    }
    check(cases==66,"Original release coverage changed; revalidate scope");
    bool stopped=false;
    try{release_source_object_storage({0,0},0,{});}catch(const std::runtime_error &){stopped=true;}
    check(stopped,"Missing retained backing must stop");
    std::cout<<"PASS "<<cases<<" complete original object-storage records and ordered writes; missing backing stops\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
