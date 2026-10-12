#include "halo2/sound_driver_listener.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace halo2;
namespace {
void check(bool value,const std::string &message){if(!value)throw std::runtime_error(message);}
std::uint32_t number(const std::string &value){return std::uint32_t(std::stoul(value,nullptr,16));}
std::vector<std::byte> bytes(const std::string &value){check(value.size()%2==0,"odd byte extent");std::vector<std::byte> result(value.size()/2);for(std::size_t i=0;i<result.size();++i)result[i]=std::byte(number(value.substr(2*i,2)));return result;}
struct Image {
  std::uint32_t driver_pointer,input;
  SoundDriverListenerConstants constants;
  std::map<std::uint32_t,std::vector<std::byte>> regions;
};
struct Request {SoundDriverListenerOperation operation;std::uint32_t device;std::vector<std::uint32_t> words;};
struct Case {
  std::string image,outcome;
  std::vector<SoundDriverStartupStore> controls,stores;
  std::optional<Request> request;
  std::optional<std::pair<std::uint32_t,std::size_t>> invalid;
};
struct Boundary {};
std::span<std::byte> memory(std::map<std::uint32_t,std::vector<std::byte>> &regions,
                            std::uint32_t address,std::size_t extent) {
  auto owner=regions.upper_bound(address);
  if(owner==regions.begin())throw std::out_of_range("Original listener backing unavailable");
  --owner;auto &raw=owner->second;const auto offset=std::uint64_t(address)-owner->first;
  if(offset>raw.size()||extent>raw.size()-offset)throw std::out_of_range("Original listener span unavailable");
  return std::span<std::byte>(raw).subspan(std::size_t(offset),extent);
}
void apply(std::map<std::uint32_t,std::vector<std::byte>> &regions,const SoundDriverStartupStore &store) {
  auto output=memory(regions,store.address,store.bytes);std::uint32_t before=0;
  for(unsigned i=0;i<store.bytes;++i)before|=std::to_integer<std::uint32_t>(output[i])<<(8*i);
  check(before==store.before,"Original fixture before-store differs");
  for(unsigned i=0;i<store.bytes;++i)output[i]=std::byte(store.after>>(8*i));
}
}
int main(int argc,char **argv) {
  try {
    check(argc==2,"usage: sound_driver_listener_tests original-fixtures.tsv");
    std::ifstream fixture(argv[1]);check(bool(fixture),"Original listener fixture unavailable");
    std::map<std::string,Image> images;std::map<std::string,Case> cases;std::string line;
    while(std::getline(fixture,line)) {
      if(line.empty()||line[0]=='#')continue;
      std::vector<std::string> row;std::stringstream input(line);std::string word;
      while(std::getline(input,word,'\t'))row.push_back(word);
      if(row.at(0)=="state") {
        Image image{};image.driver_pointer=number(row.at(2));image.input=number(row.at(3));
        image.constants.orientation_epsilon_bits=number(row.at(4));image.constants.environment_epsilon_bits=number(row.at(5));
        const auto payload=bytes(row.at(6));check(payload.size()==72,"Original default environment extent");
        std::copy(payload.begin(),payload.end(),image.constants.default_environment.begin());
        images.emplace(row.at(1),std::move(image));
      } else if(row.at(0)=="region") images.at(row.at(1)).regions.emplace(number(row.at(2)),bytes(row.at(3)));
      else if(row.at(0)=="case") {Case c;c.image=row.at(2);c.outcome=row.at(3);cases.emplace(row.at(1),std::move(c));}
      else if(row.at(0)=="control") cases.at(row.at(1)).controls.push_back({0,number(row.at(2)),unsigned(std::stoul(row.at(3))),number(row.at(4)),number(row.at(5))});
      else if(row.at(0)=="store") cases.at(row.at(1)).stores.push_back({number(row.at(2)),number(row.at(3)),unsigned(std::stoul(row.at(4))),number(row.at(5)),number(row.at(6))});
      else if(row.at(0)=="invalid") cases.at(row.at(1)).invalid={{number(row.at(2)),std::size_t(std::stoul(row.at(3)))}};
      else if(row.at(0)=="request") {
        Request request{};check(row.at(2)=="position"||row.at(2)=="orientation","Unknown original listener operation");
        request.operation=row[2]=="position"?SoundDriverListenerOperation::Position:SoundDriverListenerOperation::Orientation;
        request.device=number(row.at(3));for(std::size_t i=4;i<row.size();++i)request.words.push_back(number(row[i]));
        cases.at(row.at(1)).request=std::move(request);
      } else throw std::runtime_error("Unknown listener fixture row");
    }
    std::size_t stores=0,full_bytes=0,returned=0,stopped=0,unavailable=0;
    for(const auto &[name,c]:cases) {
      const auto &image=images.at(c.image);auto actual=image.regions;
      for(const auto &control:c.controls)apply(actual,control);
      auto expected=actual;for(const auto &store:c.stores)apply(expected,store);
      SoundDriverListenerServices services;std::optional<std::pair<std::uint32_t,std::size_t>> invalid;
      services.memory=[&](std::uint32_t address,std::size_t extent){
        try{return memory(actual,address,extent);}catch(const std::out_of_range&){if(!invalid)invalid={{address,extent}};throw;}
      };
      unsigned requests=0;
      services.native_operation=[&](const SoundDriverListenerRequest &request){
        ++requests;check(bool(c.request),name+" unowned platform operation");const auto &source=*c.request;
        check(request.operation==source.operation&&request.device_identity==source.device&&
              std::equal(request.words.begin(),request.words.end(),source.words.begin(),source.words.end()),name+" original listener platform request differs");
        throw Boundary{};
      };
      std::string outcome="returned";std::vector<SoundDriverStartupStore> observed;
      try {source_sound_driver_listener_install(image.driver_pointer,image.input,image.constants,services,
                                               [&](const auto &store){observed.push_back(store);});}
      catch(const Boundary&){outcome="platform_stop";}catch(const std::out_of_range&){outcome="invalid_storage";}
      check(outcome==c.outcome&&invalid==c.invalid,name+" listener original boundary differs");
      check(requests==(c.request?1u:0u),name+" listener platform boundary bypassed");
      check(observed==c.stores,name+" listener ordered writes differ");
      check(actual==expected,name+" listener complete regions differ");
      for(const auto &[address,raw]:actual)full_bytes+=raw.size();
      stores+=observed.size();returned+=outcome=="returned";stopped+=outcome=="platform_stop";unavailable+=outcome=="invalid_storage";
    }
    check(cases.size()==128&&stores==2286,"Listener original coverage changed");
    std::cout<<"listenerCases="<<cases.size()<<" orderedStores="<<stores<<" completeOutputBytes="<<full_bytes
             <<" rawControlReturns="<<returned<<" platformRequestStops="<<stopped<<" unavailableStops="<<unavailable
             <<" sourcePlatformReturns=0 liveListenerAdmission=unverified\n";
    return 0;
  } catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
