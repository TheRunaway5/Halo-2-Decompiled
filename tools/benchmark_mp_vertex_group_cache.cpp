#include "halo2/cache.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <tuple>

namespace {
using namespace halo2;
using Key=std::tuple<std::uint32_t,std::uint32_t,int,std::uint32_t,std::uint32_t>;
Key key(const Vertex &v){return {v.material,v.lightmap,v.bsp_cluster,v.source_section,v.source_part};}
// GPU descriptor service is retained as its exact ordered material/lightmap
// request. This CPU-only tool does not manufacture or query Vulkan handles.
struct Draw {unsigned first{},count{};Key key;auto operator<=>(const Draw&)const=default;};
struct Output {std::vector<Vertex> vertices;std::vector<Draw> draws;};
void group(std::span<const Vertex> input,Output &out,unsigned material_count,unsigned lightmap_count,bool optimized) {
  std::map<Key,unsigned> counts;auto counted=counts.end();
  for(const auto &v:input) {
    if(v.material>=material_count || v.lightmap>=lightmap_count)throw FormatError("invalid dynamic material/lightmap index");
    const auto k=key(v);
    if(optimized) {
      if(counted==counts.end() || counted->first!=k)counted=counts.try_emplace(k,0).first;
      ++counted->second;
    } else ++counts[k];
  }
  auto first=unsigned(out.vertices.size());std::map<Key,unsigned> cursor;
  for(const auto &[k,count]:counts){out.draws.push_back({first,count,k});cursor[k]=first;first+=count;}
  out.vertices.resize(first);auto destination=cursor.end();
  for(const auto &v:input) {
    const auto k=key(v);
    if(optimized) {
      if(destination==cursor.end() || destination->first!=k)destination=cursor.find(k);
      out.vertices[destination->second++]=v;
    } else out.vertices[cursor[k]++]=v;
  }
}
Output evaluate(std::span<const Vertex> input,unsigned views,unsigned materials,unsigned lightmaps,bool optimized) {
  Output out;for(unsigned view=0;view<views;++view)group(input,out,materials,lightmaps,optimized);return out;
}
void equal(const Output &a,const Output &b) {
  if(a.draws!=b.draws || a.vertices.size()!=b.vertices.size() ||
      std::memcmp(a.vertices.data(),b.vertices.data(),a.vertices.size()*sizeof(Vertex)))
    throw FormatError("Consecutive-key cache changed full stable vertex/draw request output");
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=3)throw halo2::FormatError("Supply ORIGINAL_SELECTED_VERTEX_BIN OUTPUT_TSV");
    static_assert(sizeof(halo2::Vertex)==132);
    std::ifstream file(argv[1],std::ios::binary|std::ios::ate);
    const auto bytes=file.tellg();if(bytes<=0 || bytes%sizeof(halo2::Vertex))throw halo2::FormatError("Invalid full selected vertex fixture");
    std::vector<halo2::Vertex> vertices(std::size_t(bytes)/sizeof(halo2::Vertex));
    file.seekg(0);file.read(reinterpret_cast<char*>(vertices.data()),bytes);if(!file)throw halo2::FormatError("Read selected vertex fixture");
    unsigned materials{},lightmaps{};for(const auto &v:vertices){materials=std::max(materials,v.material+1);lightmaps=std::max(lightmaps,v.lightmap+1);}
    std::ofstream out(argv[2]);if(!out)throw halo2::FormatError("Open vertex group benchmark output");
    out<<"views\trepeat\tbaseline_ms\tcandidate_ms\tvertices\tdraws\truns\n"<<std::setprecision(12);
    unsigned runs{};std::optional<Key> last;for(const auto &v:vertices){if(!last || *last!=key(v))++runs;last=key(v);}
    unsigned comparisons{};
    for(unsigned views:{1u,4u})for(unsigned repeat=0;repeat<128;++repeat) {
      using Clock=std::chrono::steady_clock;
      Output baseline,candidate;double before{},after{};
      const auto timed=[&](bool optimized,Output &result,double &ms){const auto start=Clock::now();result=evaluate(vertices,views,materials,lightmaps,optimized);ms=std::chrono::duration<double,std::milli>(Clock::now()-start).count();};
      if(repeat%2){timed(true,candidate,after);timed(false,baseline,before);}else{timed(false,baseline,before);timed(true,candidate,after);}
      equal(baseline,candidate);++comparisons;
      if(repeat>=8)out<<views<<'\t'<<repeat-8<<'\t'<<before<<'\t'<<after<<'\t'<<baseline.vertices.size()<<'\t'<<baseline.draws.size()<<'\t'<<runs*views<<'\n';
    }
    // Explicit algorithm controls, not naturally emitted render fixtures:
    // empty data, reversed triangle blocks, alternating distant source parts,
    // repeated calls with a nonempty destination, and both bounds rejections.
    auto reversed=vertices;for(unsigned i=0;i<vertices.size()/3;++i)for(unsigned j=0;j<3;++j)reversed[i*3+j]=vertices[(vertices.size()/3-1-i)*3+j];
    std::vector<Vertex> alternating;for(unsigned i=0;i<600;++i){const auto at=i%2?vertices.size()-3:0;alternating.insert(alternating.end(),vertices.begin()+at,vertices.begin()+at+3);}
    for(const auto *input:{&reversed,&alternating}){equal(evaluate(*input,4,materials,lightmaps,false),evaluate(*input,4,materials,lightmaps,true));++comparisons;}
    for(bool optimized:{false,true}) {
      Output result;group({},result,materials,lightmaps,optimized);
      if(!result.draws.empty() || !result.vertices.empty())throw halo2::FormatError("Empty group changed output");
      for(bool material:{false,true}) {
        auto bad=vertices.front();if(material)bad.material=materials;else bad.lightmap=lightmaps;
        bool rejected{};try{group(std::span{&bad,1},result,materials,lightmaps,optimized);}catch(const halo2::FormatError&){rejected=true;}
        if(!rejected)throw halo2::FormatError("Group cache omitted original bounds validation");
      }
    }
    if(!out)throw halo2::FormatError("Write group timing output");
    std::cout<<"PASS "<<comparisons<<" full stable vertex/draw-request comparisons, empty/reordered/repeated inputs and bounds controls.\n"
        <<"LIMIT native CPU grouping; descriptor request arguments/order retained, no Vulkan handles/GPU timing supplied.\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
