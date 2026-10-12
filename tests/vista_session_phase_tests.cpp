#include "halo2/vista_session_phase.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <fstream>
#include <iostream>
using namespace halo2;
using J=nlohmann::json;
namespace {
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
void put(std::span<std::byte> b,unsigned at,std::uint32_t v,unsigned n=4){for(unsigned i=0;i<n;++i)b[at+i]=std::byte(v>>(8*i));}
std::string digest(std::span<const std::byte> b){unsigned char hash[SHA256_DIGEST_LENGTH];SHA256(reinterpret_cast<const unsigned char*>(b.data()),b.size(),hash);static constexpr char hex[]="0123456789abcdef";std::string out;for(auto v:hash){out+=hex[v>>4];out+=hex[v&15];}return out;}
std::array<std::byte,vista_session_source_size> initial(const J& q){
 std::array<std::byte,vista_session_source_size> b;for(unsigned i=0;i<b.size();++i)b[i]=std::byte((i*37+q.at("seed").get<unsigned>()*13+(i>>8)*17)&255);
 for(auto [at,value]:{std::pair<unsigned,std::uint32_t>{0x73a0,q.at("state")},{0x84,q.at("count")},{0x4c68,q.at("before")},{0x4c64,0xffffffff},{0x4c60,0xfffffffe},{0x71c0,7},{0x74,q.at("host").get<unsigned>()?7u:8u},{0x64,0},{8,0x13371337},{0x14,2}})put(b,at,value);
 for(unsigned i=0;i<17;++i)put(b,0x71c5+i*0x1c,(i%3==0)*q.at("present").get<unsigned>(),1);
 return b;
}
}
int main(int argc,char** argv){try{
 require(argc==2,"fixture argument");std::ifstream file(argv[1]);J records;file>>records;unsigned checked{};
 for(const auto& r:records){const auto& q=r.at("input");auto b=initial(q);J calls=J::array();unsigned clocks{};
  VistaSessionPhaseServices s;
  s.network_clock=[&](){calls.push_back({"clock",digest(b)});++clocks;if(q.at("mutate").get<unsigned>()){put(b,0x4c64,0x55555555+clocks);put(b,0x796c,0x80,1);}return clocks==1?0x12345678u:0xfedcba98u;};
  s.send=[&](const VistaSessionPhaseMessage& m){calls.push_back({"send",m.transport_owner,m.session_slot,m.channel,0,0x22,0x4b4,digest(m.payload)});};
  bool result{};const auto op=q.at("op").get<std::string>();
  if(op=="write")vista_session_phase_write(b,q.at("requested"),s);
  else if(op=="request")result=vista_session_phase_request(b,q.at("requested"),s);
  else if(op=="continue")result=vista_session_pcr_continue(b,q.at("kind"),s);
  else result=vista_session_pcr_host(b,q.at("enabled"));
  if(digest(b)!=r.at("after").get<std::string>()||calls!=r.at("events")||(!r.at("returned").is_null()&&result!=bool(r.at("returned").get<unsigned>()))){
   std::cerr<<"case="<<checked<<" input="<<q.dump()<<"\nexpected="<<r.dump()<<"\nactual="<<digest(b)<<" calls="<<calls.dump()<<" result="<<result<<'\n';throw std::runtime_error("original phase mismatch");
  }++checked;
 }
 require(checked==624,"complete original phase corpus");
 auto b=initial(records.front().at("input"));const auto before=b;bool rejected{};
 try{vista_session_phase_write(b,999,{});}catch(const FormatError&){rejected=true;}require(rejected&&b==before,"missing clock atomic admission");
 put(b,0x4c68,999);vista_session_phase_write(b,999,{});
 require(!vista_session_pcr_host({},0),"disabled host does not read session");require(vista_session_pcr_continue({},1,{}),"kind1 does not read session");
 std::cout<<"Vista phase original cases="<<checked<<" PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
