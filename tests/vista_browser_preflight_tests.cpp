#include "halo2/vista_network_browser.hpp"
#include "halo2/vista_session_storage.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <array>
#include <fstream>
#include <iostream>
using namespace halo2;using Json=nlohmann::json;
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
int main(int argc,char**argv)try {
  if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
  std::ifstream file(argv[1]);Json rows;file>>rows;unsigned count{};
  for(const auto&row:rows) {
    const auto&q=row.at("input");std::array<std::byte,vista_session_source_size> b;
    for(unsigned i=0;i<b.size();++i)b[i]=std::byte((i*37+q.at("seed").get<unsigned>()*13)&255);
    for(const auto&[at,key]:std::array<std::pair<std::size_t,const char*>,5>{{{0x73a0,"state"},{0x74,"local"},{0x71c0,"host"},{0x4c68,"phase"},{0x4c60,"revision"}}})put(b,at,q.at(key).get<std::uint32_t>());
    if(hash(b)!=row.at("before").get<std::string>())throw FormatError("Original preflight input bank changed");
    const auto actual=vista_network_browser_preflight_local(b,q.at("current").get<unsigned>()!=0,q.at("value"));
    if(actual!=(row.at("returned").get<unsigned>()!=0) || hash(b)!=row.at("after").get<std::string>()) {
      std::cerr<<"case="<<count<<" input="<<q.dump()<<'\n';throw FormatError("Original full preflight bank mismatch");
    }++count;
  }
  if(count!=2060)throw FormatError("Incomplete original preflight corpus");
  for(const auto state:{3u,4u}) {
    std::array<std::byte,vista_session_source_size> b{};put(b,0x73a0,state);put(b,0x4c68,1);const auto before=b;bool rejected{};
    try{(void)vista_network_browser_preflight_local(b,true,1);}catch(const FormatError&){rejected=true;}
    if(!rejected || b!=before)throw FormatError("Remote preflight fabricated or mutated transport");
  }
  std::array<std::byte,4> short_bank{};bool rejected{};
  try{(void)vista_network_browser_preflight_local(short_bank,true,1);}catch(const FormatError&){rejected=true;}
  if(!rejected)throw FormatError("Truncated preflight bank accepted");
  std::cout<<"PASS original Vista browser preflight cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
