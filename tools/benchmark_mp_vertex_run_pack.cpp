#include "halo2/cache.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <tuple>
#include <type_traits>

// This CPU-only comparison retains the ordered descriptor request arguments.
// It does not manufacture Vulkan descriptors or measure GPU execution.
namespace {
using namespace halo2;
using Key=std::tuple<std::uint32_t,std::uint32_t,int,std::uint32_t,std::uint32_t>;
Key key(const Vertex &v) {return {v.material,v.lightmap,v.bsp_cluster,v.source_section,v.source_part};}
struct Draw {unsigned call{},first{},count{};Key key;auto operator<=>(const Draw&)const=default;};
struct Descriptor {unsigned call{},material{},lightmap{};auto operator<=>(const Descriptor&)const=default;};
struct Output {std::vector<Vertex> vertices;std::vector<Draw> draws;std::vector<Descriptor> descriptors;};
struct Call {bool preserve{};std::vector<Vertex> vertices;};
struct Fixture {unsigned materials{},lightmaps{};std::vector<Call> calls;};
enum class Method {Scatter, ScatterReserve, Runs};
struct DescriptorStop {};
void descriptor(Output &out,unsigned call,const Key &k,std::optional<unsigned> stop) {
  out.descriptors.push_back({call,std::get<0>(k),std::get<1>(k)});
  if(stop && out.descriptors.size()-1==*stop)throw DescriptorStop{};
}
void validate(const Vertex &v,const Fixture &fixture) {
  if(v.material>=fixture.materials || v.lightmap>=fixture.lightmaps)
    throw FormatError("invalid dynamic material/lightmap index");
}
void group(std::span<const Vertex> input,Output &out,const Fixture &fixture,unsigned call,
           bool preserve,Method method,std::optional<unsigned> stop={}) {
  if(preserve) {
    // Keep the current alpha-layer stream path literally, including its
    // validation/descriptor/output order on a later invalid vertex.
    std::optional<Key> previous;
    for(const auto &v:input) {
      validate(v,fixture);const auto k=key(v);
      if(!previous || *previous!=k) {
        descriptor(out,call,k,stop);
        out.draws.push_back({call,unsigned(out.vertices.size()),0,k});previous=k;
      }
      out.vertices.push_back(v);++out.draws.back().count;
    }
    return;
  }
  if(method!=Method::Runs) {
    // Current production iterator-cached count/resize/stable-scatter path.
    std::map<Key,unsigned> counts;auto counted=counts.end();
    for(const auto &v:input) {
      validate(v,fixture);const auto k=key(v);
      if(counted==counts.end() || counted->first!=k)counted=counts.try_emplace(k,0).first;
      ++counted->second;
    }
    auto first=unsigned(out.vertices.size());std::map<Key,unsigned> cursor;
    for(const auto &[k,count]:counts) {
      descriptor(out,call,k,stop);out.draws.push_back({call,first,count,k});cursor[k]=first;first+=count;
    }
    out.vertices.resize(first);auto destination=cursor.end();
    for(const auto &v:input) {
      const auto k=key(v);
      if(destination==cursor.end() || destination->first!=k)destination=cursor.find(k);
      out.vertices[destination->second++]=v;
    }
    return;
  }
  struct Run {std::size_t first{},count{};};
  struct Pack {unsigned count{};std::vector<Run> runs;};
  std::map<Key,Pack> packs;
  for(std::size_t i=0;i<input.size();) {
    const auto &v=input[i];validate(v,fixture);const auto k=key(v);auto end=i+1;
    while(end<input.size() && key(input[end])==k)++end;
    auto &pack=packs[k];pack.runs.push_back({i,end-i});pack.count+=unsigned(end-i);i=end;
  }
  // Requests must all precede this call's vertex append, as in the original
  // sorted path. This also preserves partial state at a descriptor stop.
  auto first=unsigned(out.vertices.size());
  for(const auto &[k,pack]:packs) {
    descriptor(out,call,k,stop);out.draws.push_back({call,first,pack.count,k});first+=pack.count;
  }
  for(const auto &[k,pack]:packs)for(const auto &run:pack.runs) {
    const auto span=input.subspan(run.first,run.count);
    out.vertices.insert(out.vertices.end(),span.begin(),span.end());
  }
}
std::size_t vertex_count(const Fixture &fixture) {
  std::size_t count{};for(const auto &call:fixture.calls)count+=call.vertices.size();return count;
}
Output evaluate(const Fixture &fixture,Method method,bool compare_phases=false,
                std::optional<unsigned> stop={},std::vector<Vertex> *staging=nullptr) {
  Output out;if(staging){out.vertices.swap(*staging);out.vertices.clear();}
  if(method!=Method::Scatter)out.vertices.reserve(vertex_count(fixture));
  Output reference;
  for(unsigned i=0;i<fixture.calls.size();++i) {
    const auto &call=fixture.calls[i];group(call.vertices,out,fixture,i,call.preserve,method,stop);
    if(compare_phases) {
      group(call.vertices,reference,fixture,i,call.preserve,Method::Scatter);
      if(out.draws!=reference.draws || out.descriptors!=reference.descriptors || out.vertices.size()!=reference.vertices.size() ||
         (!out.vertices.empty() && std::memcmp(out.vertices.data(),reference.vertices.data(),out.vertices.size()*sizeof(Vertex))))
        throw FormatError("run packing changed an intermediate complete group output");
    }
  }
  return out;
}
void equal(const Output &a,const Output &b) {
  if(a.draws!=b.draws || a.descriptors!=b.descriptors || a.vertices.size()!=b.vertices.size() ||
      (!a.vertices.empty() && std::memcmp(a.vertices.data(),b.vertices.data(),a.vertices.size()*sizeof(Vertex))))
    throw FormatError("run packing changed full stable vertices/draw/descriptor request output");
}
Fixture load(const std::string &stem) {
  std::ifstream file(stem+".vertices.bin",std::ios::binary|std::ios::ate);
  const auto bytes=file.tellg();if(bytes<0 || bytes%sizeof(Vertex))throw FormatError("invalid complete group vertex binary");
  std::vector<Vertex> all(std::size_t(bytes)/sizeof(Vertex));file.seekg(0);
  if(bytes)file.read(reinterpret_cast<char*>(all.data()),bytes);if(!file)throw FormatError("read complete group vertex binary");
  std::ifstream layout(stem+".tsv");Fixture fixture;unsigned stride{};std::string line;std::size_t at{};
  while(std::getline(layout,line)) {
    std::istringstream row(line);std::string field;row>>field;
    if(field=="stride")row>>stride;
    else if(field=="materials")row>>fixture.materials;
    else if(field=="lightmaps")row>>fixture.lightmaps;
    else if(field=="call" || field.empty())continue;
    else {
      const auto call=std::stoul(field);unsigned preserve{};std::size_t first{},count{};row>>preserve>>first>>count;
      if(!row || call!=fixture.calls.size() || preserve>1 || first!=at || first>all.size() || count>all.size()-first)
        throw FormatError("invalid ordered group input layout");
      fixture.calls.push_back({bool(preserve),{all.begin()+first,all.begin()+first+count}});at+=count;
    }
  }
  if(!layout.eof() || stride!=sizeof(Vertex) || !fixture.materials || !fixture.lightmaps || at!=all.size())
    throw FormatError("incomplete original group input layout");
  return fixture;
}
unsigned runs(const Fixture &fixture) {
  unsigned count{};for(const auto &call:fixture.calls) {
    std::optional<Key> last;for(const auto &v:call.vertices){if(!last || *last!=key(v))++count;last=key(v);}
  }return count;
}
void controls(const Fixture &fixture,unsigned &comparisons,unsigned &failure_controls) {
  const Vertex *first{};for(const auto &call:fixture.calls)if(!call.vertices.empty()){first=&call.vertices.front();break;}
  if(!first)throw FormatError("capture has no vertices for algorithm controls");
  auto test=[&](const Fixture &control){equal(evaluate(control,Method::Scatter),evaluate(control,Method::Runs,true));++comparisons;};
  auto reversed=fixture;for(auto &call:reversed.calls) {
    if(call.vertices.size()%3)throw FormatError("capture has incomplete triangles");
    auto original=call.vertices;for(std::size_t i=0;i<original.size()/3;++i)
      for(unsigned j=0;j<3;++j)call.vertices[i*3+j]=original[(original.size()/3-1-i)*3+j];
  }test(reversed);
  Fixture empty{fixture.materials,fixture.lightmaps,{{false,{}},{true,{}}}};test(empty);
  Fixture repeated=fixture;repeated.calls.insert(repeated.calls.end(),fixture.calls.begin(),fixture.calls.end());test(repeated);
  // Explicit controls below alter metadata/order, not purported game outputs.
  for(bool preserve:{false,true})for(unsigned pattern:{0u,1u,2u}) {
    Fixture control{fixture.materials,fixture.lightmaps,{{preserve,{}}}};
    for(unsigned i=0;i<1800;++i) {
      auto v=*first;v.source_part=pattern==0?i:pattern==1?i%2:(i/3)%17;control.calls[0].vertices.push_back(v);
    }test(control);
  }
  for(bool preserve:{false,true})for(bool material:{false,true})for(unsigned invalid_at:{0u,7u,19u}) {
    Fixture control{fixture.materials,fixture.lightmaps,{{false,{*first}},{preserve,std::vector<Vertex>(20,*first)}}};
    if(material)control.calls[1].vertices[invalid_at].material=control.materials;
    else control.calls[1].vertices[invalid_at].lightmap=control.lightmaps;
    Output baseline,candidate;bool before{},after{};
    for(unsigned i=0;i<control.calls.size();++i) {
      const auto &call=control.calls[i];
      try{group(call.vertices,baseline,control,i,call.preserve,Method::Scatter);}catch(const FormatError &){before=true;}
      try{group(call.vertices,candidate,control,i,call.preserve,Method::Runs);}catch(const FormatError &){after=true;}
      equal(baseline,candidate);if(before || after)break;
    }
    if(!before || !after)throw FormatError("run packing changed validation/failure boundary");++failure_controls;
  }
  const auto valid=evaluate(fixture,Method::Scatter);
  // A failed descriptor is an explicit unavailable provider control, not a
  // claim about a real Vulkan descriptor allocator. Check every request stop.
  for(unsigned stop=0;stop<valid.descriptors.size();++stop) {
    Output baseline,candidate;bool before{},after{};
    for(unsigned i=0;i<fixture.calls.size();++i) {
      const auto &call=fixture.calls[i];
      try{group(call.vertices,baseline,fixture,i,call.preserve,Method::Scatter,stop);}catch(const DescriptorStop &){before=true;}
      try{group(call.vertices,candidate,fixture,i,call.preserve,Method::Runs,stop);}catch(const DescriptorStop &){after=true;}
      equal(baseline,candidate);if(before || after)break;
    }
    if(!before || !after)throw FormatError("run packing changed descriptor stop boundary");++failure_controls;
  }
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=3)throw halo2::FormatError("Supply ORIGINAL_GROUP_CAPTURE_STEM OUTPUT_TSV");
    static_assert(sizeof(Vertex)==132 && std::is_trivially_copyable_v<Vertex>);
    const auto fixture=load(argv[1]);unsigned comparisons{},failure_controls{};
    equal(evaluate(fixture,Method::Scatter),evaluate(fixture,Method::Runs,true));++comparisons;
    controls(fixture,comparisons,failure_controls);
    std::ofstream out(argv[2]);if(!out)throw FormatError("open run packing timing output");
    out<<"storage\trepeat\titerator_scatter_ms\treserved_scatter_ms\trun_append_ms\tvertices\tdraws\truns\tcalls\n"<<std::setprecision(12);
    for(bool retained:{false,true}) {
      std::array<std::vector<Vertex>,3> staging;
      for(unsigned repeat=0;repeat<136;++repeat) {
      using Clock=std::chrono::steady_clock;
      std::array<Output,3> results;std::array<double,3> ms{};
      // Rotate timing order so allocation/cache/order does not always favor a
      // particular method. All full output comparisons occur after timing.
      for(unsigned j=0;j<3;++j) {
        const auto index=(repeat+j)%3;const auto start=Clock::now();
        results[index]=evaluate(fixture,Method(index),false,{},retained?&staging[index]:nullptr);
        ms[index]=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
      }
      equal(results[0],results[1]);equal(results[0],results[2]);comparisons+=2;
      if(repeat>=8)out<<(retained?"retained":"fresh")<<'\t'<<repeat-8<<'\t'<<ms[0]<<'\t'<<ms[1]<<'\t'<<ms[2]<<'\t'<<results[0].vertices.size()
          <<'\t'<<results[0].draws.size()<<'\t'<<runs(fixture)<<'\t'<<fixture.calls.size()<<'\n';
      // Only the vertex staging capacity persists. Draw/request lists remain
      // fresh, matching the production renderer's per-frame list lifetime.
      if(retained)for(unsigned i=0;i<3;++i)results[i].vertices.swap(staging[i]);
      }
    }
    if(!out)throw FormatError("write run packing timing output");
    std::cout<<"PASS "<<comparisons<<" full stable vertex/draw/descriptor comparisons and "<<failure_controls
        <<" complete failure-state controls.\nINPUT "<<fixture.calls.size()<<" exact ordered group calls, "<<vertex_count(fixture)
        <<" original vertices, "<<runs(fixture)<<" contiguous runs.\nLIMIT CPU packing only; no Vulkan handles, shader changes or GPU timing supplied.\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
