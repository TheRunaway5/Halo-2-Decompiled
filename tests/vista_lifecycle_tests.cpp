#include "halo2/vista_lifecycle.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <bit>
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
  std::int32_t s(){return std::bit_cast<std::int32_t>(w());}
  std::uint8_t b(){const auto x=w();check(x<=255,label+": invalid byte");return std::uint8_t(x);}
  void end(){check(p==v.size(),label+": trailing fixture fields");}
};
std::vector<std::byte> bytes(std::span<const std::uint32_t> words) {
  std::vector<std::byte> result;
  for(const auto x:words)for(unsigned i=0;i<4;++i)result.push_back(std::byte((x>>(i*8))&255));
  return result;
}
std::uint32_t word(std::span<const std::byte> b) {
  check(b.size()>=4,"truncated payload word");std::uint32_t result{};
  for(unsigned i=0;i<4;++i)result|=std::uint32_t(std::to_integer<unsigned>(b[i]))<<(i*8);
  return result;
}
struct OwnedManager {
  std::array<std::byte,32> payload{};VistaLifecycleManager value;
  explicit OwnedManager(Words& r) {
    value.current_kind=r.w();value.transitioning=r.b();value.updating=r.b();value.pending=r.b();
    value.requested_kind=r.w();value.payload_length=r.s();
    for(unsigned i=0;i<8;++i) {const auto x=r.w();for(unsigned j=0;j<4;++j)payload[i*4+j]=std::byte((x>>(j*8))&255);}
    value.payload=payload;
  }
  OwnedManager(const OwnedManager&)=delete;
};
std::vector<std::uint32_t> bank(const VistaLifecycleManager& m) {
  check(m.payload.size()==32,"fixture payload storage identity changed");
  std::vector<std::uint32_t> result{m.current_kind,m.transitioning,m.updating,m.pending,m.requested_kind,std::bit_cast<std::uint32_t>(m.payload_length)};
  for(unsigned i=0;i<8;++i)result.push_back(word(m.payload.subspan(i*4,4)));
  return result;
}
std::vector<std::uint32_t> read_bank(Words& r){std::vector<std::uint32_t> v;for(unsigned i=0;i<14;++i)v.push_back(r.w());return v;}
VistaLifecycleInGameQueries queries(Words& r) {
  VistaLifecycleInGameQueries q;
  q.preflight=r.b();q.pending_after_preflight=r.b();q.admitted=r.b();q.closing=r.b();q.authority=r.b();q.phase=r.w();
  q.game_over_complete=r.b();q.force_end=r.b();q.world_ready=r.b();q.retained_identity={r.w(),r.w()};q.world_identity={r.w(),r.w()};
  q.selected_map_valid=r.b();q.selected_map={r.w(),r.w()};q.world_map={r.w(),r.w()};return q;
}
std::vector<VistaLifecycleCall> calls(Words& r) {
  std::vector<VistaLifecycleCall> result;const auto n=r.w();check(n<100,r.label+": too many calls");
  for(unsigned i=0;i<n;++i) {
    const auto service=r.w();check(service<=16,r.label+": invalid service ordinal");VistaLifecycleCall c;c.service=VistaLifecycleService(service);
    c.argument_count=r.w();check(c.argument_count<=3,r.label+": invalid call arity");
    for(unsigned j=0;j<c.argument_count;++j)c.arguments[j]=r.w();result.push_back(c);
  }return result;
}
std::vector<std::vector<std::uint32_t>> events(Words& r) {
  const auto n=r.w();check(n<100,r.label+": too many callback events");std::vector<std::vector<std::uint32_t>> result;
  for(unsigned i=0;i<n;++i) {const auto count=r.w();check(count<100,r.label+": event too large");std::vector<std::uint32_t> e;for(unsigned j=0;j<count;++j)e.push_back(r.w());result.push_back(e);}return result;
}
std::vector<std::uint32_t> event(unsigned type,unsigned kind,unsigned other,std::int32_t length,std::span<const std::byte> payload,const VistaLifecycleManager& m) {
  check(payload.size()%4==0,"unaligned fixture callback payload");
  std::vector<std::uint32_t> e{type,kind,other,std::bit_cast<std::uint32_t>(length),unsigned(payload.size()/4)};
  for(std::size_t i=0;i<payload.size();i+=4)e.push_back(word(payload.subspan(i,4)));
  const auto b=bank(m);e.insert(e.end(),b.begin(),b.end());return e;
}
using Action=std::array<std::uint32_t,23>;
std::vector<Action> actions(Words& r) {
  const auto n=r.w();check(n<100,r.label+": too many actions");std::vector<Action> result(n);
  for(auto& a:result)for(auto& x:a)x=r.w();return result;
}
void apply_action(const Action& a,VistaLifecycleManager& m) {
  if(a[3]&1) {const auto source=bytes(std::span(a).subspan(6,8));vista_lifecycle_request(m,a[4],std::bit_cast<std::int32_t>(a[5]),source);}
  if(a[3]&2) {const auto source=bytes(std::span(a).subspan(14,8));std::copy(source.begin(),source.end(),m.payload.begin());}
  if(a[3]&4)m.pending=std::uint8_t(a[22]);
}
void compare_calls(const VistaLifecycleUpdate& actual,const std::vector<VistaLifecycleCall>& expected,const std::string& label) {
  check(std::ranges::equal(actual.call_intents(),expected),label+": original service order/arguments differ");
}
template<class F> void rejected(F f,const std::string& why) {
  try{f();}catch(const FormatError&){return;}throw std::runtime_error("unsupported lifecycle boundary admitted: "+why);
}
void negative_controls() {
  std::array<std::byte,32> payload{};VistaLifecycleManager m{3,0,0,0,4,0,payload};const auto original=bank(m);
  std::array<std::byte,40> source{};
  rejected([&]{vista_lifecycle_request(m,4,33,source);},"oversize request");
  rejected([&]{vista_lifecycle_request(m,4,4,std::span(source).first(3));},"truncated source");
  check(bank(m)==original,"Rejected request mutated manager");
  auto short_manager=m;short_manager.payload=std::span(payload).first(3);
  rejected([&]{vista_lifecycle_request(short_manager,4,0,{});},"missing first payload word");
  std::array<VistaLifecycleHandler,8> handlers;
  for(auto& h:handlers){h.update=[](auto&){return std::uint8_t{};};h.exit=[](auto&,auto){};h.enter=[](auto&,auto,auto,auto){};}
  rejected([&]{vista_lifecycle_pump(m,handlers,{});},"absent network owner");
  m.current_kind=8;rejected([&]{vista_lifecycle_pump(m,handlers,[](auto&){});},"absent current handler");
  m.current_kind=3;m.pending=1;m.payload_length=33;
  rejected([&]{vista_lifecycle_pump(m,handlers,[](auto&){});},"truncated pending payload");
  m.pending=0;m.payload_length=0;handlers[3].update={};
  rejected([&]{vista_lifecycle_pump(m,handlers,[](auto&){});},"absent update callback");
  m.pending=1;m.requested_kind=4;handlers[3].exit={};
  rejected([&]{vista_lifecycle_pump(m,handlers,[](auto&){});},"absent exit callback");
}
}
int main(int argc,char** argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply original Vista lifecycle TSV");std::ifstream file(argv[1]);if(!file)return 77;
    std::string line;bool version{};unsigned requests{},pumps{},ingame{},postgame{},enters{},joined{};
    while(std::getline(file,line)) {
      if(line=="# H2V_LIFECYCLE_ORIGINAL_V1")version=true;if(line.empty()||line.front()=='#')continue;
      std::istringstream row(line);char kind{};Words r;std::size_t count{};row>>kind>>r.label>>count;check(bool(row)&&count<10000,"Malformed lifecycle row");
      std::string text;while(row>>text){std::size_t used{};const auto n=std::stoull(text,&used,16);check(used==text.size()&&n<=0xffffffffu,"Invalid fixture word");r.v.push_back(std::uint32_t(n));}
      check(r.v.size()==count,r.label+": field count differs");
      if(kind=='R') {
        OwnedManager m(r);const auto requested=r.w();const auto length=r.s();std::array<std::uint32_t,8> source;for(auto& x:source)x=r.w();const auto expected=read_bank(r);
        const auto input=bytes(source);vista_lifecycle_request(m.value,requested,length,input);check(bank(m.value)==expected,r.label+": original request bytes differ");++requests;
      } else if(kind=='I' || kind=='G' || kind=='E') {
        VistaLifecycleUpdate actual;
        if(kind=='I'){const auto q=queries(r);actual=vista_lifecycle_in_game_update(q);++ingame;}
        else if(kind=='G'){const VistaLifecyclePostGameQueries q{r.b(),r.b(),r.w()};actual=vista_lifecycle_post_game_update(q);++postgame;}
        else {actual=vista_lifecycle_post_game_enter(r.w());++enters;}
        const auto expected_al=r.b();const auto expected=calls(r);if(kind!='E')check(actual.repeat==expected_al,r.label+": original raw AL differs");compare_calls(actual,expected,r.label);
      } else if(kind=='P') {
        OwnedManager m(r);const auto script=actions(r);const auto expected=read_bank(r);const auto expected_events=events(r);
        std::vector<std::vector<std::uint32_t>> actual_events;std::size_t step{};
        auto callback=[&](unsigned type,unsigned h,unsigned other,std::int32_t length,std::span<const std::byte> data,auto& state) {
          actual_events.push_back(event(type,h,other,length,data,state));check(step<script.size(),r.label+": missing callback script");const auto& a=script[step++];
          check(a[0]==type&&a[1]==h,r.label+": callback invocation differs");apply_action(a,state);return std::uint8_t(a[2]);
        };
        std::array<VistaLifecycleHandler,8> handlers;
        for(unsigned i=0;i<handlers.size();++i){handlers[i].update=[&,i](auto& s){return callback(1,i,0,0,{},s);};handlers[i].exit=[&,i](auto& s,unsigned other){callback(2,i,other,0,{},s);};handlers[i].enter=[&,i](auto& s,unsigned old,std::int32_t length,auto data){callback(3,i,old,length,data,s);};}
        vista_lifecycle_pump(m.value,handlers,[&](auto& s){callback(0,0,0,0,{},s);});
        check(step==script.size()&&bank(m.value)==expected,r.label+": final manager or action consumption differs");check(actual_events==expected_events,r.label+": original callback states/alias/order differ");++pumps;
      } else if(kind=='J') {
        OwnedManager m(r);auto q=queries(r);const auto expected=read_bank(r);const auto expected_phase=r.w();const auto expected_calls=calls(r);const auto expected_events=events(r);
        auto phase=q.phase;std::vector<VistaLifecycleCall> actual_calls;std::vector<std::vector<std::uint32_t>> actual_events;
        auto consume=[&](auto& state,const VistaLifecycleUpdate& out){for(const auto& c:out.call_intents()){actual_calls.push_back(c);if(c.service==VistaLifecycleService::phase_write)phase=c.arguments[0];if(c.service==VistaLifecycleService::request)vista_lifecycle_request(state,c.arguments[0],std::bit_cast<std::int32_t>(c.arguments[1]),{});}return out.repeat;};
        std::array<VistaLifecycleHandler,8> handlers;
        handlers[3].update=[&](auto& s){q.phase=phase;q.pending_after_preflight=s.pending;return consume(s,vista_lifecycle_in_game_update(q));};
        handlers[3].exit=[&](auto& s,unsigned next){actual_events.push_back(event(2,3,next,0,{},s));};
        handlers[4].enter=[&](auto& s,unsigned old,std::int32_t,auto){consume(s,vista_lifecycle_post_game_enter(old));};
        handlers[4].update=[&](auto& s){return consume(s,vista_lifecycle_post_game_update({q.preflight,s.pending,phase}));};
        vista_lifecycle_pump(m.value,handlers,[&](auto& s){actual_events.push_back(event(0,0,0,0,{},s));});
        check(bank(m.value)==expected&&phase==expected_phase,r.label+": joined manager/phase differs");check(actual_calls==expected_calls&&actual_events==expected_events,r.label+": joined callback/intent order differs");++joined;
      } else throw std::runtime_error("Unknown lifecycle fixture kind");
      r.end();
    }
    check(version&&requests==45&&pumps==12&&ingame==425&&postgame==45&&enters==5&&joined==1,"Incomplete independent lifecycle coverage");negative_controls();
    std::cout<<requests<<" request, "<<pumps<<" reentrant pump, "<<ingame<<" in-game, "<<postgame<<" postgame, "<<enters<<" entry and "<<joined<<" joined original cases passed\n";return 0;
  }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
