#include "halo2/weapon_trigger.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
using namespace halo2;
namespace {
template<std::size_t N> std::array<std::uint8_t,N> decode(const std::string &text) {
  if(text.size()!=N*2)throw std::runtime_error("Invalid original trigger byte count");
  std::array<std::uint8_t,N> result;
  for(unsigned n=0;n<N;++n)result[n]=std::uint8_t(std::stoul(text.substr(n*2,2),nullptr,16));
  return result;
}
std::uint32_t u32(const std::array<std::uint8_t,64> &data,unsigned p) {
  return unsigned(data[p])|(unsigned(data[p+1])<<8)|(unsigned(data[p+2])<<16)|(unsigned(data[p+3])<<24);
}
std::int16_t i16(const std::array<std::uint8_t,64> &data,unsigned p) {
  return std::bit_cast<std::int16_t>(std::uint16_t(unsigned(data[p])|(unsigned(data[p+1])<<8)));
}
WeaponTriggerDefinition definition(const std::array<std::uint8_t,64> &data) {
  auto real=[&](unsigned p){return std::bit_cast<float>(u32(data,p));};
  return {u32(data,0),i16(data,4),i16(data,6),i16(data,8),i16(data,10),real(0x10),real(0x14),
      i16(data,0x18),i16(data,0x1a),real(0x1c),real(0x20),i16(data,0x24),real(0x2c),u32(data,0x34),u32(data,0x3c)};
}
void none_barrel_tests() {
  const auto check=[](bool value,const char* message){if(!value)throw std::runtime_error(message);};
  constexpr std::array<std::array<std::int16_t,2>,3> indices{{{-1,-1},{-1,0},{0,-1}}};
  for(const auto behavior:{0,1,4})for(const auto pair:indices)for(bool alternate:{false,true}) {
    WeaponTriggerDefinition d{};d.behavior=std::int16_t(behavior);
    d.primary_barrel=pair[0];d.secondary_barrel=pair[1];
    WeaponTriggerState state{};state.ticks=2;state.flags=1;
    std::array<SmgBarrelState,2> barrels{};barrels[0].flags=0x40;barrels[1].flags=0x80;
    WeaponTriggerInputState analog{};WeaponTriggerTarget target{};
    WeaponTriggerPhaseContext context{};context.held=true;context.pressed=true;
    context.alternate=alternate;context.tick_rate=30;context.barrel_count=1;
    std::vector<std::int16_t> requests;
    WeaponTriggerPhaseServices services;
    services.barrel_admitted=[&](auto index){requests.push_back(index);return true;};
    (void)weapon_trigger_step(state,d,barrels,analog,target,0,context,services);
    const bool selected=(behavior==4 && alternate?pair[1]:pair[0])==0;
    check(requests==(selected?std::vector<std::int16_t>{0}:std::vector<std::int16_t>{}),
        "NONE barrel requested admission or replaced the actual secondary selection");
    // Automatic primary firing also clears the barrel's0x40 recovery latch
    // on a pressed input. NONE never reaches that actual-barrel clear.
    const auto expected=selected?(behavior==0?1u:0x41u):0x40u;
    check(barrels[0].flags==expected&&barrels[1].flags==0x80u,
        "NONE barrel changed retained barrel storage");
    check(state.ticks==1,"NONE barrel skipped trigger clock update");
    check((state.flags&1u)==(selected&&behavior!=0?0u:1u),"NONE barrel consumed the firing latch");
    check(d.primary_barrel==pair[0]&&d.secondary_barrel==pair[1],"Authored trigger indices changed");
  }
}
}
int main(int argc,char **argv)try {
  if(argc!=2 && argc!=3)throw std::runtime_error("Supply original trigger fixtures");
  none_barrel_tests();
  std::string line;unsigned cases=0,events=0,phases=0;
  for(int file=1;file<argc;++file) {
  std::ifstream input(argv[file]);if(!input)return 77;
  for(;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);char type;std::uint32_t slot,argument,rate,count,flags,owner,spent,allocated;
    std::string raw_definition,raw_state,raw_barrels,raw_input,wanted_state,wanted_barrels,wanted_input;
    std::uint32_t wanted_result;
    if(!(row>>type>>std::hex>>slot>>argument>>rate>>count>>flags>>owner>>spent>>allocated>>raw_definition>>raw_state>>raw_barrels>>raw_input))
      throw std::runtime_error("Truncated original trigger fixture");
    std::uint32_t context=0,target=0,marker=0,gate_mask=0,predicates=0,query=0,query_marker=0,detonation=0,wanted_target=0,wanted_marker=0;
    if(type=='P' && !(row>>context>>target>>marker>>gate_mask>>predicates>>query>>query_marker>>detonation>>wanted_target>>wanted_marker))
      throw std::runtime_error("Truncated original phase context");
    if(!(row>>wanted_state>>wanted_barrels>>wanted_input>>wanted_result))throw std::runtime_error("Missing original trigger results");
    const auto d=definition(decode<64>(raw_definition));
    auto state=std::bit_cast<WeaponTriggerState>(decode<12>(raw_state));
    auto barrels=std::bit_cast<std::array<SmgBarrelState,2>>(decode<104>(raw_barrels));
    auto analog=std::bit_cast<WeaponTriggerInputState>(decode<5>(raw_input));
    const auto tick_rate=std::bit_cast<std::int16_t>(std::uint16_t(rate));
    const bool has_unit=(flags&1u) && owner!=0xffffffffu;
    std::vector<std::vector<std::uint32_t>> actual_calls,wanted_calls;
    WeaponTriggerPhaseServices services;
    services.allocate_effect=[&](std::uint32_t tag){actual_calls.push_back({1,tag,0xffffffffu,0,0});return allocated;};
    services.reset_zoom=[&](){actual_calls.push_back({2,owner,1});};
    services.weapon_state=[&](unsigned value,bool force){actual_calls.push_back({3,value,std::uint32_t(force)});};
    services.activity=[&](unsigned value){actual_calls.push_back({4,value});};
    services.barrel_admitted=[&](std::int16_t barrel){actual_calls.push_back({5,std::uint32_t(barrel),0});return (gate_mask&(1u<<barrel))!=0;};
    services.stop_effect=[&](std::uint32_t effect){actual_calls.push_back({6,effect,1});};
    services.acquire_target=[&]()->std::optional<WeaponTriggerTarget>{actual_calls.push_back({7});return predicates&1u?std::optional(WeaponTriggerTarget{query,std::uint8_t(query_marker)}):std::nullopt;};
    services.track_target=[&]()->std::optional<WeaponTriggerTarget>{actual_calls.push_back({8});return predicates&2u?std::optional(WeaponTriggerTarget{query,std::uint8_t(query_marker)}):std::nullopt;};
    services.destroy_weapon=[&](){actual_calls.push_back({9});};
    WeaponTriggerTarget target_state{target,std::uint8_t(marker)};
    unsigned result=0;
    switch(type) {
      case 'P': {
        WeaponTriggerPhaseContext c;c.tick_rate=tick_rate;c.held=context&1u;c.pressed=context&2u;c.blocked=context&4u;
        c.alternate=context&8u;c.has_unit=has_unit;c.has_player=context&32u;c.editor=context&64u;
        c.spent=std::bit_cast<float>(spent);c.barrel_count=count;c.detonation_effect=detonation;
        result=weapon_trigger_step(state,d,barrels,analog,target_state,slot,c,services);++phases;break;
      }
      case 'I': {
        const auto e=weapon_trigger_input(analog,state,d,tick_rate);
        result=unsigned(e.primary)|(unsigned(e.charged)<<1)|(unsigned(e.released)<<2);break;
      }
      case 'A':result=weapon_trigger_charge_admitted(d,std::span(barrels).first(count),std::bit_cast<float>(spent));break;
      case 'B':weapon_trigger_start_charge(state,d,tick_rate,has_unit,services);break;
      case 'C':weapon_trigger_finish_charge(state,d,tick_rate,slot,services);break;
      case 'D':weapon_trigger_release_charge(state,d,barrels,tick_rate);break;
      case 'E':weapon_trigger_lock(state,WeaponTriggerLockPhase::Seeking,tick_rate,has_unit,services);break;
      case 'F':weapon_trigger_lock(state,WeaponTriggerLockPhase::Lost,tick_rate,has_unit,services);break;
      case 'G':weapon_trigger_lock(state,WeaponTriggerLockPhase::Acquiring,tick_rate,has_unit,services);break;
      case 'H':weapon_trigger_lock(state,WeaponTriggerLockPhase::Locked,tick_rate,has_unit,services);break;
      case 'J':weapon_trigger_clear_barrel(barrels,std::bit_cast<std::int16_t>(std::uint16_t(argument)));break;
      default:throw std::runtime_error("Unknown original trigger owner");
    }
    unsigned call_count;if(!(row>>std::dec>>call_count))throw std::runtime_error("Missing original trigger call count");
    for(unsigned n=0;n<call_count;++n) {
      unsigned words;if(!(row>>std::dec>>words))throw std::runtime_error("Missing original trigger call size");
      std::vector<std::uint32_t> call(words);
      for(auto &word:call)if(!(row>>std::hex>>word))throw std::runtime_error("Truncated original trigger call");
      wanted_calls.push_back(std::move(call));
    }
    const auto after=std::bit_cast<std::array<std::uint8_t,12>>(state);
    const auto after_barrels=std::bit_cast<std::array<std::uint8_t,104>>(barrels);
    const auto after_input=std::bit_cast<std::array<std::uint8_t,5>>(analog);
    if(after!=decode<12>(wanted_state) || after_barrels!=decode<104>(wanted_barrels) ||
        after_input!=decode<5>(wanted_input) || result!=wanted_result || actual_calls!=wanted_calls ||
        (type=='P' && (target_state.datum!=wanted_target || target_state.marker!=wanted_marker))) {
      std::cerr<<"Original trigger case "<<cases<<" owner="<<type<<" state="<<(after==decode<12>(wanted_state))
          <<" barrels="<<(after_barrels==decode<104>(wanted_barrels))<<" input="<<(after_input==decode<5>(wanted_input))
          <<" target="<<std::hex<<target_state.datum<<'/'<<wanted_target<<std::dec<<" result="<<result<<'/'<<wanted_result<<" calls="<<(actual_calls==wanted_calls)<<'\n';
      throw std::runtime_error("Native trigger owner differs from unchanged original");
    }
    ++cases;events+=call_count;
  }
  }
  if(cases!=unsigned(argc==3?48128:34816) || phases!=unsigned(argc==3?13312:0))
    throw std::runtime_error("Incomplete original trigger coverage");
  std::cout<<"PASS "<<cases<<" original TU1.1/TU1.5 trigger cases: full12-byte states,two52-byte barrels,five analog bytes and "
      <<events<<" ordered effect/zoom/action requests; "<<phases<<" complete per-trigger phases\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
