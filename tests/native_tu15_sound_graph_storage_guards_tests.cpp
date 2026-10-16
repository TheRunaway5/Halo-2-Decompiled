#include "halo2/native_tu15_sound_graph_storage.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>
using namespace halo2;
using namespace halo2::tu15_sound_graph_storage;
namespace {
void require(bool x){if(!x)throw std::runtime_error("Independent native graph owner control failed");}
unsigned rejects=0;
template<class F>void reject(F f){try{f();}catch(const std::invalid_argument&){++rejects;return;}throw std::runtime_error("Missing invalid domain rejection");}
std::vector<Word24*> common(GraphStorage& g){
 std::vector<Word24*> r;for(auto& w:g.workspace())r.push_back(&w);
 for(unsigned j=0;j<2;j++){for(auto& w:g.reverb_common_input(j))r.push_back(&w);for(unsigned c=0;c<4;c++)for(auto& w:g.reverb_common_output(j,c))r.push_back(&w);}return r;
}
std::vector<std::uint32_t*> raw(GraphStorage& g){
 std::vector<std::uint32_t*> r;for(auto& b:g.raw_reverb_workspace())for(auto& w:b)r.push_back(&w);
 for(unsigned j=0;j<2;j++){for(auto& w:g.reverb(j).input)r.push_back(&w);for(auto& b:g.reverb(j).outputs)for(auto& w:b)r.push_back(&w);}return r;
}
std::uint32_t pattern(unsigned j,unsigned k,unsigned n){return (0x12345u+173*j+29*k+19*n)&0xffffffu;}
}
int main(){try{
 static_assert(!std::is_move_constructible_v<GraphStorage> && !std::is_copy_constructible_v<GraphStorage>);
 auto p=GraphStorage::fresh_literal();auto& g=*p;
 require(g.actual_title_scratch_bytes()==774144);
 require(reinterpret_cast<std::uintptr_t>(p.get())>0xffffffffu);
 for(auto w:g.unresolved_prefix())require(w.bits==0);
 for(unsigned j=0;j<32;j++)for(auto w:g.mixbin(j))require(w.bits==0);
 for(unsigned j=0;j<5;j++)for(auto w:g.fanout_destination(j))require(w.bits==0);
 for(unsigned j=0;j<4;j++)for(auto w:g.chorus_history(j))require(w.bits==0);
 auto cw=common(g);auto rw=raw(g);require(cw.size()==864&&rw.size()==864);
 for(unsigned i=0;i<cw.size();i++)*cw[i]=Word24(i+31);
 g.copy_common_to_reverb_buffers();
 for(unsigned i=0;i<rw.size();i++)require(*rw[i]==i+31);
 for(unsigned bad=0;bad<cw.size();bad++){
  auto before=*cw[bad];cw[bad]->bits=0x1000000;
  reject([&]{g.copy_common_to_reverb_buffers();});
  for(unsigned i=0;i<rw.size();i++)require(*rw[i]==i+31);
  *cw[bad]=before;
 }
 for(unsigned bad=0;bad<rw.size();bad++){
  auto before=*rw[bad];*rw[bad]=0x1000000;
  reject([&]{g.copy_reverb_buffers_to_common();});
  for(unsigned i=0;i<cw.size();i++)require(cw[i]->bits==i+31);
  *rw[bad]=before;
 }
 // All four active states cache these actual owner bindings across retained calls.
 std::array<Word24,32> decoy{};std::array<tu15_sound_chorus::SampleTrace,32> trace;
 unsigned calls=0;
 for(unsigned frame=0;frame<20;frame++)for(unsigned j=0;j<4;j++){
  auto supplied=frame?tu15_sound_chorus::Bindings{decoy,decoy,decoy}:g.chorus_bindings(j);
  tu15_sound_chorus::process({Word24(0),{}},g.effects().chorus_parameters[j],g.effects().chorus_states[j],supplied,g.chorus_history(j),trace);
  const auto& active=*g.effects().chorus_states[j].active;
  require(active.bindings.audio.data()==g.mixbin(j+6).data()&&active.bindings.modulation.data()==g.mixbin(14).data()&&active.bindings.output.data()==g.fanout_destination(j).data());
  require(g.chorus_history(j).data()+400==(j<3?g.chorus_history(j+1).data():g.chorus_history(3).data()+400));calls++;
 }
 // Caller seed owners die before validating every retained history/sink/padding.
 std::unique_ptr<GraphStorage> retained;
 unsigned seed_guards=0;
 {
  std::array<std::array<std::vector<std::uint32_t>,14>,2> h;
  std::array<std::vector<std::byte>,2> tails;
  std::array<std::vector<Word24>,4> sinks;
  std::vector<std::byte> final(3904,std::byte{0x5a});RetainedScratchSeed seed;
  for(unsigned j=0;j<2;j++){
   tails[j].assign(51008,std::byte{static_cast<unsigned char>(0x31+j)});seed.reverb_unused_tails[j]=tails[j];
   for(unsigned k=0;k<14;k++){h[j][k].resize(g.reverb(j).state.rings[k].words.size());for(unsigned n=0;n<h[j][k].size();n++)h[j][k][n]=pattern(j,k,n);seed.reverb_histories[j][k]=h[j][k];}
  }
  for(unsigned j=0;j<4;j++){sinks[j].resize(640);for(unsigned n=0;n<640;n++)sinks[j][n]=Word24(pattern(j,14,n));seed.controller_sinks[j]=sinks[j];}seed.final_unused_tail=final;
  for(unsigned j=0;j<2;j++){
   auto tail=seed.reverb_unused_tails[j];seed.reverb_unused_tails[j]=tail.first(tail.size()-1);reject([&]{GraphStorage::retained_scratch_literal(seed);});seed_guards++;seed.reverb_unused_tails[j]=tail;
   for(unsigned k=0;k<14;k++){
    auto span=seed.reverb_histories[j][k];seed.reverb_histories[j][k]=span.first(span.size()-1);reject([&]{GraphStorage::retained_scratch_literal(seed);});seed_guards++;seed.reverb_histories[j][k]=span;
    auto before=h[j][k].back();h[j][k].back()=0x1000000;reject([&]{GraphStorage::retained_scratch_literal(seed);});seed_guards++;h[j][k].back()=before;
   }
  }
  for(unsigned j=0;j<4;j++){
   auto span=seed.controller_sinks[j];seed.controller_sinks[j]=span.first(639);reject([&]{GraphStorage::retained_scratch_literal(seed);});seed_guards++;seed.controller_sinks[j]=span;
   auto before=sinks[j].back();sinks[j].back().bits=0x1000000;reject([&]{GraphStorage::retained_scratch_literal(seed);});seed_guards++;sinks[j].back()=before;
  }
  seed.final_unused_tail=std::span<const std::byte>(final).first(3903);reject([&]{GraphStorage::retained_scratch_literal(seed);});seed_guards++;seed.final_unused_tail=final;
  retained=GraphStorage::retained_scratch_literal(seed);
 }
 unsigned ring_words=0,padding_bytes=0,sink_words=0;
 for(unsigned j=0;j<2;j++){
  for(unsigned k=0;k<14;k++){const auto& v=retained->reverb(j).state.rings[k].words;for(unsigned n=0;n<v.size();n++)require(v[n]==pattern(j,k,n));ring_words+=v.size();}
  for(auto b:retained->reverb_unused_tail(j))require(b==std::byte{static_cast<unsigned char>(0x31+j)});padding_bytes+=retained->reverb_unused_tail(j).size();
 }
 for(unsigned j=0;j<4;j++)for(unsigned n=0;n<640;n++){require(retained->controller_sink(j)[n].bits==pattern(j,14,n));sink_words++;}
 for(auto b:retained->final_unused_tail())require(b==std::byte{0x5a});padding_bytes+=retained->final_unused_tail().size();
 require(ring_words==164496&&sink_words==2560&&padding_bytes==105920&&seed_guards==67&&rejects==1795);
 require(retained->effects().fanout_state.flags.bits==0&&retained->effects().chorus_states[0].flags.bits==0&&!retained->effects().chorus_states[0].active);
 std::cout<<"{\"passed\":true,\"adapter_no_mutation_rejections\":1728,\"seed_rejections\":"<<seed_guards<<",\"all_rejections\":"<<rejects<<",\"retained_actual_chorus_calls\":"<<calls<<",\"retained_words_after_seed_destruction\":"<<ring_words+sink_words<<",\"padding_bytes_after_seed_destruction\":"<<padding_bytes<<",\"runtime_ready\":false}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
