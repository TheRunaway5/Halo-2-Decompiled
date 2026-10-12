#include "halo2/source_result_retention.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
extern "C" {
#include <libavutil/sha.h>
#include <libavutil/mem.h>
}

using namespace halo2;using nlohmann::json;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::vector<std::uint8_t> bytes(const std::string& text) {
  check(text.size()%2==0,"Odd source hex record");std::vector<std::uint8_t> result;
  for(unsigned i=0;i<text.size();i+=2)result.push_back(std::uint8_t(std::stoul(text.substr(i,2),nullptr,16)));
  return result;
}
template<std::size_t N>std::array<std::uint8_t,N> record(const std::string& text) {
  const auto raw=bytes(text);check(raw.size()==N,"Original record size differs");
  std::array<std::uint8_t,N> result;std::copy(raw.begin(),raw.end(),result.begin());return result;
}
std::string digest(const SourceRetainedResults& results) {
  std::unique_ptr<AVSHA,decltype(&av_free)> state(av_sha_alloc(),av_free);
  check(bool(state) && av_sha_init(state.get(),256)==0,"SHA256 context unavailable");
  av_sha_update(state.get(),reinterpret_cast<const std::uint8_t*>(&results),sizeof(results));
  std::array<std::uint8_t,32> raw;av_sha_final(state.get(),raw.data());
  std::string text;const char* hex="0123456789abcdef";
  for(auto b:raw){text.push_back(hex[b>>4]);text.push_back(hex[b&15]);}return text;
}
void setup(SourceRetainedResults& results,const json& input,bool initialize) {
  auto* raw=reinterpret_cast<std::uint8_t*>(&results);
  if(initialize)for(unsigned i=0;i<sizeof(results);++i)
    raw[i]=std::uint8_t(i*197+(i>>8)*31+input.at("seed").get<unsigned>()*17);
  for(const auto& edit:input.at("edits")) {
    const auto value=bytes(edit.at("hex"));const unsigned at=edit.at("offset");
    check(at+value.size()<=sizeof(results),"Original context edit outside recovered layout");
    std::copy(value.begin(),value.end(),raw+at);
  }
}
}
int main(int argc,char** argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply the unchanged original retention fixture");
    std::ifstream stream(argv[1]);json fixture;stream>>fixture;
    check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Retention oracle supplied a dependency");
    unsigned count=0,changed=0;std::array<std::vector<std::string>,3> outputs;unsigned version=0;
    for(const auto& [name,data]:fixture.at("versions").items()) {
      SourceRetainedResults results;
      const auto policy=data.at("prune").get<bool>() ? SourceResultMachinePolicy::PruneUnreferenced : SourceResultMachinePolicy::Keep;
      check(data.at("records").size()==fixture.at("runs").size(),"Original run/result count differs");
      unsigned index=0;
      for(const auto& run:fixture.at("runs")) {
        const auto& input=fixture.at("inputs").at(run.at("input").get<unsigned>());
        const auto& original=data.at("records").at(index++);setup(results,input,run.at("initialize"));
        check(digest(results)==original.at("before_sha256").get<std::string>(),"Declared full retained-context input differs");
        SourceResultLivePlayers live;SourceResultMachineRoster roster;
        for(unsigned i=0;i<16;++i) {
          const auto& row=input.at("live").at(i);live[i].salt=row.at("salt");live[i].machine=row.at("machine");
          live[i].identity=record<12>(row.at("identity"));live[i].configuration=record<144>(row.at("configuration"));
          roster[i]=record<6>(input.at("roster").at(i));
        }
        if(run.at("operation")=="remap")source_result_remap(results,{input.at("recording")[0],input.at("recording")[1]},
            live,input.at("high_water"),roster,policy);
        else source_result_bind_machines(results,live,input.at("high_water"),roster,policy);
        const auto after=digest(results);
        if(after!=original.at("after_sha256").get<std::string>())throw std::runtime_error("Complete retained context differs: "+name+" run"+std::to_string(index-1));
        outputs.at(version).push_back(after);++count;changed+=original.at("changed_bytes").get<unsigned>()!=0;
      }
      ++version;
    }
    check(version==3 && outputs[0]==outputs[1] && outputs[1]!=outputs[2],
        "Original retail/TU1.1 machine pruning and U5 preservation distinction lost");
    std::cout<<"PASS "<<count<<" whole original retained remap/machine calls; "<<changed<<" mutations; SHA256 covers every context byte and repeated transitions\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
