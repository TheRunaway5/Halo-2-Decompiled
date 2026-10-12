#include "halo2/sound_driver_master_gain.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool value,const std::string &message){if(!value)throw std::runtime_error(message);}
std::uint32_t number(const std::string &value){return std::uint32_t(std::stoul(value,nullptr,16));}
std::vector<std::byte> raw(const std::string &value){
  require(value.size()%2==0,"Odd hex extent");std::vector<std::byte> result;
  for(std::size_t i=0;i<value.size();i+=2)result.push_back(std::byte(number(value.substr(i,2))));
  return result;
}
std::uint32_t word(const std::string &value){
  const auto data=raw(value);require(data.size()<=4,"DWORD extent");std::uint32_t result=0;
  for(unsigned i=0;i<data.size();++i)result|=std::to_integer<std::uint32_t>(data[i])<<(8*i);
  return result;
}
struct Region{std::uint32_t address;std::vector<std::byte> bytes;};
}
int main(int argc,char **argv){
  try{
    if(argc!=2)return 77;
    std::ifstream input(argv[1]);if(!input)return 77;
    std::string line;std::getline(input,line);require(line=="SOUND_DRIVER_MASTER_GAIN_V1","Fixture schema");
    halo2::SoundDriverMasterGainState state{};halo2::SoundDriverMasterGainConstants constants{};
    halo2::SoundDriverFloatingPointContext fp{};std::uint32_t parameters=0;std::string name;
    std::vector<Region> before,after;std::vector<halo2::SoundDriverStartupStore> stores;
    unsigned cases=0,writes=0;std::size_t compared=0;
    while(std::getline(input,line)){
      std::istringstream row(line);std::string kind;row>>kind;
      if(kind=="IMAGE"){
        std::string driver,globals,one,scale,quarter;row>>driver>>globals>>one>>scale>>quarter;
        state={number(driver),number(globals)};constants={number(one),number(scale),number(quarter)};
      }else if(kind=="CASE"){
        std::string address,cw,mxcsr;row>>name>>address>>cw>>mxcsr;
        parameters=number(address);fp={std::uint16_t(number(cw)),number(mxcsr)};
        before.clear();after.clear();stores.clear();
      }else if(kind=="BEFORE"||kind=="AFTER"){
        std::string address,bytes;row>>address>>bytes;
        (kind=="BEFORE"?before:after).push_back({number(address),raw(bytes)});
      }else if(kind=="STORE"){
        std::string pc,address,b,a;unsigned bytes=0;row>>pc>>address>>bytes>>b>>a;
        stores.push_back({number(pc),number(address),bytes,word(b),word(a)});
      }else if(kind=="ENDCASE"){
        auto memory=before;halo2::SoundDriverMasterGainServices services;
        services.memory=[&](std::uint32_t address,std::size_t size)->std::span<std::byte>{
          for(auto &region:memory)if(address>=region.address&&std::uint64_t(address)+size<=std::uint64_t(region.address)+region.bytes.size())
            return std::span(region.bytes).subspan(address-region.address,size);
          throw std::runtime_error(name+": Source backing unavailable");
        };
        std::vector<halo2::SoundDriverStartupStore> actual;
        halo2::source_sound_driver_master_gain(state,parameters,constants,fp,services,
            [&](const auto &event){actual.push_back(event);});
        if(actual!=stores){
          for(std::size_t i=0;i<std::min(actual.size(),stores.size());++i)if(actual[i]!=stores[i]){
            std::cerr<<name<<" store "<<i<<" expected "<<std::hex<<stores[i].after<<" got "<<actual[i].after<<std::dec<<'\n';break;
          }
          throw std::runtime_error(name+": Original ordered stores differ");
        }
        require(memory.size()==after.size(),"Region count");
        for(std::size_t i=0;i<memory.size();++i){
          require(memory[i].address==after[i].address&&memory[i].bytes==after[i].bytes,name+": Whole original output differs");
          compared+=memory[i].bytes.size();
        }
        ++cases;writes+=unsigned(actual.size());
      }else throw std::runtime_error("Unknown fixture row");
    }
    require(cases==144&&writes==2304,"Original master gain coverage changed");
    bool failed=false;try{halo2::source_sound_driver_master_gain(state,parameters,constants,fp,{});}
    catch(const std::runtime_error &){failed=true;}require(failed,"Missing backing became a successful gain update");
    std::cout<<"PASS master gains: "<<cases<<" original cases, "<<writes<<" stores, "<<compared<<" full compared bytes\n";
    return 0;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
