#include "halo2/source_ctf.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cfenv>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* reason) {if(!value)throw std::runtime_error(reason);}
std::vector<std::uint8_t> decode(const std::string& hex) {
  check((hex.size()&1u)==0,"Odd original byte encoding");
  std::vector<std::uint8_t> result(hex.size()/2);
  for(std::size_t i=0;i<result.size();++i)result[i]=std::uint8_t(std::stoul(hex.substr(i*2,2),nullptr,16));
  return result;
}
template<class T>T record(const std::string& hex) {
  const auto raw=decode(hex);check(raw.size()==sizeof(T),"Original ABI record width differs");
  T result;std::memcpy(&result,raw.data(),sizeof(result));return result;
}
void same(std::span<const std::uint8_t> actual,std::span<const std::uint8_t> expected) {
  check(actual.size()==expected.size(),"Original state extent differs");
  for(std::size_t i=0;i<actual.size();++i)if(actual[i]!=expected[i]) {
    std::cerr<<"first difference byte+"<<std::hex<<i<<" native="<<unsigned(actual[i])
        <<" original="<<unsigned(expected[i])<<std::dec<<'\n';
    throw std::runtime_error("Complete original state/output differs");
  }
}
struct GuardedState {
  std::array<std::uint8_t,32> before;
  SourceCtfState state;
  std::array<std::uint8_t,32> after;
  GuardedState() {before.fill(0xa5);state.bytes.fill(0x73);after.fill(0x5a);}
  void guards()const {
    check(std::all_of(before.begin(),before.end(),[](auto b){return b==0xa5;}),"Native state wrote before the source bank");
    check(std::all_of(after.begin(),after.end(),[](auto b){return b==0x5a;}),"Native state wrote after the source bank");
  }
};
}

int main(int argc,char** argv) {
  try {
    check(argc==2,"usage: ctf_initialization_tests ORIGINAL_FIXTURE");
    check(std::fesetround(FE_TONEAREST)==0,"Original nearest-rounding context unavailable");
    std::ifstream file(argv[1]);Json fixture;file>>fixture;
    check(fixture.at("schema")=="original-ctf-initialization-v1","Unexpected original fixture schema");
    check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Original oracle supplied function results or instruction patches");
    check(fixture.at("images").size()==3,"All three original code versions are required");
    const auto& blobs=fixture.at("raw_blobs");
    std::vector<std::vector<SourceCtfMarker>> maps;unsigned mcc_maps=0;
    for(const auto& map:fixture.at("maps")) {
      maps.emplace_back(map.at("count").get<std::size_t>(),record<SourceCtfMarker>(map.at("fill_row_hex")));
      for(const auto& patch:map.at("patches"))maps.back().at(patch.at("index"))=record<SourceCtfMarker>(patch.at("row_hex"));
      if(map.contains("mcc_source")) {
        check(map.at("mcc_source").at("cache_version")==13,"Authored markers are not from the imported MCC cache");
        check(map.at("mcc_source").at("stride")==32,"MCC authored marker ABI differs");++mcc_maps;
      }
    }
    unsigned cases=0,initializers=0,selectors=0,regions=0,queries=0;
    for(std::size_t n=0;n<fixture.at("inputs").size();++n) {
      const auto& input=fixture.at("inputs")[n];const auto& expected=fixture.at("expected")[n];
      const auto index=input.at("map").get<std::size_t>();const auto& markers=maps.at(index);
      const auto before_markers=markers;
      SourceCtfContext context{input.at("source_engine"),input.at("objective_kind")};
      const auto operation=input.at("operation").get<std::string>();GuardedState guarded;
      if(input.contains("before"))guarded.state=record<SourceCtfState>(blobs.at(input.at("before").get<std::string>()));
      try {
        if(operation=="initialize") {
          check(source_ctf_initialize(guarded.state,markers,context)==(expected.at("defined_al").get<unsigned>()!=0),"Original initializer AL differs");
          same(guarded.state.bytes,decode(blobs.at(expected.at("bank").get<std::string>())));++initializers;
        } else if(operation=="select") {
          auto second=std::bit_cast<std::int32_t>(input.at("second_before").get<std::uint32_t>());
          const auto selected=source_ctf_select_marker(markers,context,input.at("team"),input.at("spawn"),input.at("second").get<bool>()?&second:nullptr);
          check(std::bit_cast<std::uint32_t>(selected)==expected.at("returned"),"Original selected marker differs");
          check(std::bit_cast<std::uint32_t>(second)==expected.at("second"),"Original conditional second output differs");++selectors;
        } else if(operation=="regions") {
          source_ctf_build_capture_regions(guarded.state,markers,context,input.at("team"));
          same(guarded.state.bytes,decode(blobs.at(expected.at("bank").get<std::string>())));++regions;
        } else {
          check(operation=="query","Unknown original fixture operation");
          auto output=record<std::array<std::int32_t,10>>(blobs.at(input.at("output_before").get<std::string>()));
          SourceCtfMarkerQuery query;query.type=input.at("type");query.team=input.at("team");query.channel=input.at("channel");
          query.maximum_distance=std::bit_cast<float>(input.at("distance_bits").get<std::uint32_t>());
          query.maximum_vertical_distance=std::bit_cast<float>(input.at("vertical_bits").get<std::uint32_t>());
          if(!input.at("point_hex").is_null())query.point=record<std::array<float,3>>(input.at("point_hex"));
          const auto count=source_ctf_query_markers(markers,query,std::span(output).first(input.at("capacity").get<std::size_t>()));
          check(std::uint32_t(count)==expected.at("returned"),"Original marker-query count differs");
          same({reinterpret_cast<const std::uint8_t*>(output.data()),sizeof(output)},decode(blobs.at(expected.at("output").get<std::string>())));++queries;
        }
        guarded.guards();
        check(markers.empty() || std::memcmp(markers.data(),before_markers.data(),markers.size()*sizeof(SourceCtfMarker))==0,"Native initializer changed its authored MCC/input rows");
      } catch(const std::exception&) {
        std::cerr<<"case "<<n<<" map="<<fixture.at("maps")[index].at("name")<<' '<<input.dump()<<'\n';throw;
      }
      ++cases;
    }
    // Native backing admission is distinct from original game behavior. Invalid
    // extents must stop before the signed16 original iteration wraps.
    std::vector<SourceCtfMarker> excessive(32768);GuardedState guarded;const auto before=guarded.state;
    bool rejected=false;try{source_ctf_initialize(guarded.state,excessive,{});}catch(const std::runtime_error&){rejected=true;}
    check(rejected && guarded.state.bytes==before.bytes,"Unavailable signed16 marker extent was admitted/mutated");
    for(int team:{-1,9,std::numeric_limits<int>::min(),std::numeric_limits<int>::max()}) {
      rejected=false;try{source_ctf_build_capture_regions(guarded.state,{}, {},team);}catch(const std::runtime_error&){rejected=true;}
      check(rejected && guarded.state.bytes==before.bytes,"Unavailable native team backing was admitted/mutated");
    }
    guarded.guards();
    std::cout<<"Complete original CTF initializer/marker owners: "<<cases<<" cases ("<<initializers<<" initialize, "
        <<selectors<<" select, "<<regions<<" region, "<<queries<<" query); "<<mcc_maps
        <<" actual MCC marker inputs; 5 native backing rejections. Full mode remains gated.\n";
    return 0;
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
