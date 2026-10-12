#include "halo2/sound_admission.hpp"
#include <algorithm>
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
  require(value.size()%2==0,"Raw game bytes are incomplete");std::vector<std::byte> result(value.size()/2);
  for(std::size_t i=0;i<result.size();++i)result[i]=std::byte(word(value.substr(i*2,2)));
  return result;
}
using Regions=std::map<std::uint32_t,std::vector<std::byte>>;
std::span<std::byte> memory(Regions &regions,std::uint32_t address,std::size_t size){
  auto found=regions.upper_bound(address);require(found!=regions.begin(),"Uncaptured original address");--found;
  require(std::uint64_t(address)+size<=std::uint64_t(found->first)+found->second.size(),"Uncaptured original memory extent");
  return std::span<std::byte>(found->second).subspan(address-found->first,size);
}
void apply(Regions &regions,const halo2::SoundStartupStore &store){
  auto bytes=memory(regions,store.address,store.bytes);std::uint32_t before=0;
  for(unsigned n=0;n<store.bytes;++n)before|=std::to_integer<std::uint32_t>(bytes[n])<<(8*n);
  require(before==store.before,"Original store before bits differ");
  for(unsigned n=0;n<store.bytes;++n)bytes[n]=std::byte(store.after>>(8*n));
}
}

int main(int argc,char **argv)try{
  require(argc==2,"Usage: live_sound_admission_tests <original.tsv>");
  std::ifstream file(argv[1]);require(bool(file),"Original live admission fixture unavailable");
  std::string line;require(bool(std::getline(file,line))&&line=="LIVE_SOUND_ADMISSION_ORIGINAL_V1","Original fixture schema");
  halo2::SoundAdmissionState state{};halo2::SoundAdmissionConstants constants{};
  halo2::SoundDriverParameterConstants parameters{};halo2::SoundDriverFloatingPointContext fp{};
  Regions actual,expected;std::vector<halo2::SoundStartupStore> stores;
  struct Query {std::uint32_t entry,argument,result;};std::vector<Query> queries;
  std::uint32_t sound=0,request=0,local=0,reason=0,result=0;bool active=false;
  std::size_t calls=0,accepted=0,bytes=0,writes=0,numeric=0;
  while(std::getline(file,line)){
    const auto q=fields(line);if(q.empty())continue;
    if(q[0]=="K"){
      require(q.size()==13,"Twelve immutable original admission words");
      constants={{word(q[1]),word(q[2]),word(q[3]),word(q[4]),word(q[5]),word(q[6]),word(q[7]),word(q[8]),word(q[9]),word(q[10]),word(q[11])},word(q[12])};
    }else if(q[0]=="P"){
      require(q.size()==17,"Sixteen immutable original driver words");
      parameters={word(q[1]),word(q[2]),word(q[3]),word(q[4]),word(q[5]),word(q[6]),word(q[7]),word(q[8]),word(q[9]),word(q[10]),word(q[11]),word(q[12]),word(q[13]),word(q[14]),word(q[15]),word(q[16])};
    }else if(q[0]=="M"){
      require(!active&&q.size()==10,"Actual source state roots");actual.clear();
      state={word(q[1]),word(q[2]),word(q[3]),word(q[4]),word(q[5]),word(q[6]),word(q[7]),word(q[8]),word(q[9])};
    }else if(q[0]=="R"){
      require(!active&&q.size()==3,"Actual original page");const auto address=word(q[1]);auto raw=decode(q[2]);
      require(raw.size()==4096&&address%4096==0,"Complete original 4KiB page");
      // Coalesce adjacent actual pages for a source read across a page edge.
      if(!actual.empty()&&std::uint64_t(actual.rbegin()->first)+actual.rbegin()->second.size()==address){
        auto &last=actual.rbegin()->second;last.insert(last.end(),raw.begin(),raw.end());
      }else require(actual.emplace(address,std::move(raw)).second,"Repeated original page");
    }else if(q[0]=="F"){
      require(q.size()==3,"Actual original floating point context");fp={std::uint16_t(word(q[1])),word(q[2])};
    }else if(q[0]=="C"){
      require(!active&&q.size()==6,"Actual original request");active=true;
      sound=word(q[1]);request=word(q[2]);local=word(q[3]);reason=word(q[4]);result=word(q[5]);
      expected=actual;stores.clear();queries.clear();
    }else if(q[0]=="Q"){
      require(active&&q.size()==4,"Actual original numeric return");
      queries.push_back({word(q[1]),word(q[2]),word(q[3])});
    }else if(q[0]=="S"){
      require(active&&q.size()==6,"Original semantic store");
      stores.push_back({word(q[1]),word(q[2]),unsigned(std::stoul(q[3])),word(q[4]),word(q[5])});
    }else if(q[0]=="E"){
      require(active,"Missing actual original call");active=false;
      std::vector<halo2::SoundStartupStore> observed;
      halo2::SoundAdmissionServices services;
      services.memory=[&](auto address,auto size){return memory(actual,address,size);};
      for(const auto &query:queries){
        const auto got=halo2::source_sound_admission_query(state,query.entry,sound,request+12,
          query.argument,constants,parameters,fp,services);
        if(got!=query.result){
          std::cerr<<"Call "<<calls<<" numeric entry "<<std::hex<<query.entry<<" argument "<<query.argument
                   <<" result "<<got<<'/'<<query.result<<std::dec<<'\n';
          throw std::runtime_error("Actual game numeric admission return differs");
        }
        ++numeric;
      }
      require(actual==expected,"Numeric query changed captured original state");
      for(const auto &store:stores)apply(expected,store);
      const bool got=halo2::source_sound_admit(state,sound,request,local,reason,constants,parameters,fp,services,
        [&](const auto &store){observed.push_back(store);});
      if(got!=bool(result)||observed!=stores){
        std::cerr<<"Call "<<calls<<" sound "<<std::hex<<sound<<std::dec<<" accepted "<<got<<'/'<<result
                 <<" stores "<<observed.size()<<'/'<<stores.size()<<'\n';
        throw std::runtime_error("Actual game admission differs from native owner");
      }
      require(actual==expected,"Captured original pages differ after native admission");
      for(const auto &[address,data]:actual){(void)address;bytes+=data.size();}
      ++calls;accepted+=got;writes+=observed.size();
    }else throw std::runtime_error("Unknown original fixture row");
  }
  require(!active&&calls&&accepted,"No genuine successful original admission checked");
  std::cout<<"PASS "<<calls<<" actual original game requests, "<<accepted<<" accepted; "<<writes
           <<" ordered semantic stores, "<<numeric<<" internal numeric returns and "<<bytes
           <<" full captured bytes. CPU admission only.\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
