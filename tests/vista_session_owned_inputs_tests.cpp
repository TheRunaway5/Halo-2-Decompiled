#include "halo2/vista_session_storage.hpp"
#include "halo2/vista_session_local_peer.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const std::string& why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> pattern(std::size_t size,unsigned seed) {
  std::vector<std::byte> out(size+32,std::byte{0xa5});
  for(std::size_t i=0;i<size;++i)out[i]=std::byte((i*37+(i>>8)*17+seed)&255);
  return out;
}
std::vector<std::byte> hex(const std::string& text) {
  check(text.size()%2==0,"invalid original hex");std::vector<std::byte> out(text.size()/2);
  for(std::size_t i=0;i<out.size();++i)out[i]=std::byte(std::stoul(text.substr(i*2,2),nullptr,16));
  return out;
}
std::u16string utf16(const std::string& text) {
  const auto b=hex(text);check(b.size()%2==0,"odd source UTF16");std::u16string out;
  for(std::size_t i=0;i<b.size();i+=2)out+=char16_t(std::to_integer<unsigned>(b[i])|(std::to_integer<unsigned>(b[i+1])<<8));
  return out;
}
void patches(std::vector<std::byte>& b,const Json& patches,std::size_t limit) {
  for(const auto& p:patches) {
    const auto o=p.at(0).get<std::size_t>();const auto v=hex(p.at(1).get<std::string>());
    check(o+v.size()<=limit,"unowned fixture patch");std::copy(v.begin(),v.end(),b.begin()+o);
  }
}
void put(std::vector<std::byte>& b,std::size_t o,std::uint32_t v,unsigned n=4) {
  for(unsigned i=0;i<n;++i)b[o+i]=std::byte(v>>(8*i));
}
template<class F> void rejected(F&& f,const std::string& why) {
  try{f();}catch(const FormatError&){return;}throw std::runtime_error("unsupported owned input admitted: "+why);
}
void same(const std::vector<std::byte>& actual,const std::vector<std::byte>& expected,const std::string& why) {
  if(actual==expected)return;
  const auto offset=std::mismatch(actual.begin(),actual.end(),expected.begin()).first-actual.begin();
  throw std::runtime_error(why+" at offset "+std::to_string(offset));
}
void boundaries() {
  auto b=pattern(vista_session_source_size,19),owners=pattern(0x1000,19);auto before=b,owner_before=owners;
  VistaSessionStorageBindings bindings{{1,2,3,4},5,std::span(owners).first(0x400),std::span(owners).subspan(0x400,0x400)};
  rejected([&]{vista_session_storage_initialize(b,0xffffffffu,1,0,bindings);},"unowned registry slot");
  check(b==before && owners==owner_before,"storage rejection changed owner");
  rejected([&]{vista_session_storage_initialize(std::span(b).first(vista_session_source_size-1),0,1,0,bindings);},"truncated session");
  check(b==before && owners==owner_before,"storage truncation changed owner");
  put(b,0x73a0,5);put(b,0x84,1);put(b,0x71c0,0);put(b,0xac,0,1);before=b;
  VistaSessionLocalPeerValues v{u"Actual Name",u"Actual Party",0,0,0,2,100,1,{{},{}}};
  std::uint32_t last{};
  rejected([&]{vista_session_publish_local_peer(b,last,100,v,{});},"missing accepted publication clock");
  check(b==before && last==0,"publication clock rejection changed owner");
  last=950;rejected([&]{vista_session_publish_local_peer(b,last,100,v,{});},"missing elapsed clock");
  check(b==before && last==950,"elapsed rejection changed owner");
  put(b,0x71c0,0xffffffffu);before=b;
  const auto throttled=vista_session_publish_local_peer(b,last,100,v,{1000,{}});
  check(!throttled.built && throttled.elapsed_clock_consumed && b==before && last==950,"throttle inspected unconsumed membership");
  put(b,0x71c0,0);put(b,0x84,2);put(b,0x71c5+0x1c,1,1);put(b,0x71c8+0x1c,0);
  rejected([&]{vista_session_peer_connectivity(b,{{},{}});},"missing consumed channel owner");
  std::vector<std::byte> channel(0x740);put(channel,0x80,7);put(channel,0x88,0x10,1);put(channel,0x100,100);
  rejected([&]{vista_session_peer_connectivity(b,{channel,{}});},"missing consumed statistic scale");
  rejected([&]{vista_session_peer_connectivity(b,{channel,0x3f800000u});},"trim destroys all samples");
  put(channel,0x88,0,1);
  check(vista_session_peer_connectivity(b,{channel,{}})==std::array<std::uint32_t,4>{3,0,0,0},"unconsumed scale required");
}
}
int main(int argc,char** argv) {
  try {
    check(argc==2,"provide original fixture path");std::ifstream file(argv[1]);check(bool(file),"fixture open failed");
    std::string line;unsigned count{};
    while(std::getline(file,line)) {
      if(line.empty())continue;
      const auto row=Json::parse(line);const auto label=row.at("label").get<std::string>();const auto kind=row.at("kind").get<std::string>();
      const auto& in=row.at("inputs");const auto seed=row.at("seed").get<unsigned>();
      auto actual=pattern(vista_session_source_size,seed);patches(actual,row.at("before_patches"),vista_session_source_size);
      auto expected=actual;patches(expected,row.at("after_patches"),vista_session_source_size);
      if(kind=="S") {
        auto owners=pattern(0x1000,seed);auto owner_expected=owners;patches(owner_expected,row.at("owner_patches"),0x1000);
        VistaSessionStorageBindings bindings{{0x1a10200u,0x1a20000u,0x1a20400u,0x1a20800u},0x1a00000u,
          std::span(owners).first(0x400),std::span(owners).subspan(0x400,0x400)};
        vista_session_storage_initialize(actual,in.at("slot").get<std::uint32_t>(),in.at("mode").get<std::uint32_t>(),in.at("network_slot").get<std::uint32_t>(),bindings);
        check(row.at("al")==1,label+": unexpected original initializer result");same(owners,owner_expected,label+": actual registration owner differs");
      } else {
        const auto channels=hex(in.at("channels").get<std::string>());
        const VistaSessionConnectivityValues connectivity{channels,in.at("scale").get<std::uint32_t>()};
        if(kind=="C")check(vista_session_peer_connectivity(actual,connectivity)==row.at("connectivity").get<std::array<std::uint32_t,4>>(),label+": source connectivity differs");
        else {
          check(kind=="P",label+": invalid fixture kind");
          const auto name=utf16(in.at("name_utf16_hex").get<std::string>()),party=utf16(in.at("party_utf16_hex").get<std::string>());
          const VistaSessionLocalPeerValues values{name,party,in.at("optional").get<std::uint8_t>(),in.at("metadata68").get<std::uint32_t>(),
            in.at("metadata70").get<std::uint32_t>(),in.at("map_status").get<std::uint32_t>(),in.at("map_progress").get<std::uint32_t>(),in.at("bandwidth").get<std::uint32_t>(),connectivity};
          auto last=in.at("last").get<std::uint32_t>();
          const auto result=vista_session_publish_local_peer(actual,last,in.at("threshold").get<std::int32_t>(),values,
            {in.at("elapsed").get<std::uint32_t>(),in.at("publication").get<std::uint32_t>()});
          check(result.built==row.at("built").get<bool>() && result.publication_attempted==row.at("attempted").get<bool>() && result.accepted==row.at("accepted").get<bool>(),label+": source publication admission differs");
          check(result.elapsed_clock_consumed==(row.at("elapsed_calls").get<unsigned>()!=0) && result.publication_clock_consumed==(row.at("publication_calls").get<unsigned>()!=0) && last==row.at("last_clock").get<std::uint32_t>(),label+": clock consumption differs");
          if(result.built)check(std::ranges::equal(result.payload,hex(row.at("payload_hex").get<std::string>())),label+": complete source packet differs");
          check(bool(result.message)==!row.at("message").is_null(),label+": transport call differs");
          if(result.message) {
            const auto& message=*result.message;const auto& original=row.at("message");
            check(message.transport_owner==original.at("transport_owner").get<std::uint32_t>() && message.session_slot==original.at("session_slot").get<std::uint32_t>() && message.channel==original.at("channel").get<std::uint32_t>() && std::ranges::equal(message.payload,hex(original.at("payload_hex").get<std::string>())),label+": original outgoing transport operands differ");
          }
        }
      }
      same(actual,expected,label+": complete original session bank differs");++count;
    }
    check(count==373,"original owned-input fixture count changed");boundaries();
    std::cout<<"PASS "<<count<<" original complete Vista storage/local-peer/channel banks; atomic owner boundaries and suffix preservation\n";return 0;
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
