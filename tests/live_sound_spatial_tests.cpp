#include "halo2/sound_spatial.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::uint32_t word(const std::string &value){return std::uint32_t(std::stoul(value,nullptr,16));}
std::vector<std::string> fields(const std::string &line){
  std::istringstream stream(line);std::vector<std::string> result;std::string value;
  while(std::getline(stream,value,'\t'))result.push_back(value);
  return result;
}
std::vector<std::byte> decode(const std::string &value){
  require(value.size()%2==0,"Incomplete original bytes");std::vector<std::byte> result(value.size()/2);
  for(std::size_t i=0;i<result.size();++i)result[i]=std::byte(word(value.substr(i*2,2)));
  return result;
}
using Regions=std::map<std::uint32_t,std::vector<std::byte>>;
std::span<std::byte> memory(Regions &regions,std::uint32_t address,std::size_t size){
  auto found=regions.upper_bound(address);require(found!=regions.begin(),"Uncaptured original address");--found;
  require(std::uint64_t(address)+size<=std::uint64_t(found->first)+found->second.size(),"Uncaptured original extent");
  return std::span<std::byte>(found->second).subspan(address-found->first,size);
}
void apply(Regions &regions,const halo2::SoundStartupStore &store){
  auto raw=memory(regions,store.address,store.bytes);std::uint32_t before=0;
  for(unsigned i=0;i<store.bytes;++i)before|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);
  require(before==store.before,"Original store before bits differ");
  for(unsigned i=0;i<store.bytes;++i)raw[i]=std::byte(store.after>>(8*i));
}
std::array<std::byte,12> vector_bytes(const std::string &hex){
  const auto raw=decode(hex);require(raw.size()==12,"Original caller position backing");
  std::array<std::byte,12> result;std::copy(raw.begin(),raw.end(),result.begin());return result;
}
}

int main(int argc,char **argv)try{
  require(argc==2,"Usage: live_sound_spatial_tests <original.tsv>");
  std::ifstream file(argv[1]);require(bool(file),"Original spatial fixture unavailable");
  std::string line;require(bool(std::getline(file,line)),"Original spatial schema");
  const bool declared=line=="DECLARED_SOUND_SPATIAL_ORIGINAL_V1";
  require(declared||line=="LIVE_SOUND_SPATIAL_ORIGINAL_V1","Original spatial schema");
  halo2::SoundSpatialState state{};halo2::SoundSpatialConstants constants{};
  halo2::SoundDriverFloatingPointContext fp{};Regions actual,expected;
  std::vector<halo2::SoundStartupStore> stores;
  std::array<std::byte,12> position{},expected_position{};
  std::array<std::uint32_t,9> expected_driver{};
  std::int16_t index=0;std::int32_t driver_index=-1;std::uint32_t attenuation=0;
  bool active=false,driver_expected=false;
  std::size_t calls=0,drivers=0,bytes=0,writes=0,missing_provider_checks=0;
  while(std::getline(file,line)){
    const auto q=fields(line);if(q.empty())continue;
    if(q[0]=="K"){
      require(q.size()==4,"Three original immutable operands");constants={word(q[1]),word(q[2]),word(q[3])};
    }else if(q[0]=="M"){
      require(!active&&q.size()==9,"Actual original spatial roots");actual.clear();
      state={word(q[1]),word(q[2]),word(q[3]),word(q[4]),word(q[5]),word(q[6]),word(q[7]),word(q[8])};
    }else if(q[0]=="R"){
      require(!active&&q.size()==3,"Actual original page");const auto address=word(q[1]);auto raw=decode(q[2]);
      require(raw.size()==4096&&address%4096==0,"Complete original page");
      if(!actual.empty()&&std::uint64_t(actual.rbegin()->first)+actual.rbegin()->second.size()==address){
        auto &last=actual.rbegin()->second;last.insert(last.end(),raw.begin(),raw.end());
      }else require(actual.emplace(address,std::move(raw)).second,"Repeated original page");
    }else if(q[0]=="F"){
      require(q.size()==3,"Original floating-point context");fp={std::uint16_t(word(q[1])),word(q[2])};
    }else if(q[0]=="C"){
      require(!active&&q.size()==7,"Actual original spatial call");active=true;
      index=std::bit_cast<std::int16_t>(std::uint16_t(word(q[1])));attenuation=word(q[2]);
      position=vector_bytes(q[3]);expected_position=vector_bytes(q[4]);
      driver_index=std::bit_cast<std::int32_t>(word(q[5]));driver_expected=q[6]!="-";
      if(driver_expected){
        const auto raw=decode(q[6]);require(raw.size()==36,"Original driver request size");
        expected_driver.fill(0);
        for(unsigned i=0;i<9;++i)for(unsigned j=0;j<4;++j)expected_driver[i]|=std::to_integer<std::uint32_t>(raw[i*4+j])<<(8*j);
      }
      expected=actual;stores.clear();
    }else if(q[0]=="S"){
      require(active&&q.size()==6,"Original retained spatial store");
      stores.push_back({word(q[1]),word(q[2]),unsigned(std::stoul(q[3])),word(q[4]),word(q[5])});
    }else if(q[0]=="E"){
      require(active,"Missing original spatial call");active=false;
      const auto initial=actual;const auto initial_position=position;
      std::vector<halo2::SoundStartupStore> observed;std::size_t driver_calls=0;
      halo2::SoundSpatialServices services;
      services.memory=[&](auto address,auto size){return memory(actual,address,size);};
      services.driver_update=[&](auto index,const auto &request){
        require(driver_expected&&index==driver_index,"Original driver routing differs");
        if(request!=expected_driver){
          std::cerr<<"Call "<<calls<<" driver words"<<std::hex;
          for(unsigned i=0;i<9;++i)std::cerr<<' '<<request[i]<<'/'<<expected_driver[i];
          std::cerr<<std::dec<<'\n';throw std::runtime_error("Original spatial driver request differs");
        }
        ++driver_calls;
      };
      for(const auto &store:stores)apply(expected,store);
      halo2::source_sound_spatial_update(state,index,attenuation,position,constants,fp,services,
          [&](const auto &store){observed.push_back(store);});
      require(driver_calls==std::size_t(driver_expected),"Original driver call gate differs");
      if(observed!=stores){
        std::cerr<<"Call "<<calls<<" stores "<<observed.size()<<'/'<<stores.size()<<'\n';
        throw std::runtime_error("Original ordered spatial stores differ");
      }
      require(position==expected_position,"Original private vector backing differs");
      require(actual==expected,"Original retained spatial pages differ");
      for(const auto &[address,data]:actual){(void)address;bytes+=data.size();}
      ++calls;drivers+=driver_calls;writes+=observed.size();
      if(driver_expected){
        // A required driver must stop at the measured call, after CPU writes.
        actual=initial;position=initial_position;services.driver_update={};bool stopped=false;
        try{halo2::source_sound_spatial_update(state,index,attenuation,position,constants,fp,services);}
        catch(const std::runtime_error &error){stopped=std::string(error.what())=="Complete original spatial driver update unavailable";}
        require(stopped&&actual==expected,"Missing original driver owner fabricated a completed call");
        ++missing_provider_checks;
      }
    }else throw std::runtime_error("Unknown original spatial fixture row");
  }
  require(!active&&calls&&drivers&&missing_provider_checks,"No actual original driver request checked");
  std::cout<<"PASS "<<calls<<(declared?" declared original ABI cases, ":" actual original spatial calls, ")
           <<drivers<<" exact36-byte driver requests, "
           <<writes<<" ordered retained stores, "<<bytes<<" full captured bytes and "<<missing_provider_checks
           <<" missing-provider stops. CPU boundary only.\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
