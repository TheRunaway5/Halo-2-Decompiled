#include "halo2/vista_lobby_leave_task.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <fstream>
#include <iostream>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void put(std::span<std::byte>b,std::size_t at,std::uint32_t v) {
  for(unsigned i=0;i<4;++i)b[at+i]=std::byte(v>>(8*i));
}
std::string hash(std::span<const std::byte>b) {
  std::array<unsigned char,SHA256_DIGEST_LENGTH>d;SHA256(reinterpret_cast<const unsigned char*>(b.data()),b.size(),d.data());
  constexpr char hex[]="0123456789abcdef";std::string out;for(auto c:d){out+=hex[c>>4];out+=hex[c&15];}return out;
}
}
int main(int argc,char**argv)try {
  if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
  std::ifstream f(argv[1]);Json rows;f>>rows;unsigned count{};
  for(const auto&row:rows) {
    const auto&q=row.at("input");std::array<std::byte,vista_lobby_leave_task_size>b;
    for(unsigned i=0;i<b.size();++i)b[i]=std::byte((i*37+q.at("seed").get<unsigned>()*13)&255);
    b[0x42]=std::byte(q.at("flags").get<unsigned>());put(b,0xa68,0xffffffffu);put(b,0xa5c,q.at("callback"));
    if(hash(b)!=row.at("before").get<std::string>())throw FormatError("Original task initial bank changed");
    Json events=Json::array(),frames=Json::array();
    if(q.at("op")=="bind") {
      Json e=Json::array({"factory"});for(auto word:vista_lobby_leave_task_descriptor(q.at("controller")))e.push_back(word);
      events.push_back(e);vista_lobby_leave_task_bind(b);
    }else {
      VistaLobbyLeaveTaskFrameServices s;std::uint32_t context{};
      s.lifecycle_context=[&](){events.push_back({"context",context});return context;};
      s.set_status=[&](auto status){events.push_back({"status",status});};
      s.close=[&](auto reason){events.push_back({"close",reason});};
      s.base_update=[&](){events.push_back({"base"});};
      for(const auto&c:q.at("contexts")){context=c.get<std::uint32_t>();vista_lobby_leave_task_frame(b,s);frames.push_back(hash(b));}
    }
    if(events!=row.at("events") || frames!=row.at("frames") || hash(b)!=row.at("after").get<std::string>()) {
      std::cerr<<"case="<<count<<" expected="<<row.dump()<<"\nactual="<<events.dump()<<" bank="<<hash(b)<<'\n';
      throw FormatError("Original task binding/frame owner mismatch");
    }++count;
  }
  if(count!=196)throw FormatError("Incomplete original Leave task corpus");
  std::array<std::byte,vista_lobby_leave_task_size>b{};vista_lobby_leave_task_bind(b);put(b,0xa68,1);const auto before=b;bool rejected{};
  try{vista_lobby_leave_task_frame(b,{});}catch(const FormatError&){rejected=true;}
  if(!rejected || b!=before)throw FormatError("Unbound async task mutated or fabricated progress");
  std::cout<<"PASS original Vista Leave task binding and sequential frames cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
