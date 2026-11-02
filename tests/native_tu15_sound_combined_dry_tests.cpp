#include "halo2/native_tu15_sound_combined_dry.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using namespace halo2::tu15_sound_combined_dry;
namespace {
using Json=nlohmann::json;
using Word24=tu15_sound::Word24;
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void copy(const Json& values,std::span<Word24,32> output){need(values.size()==32,"Original block extent differs");
 for(unsigned i=0;i<32;++i)output[i]=Word24(values[i]);}
void same(std::span<const Word24,32> words,const Json& expected,const char* why){
 for(unsigned i=0;i<32;++i)if(words[i].bits!=expected.at(i))throw std::runtime_error(std::string(why)+" at sample "+std::to_string(i));}
}
int main(int argc,char** argv)try{
 need(argc==2,"Supply the original combined dry graph reference");
 const auto fixture=Json::parse(std::ifstream(argv[1]));
 need(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
  fixture.at("original_program_patches").empty() && fixture.at("callee_or_availability_results_supplied").empty() &&
  fixture.at("all15_original_records_execute_each_frame").get<bool>() &&
  fixture.at("original_chorus_history_carries_without_overlay").get<bool>() &&
  !fixture.at("state_history_status_overlays").get<bool>(),"Original dry graph authority/scope differs");
 unsigned cases{},frames{},nonzero{};
 for(const auto& selected:fixture.at("cases")){
  auto graph=tu15_sound_graph_storage::GraphStorage::fresh_literal();Trace trace{};
  need(reinterpret_cast<std::uintptr_t>(graph.get())>0xffffffffu,"Native graph storage is not above4GiB");
  unsigned frame{};
  for(const auto& row:selected.at("frames")){
   const auto& input=row.at("inputs");Controls controls{};
   for(unsigned i=0;i<4;++i){copy(input.at("audio")[i],graph->mixbin(6+i));
    graph->effects().chorus_parameters[i].gain=Word24(input.at("gains")[i]);
    controls.chorus[i].entry_status=Word24(row.at("chorus_status")[i]);}
   copy(input.at("modulation"),graph->mixbin(14));controls.crosstalk.entry_status=Word24(row.at("crosstalk_status"));
   constexpr std::array ports{0u,1u,4u,5u};for(unsigned i=0;i<4;++i)copy(row.at("base_outputs")[i],graph->mixbin(ports[i]));
   try{process(controls,*graph,trace);}catch(const std::exception& e){throw std::runtime_error(selected.at("name").get<std::string>()+" frame "+std::to_string(frame)+": "+e.what());}
   for(unsigned i=0;i<4;++i){const auto& state=graph->effects().chorus_states[i];
    need(state.active && state.flags.bits==row.at("chorus_flags")[i] && state.active->cursor==row.at("cursors")[i],"Original retained chorus state differs");
    need(state.active->bindings.audio.data()==graph->mixbin(6+i).data() && state.active->bindings.modulation.data()==graph->mixbin(14).data(),"Chorus cached bindings lost the actual graph owner");
    same(graph->fanout_destination(i),row.at("chorus_outputs")[i],"Original four-bin chorus output differs");
    same(graph->mixbin(ports[i]),row.at("outputs")[i],"Original combined dry speaker output differs");
    for(unsigned j=i?1:0;j<401;++j)need(graph->chorus_history(i)[j].bits==row.at("history")[i*400+j],"Original shared1601-word history differs");
    nonzero+=std::ranges::any_of(graph->mixbin(ports[i]),[](auto w){return w.bits!=0;});
   }
   const auto& actual=graph->effects().crosstalk_state;const auto& expected=row.at("crosstalk_state");
   need(actual.flags.bits==expected[4] && actual.coefficients[0].bits==expected[63] && actual.coefficients[1].bits==expected[64],"Original crosstalk controls differ");
   for(unsigned pair=0;pair<2;++pair){const auto& p=actual.pairs[pair];
    need(p.cursor==expected[37+25*pair],"Original retained crosstalk cursor differs");
    for(unsigned channel=0;channel<2;++channel){for(unsigned i=0;i<11;++i)
     need(p.histories[channel][i].bits==expected[13+25*pair+channel*12+i],"Original retained crosstalk history differs");
     need(p.previous[channel].bits==expected[13+25*pair+channel*12+11],"Original crosstalk filtered word differs");}
   }++frame;++frames;
  }++cases;
 }
 need(cases==5 && frames==32 && nonzero,"Original dry graph signal coverage differs");
 // A late unsupported mode must be rejected before an earlier module starts.
 auto graph=tu15_sound_graph_storage::GraphStorage::fresh_literal();Controls controls{};Trace trace{};
 controls.crosstalk.entry_status=Word24(1u<<20);bool refused=false;
 try{process(controls,*graph,trace);}catch(const std::invalid_argument&){refused=true;}
 need(refused && !graph->effects().chorus_states[0].active && graph->effects().chorus_states[0].flags.bits==0,
  "Late unsupported control mutated an earlier DSP owner");
 std::cout<<"PASS "<<cases<<" original combined dry graph cases, "<<frames<<" retained full-original frames, "<<nonzero
  <<" nonzero speaker blocks; four native chorus calls and retained crosstalk; no voice/device readiness supplied\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
