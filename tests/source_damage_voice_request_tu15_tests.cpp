#include "halo2/source_damage_voice_request_tu15.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <iostream>
#include <map>
#include <cstring>

using namespace halo2;
namespace {
using namespace physics_world_runtime_detail;
void check(bool yes,const char *message){if(!yes)throw std::runtime_error(message);}
struct Memory final:SourcePhysicsContactMemory {
  std::array<std::byte,0x5000> bytes{};
  void read(std::uint32_t p,std::span<std::byte> out)const override {
    if(p<0x10000 || out.size()>bytes.size() || p-0x10000>bytes.size()-out.size())throw std::runtime_error("Missing declared voice caller bytes");
    std::copy_n(bytes.begin()+p-0x10000,out.size(),out.begin());
  }
  void write(std::uint32_t p,std::span<const std::byte> in)override {
    if(p<0x10000 || in.size()>bytes.size() || p-0x10000>bytes.size()-in.size())throw std::runtime_error("Missing declared voice caller bytes");
    std::copy(in.begin(),in.end(),bytes.begin()+p-0x10000);
  }
  void word(std::uint32_t p,std::uint32_t value){for(unsigned n=0;n<4;++n)bytes.at(p-0x10000+n)=std::byte(value>>(n*8));}
};
struct Boundary {};
struct ObservedDamageState:WorldState {
  using WorldState::WorldState;
  unsigned calls{};
};
std::uint32_t damage_observation_pc(std::uint32_t disc,std::uint32_t tu15,const ObservedDamageState &s) {
  return s.image==SourcePhysicsContactImage::TitleUpdate15?tu15:disc;
}
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image) {
  check(p==0xcb7c2u,"Unexpected extracted caller dependency");
  return image==SourcePhysicsContactImage::TitleUpdate15?0xcc942u:p;
}
void run_115fa0(ObservedDamageState &s) {
  ++s.calls;
  // Stop at the actual native invocation boundary. No selected sound,
  // cooldown, original return value or downstream instruction is supplied.
  throw Boundary{};
}
#include HALO2_DAMAGE_VOICE_CALLER
struct Result {
  Memory memory;
  SourcePhysicsContactRuntimeAbi abi{};
  unsigned calls{};
  std::vector<std::uint32_t> trace;
  std::vector<SourceDamageVoiceRequestTu15> requests;
};
Result run(SourcePhysicsContactImage image,std::uint32_t flags,std::uint32_t body,
    std::uint32_t shield,bool listen,bool foreign=false) {
  Result result;auto &m=result.memory;
  constexpr std::uint32_t report=0x10000,unit=0x11000,stack=0x14000,target=0x12340007;
  m.word(report+4,flags);m.word(report+8,0xdead1234);m.word(report+0x44,body);m.word(report+0x48,shield);
  m.word(unit+0x100,0x3f400000);m.word(unit+0xf4,0x3e000000);m.word(unit+0xf8,0x3f000000);
  m.word(stack+0x20,flags&1);
  SourcePhysicsContactRuntimeServices services;
  services.instruction=[&](auto pc){result.trace.push_back(pc);};
  const SourcePhysicsContactRecordObserver stores;
  ObservedDamageState state(m,stores,services,image);
  auto &a=result.abi;a.gp[3]=target;a.gp[4]=stack;a.gp[5]=report;a.gp[6]=unit;a.eflags=0x202;
  state.load(a);state.xmm[1]=_mm_setzero_ps();
  Memory other;
  SourceDamageVoiceRequestScopeTu15 scope(listen?(foreign?&other:&m):nullptr,
      [&](const auto &request){result.requests.push_back(request);});
  try {extracted_damage_voice_caller(state);}catch(const Boundary &){}
  result.calls=state.calls;result.abi=state.save();
  if(result.calls) {
    check(result.abi.gp[4]==stack-20,"Native voice request changed its original caller stack");
    check(source_damage_voice_request_detail::word(m,stack-16)==target,"Native voice target argument differs");
    check(source_damage_voice_request_detail::word(m,stack-12)==((flags>>6)&0xffffff01u),"Native voice flag argument differs");
    check(source_damage_voice_request_detail::word(m,stack-8)==0x3f000000,"Native voice visible-body argument differs");
    check(source_damage_voice_request_detail::word(m,stack-4)==0x3e000000,"Native voice recent-shield argument differs");
  }
  return result;
}
void same(const Result &a,const Result &b) {
  check(a.memory.bytes==b.memory.bytes && a.abi.gp==b.abi.gp && a.abi.xmm==b.abi.xmm &&
      a.abi.eflags==b.abi.eflags && a.abi.x87_status==b.abi.x87_status && a.trace==b.trace && a.calls==b.calls,
      "Voice observer changed the original caller ABI, bytes, flags or instruction trace");
}
void scope_restore() {
  Memory owner;unsigned foreign_requests{};
  SourceDamageVoiceRequestScopeTu15 outer(&owner,[&](const auto &){++foreign_requests;});
  const auto *binding=source_damage_voice_request_detail::active;
  const auto nested=run(SourcePhysicsContactImage::TitleUpdate15,1,0,0,true);
  check(nested.requests.size()==1 && !foreign_requests && source_damage_voice_request_detail::active==binding,
      "Nested native world failed to restore its preceding voice observer");
  try {SourceDamageVoiceRequestScopeTu15 disabled(nullptr,{});throw Boundary{};}
  catch(const Boundary &){}
  check(source_damage_voice_request_detail::active==binding,
      "Failed native caller did not restore its preceding voice observer");
}
}
int main()try {
  struct Case {std::uint32_t flags,body,shield;bool admitted;};
  const std::array cases{
      Case{0,0,0,false},Case{0,0x3e800000,0,true},Case{0,0,0x3f000000,true},
      Case{1,0,0,true},Case{1,0xbf800000,0xbf800000,true},Case{0x20,0x3f800000,0,false},
      Case{0x21,0,0,false},Case{0x40,0x3f800000,0,true},Case{0,0xbf800000,0,false},
      Case{0,0x7fc12345,0,false},Case{0,0,0x7fc12345,false},
      Case{0,0x7fc12345,0x3f800000,true},Case{0,0x3f800000,0x7fc12345,true},
      Case{0,0xbf800000,0x3f800000,true}};
  unsigned checked{},calls{},events{};
  for(auto image:{SourcePhysicsContactImage::MapPack11,SourcePhysicsContactImage::TitleUpdate15})for(const auto &c:cases) {
    const auto silent=run(image,c.flags,c.body,c.shield,false);
    const auto observed=run(image,c.flags,c.body,c.shield,true);same(silent,observed);
    const auto foreign=run(image,c.flags,c.body,c.shield,true,true);same(silent,foreign);
    check(silent.calls==unsigned(c.admitted),"Extracted original voice caller admission differs");
    check(foreign.requests.empty(),"Foreign native world published into this voice journal");
    if(observed.requests.size()!=observed.calls) {
      std::cerr<<"Native voice request regression: caller_calls="<<observed.calls<<" published_requests="<<observed.requests.size()<<'\n';
      throw std::runtime_error("Admitted native hurt/death request was not published exactly once");
    }
    for(const auto &e:observed.requests) {
      check(e.caller==(image==SourcePhysicsContactImage::TitleUpdate15?0xcc93du:0xcb7bdu) &&
          e.target==0x12340007 && e.definition==0xdead1234 && e.report_flags==c.flags &&
          std::bit_cast<std::uint32_t>(e.recent_body)==0x3f400000 &&
          std::bit_cast<std::uint32_t>(e.recent_shield)==0x3e000000 &&
          std::bit_cast<std::uint32_t>(e.visible_body)==0x3f000000,
          "Native hurt/death publication changed its original source values");
    }
    ++checked;calls+=observed.calls;events+=observed.requests.size();
  }
  scope_restore();
  check(source_damage_voice_request_detail::active==nullptr,"Voice request scope survived its native caller");
  std::cout<<"PASS native voice caller: "<<checked<<" input cases, "<<calls<<" admitted calls, "<<events<<" published requests; caller ABI/stores/trace unchanged\n";
  std::cout<<"LIMIT caller component only; original callee selection, live fatal world and audible delivery require aggregate validation\n";
}catch(const std::exception &error){std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}
