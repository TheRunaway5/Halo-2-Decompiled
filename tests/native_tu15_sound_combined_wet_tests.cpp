#include "halo2/native_tu15_sound_combined_wet.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
using namespace halo2;
using namespace halo2::tu15_sound_combined_wet;
namespace {
using Json=nlohmann::json;
using Word24=tu15_sound::Word24;
using Graph=tu15_sound_graph_storage::GraphStorage;
std::string where;
void need(bool v,const std::string& text){if(!v)throw std::runtime_error(where+": "+text);}
void same(unsigned v,const Json& expected,const char* text){need(v==expected,text);}
void fill(const Json& values,std::span<Word24> output){need(values.size()==output.size(),"Original block extent");
 for(unsigned i=0;i<output.size();++i)output[i]=Word24(values.at(i));}
void block(std::span<const Word24> actual,const Json& values,const char* text){need(values.size()==actual.size(),"Original comparison extent");
 for(unsigned i=0;i<actual.size();++i)need(actual[i].bits==values.at(i),std::string(text)+" sample "+std::to_string(i));}
std::uint64_t add(std::uint64_t h,unsigned w){for(unsigned b=0;b<4;++b){h^=(w>>(8*b))&255u;h*=1099511628211ULL;}return h;}
template<class Words>std::string digest(const Words& values){auto h=14695981039346656037ULL;
 for(auto w:values){if constexpr(std::is_same_v<decltype(w),Word24>)h=add(h,w.bits);else h=add(h,w);}
 std::ostringstream out;out<<std::hex<<std::setfill('0')<<std::setw(16)<<h;return out.str();}
void load(Graph& g,const Json& input){
 for(unsigned i=0;i<32;++i){if(i<6)g.unresolved_prefix()[i]=Word24(input.at("fanout")[i]);
  else g.mixbin(0)[i-6]=Word24(input.at("fanout")[i]);}
 for(unsigned i=0;i<4;++i)fill(input.at("mix")[i],g.mixbin(10+i));
 for(unsigned i=0;i<8;++i)fill(input.at("output_baselines")[i],g.mixbin(15+i));
}
void prepared(Graph& g,const Json& row){
 auto& m=g.effects();const auto& d=row.at("distortion_state");const auto& p=m.distortion_parameters;const auto& s=m.distortion_state;
 same(m.fanout_state.flags.bits,row.at("fanout_state")[4],"Original fanout flags");
 need(m.fanout_state.published_gain_source==tu15_sound_gain_fanout::GainSource::sequence_begin,"Fanout logical gain owner");
 same(s.flags.bits,d[4],"Original distortion flags");same(s.last_dry_word.bits,d[18],"Original distortion dry word");same(p.gain.bits,d[7],"Original distortion gain");
 const std::array coefficients{p.before_clip.current_input,p.before_clip.previous_input,p.before_clip.older_input,p.before_clip.previous_output,p.before_clip.older_output,
  p.after_clip.current_input,p.after_clip.previous_input,p.after_clip.older_input,p.after_clip.previous_output,p.after_clip.older_output};
 for(unsigned j=0;j<10;++j)same(coefficients[j].bits,d[8+j],"Original distortion coefficients");
 const std::array histories{s.before_clip.previous_input,s.before_clip.older_input,s.before_clip.previous_output,s.before_clip.older_output,
  s.after_clip.previous_input,s.after_clip.older_input,s.after_clip.previous_output,s.after_clip.older_output};
 for(unsigned j=0;j<8;++j)same(histories[j].bits,row.at("distortion_history")[j],"Original distortion retained history");
 for(unsigned j=0;j<2;++j){const auto& a=row.at("accumulator_states");same(m.accumulator_states[j].flags.bits,a[j*28+4],"Original accumulator flags");
  same(m.accumulator_parameters[j].primary_gain.bits,a[j*28+8],"Original primary gain");same(m.accumulator_parameters[j].secondary_gain.bits,a[j*28+9],"Original secondary gain");}
 for(unsigned j=0;j<4;++j)block(g.mixbin(10+j),Json(row.at("prepared_mix").begin()+32*j,row.at("prepared_mix").begin()+32*(j+1)),"Prepared wet send");
 need(digest(g.workspace())==row.at("prepared_workspace_hash").get<std::string>(),"Original fanout shared workspace");
}
void state(const tu15_sound_ring_effect::State& s,const Json& row,unsigned index){
 const auto& e=row.at("state");const unsigned base=index==0?0x22d:0x457;
 same(s.flags,e[4],"Original reverb flags");
 const auto words=[&](const auto& values,unsigned at,const char* label){for(unsigned j=0;j<values.size();++j)same(values[j],e[at+j],label);};
 words(s.delays,70,"Original delays");words(s.reflection_gains,89,"Original reflection gains");
 same(s.cross_gain,e[97],"Original cross gain");same(s.mono_gain,e[98],"Original mono gain");words(s.target_gains,99,"Original target gains");
 same(s.output_mode,e[105],"Original reverb output mode");
 for(unsigned j=0;j<10;++j){same(s.filters[j].previous,e[106+j*3],"Original one-pole retained value");same(s.filters[j].feedback,e[107+j*3],"Original feedback coefficient");same(s.filters[j].feedforward,e[108+j*3],"Original feedforward coefficient");}
 words(s.diffusion_gains,136,"Original diffusion gains");words(s.wet_gains,143,"Original wet gains");words(s.cluster_taps,147,"Original cluster taps");
 words(s.dormant_biquad_coefficients,179,"Original dormant coefficients");words(s.current_gains,504,"Original current gains");
 const auto& f=s.fade;same(f.pending_mask,e[510],"Original transition mask");same(f.sample_count,e[511],"Original transition samples");same(f.step,e[512],"Original transition step");same(f.amplitude,e[513],"Original transition amplitude");
 same(f.write_stride*7,e[516],"Original transition stride");words(f.adjusted_taps,518,"Original adjusted taps");same(s.smoothing_count,e[526],"Original smoothing count");
 need(f.bindings_initialized==(e[514]!=0),"Original transition binding ownership");
 if(f.bindings_initialized){same(base+287+f.selected_read*7,e[514],"Original selected read");same(base+189+f.selected_write*7,e[515],"Original selected write");
  same(base+(f.tap_owner==tu15_sound_ring_effect::TapOwner::targets?70:518)+f.tap_cursor,e[517],"Original selected tap");}
 for(unsigned j=0;j<14;++j){same(s.rings[j].write_byte_cursor,e[189+j*7+4],"Original retained write cursor");
  need(digest(s.rings[j].words)==row.at("ring_hashes")[j].get<std::string>(),"Original complete logical ring "+std::to_string(j));}
 for(unsigned j=0;j<31;++j)same(s.taps[j].read_byte_cursor,e[287+j*7+4],"Original retained read cursor");
}
unsigned finish(Graph& g,const Json& row){unsigned nonzero{};
 for(unsigned i=0;i<2;++i){const auto& r=row.at("reverb")[i];state(g.reverb(i).state,r,i);
  block(g.reverb_common_input(i),r.at("input"),"Original filtered/decimated input");
  for(unsigned channel=0;channel<4;++channel){const auto& expected=r.at("outputs");
   block(g.reverb_common_output(i,channel),Json(expected.begin()+32*channel,expected.begin()+32*(channel+1)),"Original additive reverb output");
   nonzero+=std::ranges::any_of(g.reverb_common_output(i,channel),[](auto w){return w.bits!=0;});}
 }
 need(digest(g.workspace())==row.at("reverb")[1].at("workspace_hash").get<std::string>(),"Original shared final reverb workspace");return nonzero;
}
}
int main(int argc,char** argv)try{
 need(argc==2,"Supply original wet-chain numeric fixture");const auto fixture=Json::parse(std::ifstream(argv[1]));
 need(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
  fixture.at("original_program_patches").empty() && fixture.at("callee_or_availability_results_supplied").empty() &&
  fixture.at("all15_original_records_execute_each_frame").get<bool>() && fixture.at("original_ring_history_carries_without_overlay").get<bool>() &&
  !fixture.at("state_history_status_overlays").get<bool>(),"Original wet graph authority/scope differs");
 unsigned cases{},frames{},nonzero{},tail{},wraps{};
 for(const auto& selected:fixture.at("cases")){
  auto graph=Graph::fresh_literal(),joined=Graph::fresh_literal();Trace trace{},joined_trace{};
  need(reinterpret_cast<std::uintptr_t>(graph.get())>0xffffffffu,"Native owner must be above4GiB");
  unsigned frame{},previous_cursor{};std::array<const std::uint32_t*,28> owners{};
  for(unsigned i=0;i<2;++i)for(unsigned j=0;j<14;++j)owners[i*14+j]=graph->reverb(i).state.rings[j].words.data();
  for(const auto& row:selected.at("frames")){
   where=selected.at("name").get<std::string>()+" frame "+std::to_string(frame);Controls controls{};
   for(auto& r:controls.reverbs)r.modifiers.fill(0xffffffu);
   for(unsigned i=0;i<2;++i){controls.prepare.accumulators[i].entry_status=Word24(row.at("prepare_status")[2+i]);controls.reverbs[i].entry_status=row.at("reverb")[i].at("status");}
   // Fanout/distortion use typed mode fields. Check their original entry modes
   // rather than importing a register bank or inventing a status producer.
   for(unsigned i=0;i<2;++i)need((row.at("prepare_status")[i].get<unsigned>()&((3u<<10)|(1u<<14)|(1u<<17)|(1u<<18)|(1u<<20)))==0,"Original preparation mode outside typed domain");
   load(*graph,row.at("inputs"));load(*joined,row.at("inputs"));
   prepare(controls.prepare,*graph,trace);prepared(*graph,row);
   reverbs(controls.reverbs,*graph,trace);const auto count=finish(*graph,row);nonzero+=count;
   process(controls,*joined,joined_trace);finish(*joined,row);
   if(frame>=24)tail+=count;
   const auto cursor=graph->reverb(0).state.rings[4].write_byte_cursor;wraps+=cursor<previous_cursor;previous_cursor=cursor;
   for(unsigned i=0;i<2;++i)for(unsigned j=0;j<14;++j)need(owners[i*14+j]==graph->reverb(i).state.rings[j].words.data(),"Native history owner moved");
   ++frame;++frames;
  }++cases;
 }
 where="late mode preflight";auto graph=Graph::fresh_literal();Controls controls{};Trace trace{};
 for(auto& r:controls.reverbs)r.modifiers.fill(0xffffffu);controls.reverbs[1].entry_status=1u<<20;
 bool rejected=false;try{process(controls,*graph,trace);}catch(const std::invalid_argument&){rejected=true;}
 need(rejected && graph->effects().fanout_state.flags.bits==0 && graph->effects().distortion_state.flags.bits==0 && graph->reverb(0).state.flags==2,"Late reverb mode mutated an earlier record");
 need(cases==5 && frames==128 && nonzero && tail && wraps,"Original wet-chain coverage differs");
 std::cout<<"PASS "<<cases<<" original wet-chain cases, "<<frames<<" retained full-original frames, "<<nonzero<<" nonzero wet output blocks, "<<tail
  <<" tail blocks, "<<wraps<<" ring wraps; native fanout/distortion/two accumulators/two reverbs; physical voice/device parity remains open\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
