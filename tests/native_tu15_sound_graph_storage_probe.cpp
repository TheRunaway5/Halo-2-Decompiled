#include "halo2/native_tu15_sound_graph_storage.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>
using namespace halo2;
using namespace halo2::tu15_sound_graph_storage;
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
unsigned rejections=0;
template<class F>void rejects(F f){try{f();}catch(const std::invalid_argument&){++rejections;return;}catch(const std::out_of_range&){++rejections;return;}throw std::runtime_error("Missing storage rejection");}
void dump(const GraphStorage& storage){
    const auto& m=storage.effects();bool first=true;
    auto row=[&](const std::vector<std::uint32_t>& a){if(!first)std::cout<<',';first=false;std::cout<<'[';for(unsigned j=0;j<a.size();j++)std::cout<<(j?",":"")<<a[j];std::cout<<']';};
    std::cout<<"\"literal_nodes\":[";std::vector<std::uint32_t> v{m.fanout_state.flags.bits};for(auto x:m.fanout_parameters.gains)v.push_back(x.bits);v.push_back(bool(m.fanout_state.published_gain_source));row(v);
    const auto& d=m.distortion_parameters;const auto& s=m.distortion_state;v={s.flags.bits,s.last_dry_word.bits,d.gain.bits,d.before_clip.current_input.bits,d.before_clip.previous_input.bits,d.before_clip.older_input.bits,d.before_clip.previous_output.bits,d.before_clip.older_output.bits,d.after_clip.current_input.bits,d.after_clip.previous_input.bits,d.after_clip.older_input.bits,d.after_clip.previous_output.bits,d.after_clip.older_output.bits,s.before_clip.previous_input.bits,s.before_clip.older_input.bits,s.before_clip.previous_output.bits,s.before_clip.older_output.bits,s.after_clip.previous_input.bits,s.after_clip.older_input.bits,s.after_clip.previous_output.bits,s.after_clip.older_output.bits};row(v);
    for(unsigned j=0;j<2;j++)row({m.accumulator_states[j].flags.bits,m.accumulator_parameters[j].primary_gain.bits,m.accumulator_parameters[j].secondary_gain.bits});
    for(unsigned j=0;j<4;j++)row({m.chorus_states[j].flags.bits,m.chorus_parameters[j].gain.bits,m.chorus_parameters[j].depth.bits,std::uint32_t(bool(m.chorus_states[j].active))});
    for(unsigned j=0;j<2;j++){const auto& r=storage.reverb(j).state;v={r.flags,r.cross_gain,r.mono_gain,r.output_mode,r.smoothing_count};for(auto x:r.delays)v.push_back(x);for(auto x:r.reflection_gains)v.push_back(x);for(auto x:r.target_gains)v.push_back(x);for(auto x:r.current_gains)v.push_back(x);for(auto x:r.filters){v.push_back(x.previous);v.push_back(x.feedback);v.push_back(x.feedforward);}for(auto x:r.diffusion_gains)v.push_back(x);for(auto x:r.wet_gains)v.push_back(x);for(auto x:r.cluster_taps)v.push_back(x);for(auto x:r.dormant_biquad_coefficients)v.push_back(x);v.insert(v.end(),{r.fade.pending_mask,r.fade.sample_count,r.fade.step,r.fade.amplitude,r.fade.selected_read,r.fade.selected_write,r.fade.write_stride,r.fade.tap_cursor,std::uint32_t(r.fade.bindings_initialized),std::uint32_t(r.fade.tap_owner)});for(auto x:r.fade.adjusted_taps)v.push_back(x);for(const auto& x:r.rings)v.push_back(x.write_byte_cursor);for(auto x:r.taps)v.push_back(x.read_byte_cursor);row(v);}
    const auto& c=m.crosstalk_state;v={c.flags.bits,c.coefficients[0].bits,c.coefficients[1].bits};for(const auto& p:c.pairs){v.push_back(p.cursor);for(const auto& h:p.histories)for(auto x:h)v.push_back(x.bits);for(auto x:p.previous)v.push_back(x.bits);}row(v);
    for(unsigned j=0;j<4;j++)row({m.controller_states[j].flags.bits,m.controller_states[j].counter.bits,m.controller_parameters[j].threshold.bits,m.controller_parameters[j].gain.bits});std::cout<<']';
}
int main(){try{
    static_assert(!std::is_move_constructible_v<GraphStorage>&&!std::is_copy_constructible_v<GraphStorage>);
    static_assert(std::is_same_v<tu15_sound_chorus::Word24,tu15_sound_gain_fanout::Word24>);
    auto owner=GraphStorage::fresh_literal();auto& g=*owner;require(g.actual_title_scratch_bytes()==774144,"Incomplete backing extent");
    require(reinterpret_cast<std::uintptr_t>(&g)>0xffffffffu,"Owner not above4GiB");
    require(g.fanout_input().data()+6==g.mixbin(0).data(),"Prefix and mix need one array");for(unsigned j=0;j<32;j++)require(g.mixbin(j).data()==g.mixbin(0).data()+j*32,"Mix array discontinuity");
    const auto routes=g.fanout_routes();for(unsigned j=4;j<8;j++)require(routes[j].data()==g.workspace().data(),"Fanout workspace alias");
    for(unsigned j=0;j<4;j++){const auto& b=g.chorus_bindings(j);require(b.audio.data()==g.mixbin(6+j).data()&&b.modulation.data()==g.mixbin(14).data()&&b.output.data()==routes[j].data(),"Chorus binding ownership");if(j)require(g.chorus_history(j-1).data()+400==g.chorus_history(j).data(),"Shared chorus endpoint");}
    for(unsigned j=0;j<2;j++)for(const auto& ring:g.reverb(j).state.rings){require(reinterpret_cast<std::uintptr_t>(ring.words.data())>0xffffffffu,"Ring not above4GiB");require(std::all_of(ring.words.begin(),ring.words.end(),[](auto w){return w==0;}),"Fresh history not zero");}
    for(unsigned j=0;j<2;j++)require(std::all_of(g.reverb_unused_tail(j).begin(),g.reverb_unused_tail(j).end(),[](auto b){return b==std::byte{};}),"Fresh ring tail not zero");for(unsigned j=0;j<4;j++)require(std::all_of(g.controller_sink(j).begin(),g.controller_sink(j).end(),[](auto w){return w.bits==0;})&&g.controller_sink_byte_cursor(j)==0,"Fresh sink not zero");require(std::all_of(g.final_unused_tail().begin(),g.final_unused_tail().end(),[](auto b){return b==std::byte{};}),"Fresh final tail not zero");
    std::cout<<'{';dump(g);
    // Real processor call caches spans into this nonmoving owner's members.
    tu15_sound_chorus::Controls controls{Word24(0),{}};std::array<tu15_sound_chorus::SampleTrace,32> trace;
    tu15_sound_chorus::process(controls,g.effects().chorus_parameters[0],g.effects().chorus_states[0],g.chorus_bindings(0),g.chorus_history(0),trace);
    require(g.effects().chorus_states[0].active->bindings.audio.data()==g.mixbin(6).data(),"Cached chorus audio not retained");
    std::array<Word24,32> replacement{};tu15_sound_chorus::process(controls,g.effects().chorus_parameters[0],g.effects().chorus_states[0],{replacement,replacement,replacement},g.chorus_history(0),trace);require(g.effects().chorus_states[0].active->bindings.output.data()==routes[0].data()&&g.effects().chorus_states[0].active->cursor==64,"Warm binding changed");
    for(unsigned j=0;j<544;j++)g.workspace()[j]=Word24(j+1);for(unsigned j=0;j<2;j++){for(unsigned k=0;k<32;k++)g.reverb_common_input(j)[k]=Word24(10000+j*32+k);for(unsigned c=0;c<4;c++)for(unsigned k=0;k<32;k++)g.reverb_common_output(j,c)[k]=Word24(20000+j*128+c*32+k);}
    g.copy_common_to_reverb_buffers();require(g.raw_reverb_workspace()[33][15]==544,"Raw workspace copy");g.raw_reverb_workspace()[10][4]=98765;g.reverb(0).input[3]=12345;g.reverb(1).outputs[2][7]=54321;g.copy_reverb_buffers_to_common();require(g.workspace()[164].bits==98765&&g.mixbin(12)[3].bits==12345&&g.mixbin(17)[7].bits==54321,"Adapter mutation lost");
    const auto original=g.workspace()[0];g.raw_reverb_workspace()[0][0]=456789;g.reverb(1).outputs[3][31]=0x1000000;rejects([&]{g.copy_reverb_buffers_to_common();});require(g.workspace()[0]==original,"Failed raw copy changed common owner");g.reverb(1).outputs[3][31]=0;const auto previous=g.raw_reverb_workspace()[0][0];g.workspace()[0].bits=987654;g.workspace()[543].bits=0x1000000;rejects([&]{g.copy_common_to_reverb_buffers();});require(g.raw_reverb_workspace()[0][0]==previous,"Failed common copy changed raw owner");g.workspace()[0]=original;g.workspace()[543]=Word24(544);
    rejects([&]{g.mixbin(32);});rejects([&]{g.fanout_destination(5);});rejects([&]{g.chorus_history(4);});rejects([&]{g.chorus_bindings(4);});rejects([&]{g.reverb(2);});rejects([&]{g.reverb_common_output(0,4);});rejects([&]{g.controller_sink(4);});rejects([&]{g.controller_sink_byte_cursor(4);});rejects([&]{g.reverb_unused_tail(2);});
    RetainedScratchSeed seed;rejects([&]{GraphStorage::retained_scratch_literal(seed);});
    auto source=GraphStorage::fresh_literal();std::array<std::array<std::vector<std::uint32_t>,14>,2> histories;std::array<std::vector<std::byte>,2> tails;std::array<std::vector<Word24>,4> sinks;std::vector<std::byte> final_tail(3904,std::byte{0xa5});
    for(unsigned j=0;j<2;j++){tails[j].resize(51008,std::byte{0xa5});seed.reverb_unused_tails[j]=tails[j];for(unsigned k=0;k<14;k++){histories[j][k].resize(source->reverb(j).state.rings[k].words.size(),0x12345u+j*14+k);seed.reverb_histories[j][k]=histories[j][k];}}for(unsigned j=0;j<4;j++){sinks[j].resize(640,Word24(0x54321+j));seed.controller_sinks[j]=sinks[j];}seed.final_unused_tail=final_tail;
    auto retained=GraphStorage::retained_scratch_literal(seed);require(retained->actual_title_scratch_bytes()==774144&&retained->reverb(1).state.rings[13].words.back()==0x12345+27&&retained->controller_sink(3).back().bits==0x54324&&retained->final_unused_tail().back()==std::byte{0xa5},"Retained scratch content lost");histories[1][13][0]=0;require(retained->reverb(1).state.rings[13].words[0]==0x12345+27,"Borrowed seed retained");histories[0][0][0]=0x1000000;rejects([&]{GraphStorage::retained_scratch_literal(seed);});
    std::cout<<",\"rejections\":"<<rejections<<",\"passed\":true,\"scratch_bytes\":774144,\"cached_chorus_calls\":2,\"runtime_ready\":false}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
