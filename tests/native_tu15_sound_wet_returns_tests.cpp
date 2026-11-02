#include "halo2/native_tu15_sound_wet_return_sources.hpp"
#include "halo2/native_tu15_sound_combined_wet.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>
using namespace halo2;
namespace {
using Json=nlohmann::json;using Word=tu15_sound::Word24;
void need(bool v,const std::string& text){if(!v)throw std::runtime_error(text);}
void fill(const Json& input,std::span<Word> values){need(input.size()==values.size(),"Original mix extent");for(unsigned i=0;i<values.size();++i)values[i]=Word(input[i]);}
}
int main(int argc,char** argv)try{
 need(argc==3,"Supply original wet-return CPU and wet-graph fixtures");const auto fixture=Json::parse(std::ifstream(argv[1]));
 need(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
  fixture.at("source_instruction_patches").empty() && fixture.at("declared_external_results_are_not_device_acceptance").get<bool>(),"Original authority/boundary scope");
 const auto bank=fixture.at("bank").get<unsigned>();const auto drivers=fixture.at("drivers").get<std::array<unsigned,2>>();unsigned calls{},creates{},cases{};
 for(const auto& selected:fixture.at("cases")){
  std::array<std::array<std::byte,0x2ad8>,2> blocks{};SoundDriverStartupState state{};state.driver_address=drivers[0];
  const auto memory=[&](unsigned address,std::size_t size)->std::span<std::byte>{for(unsigned i=0;i<2;++i)if(address>=drivers[i] && std::uint64_t(address-drivers[i])+size<=blocks[i].size())return {blocks[i].data()+address-drivers[i],size};throw std::runtime_error("Unowned native test storage");};
  const auto put=[&](unsigned address,unsigned value){auto raw=memory(address,4);for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(8*i));};
  const auto read=[&](unsigned address){unsigned v{};auto raw=memory(address,4);for(unsigned i=0;i<4;++i)v|=std::to_integer<unsigned>(raw[i])<<(8*i);return v;};
  for(unsigned i=0;i<2;++i){blocks[i].fill(std::byte(0x5b+i));put(drivers[i]+0x2ab0,bank+0x18000+i*0x100);}
  unsigned ordinal{};const auto next=[&](const char* kind)->const Json&{const auto& call=selected.at("calls").at(ordinal++);need(call.at("kind")==kind,selected.at("name").get<std::string>()+": original external-call order");++calls;return call;};
  const auto actions=[&](const Json& call){for(const auto& action:call.at("actions")){const auto address=action[0].get<unsigned>(),value=action[1].get<unsigned>();if(address==fixture.at("global_driver"))state.driver_address=value;else put(address,value);}};
  SoundDriverStartupServices services;services.memory=memory;
  services.native_buffer_create=[&](const SoundDriverBufferCreateRequest& request){const auto& call=next("create");++creates;
   need(request.device_identity==call.at("args")[0] && request.output_address==call.at("args")[2] && request.outer_identity==call.at("args")[3],"Original constructor identities");
   need(Json(request.descriptor)==call.at("descriptor"),"Original constructor descriptor");actions(call);return std::bit_cast<std::int32_t>(call.at("result").get<std::uint32_t>());};
  services.native_buffer_operation=[&](const SoundDriverBufferRequest& request){
   need(request.operation==SoundDriverBufferOperation::Volume || request.operation==SoundDriverBufferOperation::MixBins || request.operation==SoundDriverBufferOperation::Play,"Unexpected native buffer operation");
   const char* kind=request.operation==SoundDriverBufferOperation::Volume?"volume":request.operation==SoundDriverBufferOperation::MixBins?"bins":"play";
   const auto& call=next(kind);need(request.buffer_identity==call.at("args")[0],"Original post-callback identity reload");
   if(request.operation==SoundDriverBufferOperation::MixBins){need(request.words[0]==call.at("count"),"Original route count");need(Json(std::vector<std::uint32_t>(request.mix_bin_pairs.begin(),request.mix_bin_pairs.end()))==call.at("pairs"),"Original mix-bin pairs");}
   else if(request.operation==SoundDriverBufferOperation::Volume)need(request.words[0]==call.at("args")[1],"Original volume");
   else for(unsigned i=0;i<3;++i)need(request.words[i]==call.at("args")[i+1],"Original play arguments");actions(call);
  };
  source_sound_driver_wet_returns_tu15(state,services);need(ordinal==selected.at("calls").size(),"Original call count");need(state.driver_address==selected.at("final_driver"),"Original driver reload");
  for(unsigned i=0;i<2;++i)for(unsigned j=0;j<8;++j)need(read(drivers[i]+0x2a70+4*j)==selected.at("final_slots")[i][j],"Original final buffer slots");++cases;
 }
 need(cases==14 && creates==112,"Original wet-return CPU coverage");
 // Real native resources expose actual qualified wet samples, preserving the
 // original CPU source/destination plan. No VP clock, mix or scale is supplied.
 const auto wet=Json::parse(std::ifstream(argv[2]));unsigned frames{},samples{};
 for(const auto& selected:wet.at("cases")){
  auto graph=tu15_sound_graph_storage::GraphStorage::fresh_literal();tu15_sound_combined_wet::Trace trace{};
  std::array<Word,tu15_gp_output::owned_words> zero{};auto output=tu15_gp_output::OutputOwner::cold_cursor_phase(zero);
  std::optional<tu15_sound_wet_return::Sources> retained;const Word* first{};
  for(const auto& row:selected.at("frames")){
   const auto& input=row.at("inputs");tu15_sound_combined_wet::Controls control{};
   for(unsigned i=0;i<32;++i){if(i<6)graph->unresolved_prefix()[i]=Word(input.at("fanout")[i]);else graph->mixbin(0)[i-6]=Word(input.at("fanout")[i]);}
   for(unsigned i=0;i<4;++i)fill(input.at("mix")[i],graph->mixbin(10+i));for(unsigned i=0;i<8;++i)fill(input.at("output_baselines")[i],graph->mixbin(15+i));
   for(unsigned i=0;i<2;++i){control.prepare.accumulators[i].entry_status=Word(row.at("prepare_status")[2+i]);control.reverbs[i].entry_status=row.at("reverb")[i].at("status");control.reverbs[i].modifiers.fill(0xffffffu);}
   tu15_sound_combined_wet::process(control,*graph,trace);
   std::array<Word,1024> mix{};for(unsigned i=0;i<32;++i)std::copy(graph->mixbin(i).begin(),graph->mixbin(i).end(),mix.begin()+i*32);
   const auto receipt=output->copy_values(mix);tu15_sound_wet_return::Sources sources(receipt);
   if(!retained){retained.emplace(sources);first=retained->view(0).samples.data();}
   need(first==sources.view(0).samples.data() && reinterpret_cast<std::uintptr_t>(first)>0xffffffffu,"Real stable native GP return owner");
   for(unsigned i=0;i<8;++i){const auto view=sources.view(i);const auto& expected=row.at("reverb")[i<4?1:0].at("outputs");
    need(view.route.input_bin==15+i && view.route.output_bin==6+i%4 && view.route.driver_slot_offset==0x2a70+i*4 && view.route.flags==0x100000 && view.route.volume==0,"Original route metadata");
    for(unsigned j=0;j<32;++j){need(view.samples[j].bits==expected[(i%4)*32+j],"Original wet result lost in GP output/return source");need(retained->view(i).samples[j]==view.samples[j],"Retained views must expose current GP resources");++samples;}
   }++frames;
  }
  // A completed receipt owns the actual resources after its producer dies.
  output.reset();need(retained->view(0).samples.data()==first,"Return resource lifetime ended with producer");
 }
 need(frames==128 && samples==32768,"Whole original wet boundary coverage");
 bool rejected=false;try{(void)source_sound_wet_return_route_tu15(8);}catch(const std::out_of_range&){rejected=true;}need(rejected,"Invalid return index");
 SoundDriverStartupState state{};SoundDriverStartupServices missing;rejected=false;
 try{source_sound_driver_wet_returns_tu15(state,missing);}catch(const std::runtime_error&){rejected=true;}need(rejected,"Missing constructor invented a resource");
 std::cout<<"PASS "<<cases<<" original CPU fragment cases, "<<creates<<" create attempts, "<<calls<<" external ABI calls; "<<frames<<" native wet/GP frames, "<<samples
  <<" original return-source words; exact routes/identity reload/failures and retained native resources; no device or timing admission\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
