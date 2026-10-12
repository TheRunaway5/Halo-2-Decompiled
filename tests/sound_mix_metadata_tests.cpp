#include "halo2/source_tag_metadata_store.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
void check(bool condition,const char *message) {
  if (!condition) throw std::runtime_error(message);
}
std::uint32_t number(const std::string &value) { return std::stoul(value,nullptr,16); }
std::vector<std::byte> bytes(const std::string &raw) {
  check(!raw.empty() && raw.size()%2==0,"Incomplete owned sound mix payload");
  std::vector<std::byte> result(raw.size()/2);
  for (std::size_t i=0;i<result.size();++i) result[i]=std::byte(number(raw.substr(i*2,2)));
  return result;
}
}
int main(int argc,char **argv) {
  try {
    check(argc==3,"Supply complete owned sound mix fixture and project root");
    if (!std::filesystem::exists(argv[1])) return 77;
    std::ifstream input(argv[1]);
    halo2::SourceTagMetadataProviders provider;
    std::function<std::uint32_t(std::uint16_t)> index;
    std::weak_ptr<const halo2::Cache> lifetime;
    std::size_t maps=0,tags=0,extent=0,current=0;
    auto finish=[&] {
      if (!maps) return;
      check(current==1,"Incomplete owned map sound mix coverage");
      provider={};
      check(!lifetime.expired(),"Literal sound mix lookup lost its owned cache");
      index={};
      check(lifetime.expired(),"Released mix callbacks retained their cache");
      current=0;
    };
    for (std::string line;std::getline(input,line);) {
      std::istringstream row(line);char kind;row>>kind;
      if (kind=='M') {
        finish();std::string label;unsigned count;
        check(bool(row>>std::quoted(label)>>count) && count==1,"Incomplete owned sound mix map row");
        auto path=std::filesystem::path(label);
        path=path.is_absolute()?path:std::filesystem::path(argv[2])/path;
        if (!std::filesystem::exists(path)) return 77;
        {
          auto cache=std::make_shared<halo2::Cache>(path);lifetime=cache;
          halo2::SourceTagMetadataStore store(cache,{"snmx"});
          check(store.tag_count()==count,"Actual complete sound mix count differs");
          provider=store.providers();index=store.index_address_provider();
        }
        check(!lifetime.expired(),"Owned sound mix backing disappeared with facade");
        ++maps;
      } else if (kind=='T') {
        std::string id,group,address,raw;
        check(bool(row>>id>>group>>address>>raw) && group=="snmx","Incomplete literal mix record");
        const auto datum=number(id),va=number(address);const auto expected=bytes(raw);
        const auto &definition=provider.definition(datum);
        check(definition.group()==group && definition.datum()==datum && definition.source_address()==va &&
              definition.size()==expected.size() && provider.address(datum)==va && index(std::uint16_t(datum))==va,
              "Original sound mix identity/extent/table slot differs");
        const auto direct=definition.bytes_at(va,expected.size()),forwarded=provider.read(va,expected.size());
        check(std::equal(direct.begin(),direct.end(),expected.begin(),expected.end()) &&
              std::equal(forwarded.begin(),forwarded.end(),expected.begin(),expected.end()),
              "Complete original sound mix metadata bytes differ");
        ++tags;++current;extent+=expected.size();
      } else throw std::runtime_error("Unknown sound mix fixture row");
    }
    finish();
    check(maps==23 && tags==23 && extent==2024,"Incomplete owned sound mix inventory coverage");
    std::cout<<"ownedMaps=23 completeSnmxPayloads=23 exactBytes=2024 literalSlots=23 "
                 "snclRemainsSeparate sourceMapDriverAdmission=unverified\n";
  } catch (const std::exception &error) { std::cerr<<error.what()<<'\n';return 1; }
}
