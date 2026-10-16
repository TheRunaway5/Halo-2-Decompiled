#include "halo2/native_tu15_sound_record0_context.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
using J=nlohmann::json;namespace g=halo2::tu15_sound_gain_fanout;namespace sc=halo2::tu15_status_context;using Word=g::Word24;
int main(){try{
 J cases;std::cin>>cases;unsigned index=0;
 const g::Controls controls{g::ScalingMode::none,false,false,false,g::AddressingMode::linear,g::AddressingMode::linear,g::AddressingMode::linear,g::AddressingMode::linear,g::AddressingMode::linear};
 for(const auto& item:cases){
  g::Parameters params;g::State state{Word(item["flags"].get<unsigned>()),{}};std::array<Word,32> input;std::array<std::array<Word,32>,5> out;
  for(unsigned i=0;i<32;i++)input[i]=Word(item["input"][i]);
  for(unsigned i=0;i<8;i++)params.gains[i]=Word(item["gains"][i]);
  for(unsigned i=0;i<160;i++)out[i/32][i%32]=Word(item["outputs"][i]);
  const auto route=item["routes"].get<std::array<unsigned,8>>();std::array<g::Output,8> outputs{out[route[0]],out[route[1]],out[route[2]],out[route[3]],out[route[4]],out[route[5]],out[route[6]],out[route[7]]};
  auto legacy_out=out;auto legacy_state=state;
  std::array<g::Output,8> legacy_outputs{legacy_out[route[0]],legacy_out[route[1]],legacy_out[route[2]],legacy_out[route[3]],legacy_out[route[4]],legacy_out[route[5]],legacy_out[route[6]],legacy_out[route[7]]};
  auto status=sc::Status::incoming(item["sr"],item["known"],index+1);sc::Trace trace;
  for(unsigned frame=0;frame<2;frame++){
   halo2::tu15_record0_context::process(controls,params,state,input,outputs,status,trace);
   g::process(controls,params,legacy_state,input,legacy_outputs);
   if(out!=legacy_out||state!=legacy_state)throw std::runtime_error("Previously approved arithmetic changed");
   J result{{"case",index},{"frame",frame},{"sr",status.value()},{"known",status.known_mask()},{"flags",state.flags.bits},{"published",state.published_gain_source==g::GainSource::sequence_begin},{"trace",J::array()},{"outputs",J::array()},{"origins",J::array()}};
   for(unsigned i=0;i<trace.count;i++){const auto& t=trace.steps[i];result["trace"].push_back({{"kind",static_cast<unsigned>(t.operation)},{"pc",t.source_pc},{"ordinal",t.ordinal},{"written_mask",t.written_mask},{"before_sr",t.before_value},{"after_sr",t.after_value},{"before_known",t.before_known},{"known",t.after_known},{"A_raw56",static_cast<std::uint64_t>(t.accumulator)&((std::uint64_t{1}<<56)-1)},{"word",t.transferred_word},{"limited",t.limited}});}
   for(const auto& b:out)for(auto word:b)result["outputs"].push_back(word.bits);
   for(const auto& o:status.origins())result["origins"].push_back({static_cast<unsigned>(o.producer),o.evidence,o.source_pc,o.ordinal});
   std::cout<<result.dump()<<'\n';
  }
  index++;
 }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
