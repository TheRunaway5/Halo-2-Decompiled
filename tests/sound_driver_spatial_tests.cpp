#include "halo2/sound_driver_spatial.hpp"
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
std::uint32_t word(const std::string &s){return std::uint32_t(std::stoul(s,nullptr,16));}
std::vector<std::string> fields(const std::string &line){std::istringstream in(line);std::vector<std::string> out;std::string value;while(std::getline(in,value,'\t'))out.push_back(value);return out;}
std::vector<std::byte> decode(const std::string &s){
  require(s.size()%2==0,"Incomplete original byte string");std::vector<std::byte> out(s.size()/2);
  for(std::size_t i=0;i<out.size();++i)out[i]=std::byte(word(s.substr(2*i,2)));
  return out;
}
std::vector<std::uint32_t> words(const std::string &s){
  const auto raw=decode(s);require(raw.size()%4==0,"Incomplete original word array");std::vector<std::uint32_t> out(raw.size()/4);
  for(std::size_t i=0;i<out.size();++i)for(unsigned j=0;j<4;++j)out[i]|=std::to_integer<std::uint32_t>(raw[4*i+j])<<(8*j);
  return out;
}
using Regions=std::map<std::uint32_t,std::vector<std::byte>>;
std::span<std::byte> memory(Regions &regions,std::uint32_t address,std::size_t size){
  auto found=regions.upper_bound(address);require(found!=regions.begin(),"Uncaptured driver address");--found;
  require(std::uint64_t(address)+size<=std::uint64_t(found->first)+found->second.size(),"Uncaptured driver extent");return std::span<std::byte>(found->second).subspan(address-found->first,size);
}
struct Operation {halo2::SoundDriverSpatialOperation operation;std::uint32_t buffer;std::optional<std::uint32_t> apply;std::vector<std::uint32_t> words;};
struct ExternalBoundary {};
}

int main(int argc,char **argv)try{
  require(argc==2,"Usage: sound_driver_spatial_tests <original.tsv>");std::ifstream file(argv[1]);require(bool(file),"Original driver fixture unavailable");
  std::string line;require(bool(std::getline(file,line))&&line=="DRIVER_SOUND_SPATIAL_ORIGINAL_V1","Original driver fixture schema");
  halo2::SoundDriverSpatialConstants constants{};halo2::SoundDriverParameterConstants parameters{};halo2::SoundDriverFloatingPointContext fp{};
  const halo2::SoundDriverSpatialBindings bindings{0x51ebe4,0x470084,0x470088};
  Regions regions;std::vector<std::byte> expected;
  std::array<std::uint32_t,9> request{};std::vector<Operation> operations;std::vector<halo2::SoundStartupStore> stores;
  bool active=false,prefix=false;std::string label;std::uint32_t driver=0;std::int32_t index=0;
  std::size_t actual_calls=0,declared=0,helpers=0,requests=0,compared_stores=0,missing=0;
  while(std::getline(file,line)){
    const auto q=fields(line);if(q.empty())continue;
    if(q[0]=="K"){
      require(q.size()==16,"Fifteen original spatial driver operands");
      constants={word(q[1]),word(q[2]),word(q[3]),word(q[4]),word(q[5]),word(q[6]),word(q[7]),word(q[8]),word(q[9]),word(q[10]),word(q[11]),word(q[12]),word(q[13]),word(q[14]),word(q[15])};
    }else if(q[0]=="P"){
      require(q.size()==17,"Sixteen original driver parameter operands");
      parameters={word(q[1]),word(q[2]),word(q[3]),word(q[4]),word(q[5]),word(q[6]),word(q[7]),word(q[8]),word(q[9]),word(q[10]),word(q[11]),word(q[12]),word(q[13]),word(q[14]),word(q[15]),word(q[16])};
    }else if(q[0]=="C"){
      require(!active&&q.size()==7,"Original driver call fields");active=true;label=q[1];prefix=q[2]=="1";driver=word(q[3]);index=std::bit_cast<std::int32_t>(word(q[4]));
      regions.clear();regions[bindings.driver_pointer_address].resize(4);
      for(unsigned i=0;i<4;++i)regions[bindings.driver_pointer_address][i]=std::byte(driver>>(8*i));
      regions[bindings.filter_frequency_address]=decode(q[5]);require(regions[bindings.filter_frequency_address].size()==8,"Original filter controls");
      const auto values=words(q[6]);require(values.size()==9,"Original36-byte driver input");std::copy(values.begin(),values.end(),request.begin());operations.clear();stores.clear();
    }else if(q[0]=="F"){
      require(q.size()==3,"Original driver floating-point context");fp={std::uint16_t(word(q[1])),word(q[2])};
    }else if(q[0]=="B"){
      require(active&&q.size()==2,"Original retained driver backing");regions[driver]=decode(q[1]);require(regions[driver].size()==0x2ad4,"Original complete driver backing");
    }else if(q[0]=="A"){
      require(active&&q.size()==2,"Original expected retained driver backing");expected=decode(q[1]);
    }else if(q[0]=="O"){
      require(active&&q.size()==5,"Original SDK observation");operations.push_back({static_cast<halo2::SoundDriverSpatialOperation>(std::stoul(q[1])),word(q[2]),q[3]=="-"?std::nullopt:std::optional{word(q[3])},words(q[4])});
    }else if(q[0]=="S"){
      require(active&&q.size()==6,"Original CPU prefix store");stores.push_back({word(q[1]),word(q[2]),unsigned(std::stoul(q[3])),word(q[4]),word(q[5])});
    }else if(q[0]=="E"){
      require(active,"No original driver call");active=false;const auto initial=regions;
      std::size_t seen=0;std::vector<halo2::SoundStartupStore> observed;bool stopped=false;
      halo2::SoundDriverSpatialServices services;
      services.memory=[&](auto address,auto size){return memory(regions,address,size);};
      services.native_operation=[&](const halo2::SoundDriverSpatialRequest &got){
        require(seen<operations.size(),"Unexpected spatial SDK request");const auto &wanted=operations[seen++];
        if(got.operation!=wanted.operation||got.buffer_identity!=wanted.buffer||got.apply!=wanted.apply||std::vector<std::uint32_t>(got.words.begin(),got.words.end())!=wanted.words){
          std::cerr<<label<<" SDK operation "<<seen-1<<" words"<<std::hex;
          for(auto v:got.words)std::cerr<<' '<<v;
          std::cerr<<" expected";
          for(auto v:wanted.words)std::cerr<<' '<<v;
          std::cerr<<std::dec<<'\n';
          throw std::runtime_error("Original spatial SDK payload differs");
        }
        if(prefix)throw ExternalBoundary{};
      };
      try{halo2::source_sound_driver_spatial_update(bindings,index,request,constants,parameters,fp,services,[&](const auto &store){observed.push_back(store);});}
      catch(const ExternalBoundary &){stopped=true;}
      require(stopped==(prefix&&!operations.empty())&&seen==operations.size(),"Original spatial SDK boundary differs");
      if(regions.at(driver)!=expected){
        for(std::size_t i=0;i<expected.size();++i)if(regions.at(driver)[i]!=expected[i]){std::cerr<<label<<" driver byte "<<std::hex<<i<<" differs\n";break;}
        throw std::runtime_error("Original spatial driver state differs");
      }
      require(regions.at(bindings.driver_pointer_address)==initial.at(bindings.driver_pointer_address)&&regions.at(bindings.filter_frequency_address)==initial.at(bindings.filter_frequency_address),"Unexpected spatial globals write");
      if(prefix){require(observed==stores,"Original ordered driver prefix stores differ");++declared;compared_stores+=stores.size();}
      else{
        ++actual_calls;regions=initial;services.native_operation={};bool unavailable=false;
        try{halo2::source_sound_driver_spatial_update(bindings,index,request,constants,parameters,fp,services);}
        catch(const std::runtime_error &e){unavailable=std::string(e.what())=="Required spatial SDK/platform operation unavailable";}
        require(unavailable&&regions==initial,"Missing SDK provider fabricated a completed spatial update");++missing;
      }
      requests+=seen;
    }else if(q[0]=="Q"){
      require(!active&&q.size()==4,"Original filter helper fields");
      const auto value=word(q[2]);const auto got=q[1]=="frequency"?halo2::source_sound_filter_frequency(value,constants,fp):halo2::source_sound_filter_gain(value,constants,fp);
      if(got!=word(q[3])){std::cerr<<q[1]<<" input "<<q[2]<<" output "<<std::hex<<got<<'/'<<word(q[3])<<std::dec<<'\n';throw std::runtime_error("Original filter helper result differs");}
      ++helpers;
    }else throw std::runtime_error("Unknown original spatial driver row");
  }
  require(!active&&actual_calls&&declared&&helpers&&missing,"Incomplete original spatial driver evidence");
  std::cout<<"PASS "<<actual_calls<<" actual driver call, "<<declared<<" declared first-SDK boundaries, "<<helpers
           <<" complete filter helper cases, "<<requests<<" exact SDK payloads, "<<compared_stores<<" ordered prefix stores and "<<missing<<" missing-provider stop. CPU/ABI only.\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
