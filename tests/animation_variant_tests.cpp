#include "halo2/animation_variant.hpp"
#if HALO2_VARIANT_CACHE_TESTS
#include "halo2/original_data.hpp"
#include "halo2/unit_animation_metadata_store.hpp"
#endif
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {
using namespace halo2;
using Buffer = std::vector<std::byte>;
void check(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}
std::uint32_t hex(std::istringstream &row) {
  std::uint32_t value;
  check(bool(row >> std::hex >> value), "Incomplete original variant fixture");
  return value;
}
Buffer bytes(const std::string &raw) {
  check(raw.size()%2==0, "Incomplete original variant packet");
  Buffer out;
  for (std::size_t i=0;i<raw.size();i+=2)
    out.push_back(std::byte(std::stoul(raw.substr(i,2),nullptr,16)));
  return out;
}
struct Event {
  bool rng;
  std::uint32_t graph{}, packed{};
  AnimationVariantRngStore store{};
  friend bool operator==(const Event &,const Event &) = default;
};
struct Case {
  bool wrapper;
  std::uint32_t id, graph, packed, selector, selected;
  std::array<std::uint32_t,2> before, after;
  Buffer manager;
  std::vector<Event> events;
};
struct Fixture {
  std::map<std::uint32_t,Buffer> memory;
  std::map<std::uint32_t,std::uint32_t> graphs;
  AnimationVariantConstants constants;
  std::vector<Case> cases;
};
Fixture load(const char *path) {
  std::ifstream file(path);check(bool(file),"Original variant fixture unavailable");
  Fixture fixture;
  for (std::string line;std::getline(file,line);) {
    if (line.empty()||line[0]=='#')continue;
    std::istringstream row(line);std::string kind,raw;row>>kind;
    if (kind=="MEM") {
      auto address=hex(row);row>>raw;auto data=bytes(raw);
      auto [it,new_region]=fixture.memory.emplace(address,data);
      check(new_region||it->second==data,"Conflicting original graph backing");
    } else if(kind=="GRAPH") {
      auto datum=hex(row),address=hex(row);fixture.graphs.emplace(datum,address);
    } else if(kind=="CONSTANTS") {
      fixture.constants={hex(row),hex(row),hex(row)};
    } else if(kind=="CASE") {
      std::string function;row>>function;std::uint32_t id;row>>id;
      Case c{};c.wrapper=function=="WRAPPER";c.id=id;
      c.graph=hex(row);c.packed=hex(row);c.selector=hex(row);
      c.before={hex(row),hex(row)};c.after={hex(row),hex(row)};c.selected=hex(row);
      fixture.cases.push_back(std::move(c));
    } else if(kind=="MANAGER") {
      row>>raw;fixture.cases.back().manager=bytes(raw);
    } else if(kind=="RESOURCE") {
      Event event{};event.graph=hex(row);event.packed=hex(row);
      fixture.cases.back().events.push_back(event);
    } else if(kind=="RNG") {
      Event event{};event.rng=true;
      event.store={hex(row),hex(row),hex(row),hex(row)};
      fixture.cases.back().events.push_back(event);
    } else check(kind=="ENDCASE","Unknown original variant fixture row");
  }
  return fixture;
}
SourceAnimationGraphMemory literal_memory(const Fixture &fixture) {
  return {
    [&](std::uint32_t datum) {
      auto at=fixture.graphs.find(datum);
      check(at!=fixture.graphs.end(),"Original graph identity unavailable");
      return at->second;
    },
    [&](std::uint32_t address,std::size_t size)->std::span<const std::byte> {
      auto region=fixture.memory.upper_bound(address);
      check(region!=fixture.memory.begin(),"Original graph address unavailable");--region;
      const auto offset=std::uint64_t(address)-region->first;
      check(offset<=region->second.size()&&size<=region->second.size()-offset,
            "Original graph read exceeds backing");
      return std::span<const std::byte>(region->second).subspan(std::size_t(offset),size);
    }
  };
}
void verify(const Fixture &fixture,const SourceAnimationGraphMemory &memory,
            AnimationVariantConstants constants,const char *label) {
  std::size_t resources=0, stores=0, changed=0;
  for (const auto &c:fixture.cases) {
    auto rng=c.before;std::vector<Event> observed;
    AnimationVariantServices services{memory,constants,
      [&](std::uint32_t graph,std::uint32_t packed) {
        Event event{};event.graph=graph;event.packed=packed;observed.push_back(event);
      },
      [&](const AnimationVariantRngStore &store) {
        Event event{};event.rng=true;event.store=store;observed.push_back(event);
      }};
    const auto output=c.wrapper?select_source_animation_variant(c.manager,c.packed,rng,services):
      choose_source_animation_variant(c.graph,c.packed,std::uint8_t(c.selector),rng,services);
    const auto identity=std::string(label)+" sourceCase="+std::to_string(c.id);
    check(output==c.selected,identity+" selected packed differs");
    check(rng==c.after,identity+" complete two-stream RNG differs");
    check(observed==c.events,identity+" ordered resource/RNG stores differ");
    resources+=std::count_if(observed.begin(),observed.end(),[](const auto &e){return !e.rng;});
    stores+=std::count_if(observed.begin(),observed.end(),[](const auto &e){return e.rng;});
    changed+=output!=c.packed;
    // A required resource producer must fail at its first actual invocation,
    // before later RNG/output, instead of silently keeping the old variant.
    if(!c.events.empty()&&!c.events.front().rng) {
      services.resource={};auto stopped_rng=c.before;bool stopped=false;
      try {
        if(c.wrapper)(void)select_source_animation_variant(c.manager,c.packed,stopped_rng,services);
        else (void)choose_source_animation_variant(c.graph,c.packed,std::uint8_t(c.selector),stopped_rng,services);
      } catch(const std::runtime_error &) {stopped=true;}
      check(stopped&&stopped_rng==c.before,identity+" missing resource became successful");
    }
  }
  std::cout<<label<<" cases="<<fixture.cases.size()<<" orderedResources="<<resources
           <<" rngStores="<<stores<<" selectedChanges="<<changed<<'\n';
}
} // namespace

int main(int argc,char **argv) {
  try {
    check(argc==2||argc==5,"Supply fixtures and optionally map plus1.1/u5 images");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    const auto fixture=load(argv[1]);check(!fixture.cases.empty(),"No actual source variant cases");
    verify(fixture,literal_memory(fixture),fixture.constants,"literal");
#if HALO2_VARIANT_CACHE_TESTS
    if(argc==5) {
      auto cache=std::make_shared<Cache>(argv[2]);
      UnitAnimationMetadataStore store(cache);const auto memory=store.graph_memory();
      for(const auto &[datum,address]:fixture.graphs)
        check(memory.graph_address(datum)==address,"Actual Cache graph address differs");
      for(const auto &[address,data]:fixture.memory) {
        const auto actual=memory.read(address,data.size());
        check(std::equal(actual.begin(),actual.end(),data.begin(),data.end()),
              "Actual Cache graph bytes differ from original oracle");
      }
      for(int image=3;image<=4;++image) {
        const auto original=OriginalData::load_xbe(argv[image]);
        check(bool(original->unit_animation_constants),"Original variant constants unavailable");
        const auto &words=*original->unit_animation_constants;
        AnimationVariantConstants constants{words.one_bits,words.rng_scale_bits,words.unsigned_conversion_correction_bits};
        check(constants.one_bits==fixture.constants.one_bits&&
                  constants.rng_scale_bits==fixture.constants.rng_scale_bits&&
                  constants.unsigned_conversion_correction_bits==fixture.constants.unsigned_conversion_correction_bits,
              "Original executable variant words differ");
        verify(fixture,memory,constants,image==3?"actualCache1.1":"actualCache1.5");
      }
    }
#else
    check(argc==2,"Actual Cache provider comparison requires its compiled provider");
#endif
  } catch(const std::exception &error) {
    std::cerr<<error.what()<<'\n';return 1;
  }
}
