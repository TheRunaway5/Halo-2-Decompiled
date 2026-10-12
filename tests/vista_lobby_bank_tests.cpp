#include "halo2/vista_lobby_input.hpp"
#include "halo2/vista_session_storage.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void put(std::span<std::byte> b,std::size_t at,std::uint32_t v) {
  for(unsigned i=0;i<4;++i)b[at+i]=std::byte(v>>(8*i));
}
std::string hash(std::span<const std::byte> b) {
  std::array<unsigned char,SHA256_DIGEST_LENGTH> d;SHA256(reinterpret_cast<const unsigned char*>(b.data()),b.size(),d.data());
  constexpr char hex[]="0123456789abcdef";std::string out;
  for(const auto c:d){out+=hex[c>>4];out+=hex[c&15];}return out;
}
}
int main(int argc,char** argv)try {
  if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
  std::ifstream file(argv[1]);Json rows;file>>rows;unsigned count{};
  for(const auto& row:rows) {
    const auto& q=row.at("input");std::array<std::byte,vista_session_source_size> b;
    for(unsigned i=0;i<b.size();++i)b[i]=std::byte((i*37+q.at("seed").get<unsigned>()*13)&255);
    for(const auto& [at,key]:std::array<std::pair<std::size_t,const char*>,6>{{{0x73a0,"state"},{0x74,"local"},{0x71c0,"host"},{0x4c68,"phase"},{0x84,"count"},{0x4c60,"revision"}}})
      put(b,at,q.at(key).get<std::uint32_t>());
    b[0x73a4]=std::byte(q.at("closing").get<unsigned>());b[0x4c74]=std::byte(q.at("local_flag").get<unsigned>());
    if(hash(b)!=row.at("before").get<std::string>())throw FormatError("Original full input bank changed");
    std::array<std::byte,4*0x127c> controllers{};std::array<std::byte,4*0xb8> users{};
    for(unsigned i=0;i<4;++i) {
      put(controllers,i*0x127c,q.at("controller_flags"));users[i*0xb8]=std::byte(q.at("present").get<unsigned>());
      put(users,i*0xb8+1,0x12345678+i);put(users,i*0xb8+5,0xfedcba98-i);
    }
    Json events=Json::array();std::uint32_t returned{};const bool current=q.at("current").get<unsigned>()!=0;
    const auto op=q.at("op").get<std::string>();
    if(op=="event")returned=vista_lobby_event_admitted(b,q.at("enabled").get<unsigned>()!=0);
    else if(op=="ready")returned=vista_lobby_setup_ready(b,current);
    else if(op=="controller")returned=vista_lobby_controller_signed_in(controllers,q.at("controller"));
    else if(op=="count")returned=std::bit_cast<std::uint32_t>(vista_lobby_peer_count(b,current));
    else if(op=="settings")returned=vista_lobby_settings_change_local(b,current,q.at("value"));
    else if(op=="wrapper")returned=vista_lobby_countdown_write_request(b,users,current,q.at("running"),q.at("stage"),q.at("reason"),q.at("user"),q.at("flag"),[&](const auto& write) {
      if(!write.identity)throw FormatError("Wrapper lost its actual source identity");
      events.push_back({"write",write.running,std::bit_cast<std::uint32_t>(write.stage),write.flag,write.reason,(*write.identity)[0],(*write.identity)[1]});
      return q.at("write").get<std::uint8_t>();
    });
    if(returned!=row.at("returned").get<std::uint32_t>() || events!=row.at("events") || hash(b)!=row.at("after").get<std::string>()) {
      std::cerr<<"case="<<count<<" input="<<q.dump()<<"\nexpected="<<row.dump()<<"\nactual="<<events.dump()<<" returned="<<returned<<" bank="<<hash(b)<<'\n';
      throw FormatError("Original lobby bank mismatch");
    }++count;
  }
  if(count!=2928)throw FormatError("Incomplete original lobby-bank corpus");
  std::array<std::byte,vista_session_source_size> remote{};put(remote,0x73a0,3);
  const auto before=remote;bool rejected{};
  try{vista_lobby_settings_change_local(remote,true,1);}catch(const FormatError&){rejected=true;}
  if(!rejected || before!=remote)throw FormatError("Remote settings branch mutated or fabricated transport");
  std::cout<<"PASS original Vista lobby bank guards/wrapper/settings cases="<<count<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
