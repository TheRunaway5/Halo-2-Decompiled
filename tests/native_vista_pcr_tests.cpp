#include "halo2/native_vista_pcr.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void word(std::span<std::byte> b,std::size_t at,std::uint32_t value){for(unsigned i=0;i<4;++i)b[at+i]=std::byte(value>>(i*8));}
std::array<std::byte,vista_session_source_size> initial(const Json& q) {
  std::array<std::byte,vista_session_source_size> b;
  for(unsigned i=0;i<b.size();++i)b[i]=std::byte((i*37+q.at("seed").get<unsigned>()*13+(i>>8)*17)&255);
  word(b,0x73a0,q.at("state"));word(b,0x1258,q.at("mask"));word(b,0x71c0,q.at("peer"));
  word(b,0x64,q.at("peer").get<unsigned>()^0x1234);
  for(unsigned i=0;i<16;++i) {
    const auto at=0x125c+i*0x128;word(b,at,0x12345678+i);word(b,at+4,0xaabbccdd);
    word(b,at+8,q.at("matching_peer").get<unsigned>()?i%3:99);
  }
  if(q.at("duplicate").get<unsigned>())std::copy_n(b.begin()+0x125c,8,b.begin()+0x125c+15*0x128);
  return b;
}
std::u16string wide(const std::string& source){return {source.begin(),source.end()};}
std::string narrow(const std::u16string& source){return {source.begin(),source.end()};}
}
int main(int argc,char** argv)try {
  require(argc==2,"actual original PCR query fixture required");
  std::ifstream file(argv[1]);Json rows;file>>rows;unsigned checked{};
  for(const auto& row:rows) {
    const auto& q=row.at("input");auto b=initial(q);const auto before=b;
    VistaLocalUserIdentity identity;word(identity,0,0x12345678+q.at("identity_index").get<unsigned>());word(identity,4,0xaabbccdd);
    Json actual;const auto op=q.at("op").get<std::string>();
    if(op=="identity")actual=unsigned(native_vista_pcr_identity_query(b,q.at("active"),q.at("kind"),identity));
    else if(op=="primary")actual=native_vista_pcr_primary_player_query(b,q.at("active"),identity);
    else if(op=="player")actual=unsigned(native_vista_pcr_player_query(b,q.at("active"),q.at("datum")));
    else if(op=="placement")actual=native_vista_pcr_placement_key(q.at("value"));
    else {
      const auto values=q.at("values").get<std::vector<std::int32_t>>();
      actual=narrow(native_vista_pcr_format(op=="score"?u"%d":wide(q.at("format")),values));
    }
    if(actual!=row.at("returned")) {
      std::cerr<<"case="<<checked<<" input="<<q.dump()<<" expected="<<row.at("returned")<<" actual="<<actual<<'\n';
      throw std::runtime_error("Actual supplied Vista PCR query/CRT formatter differs");
    }
    require(b==before,"PCR query changed actual session storage");++checked;
  }
  require(checked==3097,"actual original PCR query corpus incomplete");
  bool rejected{};try{native_vista_pcr_primary_player_query({},1,{});}catch(const FormatError&){rejected=true;}
  require(rejected,"truncated actual session storage accepted");
  std::array<std::byte,vista_session_source_size> bank{};word(bank,0x73a0,5);word(bank,0x1258,0x80000000);
  rejected=false;try{native_vista_pcr_player_query(bank,1,0xffffffffu);}catch(const FormatError&){rejected=true;}
  require(rejected,"actual source peer read beyond owned bank accepted");
  for(const auto format:{u"%s",u"%f",u"%",u"%129d"}) {
    rejected=false;const std::int32_t value=1;
    try{(void)native_vista_pcr_format(format,std::span(&value,1));}catch(const FormatError&){rejected=true;}
    require(rejected,"unsupported source formatter silently substituted");
  }
  std::cout<<"Actual Vista PCR identity/peer/placement/CRT original cases="<<checked<<" PASS\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
