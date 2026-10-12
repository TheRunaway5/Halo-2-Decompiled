#include "halo2/vista_pcr_achievement.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace halo2;
namespace {
void require(bool v,const char* why){if(!v)throw std::runtime_error(why);}
std::array<std::uint8_t,5> flags(const std::string& s){require(s.size()==10,"Original five manager flags");std::array<std::uint8_t,5> out{};for(unsigned i=0;i<5;++i)out[i]=std::uint8_t(std::stoul(s.substr(i*2,2),nullptr,16));return out;}
}
int main(int argc,char** argv){try{
  require(argc==2,"Original achievement fixture argument");std::ifstream file(argv[1]);require(bool(file),"Open original achievement fixture");std::string line;unsigned constructors{},guards{},offline{};VistaPcrOfflineAchievementOwner owner;
  while(std::getline(file,line)){
    const auto r=nlohmann::json::parse(line);const auto kind=r.at("kind").get<std::string>();
    if(kind=="constructor"){require(owner.flags()==flags(r.at("flags")),"Original initialized manager projection");++constructors;}
    if(kind=="dispatch"){
      const auto f=flags(r.at("flags"));const bool admitted=!r.at("events").empty();require(vista_pcr_achievement_dispatch_admitted(f)==admitted,"Original44B553 admission");++guards;
      if(f[1]==0 && f[4]==0){const auto count=r.at("count").get<std::int32_t>();const auto request=owner.request_medals(std::bit_cast<std::uint32_t>(count),1);require(request.type==12 && request.distinct==count && !request.dispatched,"Original offline medal event and no child dispatch");require(!vista_pcr_achievement_dispatch_admitted(owner.flags()),"Offline manager disabled");++offline;}
    }
  }
  require(constructors==1 && guards==144 && offline==9,"Original achievement case counts");require(owner.requests().size()==offline,"Reached native event retention");
  for(int state:{-1,0,2,3}){bool rejected{};try{owner.request_medals(8,state);}catch(const FormatError&){rejected=true;}require(rejected,"Unsupported sign-in state admitted");}
  require(owner.requests().size()==offline,"Rejected request mutated owner");
  std::cout<<constructors<<" original constructor projection, "<<guards<<" original guards, "<<offline<<" offline events; passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
