#include "halo2/vista_widget_bitmap_bounds.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
using namespace halo2;
using Json=nlohmann::json;
namespace {
std::vector<std::byte> unhex(const std::string& s) {
  const auto digit=[](char c)->unsigned {if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid original bitmap bounds bank hex");};
  if(s.size()%2)throw FormatError("Odd bitmap bounds bank hex");
  std::vector<std::byte> b(s.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(std::uint8_t(16*digit(s[2*i])+digit(s[2*i+1])));return b;
}
std::string hex(std::span<const std::byte> b) {constexpr char d[]="0123456789abcdef";std::string out;for(auto c:b){const auto v=std::to_integer<unsigned>(c);out+=d[v>>4];out+=d[v&15];}return out;}
template<class F>void rejects(F&& f) {try{f();}catch(const FormatError&){return;}throw FormatError("Missing genuine bitmap bounds dependency admitted");}
#if defined(__i386__) || defined(__x86_64__)
struct FloatContext {
  std::uint32_t mxcsr{_mm_getcsr()};std::uint16_t control{};
  FloatContext(std::uint32_t mx,std::uint16_t cw) {asm volatile("fnstcw %0":"=m"(control));_mm_setcsr(mx);asm volatile("fldcw %0"::"m"(cw));}
  ~FloatContext(){_mm_setcsr(mxcsr);asm volatile("fldcw %0"::"m"(control));}
};
#endif
}
int main(int argc,char** argv)try {
#if defined(__i386__) || defined(__x86_64__)
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);if(!input)return 77;
  Json fixture;input>>fixture;std::size_t count{},actual{},failures{},reads{},writes{};
  for(const auto& row:fixture.at("cases")) {
    const auto& q=row.at("input");std::map<std::uint32_t,std::vector<std::byte>> regions;
    for(const auto& [key,value]:row.at("initial").items())regions.emplace(std::stoul(key),unhex(value));
    Json trace=Json::array();
    const auto view=[&](std::uint32_t at,std::size_t n,const char* access)->std::span<std::byte> {
      auto it=regions.upper_bound(at);
      if(it!=regions.begin()) {--it;const auto offset=std::size_t(at-it->first);if(offset<=it->second.size()&&n<=it->second.size()-offset)return std::span(it->second).subspan(offset,n);}
      trace.push_back(Json::array({"fault",access,at,n}));throw FormatError("Unbound original bitmap bounds address");
    };
    VistaWidgetBoundsServices s;
    s.read=[&](std::uint32_t at,std::size_t n) {const auto bytes=view(at,n,"r");std::uint32_t v{};for(std::size_t i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned>(bytes[i]))<<(8*i);trace.push_back(Json::array({"r",at,n,v}));return v;};
    s.write=[&](std::uint32_t at,std::uint32_t v,std::size_t n) {if(n==2)v&=0xffffu;trace.push_back(Json::array({"w",at,n,v}));auto bytes=view(at,n,"w");for(std::size_t i=0;i<n;++i)bytes[i]=std::byte(std::uint8_t(v>>(i*8)));};
    bool failed{};std::uint32_t returned{},mxcsr{};std::uint16_t control{};
    {
      FloatContext context(q.at("mxcsr"),q.at("fpcw"));
      try{returned=vista_widget_bitmap_bounds(0x1a00000,q.at("output"),s);}catch(const FormatError&){if(row.at("fault").is_null())throw;failed=true;}
      mxcsr=_mm_getcsr();asm volatile("fnstcw %0":"=m"(control));
    }
    Json final=Json::object();for(const auto& [id,bytes]:regions)final[std::to_string(id)]=hex(bytes);
    if(trace!=row.at("trace")||final!=row.at("final")||failed!=!row.at("fault").is_null()||(!failed&&returned!=row.at("returned"))||mxcsr!=row.at("mxcsr")||control!=row.at("fpcw")) {
      std::cerr<<"bitmap bounds case "<<count<<" trace "<<(trace==row.at("trace"))<<" bank "<<(final==row.at("final"))<<" fault "<<failed<<'\n';
      if(trace!=row.at("trace"))for(std::size_t i=0;i<std::max(trace.size(),row.at("trace").size());++i)if(i>=trace.size()||i>=row.at("trace").size()||trace[i]!=row.at("trace")[i]){std::cerr<<"first access "<<i<<" native "<<(i<trace.size()?trace[i].dump():"NONE")<<" original "<<(i<row.at("trace").size()?row.at("trace")[i].dump():"NONE")<<'\n';break;}
      throw FormatError("Original61C364 bitmap bounds differential mismatch");
    }
    ++count;if(!q.at("actual").is_null())++actual;if(failed)++failures;
    for(const auto& access:trace){if(access[0]=="r")++reads;if(access[0]=="w")++writes;}
  }
  VistaWidgetBoundsServices missing;
  rejects([&]{vista_widget_bitmap_bounds(1,2,missing);});
  missing.read=[](auto,auto){return 0u;};rejects([&]{vista_widget_bitmap_bounds(1,2,missing);});
  std::size_t consumed{};missing.read=[&](auto,auto){++consumed;return 0u;};missing.write=[&](auto,auto,auto){++consumed;};
  {FloatContext context(0x1f00,0x37f);rejects([&]{vista_widget_bitmap_bounds(1,2,missing);});}
  if(consumed)throw FormatError("Unmasked FP context consumed bitmap storage");
  std::cout<<"PASS "<<count<<" original61C364 cases, "<<actual<<" actual frame/scale inputs, "<<reads<<" exact raw reads, "<<writes<<" exact raw writes, "<<failures<<" partial output fault prefixes\n";return 0;
#else
  (void)argc;(void)argv;return 77;
#endif
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
