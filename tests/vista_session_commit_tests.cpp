#include "halo2/vista_session_commit.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace halo2;
namespace {
void check(bool ok,const std::string& why) {if(!ok)throw std::runtime_error(why);}
struct Words {
  std::vector<std::uint32_t> values;std::size_t cursor{};std::string label;
  std::uint32_t w() {check(cursor<values.size(),label+": truncated original row");return values[cursor++];}
  std::int32_t s() {return std::bit_cast<std::int32_t>(w());}
  std::uint8_t b() {auto value=w();check(value<=255,label+": original byte out of range");return std::uint8_t(value);}
  void end() {check(cursor==values.size(),label+": trailing original words");}
};
VistaSessionCommitContext context(Words& r) {return {r.w(),r.w(),r.w(),r.b()};}
VistaSessionCountdownCommitState countdown(Words& r) {
  return {r.b(),r.s(),r.b(),r.w(),r.s(),{r.w(),r.w()},r.w()};
}
VistaSessionCountdownWrite request(Words& r) {
  VistaSessionCountdownWrite value{r.b(),r.s(),r.b(),r.w(),std::nullopt};
  const auto present=r.w();check(present<=1,r.label+": invalid identity admission");
  const std::array<std::uint32_t,2> identity{r.w(),r.w()};
  if(present)value.identity=identity;
  return value;
}
VistaSessionPhaseState phase(Words& r) {return {r.w(),r.w(),r.w(),r.b(),r.w(),r.w()};}
std::vector<VistaSessionPhasePeer> peers(Words& r,std::uint32_t count) {
  check(count<=64,r.label+": unsupported fixture peer extent");
  std::vector<VistaSessionPhasePeer> result;
  for(std::uint32_t i=0;i<count;++i)result.push_back({r.b(),r.b()});
  return result;
}
void calls(Words& r,const VistaSessionZeroResult& result) {
  const auto count=r.w();check(count==result.call_count,r.label+": original service count differs");
  for(unsigned i=0;i<count;++i) {
    VistaSessionZeroCall expected;
    const auto service=r.w();check(service<=5,r.label+": invalid original service");
    expected.service=VistaSessionZeroService(service);expected.argument_count=r.w();
    check(expected.argument_count<=5,r.label+": invalid original argument count");
    for(unsigned j=0;j<expected.argument_count;++j)expected.arguments[j]=r.w();
    check(result.calls[i]==expected,r.label+": original service/arguments/order differ");
  }
}
template<class F> void rejected(F function,const std::string& label) {
  try {function();}catch(const FormatError&){return;}
  throw std::runtime_error(label+": unsupported context admitted");
}
void unsupported_controls() {
  VistaSessionCountdownCommitState state{1,3,0,0,-1,{7,8},0xFFFFFFFF};
  const auto before=state;
  const VistaSessionCommitContext local{5,3,3,0};
  rejected([&]{vista_session_countdown_commit(state,local,3,{1,10,0,1,std::nullopt});},"consumed identity");
  check(state==before,"Missing identity partially committed session fields");
  rejected([&]{vista_session_countdown_local_write(state,{4,3,3,0},{1,10,0,0,std::nullopt});},"remote state");
  rejected([&]{vista_session_countdown_local_write(state,{5,3,4,0},{1,10,0,0,std::nullopt});},"nonhost local service");
  check(state==before,"Unsupported local context changed countdown");
  std::vector<std::int32_t> statuses{3};
  rejected([&]{vista_session_peer_readiness(2,statuses);},"truncated readiness");
  VistaSessionPhaseState p{1,2,3,4,5,6};const auto pbefore=p;std::uint32_t revision=7;
  std::vector<VistaSessionPhasePeer> flags{{1,0x80}};const auto flags_before=flags;
  rejected([&]{vista_session_phase_write(p,revision,2,2,flags,{11,12});},"truncated phase peers");
  check(p==pbefore && revision==7 && flags==flags_before,"Rejected phase write partially mutated fields");
  auto zero_state=state;zero_state.stage=0;const auto zero_before=zero_state;
  rejected([&]{vista_session_countdown_zero(zero_state,p,local,0,2,statuses,flags,{11,12});},"truncated zero readiness");
  check(zero_state==zero_before && p==pbefore && flags==flags_before,
      "Rejected zero-stage readiness partially mutated fields");
  // An unchanged phase consumes no peer fields or clock samples.
  check(vista_session_phase_write(p,revision,1,2,{}, {})==0 && p==pbefore && revision==7,
      "Unchanged source phase consumed missing peer fields");
}
}
int main(int argc,char** argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply original Vista session commit fixture");
    std::ifstream input(argv[1]);check(bool(input),"Missing Vista session commit fixture");
    std::string line;bool version{};unsigned commits{},locals{},readiness{},phases{},zeros{};
    while(std::getline(input,line)) {
      if(line=="# H2V_SESSION_COMMIT_ORIGINAL_V1")version=true;
      if(line.empty() || line.front()=='#')continue;
      std::istringstream row(line);char kind{};Words r;std::size_t size{};row>>kind>>r.label>>size;
      check(bool(row) && size<4096,"Malformed original Vista session row");
      std::string text;
      while(row>>text) {
        std::size_t used{};const auto value=std::stoull(text,&used,16);
        check(used==text.size() && value<=0xFFFFFFFFu,"Invalid original Vista word");
        r.values.push_back(std::uint32_t(value));
      }
      check(r.values.size()==size,r.label+": original word count differs");
      if(kind=='C' || kind=='L') {
        const auto ctx=context(r);const auto sender=kind=='C'?r.w():ctx.host_peer;
        const auto write=request(r);auto state=countdown(r);const auto expected=countdown(r);const bool al=r.w()!=0;
        if(kind=='C') {
          check(vista_session_countdown_commit(state,ctx,sender,write)==al,r.label+": original commit AL differs");++commits;
        } else {
          check(vista_session_countdown_local_write(state,ctx,write).accepted==al,r.label+": original local wrapper AL differs");++locals;
        }
        check(state==expected,r.label+": original committed fields differ");
      } else if(kind=='R') {
        const auto count=r.s();const auto extent=r.w();check(extent<=64,r.label+": readiness extent out of range");
        std::vector<std::int32_t> statuses;for(unsigned i=0;i<extent;++i)statuses.push_back(r.s());
        const VistaSessionReadiness expected{r.w()!=0,r.w()};
        check(vista_session_peer_readiness(count,statuses)==expected,r.label+": original readiness/mask differs");++readiness;
      } else if(kind=='P') {
        const auto requested=r.w();const auto count=r.s();const std::array<std::uint32_t,2> clocks{r.w(),r.w()};
        auto state=phase(r);auto revision=r.w();const auto extent=r.w();auto flags=peers(r,extent);
        const auto expected=phase(r);const auto expected_revision=r.w();const auto expected_flags=peers(r,extent);const auto clock_count=r.w();
        check(vista_session_phase_write(state,revision,requested,count,flags,clocks)==clock_count,
            r.label+": original phase clock count differs");
        check(state==expected && revision==expected_revision && flags==expected_flags,r.label+": original phase fields differ");++phases;
      } else if(kind=='Z') {
        const auto ctx=context(r);const auto preflight=r.w();const auto count=r.s();
        const std::array<std::uint32_t,2> clocks{r.w(),r.w()};auto state=countdown(r);auto p=phase(r);
        const auto extent=r.w();check(extent<=64,r.label+": zero peer extent out of range");
        std::vector<std::int32_t> statuses;for(unsigned i=0;i<extent;++i)statuses.push_back(r.s());auto flags=peers(r,extent);
        const auto expected=countdown(r);const auto expected_phase=phase(r);const auto expected_flags=peers(r,extent);const auto action=r.w();
        const auto result=vista_session_countdown_zero(state,p,ctx,preflight,count,statuses,flags,clocks);
        check(unsigned(result.action)==action,r.label+": original zero-stage action differs");calls(r,result);
        check(state==expected && p==expected_phase && flags==expected_flags,r.label+": original joined zero-stage fields differ");++zeros;
      } else throw std::runtime_error("Unknown original Vista session record");
      r.end();
    }
    check(input.eof() && version && commits==12611 && locals==720 && readiness==17 && phases==36 && zeros==480,
        "Original Vista session commit coverage incomplete");
    unsupported_controls();
    std::cout<<commits<<" commits, "<<locals<<" actual local wrappers, "<<readiness<<" readiness, "
        <<phases<<" phase writes and "<<zeros<<" joined zero-stage cases matched; no live activation\n";
    return 0;
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
