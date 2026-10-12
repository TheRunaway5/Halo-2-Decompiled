#include "halo2/object_preparation_pressure.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

using namespace halo2;
using Json=nlohmann::json;
namespace {
using Bytes=std::vector<std::byte>;
std::uint32_t hex(const std::string&s){return std::stoul(s,nullptr,16);}
void require(bool value,const std::string&message){if(!value)throw std::runtime_error(message);}
Bytes decode(const std::string&s){require(!(s.size()%2),"odd raw buffer");Bytes out(s.size()/2);for(std::size_t i=0;i<out.size();++i)out[i]=std::byte(hex(s.substr(i*2,2)));return out;}
std::uint32_t word(const std::string&s){auto b=decode(s);require(b.size()==4,"operand width");std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;}
struct Region {std::uint32_t address;Bytes raw;};
struct Memory {
 std::vector<Region> regions;
 explicit Memory(const Json&rows){for(const auto&r:rows)regions.push_back({hex(r.at("address")),decode(r.at("before"))});}
 std::span<std::byte> memory(std::uint32_t a,std::size_t n){for(auto&r:regions)if(a>=r.address&&std::uint64_t(a)+n<=std::uint64_t(r.address)+r.raw.size())return std::span(r.raw).subspan(a-r.address,n);throw std::out_of_range("missing exact backing at "+std::to_string(a)+" width "+std::to_string(n));}
 std::span<const std::byte> read(std::uint32_t a,std::size_t n){return memory(a,n);}
 void compare(const Json&rows,const char*field,const std::string&where){for(const auto&r:rows){auto expected=decode(r.at(field));auto actual=read(hex(r.at("address")),expected.size());require(std::equal(expected.begin(),expected.end(),actual.begin()),where+" buffer "+r.at("address").get<std::string>());}}
 void apply(const Json&rows){for(const auto&r:rows){auto raw=decode(r.at("raw"));auto target=memory(hex(r.at("address")),raw.size());std::copy(raw.begin(),raw.end(),target.begin());}}
};
}
int main(int argc,char**argv){try {
 require(argc==2,"usage: object_preparation_pressure_tests ORIGINAL_REPORT");if(!std::filesystem::exists(argv[1]))return 77;
 Json report;std::ifstream(argv[1])>>report;std::array<std::uint32_t,6>words{};for(unsigned i=0;i<words.size();++i)words[i]=word(report.at("cases").at(0).at("operands").at(i));
 SourceObjectPressureOperands operands{words[0],words[1],words[2],words[3],words[4],words[5]};
 std::size_t trials=0,callbacks=0,stores=0,whole=0,natural=0;
 for(const auto*group:{&report.at("natural"),&report.at("cases")})for(const auto&t:*group){
  const auto name=t.at("name").get<std::string>();Memory memory(t.at("regions"));std::array<std::uint32_t,8>s{};for(unsigned i=0;i<s.size();++i)s[i]=hex(t.at("slots").at(i));
  const SourceObjectPressureState state{s[1],s[2],s[3],s[4],s[5]};const SourceHeapPressureState heaps{s[6],s[4],s[7]};
  ObjectConstructionFloatingPointContext fp{hex(t.at("mxcsr"))};SourceObjectPreparationPressureServices services;
  services.read=[&](auto a,auto n){return memory.read(a,n);};services.memory=[&](auto a,auto n){return memory.memory(a,n);};
  std::vector<ObjectConstructionStore>observed;auto observe=[&](const auto&v){observed.push_back(v);};std::size_t cursor=0;
  const auto&lower=t.at("lower_calls");
  services.invoke=[&](const SourceObjectPressureCall&call,ObjectConstructionFloatingPointContext&context)->std::uint32_t{
   if(lower.empty()&&call.entry==0x148740)return source_object_heap_pressure(heaps,operands,services,context);
   if(call.entry==0x148510){collect_source_object_memory(state,services,context,observe);return 0;}
   require(cursor<lower.size(),name+" unexpected lower owner "+std::to_string(call.entry));const auto&e=lower.at(cursor++);
   require(call.entry==hex(e.at("entry"))&&call.instruction==hex(e.at("instruction")),name+" callback identity "+std::to_string(cursor));
   if(call.ecx_input)require(call.ecx==hex(e.at("ecx")),name+" callback ECX");
   if(call.al_input)require(call.al==hex(e.at("al")),name+" callback AL");
   require(call.count==e.at("arguments").size(),name+" callback argument count");for(unsigned i=0;i<call.count;++i)require((call.arguments[i]&call.argument_masks[i])==(hex(e.at("arguments").at(i))&call.argument_masks[i]),name+" defined callback argument");
   memory.compare(e.at("before"),"raw",name+" callback before");memory.apply(e.at("after"));return hex(e.at("return_eax"));
  };
  // Unicorn records do not reliably model SSE sticky exception status.
  // Keep literal observed records; compare control bits and actual results,
  // while native x86 owns its real status and restores the caller environment.
#if defined(__i386__) || defined(__x86_64__)
  auto host_mxcsr=_mm_getcsr();
#endif
  auto entry=hex(t.at("entry")),input=hex(t.at("input"));std::uint32_t result=0;
  if(entry==0xddac0)prepare_source_biped_request({s[0]},input,services,fp,observe);
  else if(entry==0x148360)make_source_object_memory_available(state,services,fp,observe);
  else if(entry==0x148510)collect_source_object_memory(state,services,fp,observe);
  else if(entry==0x148740){result=source_object_heap_pressure(heaps,operands,services,fp);require(result==hex(t.at("return_eax")),name+" pressure result");}
  else if(entry==0x22ecb0){result=source_object_pressure_statistics(input,operands,services,fp);require(result==hex(t.at("return_xmm0")),name+" statistic XMM0");}
  else throw std::runtime_error("unsupported fixture entry");
#if defined(__i386__) || defined(__x86_64__)
  require(_mm_getcsr()==host_mxcsr,name+" host MXCSR restoration");
#endif
  require((fp.mxcsr&~63u)==(hex(t.at("return_mxcsr"))&~63u),name+" returned MXCSR actual "+std::to_string(fp.mxcsr));require(cursor==lower.size(),name+" omitted lower owner");memory.compare(t.at("mutable"),"after",name+" final");
  const auto&expected=t.at("stores");require(observed.size()==expected.size(),name+" store count actual "+std::to_string(observed.size())+" expected "+std::to_string(expected.size()));
  for(unsigned i=0;i<observed.size();++i){const auto&e=expected.at(i);const auto&v=observed[i];require(v.instruction==hex(e.at("instruction"))&&v.address==hex(e.at("address"))&&v.bytes==e.at("bytes").get<unsigned>(),name+" store identity "+std::to_string(i));auto b=decode(e.at("before")),a=decode(e.at("after"));std::uint32_t before=0,after=0;for(unsigned j=0;j<v.bytes;++j){before|=std::to_integer<std::uint32_t>(b[j])<<(8*j);after|=std::to_integer<std::uint32_t>(a[j])<<(8*j);}require(v.before==before&&v.after==after,name+" ordered store values");}
  callbacks+=cursor;stores+=observed.size();++trials;if(group==&report.at("natural"))++natural;for(const auto&r:t.at("mutable"))whole+=r.at("after").get_ref<const std::string&>().size()/2;
 }
 // The source's absent lower boundary must retain its actual prefix; a host
 // exception does not silently clear the busy byte or complete collection.
 const auto&high=*std::find_if(report.at("cases").begin(),report.at("cases").end(),[](const auto&v){return v.at("name")=="high_failed"&&v.at("image")==1;});
 Memory m(high.at("regions"));auto slots=high.at("slots");SourceObjectPressureState state{hex(slots[1]),hex(slots[2]),hex(slots[3]),hex(slots[4]),hex(slots[5])};SourceObjectPreparationPressureServices services;services.read=[&](auto a,auto n){return m.read(a,n);};services.memory=[&](auto a,auto n){return m.memory(a,n);};services.invoke=[](const auto&call,auto&)->std::uint32_t{if(call.entry==0x148740)return 3;throw std::runtime_error("required world owner unavailable");};ObjectConstructionFloatingPointContext fp{0x1f80};bool stopped=false;
 try{make_source_object_memory_available(state,services,fp);}catch(const std::runtime_error&){stopped=true;}require(stopped,"missing lower owner must stop");require(std::to_integer<unsigned>(m.read(state.busy_byte,1)[0])==1,"interrupted source busy prefix preserved");
 services.read=[](auto,auto)->std::span<const std::byte>{return {};};stopped=false;try{source_object_pressure_statistics(1,operands,services,fp);}catch(const std::out_of_range&){stopped=true;}require(stopped,"truncated statistic backing stops");
 std::cout<<"PASS "<<natural<<" owning-scope entries (40 natural,15 declared preparation controls) + "<<trials-natural<<" paired original controls, "<<callbacks<<" explicit test-only lower calls, "<<stores<<" ordered stores, "<<whole<<" retained output bytes; no live reclamation/Unit readiness claim\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
