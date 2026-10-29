#include "halo2/native_campaign_ai_query_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>

namespace {
using namespace halo2;
using Json=nlohmann::json;
static_assert(unsigned(NativeCampaignAiQuery::living_count)==0 && unsigned(NativeCampaignAiQuery::swarm_count)==1 &&
  unsigned(NativeCampaignAiQuery::nonswarm_count)==2 && unsigned(NativeCampaignAiQuery::strength)==3 &&
  unsigned(NativeCampaignAiQuery::fighting_count)==4 && unsigned(NativeCampaignAiQuery::living_fraction)==5);
void check(bool v,const std::string &message) {if(!v)throw std::runtime_error(message);}
class Memory final:public SourcePhysicsContactMemory {
public:
  std::map<std::uint32_t,std::byte> bytes;
  mutable Json reads=Json::array();
  int fault_at=-1;
  bool reject_all_writes=true;
  void read(std::uint32_t address,std::span<std::byte> out)const override {
    constexpr char digits[]="0123456789abcdef";
    std::string hex;
    for(unsigned i=0;i<out.size();++i) {
      const auto at=bytes.find(address+i);
      if(at==bytes.end())throw std::runtime_error("Missing declared native query input");
      out[i]=at->second;
      const auto value=std::to_integer<unsigned>(out[i]);
      hex+=digits[value>>4];hex+=digits[value&15u];
    }
    reads.push_back(Json::array({address,out.size(),hex}));
    if(int(reads.size())-1==fault_at)throw std::runtime_error("Declared original read fault");
  }
  void write(std::uint32_t,std::span<const std::byte>)override {
    throw std::runtime_error("Read-only campaign query mutated the source world");
  }
};
Memory declared(const Json &fixture,const Json &input) {
  Memory result;
  for(const auto &region:fixture.at("query_regions"))
    for(unsigned i=0;i<region[1].get<unsigned>();++i)
      result.bytes.emplace(region[0].get<unsigned>()+i,std::byte{});
  for(const auto &write:input.at("writes"))
    for(unsigned i=0;i<write[1].get<unsigned>();++i)
      result.bytes.at(write[0].get<unsigned>()+i)=std::byte(write[2].get<unsigned>()>>(i*8));
  if(!input.at("fault_at").is_null())result.fault_at=input.at("fault_at");
  return result;
}
void replay(const Json &fixture,const Json &input) {
  auto memory=declared(fixture,input);
  const auto before=memory.bytes;
  NativeCampaignAiQueryFp fp{input.value("initial_mxcsr",0x1f80u)};
  _mm_setcsr(0xbfc0u);
  bool failed=false;
  std::uint32_t result{};
  const auto name_of_query=input.value("query",std::string("living_count"));
  const auto builtin=native_campaign_ai_query_builtin_tu15("ai_"+name_of_query);
  check(builtin.has_value(),"Original query name has no native script binding");
  const auto query=builtin->operation;
  if(fixture.contains("function_bindings"))
    check(builtin->return_type==fixture.at("function_bindings").at("ai_"+name_of_query).at("return_type"),"Native script result type differs from original evaluator definition");
  try {
    if(query==NativeCampaignAiQuery::living_count)result=native_campaign_ai_living_count_tu15(memory,input.at("reference"),fp);
    else result=native_campaign_ai_query_tu15(memory,input.at("reference"),query,fp);
  }
  catch(const std::exception &) {failed=true;}
  const auto host_mxcsr=_mm_getcsr();
  const auto name=name_of_query+":"+input.at("name").get<std::string>();
  check(failed==!input.at("fault_at").is_null(),name+": native failure boundary differs");
  check(memory.reads==input.at("reads"),name+": ordered source reads differ: "+memory.reads.dump());
  check(fp.mxcsr==input.at("mxcsr"),name+": floating-point status differs");
  check(host_mxcsr==0xbfc0u,name+": caller SSE policy/status changed");
  if(!failed)check(result==input.at("result"),name+(query==NativeCampaignAiQuery::living_fraction?": REAL living fraction differs":": signed SHORT count differs"));
  check(memory.bytes==before,name+": query changed retained backing");
}
}
int main(int argc,char **argv) {try {
  check(argc==2,"Expected independently executed original fixture");
  Json fixture;std::ifstream(argv[1])>>fixture;
  check(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong TU1.5 authority");
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_dependency_returns").empty(),"Original outcomes were supplied");
  check(fixture.at("required_backend")=="unchanged-tu15-physical-i386-count-fp+unicorn-ordered-read-provenance" ||
        fixture.at("required_backend")=="unchanged-tu15-physical-i386-fraction-fp+unicorn-ordered-read-provenance","Original query FP was not physically observed");
  if(fixture.at("required_backend")=="unchanged-tu15-physical-i386-fraction-fp+unicorn-ordered-read-provenance") {
    const auto &binding=fixture.at("function_bindings").at("ai_living_fraction");
    check(binding.at("opcode")==327 && binding.at("evaluator")==0x2af700 && binding.at("return_type")==6,"Wrong original living-fraction evaluator");
    check(std::any_of(fixture.at("cases").begin(),fixture.at("cases").end(),[](const auto &row){return row.at("name").template get<std::string>().starts_with("fraction-full32-rounding-") && row.at("direct_full32_living").template get<std::uint32_t>()>0x1000000u;}),"Original full signed32 numerator/rounding evidence absent");
  }
  for(const auto &input:fixture.at("cases"))replay(fixture,input);
  // A deliberate vector-size replacement must fail the retained squad truth:
  // this published squad has two linked actors and a living counter of three.
  const auto control=std::find_if(fixture.at("cases").begin(),fixture.at("cases").end(),[](const auto &c){return c.at("name")=="reference-00000000" && c.at("fault_at").is_null();});
  check(control!=fixture.at("cases").end() && control->at("result")!=2,"Living-count mutation control lost its original counter distinction");
  _mm_setcsr(0x1f80u);
  std::cout<<"PASS "<<fixture.at("cases").size()<<" original count/evaluator cases and read-fault prefixes; ordered reads, source immutability, FP status and counter mutation control\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
