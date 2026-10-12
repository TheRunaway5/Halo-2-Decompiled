#include "halo2/source_result_event.hpp"
#include <nlohmann/json.hpp>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::vector<std::uint8_t> bytes(const std::string& text) {
  check(text.size()%2==0,"Odd original byte string");std::vector<std::uint8_t> out(text.size()/2);
  for(unsigned i=0;i<out.size();++i)out[i]=std::uint8_t(std::stoul(text.substr(i*2,2),nullptr,16));
  return out;
}
std::uint32_t word(std::span<const std::uint8_t> data,unsigned offset) {
  std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::uint32_t(data[offset+i])<<(i*8);return value;
}
void unavailable() {
  SourceResultEventState state;SourceResultRecording recording;SourceResultEventServices services;
  source_result_event_record(state,recording,0,4,~0u,services);
  recording={1,1};source_result_event_record(state,recording,0,4,~0u,services);
  recording.frozen=0;bool caught=false;
  try{source_result_event_record(state,recording,0,4,~0u,services);}catch(const std::invalid_argument&){caught=true;}
  check(caught,"Missing event wall-clock became successful recording");
  unsigned queries{};services.time=[&]{++queries;return 1u;};
  caught=false;try{source_result_event_record(state,recording,0,4,~0u,services);}catch(const std::invalid_argument&){caught=true;}
  check(caught && queries==2 && state.last==0 && state.index==0,"Missing pose changed source order or recorded a fake event");
}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply the whole original recording report");std::ifstream input(argv[1]);Json fixture;input>>fixture;
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Original recording proof substitutes returns");
  const auto seed=bytes(fixture.at("ring_seed_hex"));check(seed.size()==36000,"Original ring backing width differs");
  unsigned calls{},appends{},queries{},direct{};
  for(const auto& [version,data]:fixture.at("versions").items()) {
    const auto now=data.at("actual_now").get<std::uint32_t>();
    for(unsigned n=0;n<fixture.at("inputs").size();++n)try {
      const auto& row=fixture.at("inputs")[n];const auto& expected=data.at("cases")[n];
      SourceResultRecording recording{row.at("enabled"),row.at("event_freeze")};
      SourceResultEventState state;state.index=row.at("index");state.last=now+row.at("last_minus_now").get<std::uint32_t>();
      state.start=now+row.at("start_minus_now").get<std::uint32_t>();std::memcpy(state.events.data(),seed.data(),seed.size());
      auto expected_ring=seed;const auto before=bytes(expected.at("before12_hex"));
      check(recording.enabled==before[0] && recording.frozen==before[1] && state.index==word(before,4) && state.last==word(before,8),"Declared original input differs");
      unsigned clocks{},poses{};SourceResultEventServices services;
      services.time=[&]{check(!poses,"Clock sampled after original position owner");++clocks;return now;};
      services.position=[&](std::uint32_t player)->std::optional<std::array<std::uint32_t,3>> {
        check(clocks==2 && player==row.at("player"),"Position sampled before both clocks or with wrong player");++poses;
        if(row.at("pose")=="none")return std::nullopt;
        return row.at("xyz_bits").get<std::array<std::uint32_t,3>>();
      };
      source_result_event_record(state,recording,row.at("player"),row.at("first_arg"),row.at("second_arg"),services);
      if(!expected.at("appended36_hex").is_null()) {
        const auto event=bytes(expected.at("appended36_hex"));check(event.size()==36,"Original event width differs");
        std::copy(event.begin(),event.end(),expected_ring.begin()+row.at("index").get<unsigned>()*36);++appends;
      }
      check(std::memcmp(state.events.data(),expected_ring.data(),36000)==0,"Original whole event ring differs");
      check(state.index==expected.at("index") && state.last==expected.at("last") && state.start==now+row.at("start_minus_now").get<std::uint32_t>(),"Original event metadata differs");
      check(clocks==expected.at("kernel_queries") && poses==unsigned(clocks==2),"Original clock/pose service count differs");
      const auto after=bytes(expected.at("after12_hex"));check(recording.enabled==after[0] && recording.frozen==after[1],"Event mutated recording gates");
      queries+=clocks;++calls;
    }catch(const std::exception& error){throw std::runtime_error(version+" event "+std::to_string(n)+": "+error.what());}
    for(const auto& row:data.at("direct_appends")) {
      SourceResultRecording recording{row.at("enabled"),row.at("event_freeze")};SourceResultEventState state;state.index=row.at("index_before");
      state.last=0xaabbccdd;state.start=0x12345678;std::memcpy(state.events.data(),seed.data(),seed.size());
      SourceResultEvent event;const auto source=bytes(row.at("source36_hex"));check(source.size()==36,"Append source width differs");std::memcpy(event.data(),source.data(),36);
      source_result_event_append(state,recording,event);auto expected_ring=seed;const auto destination=bytes(row.at("destination36_hex"));
      std::copy(destination.begin(),destination.end(),expected_ring.begin()+row.at("index_before").get<unsigned>()*36);
      check(std::memcmp(state.events.data(),expected_ring.data(),36000)==0 && state.index==row.at("index_after"),"Original direct append differs");
      check(state.last==0xaabbccdd && state.start==0x12345678,"Direct append changed timestamps");++direct;
    }
  }
  unavailable();std::cout<<calls<<" whole original recording calls, "<<direct<<" direct appends, "<<appends<<" recorded events and "<<queries<<" real clock queries matched\n";
  std::cout<<"LIMIT platform wall clock and original unit-position are explicit services; native world lifecycle/result publication remain separate\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
