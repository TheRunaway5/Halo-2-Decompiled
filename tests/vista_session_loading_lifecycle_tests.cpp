#include "halo2/vista_session_loading_lifecycle.hpp"
#include "halo2/vista_lifecycle.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace halo2;
namespace {
void check(bool ok,const std::string& text) {if(!ok)throw std::runtime_error(text);}
struct Words {
  std::vector<std::uint32_t> v;std::size_t p{};std::string label;
  std::uint32_t w(){check(p<v.size(),label+": truncated fixture");return v[p++];}
  std::uint8_t b(){const auto x=w();check(x<=255,label+": invalid byte");return std::uint8_t(x);}
  void end(){check(p==v.size(),label+": trailing fixture fields");}
};
std::vector<VistaSessionLoadingCall> calls(Words& r) {
  std::vector<VistaSessionLoadingCall> result;const auto n=r.w();
  check(n<=16,r.label+": too many calls");
  for(unsigned i=0;i<n;++i) {
    const auto service=r.w();check(service<=15,r.label+": invalid service ordinal");
    VistaSessionLoadingCall c;c.service=VistaSessionLoadingService(service);
    c.argument_count=r.w();check(c.argument_count<=3,r.label+": invalid call arity");
    for(unsigned j=0;j<c.argument_count;++j)c.arguments[j]=r.w();
    result.push_back(c);
  }
  return result;
}
VistaSessionLoadingUpdateQueries query(Words& r) {
  VistaSessionLoadingUpdateQueries q;
  q.preflight=r.b();q.pending_after_preflight=r.b();q.admitted=r.b();q.phase=r.w();
  q.authority=r.b();q.players_prepared=r.b();
  VistaSessionLoadingGateQueries g;
  g.session_state=r.w();g.network_query=r.b();g.local_flag=r.b();
  g.session_state_after_query=r.w();g.transition_flag=r.b();g.refresh_accepted=r.b();q.gate=g;
  return q;
}
void boundaries() {
  VistaSessionLoadingHandlerState s{0x80,0xff};const auto before=s;
  try {vista_session_loading_enter(s,1,{5,2,{}});throw std::runtime_error("missing seed/random accepted");}
  catch(const FormatError&) {}
  check(s==before,"rejected enter mutated handler");
  VistaSessionLoadingUpdateQueries q{0,0,1,2,1,1,{}};
  try {vista_session_loading_update(s,q);throw std::runtime_error("missing gate accepted");}
  catch(const FormatError&) {}
  check(s==before,"rejected update mutated handler");
  // Unconsumed optional inputs are not required by an earlier source branch.
  q.pending_after_preflight=0x80;
  check(vista_session_loading_update(s,q).call_count==1,"pending request reached gate");
  q.pending_after_preflight=0;q.players_prepared=0;
  check(vista_session_loading_update(s,q).call_count==4,"failed preparation reached gate");
  auto r=vista_session_loading_enter(s,1,{4,2,{}});
  check(r.call_count==1 && s==VistaSessionLoadingHandlerState{},"inactive enter did not clear exact two bytes");
}
void same_pump() {
  // Native composition at the already-source-qualified lifecycle seam. The
  // independent original joined pump control is retained by the oracle.
  std::array<std::byte,4> payload{};VistaLifecycleManager m{2,0,0,0,0,0,payload};
  std::array<VistaLifecycleHandler,4> handlers;
  VistaSessionLoadingHandlerState flags{1,0x80};
  VistaSessionLoadingUpdateQueries q{0,0,1,2,1,1,
      VistaSessionLoadingGateQueries{5,0,0,5,0,1}};
  std::vector<unsigned> events;
  for(auto& h:handlers) {
    h.enter=[](auto&,auto,auto,auto){};h.exit=[](auto&,auto){};
    h.update=[](auto&){return std::uint8_t{};};
  }
  handlers[2].update=[&](auto& owner) {
    events.push_back(2);const auto r=vista_session_loading_update(flags,q);
    for(const auto& c:r.call_intents()) {
      if(c.service==VistaSessionLoadingService::phase_write)q.phase=c.arguments[0];
      if(c.service==VistaSessionLoadingService::request)
        vista_lifecycle_request(owner,c.arguments[0],0,{});
    }
    return r.repeat;
  };
  handlers[2].exit=[&](auto&,auto next){check(next==3,"wrong next handler");events.push_back(20);};
  handlers[3].enter=[&](auto&,auto old,auto length,auto bytes) {
    check(old==2 && length==0 && bytes.empty(),"wrong in-game enter arguments");events.push_back(30);
  };
  handlers[3].update=[&](auto&){events.push_back(3);return std::uint8_t{};};
  vista_lifecycle_pump(m,handlers,[&](auto&){events.push_back(0);});
  check(events==std::vector<unsigned>{0,2,2,20,30,3},"phase4/request3 required an invented frame gap");
  check(m.current_kind==3 && !m.pending && q.phase==4,"same-pump transition lost owner state");
}
} // namespace
int main(int argc,char** argv) {
  try {
    check(argc==2,"provide original fixture path");std::ifstream file(argv[1]);check(bool(file),"fixture open failed");
    std::string line;unsigned count{};
    while(std::getline(file,line)) {
      if(line.empty() || line[0]=='#')continue;
      std::istringstream input(line);char kind{};Words r;unsigned length{};input>>kind>>r.label>>length;
      for(unsigned i=0;i<length;++i) {std::uint32_t x{};input>>std::hex>>x;check(bool(input),r.label+": invalid fixture word");r.v.push_back(x);}
      VistaSessionLoadingHandlerState s;VistaSessionLoadingUpdate result;
      if(kind=='E') {
        const auto old=r.w();VistaSessionLoadingEnterQueries q;q.session_state=r.w();q.phase=r.w();
        q.values=VistaSessionLoadingEnterValues{r.w(),{r.w(),r.w()}};s={r.b(),r.b()};
        result=vista_session_loading_enter(s,old,q);
      } else {
        check(kind=='U',r.label+": invalid fixture type");s={r.b(),r.b()};const auto q=query(r);
        result=vista_session_loading_update(s,q);
      }
      const VistaSessionLoadingHandlerState expected{r.b(),r.b()};const auto repeat=r.b();const auto expected_calls=calls(r);r.end();
      check(s==expected,r.label+": original handler bytes differ");
      check(result.repeat==repeat,r.label+": original raw AL differs");
      check(std::ranges::equal(result.call_intents(),expected_calls),r.label+": original service order/arguments differ");++count;
    }
    check(count==5598,"original fixture case count changed");boundaries();same_pump();
    std::cout<<"PASS "<<count<<" original Vista start-game lifecycle cases; boundaries and same-pump composition\n";
    return 0;
  } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
