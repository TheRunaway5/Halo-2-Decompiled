#include "halo2/vista_session_disposal.hpp"
#include "halo2/vista_session_start_transition.hpp"
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
void put(std::span<std::byte> b,std::size_t at,std::uint32_t v) {for(unsigned i=0;i<4;++i)b[at+i]=std::byte(v>>(8*i));}
std::uint32_t get(std::span<const std::byte> b,std::size_t at) {std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[at+i])<<(8*i);return v;}
std::string hash(std::span<const std::byte> b) {
  std::array<unsigned char,SHA256_DIGEST_LENGTH> d;SHA256(reinterpret_cast<const unsigned char*>(b.data()),b.size(),d.data());
  constexpr char hex[]="0123456789abcdef";std::string out;for(const auto c:d){out+=hex[c>>4];out+=hex[c&15];}return out;
}
template<class F> void rejects(F f) {bool caught{};try{f();}catch(const FormatError&){caught=true;}if(!caught)throw FormatError("Missing disposal owner rejection");}
}
int main(int argc,char** argv)try {
  if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
  std::ifstream file(argv[1]);Json rows;file>>rows;unsigned count{};
  for(const auto& row:rows) {
    const auto& q=row.at("input");const auto seed=q.at("seed").get<unsigned>();
    std::array<std::byte,vista_session_source_size> b;std::array<std::byte,64> keys;std::array<std::byte,0x14+2*36> regs;
    std::vector<std::byte> browser(vista_browser_source_size);
    for(unsigned i=0;i<b.size();++i)b[i]=std::byte((i*37+seed*13)&255);
    for(unsigned i=0;i<keys.size();++i)keys[i]=std::byte((i*11+seed*7)&255);
    for(unsigned i=0;i<regs.size();++i)regs[i]=std::byte((i*17+seed*19)&255);
    for(unsigned i=0;i<browser.size();++i)browser[i]=std::byte((i*3+seed*5)&255);
    constexpr std::uint32_t owner_word=0x1a10000;
    for(const auto& [at,value]:std::array<std::pair<std::size_t,std::uint32_t>,8>{{{8,owner_word},{0x14,q.at("slot")},{0x5c,q.at("key_slot")},{0x74,0},{0x84,1},{0x71c0,0},{0x7b78,0},{0x73a0,q.at("state")}}})put(b,at,value);
    b[0x48]=std::byte(q.at("has_key").get<unsigned>());b[0x6c]=std::byte{};b[0x73a4]=std::byte(q.at("closing").get<unsigned>());
    for(unsigned i=0;i<17;++i)b[0x71c4+i*0x1c]=std::byte{};
    b[0x71c4]=std::byte(q.at("association").get<unsigned>());b[0x71c5]=std::byte{};put(b,0x71c8,0xffffffffu);
    const auto key=q.at("key_slot").get<unsigned>()*32;keys[key]=std::byte(q.at("key_active").get<unsigned>());keys[key+2]=std::byte(q.at("key_exempt").get<unsigned>());
    put(browser,0x428,q.at("handle"));put(browser,0xc,q.at("child"));browser[0x4044c]=std::byte(q.at("query").get<unsigned>());
    if(Json::array({hash(b),hash(keys),hash(regs),hash(browser)})!=row.at("before"))throw FormatError("Original disposal input bank changed");
    Json events=Json::array();bool complete{};unsigned kind=q.at("kind");
    VistaSessionDisposalServices ds;
    ds.release_qos=[&](auto id){events.push_back({"qos",get(id,0),get(id,4)});};
    ds.unregister_key=[&](auto id){events.push_back({"key",get(id,0),get(id,4)});};
    auto dispose=[&]{(void)vista_session_dispose_single_offline(b,{keys,regs,owner_word},q.at("network").get<unsigned>()!=0,q.at("qos").get<unsigned>()!=0,ds);};
    VistaBrowserResetServices bs;
    bs.close_handle=[&](auto v){events.push_back({"handle",v});};bs.reset_child=[&](auto v){events.push_back({"child",v});};
    bs.release_query=[&](auto data){events.push_back({"query",hash(data)});};
    auto reset=[&](auto mode){vista_browser_reset(browser,mode,bs);};
    VistaLobbyLeaveTaskServices ts;
    ts.lifecycle_context=[&]{constexpr std::array<unsigned,7> contexts{0,2,2,3,5,6,7};return kind<contexts.size()?contexts[kind]:0;};
    ts.set_status=[&](auto v){events.push_back({"status",v});};ts.complete=[&]{events.push_back({"complete"});complete=true;};
    const auto op=q.at("op").get<std::string>();
    if(op=="dispose")dispose();
    else if(op=="browser")reset(q.at("mode").get<std::uint32_t>());
    else if(op=="task")vista_lobby_leave_task_update(ts);
    else if(op=="leave") {
      VistaLobbyLeaveServices ls;
      ls.peer_count=[&]{const auto state=get(b,0x73a0);return state>=3 && state<=8?std::bit_cast<std::int32_t>(get(b,0x84)):0;};
      ls.browser_reset=[&]{reset(6u);};ls.disconnect=[&](bool full){if(!full)throw FormatError("Sole peer unexpectedly required partial disconnect");dispose();};
      ls.create_task=[&](auto c){events.push_back({"task",0xffffffffu,c,0x642e8c,0,0});};
      if(!vista_lobby_leave_single(q.at("controller"),ls))throw FormatError("Original sole peer rejected Leave");
      vista_lobby_leave_task_update(ts);
      if(q.at("pump").get<unsigned>()) {
        std::array<std::byte,32> payload{};VistaLifecycleManager m{kind,0,0,0,0,0,payload};
        VistaSessionStartTransitionState start;VistaSessionStartTransitionServices services;
        services.null_network_close=[&](auto slot,auto value){events.push_back({"mode",slot,value});};
        std::array<VistaLifecycleHandler,7> handlers;vista_session_bind_start_handlers(handlers,b,start,services);
        vista_lifecycle_pump(m,handlers,[&](auto&){events.push_back({"network_update"});});kind=m.current_kind;
        vista_lobby_leave_task_update(ts);
      }
    } else throw FormatError("Unknown disposal fixture operation");
    if(events!=row.at("events") || complete!=row.at("completed").get<bool>() || kind!=row.at("kind").get<unsigned>() ||
        Json::array({hash(b),hash(keys),hash(regs),hash(browser)})!=row.at("after")) {
      std::cerr<<"case="<<count<<" input="<<q.dump()<<"\nexpected="<<row.dump()<<"\nactual="<<events.dump()<<"\n";
      throw FormatError("Original Vista disposal mismatch");
    }
    ++count;
  }
  if(count!=697)throw FormatError("Incomplete original disposal corpus");
  std::array<std::byte,vista_session_source_size> bank{};put(bank,0x73a0,5);put(bank,0x84,2);
  const auto before=bank;rejects([&]{vista_session_dispose_single_offline(bank,{},false,false,{});});
  if(bank!=before)throw FormatError("Unbound remote disposal mutated source storage");
  std::vector<std::byte> browser(vista_browser_source_size);const auto before_browser=browser;
  rejects([&]{vista_browser_reset(browser,6,{});});if(browser!=before_browser)throw FormatError("Missing browser platform owner mutated storage");
  std::cout<<"PASS original Vista local disposal/browser/Leave cases="<<count<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
