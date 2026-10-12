#include "halo2/sound_driver_cleanup.hpp"
#include "halo2/native_sound_storage.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace halo2;
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
std::uint32_t number(const std::string &value) {return std::uint32_t(std::stoul(value,nullptr,16));}
std::vector<std::byte> bytes(const std::string &value) {
  check(value.size()%2==0,"Invalid fixture byte extent");std::vector<std::byte> result(value.size()/2);
  for(std::size_t i=0;i<result.size();++i)result[i]=std::byte(number(value.substr(2*i,2)));
  return result;
}
struct Case {
  SoundDriverCleanupBindings bindings{};
  std::uint32_t allocator=0;
  std::map<std::uint32_t,std::vector<std::byte>> input,output;
  std::vector<SoundDriverStartupStore> stores;
};
}
int main(int argc,char **argv) {
  try {
    check(argc==2,"usage: sound_driver_cleanup_tests original-fixtures.tsv");
    std::ifstream input(argv[1]);check(bool(input),"Original cleanup fixture unavailable");
    std::map<std::string,Case> cases;std::string line;
    while(std::getline(input,line)) {
      if(line.empty()||line[0]=='#')continue;
      std::vector<std::string> row;std::stringstream stream(line);std::string word;
      while(std::getline(stream,word,'\t'))row.push_back(word);
      if(row.at(0)=="image") {
        auto &c=cases[row.at(1)];c.bindings={number(row.at(2)),number(row.at(3)),number(row.at(4)),number(row.at(5))};
        c.allocator=number(row.at(6));
      } else if(row.at(0)=="input"||row.at(0)=="output") {
        auto &c=cases.at(row.at(1));auto &regions=row[0]=="input"?c.input:c.output;
        check(regions.emplace(number(row.at(2)),bytes(row.at(3))).second,"Duplicate fixture region");
      } else if(row.at(0)=="store") {
        cases.at(row.at(1)).stores.push_back({number(row.at(2)),number(row.at(3)),
          unsigned(std::stoul(row.at(4))),number(row.at(5)),number(row.at(6))});
      } else throw std::runtime_error("Unknown cleanup fixture row");
    }
    std::size_t stores=0,full_bytes=0;
    for(const auto &[name,c]:cases) {
      NativeSoundStorage storage({c.allocator,0x0a200000,0x0b000000});
      const auto page=storage.page_size();
      for(const auto &[address,raw]:c.input) {
        const auto start=std::uint32_t(address-address%page);
        storage.map_zeroed(start,std::size_t(address-start)+raw.size());
        const auto output=storage.memory(address,raw.size());std::copy(raw.begin(),raw.end(),output.begin());
      }
      SoundDriverCleanupServices services;
      services.owners.memory=[&](std::uint32_t address,std::size_t extent){return storage.memory(address,extent);};
      // No virtual method, DirectSound release, device return or source
      // helper callback is supplied. Real complete flush/queue execute.
      std::vector<SoundDriverStartupStore> observed;
      source_sound_driver_cleanup(c.bindings,services,[&](const auto &store){observed.push_back(store);});
      check(observed==c.stores,"Whole cleanup ordered stores differ");
      check(c.input.size()==c.output.size(),"Whole cleanup region inventory differs");
      for(const auto &[address,raw]:c.output) {
        const auto actual=storage.memory(address,raw.size());
        check(std::equal(raw.begin(),raw.end(),actual.begin()),"Whole cleanup output differs");full_bytes+=raw.size();
      }
      stores+=observed.size();
    }
    check(cases.size()==2&&stores==6&&full_bytes==311344,"Whole cleanup coverage changed");
    std::cout<<"cleanupCases="<<cases.size()<<" orderedStores="<<stores<<" completeOutputBytes="<<full_bytes
             <<" realFlushAndQueue=passed fixtureHelperCallbacks=0 platformReturns=0 nonemptyVoicesAndLiveAudio=unverified\n";
    return 0;
  } catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
