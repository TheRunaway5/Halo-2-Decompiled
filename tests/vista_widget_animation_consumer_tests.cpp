#include "halo2/vista_widget_animation_consumer.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>
#include <xmmintrin.h>
using namespace halo2;
using Json=nlohmann::json;
namespace {
std::vector<std::byte> unhex(const std::string& raw) {
  std::vector<std::byte> b(raw.size()/2);
  const auto digit=[](char c){return c<='9'?c-'0':c-'a'+10;};
  for(std::size_t i=0;i<b.size();++i)b[i]=std::byte{std::uint8_t(digit(raw[i*2])*16+digit(raw[i*2+1]))};
  return b;
}
std::string hex(std::span<const std::byte> b) {
  constexpr char d[]="0123456789abcdef";std::string out(b.size()*2,'0');
  for(std::size_t i=0;i<b.size();++i){const auto v=std::to_integer<unsigned>(b[i]);out[i*2]=d[v>>4];out[i*2+1]=d[v&15];}
  return out;
}
}
int main(int argc,char**argv) try {
  if(argc!=2)return 77;
  std::ifstream in(argv[1]);if(!in)return 77;Json f;in>>f;
  if(f.at("schema")!="H2_VISTA_WIDGET_ANIMATION_CONSUMER_ORIGINAL_V1")throw FormatError("Animation consumer fixture changed");
  const auto obj=f.at("object").get<std::uint32_t>(),frames=f.at("frames").get<std::uint32_t>();
  std::size_t cases{},faults{},checks{};
  const auto saved=_mm_getcsr();
  for(const auto& c:f.at("cases")) {
    std::vector<std::byte> memory(0x20000);auto bank=unhex(c.at("bank").get<std::string>()),keys=unhex(c.at("frames").get<std::string>());
    std::copy(bank.begin(),bank.end(),memory.begin());std::copy(keys.begin(),keys.end(),memory.begin()+frames-obj);
    const auto slot=[&](std::uint32_t at,std::size_t n){
      if(at<obj || std::uint64_t(at-obj)+n>memory.size())throw FormatError("Unmapped consumer test registry");
      return std::span<std::byte>(memory).subspan(at-obj,n);
    };
    Json actions=Json::array(),writes=Json::array();std::size_t clocks{};bool callback=false;VistaWidgetAnimationConsumerServices s;
    s.read=[&](std::uint32_t at,std::size_t n){
      if(at==0xd71900u && n==4){++clocks;return c.at("clock").get<std::uint32_t>();}
      std::uint32_t v{};const auto b=slot(at,n);for(std::size_t i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);return v;
    };
    s.write=[&](std::uint32_t at,std::uint32_t v,std::size_t n){auto b=slot(at,n);for(std::size_t i=0;i<n;++i)b[i]=std::byte{std::uint8_t(v>>(i*8))};if(!callback)writes.push_back(Json::array({at,n,v& (n==4?0xffffffffu:((1u<<(n*8))-1))}));};
    s.activate=[&](std::uint32_t at,std::uint32_t reason){actions.push_back(Json::array({"activate",at,reason,hex(slot(obj,0x78))}));callback=true;if(c.at("mutate_callback").get<bool>())s.write(obj+0x64,0x3e800000u,4);callback=false;};
    s.dismiss=[&](std::uint32_t at){actions.push_back(Json::array({"dismiss",at,hex(slot(obj,0x78))}));};
    s.update_child=[&](std::uint32_t child){actions.push_back(Json::array({"update",child,hex(slot(obj,0x78))}));if(c.at("update_next").get<bool>()&&child==obj+0x500){callback=true;s.write(child+0x18,obj+0x700,4);callback=false;}};
    for(const auto& a:c.at("ancestors")){callback=true;s.write(a.at(0).get<std::uint32_t>()+0x10,a.at(1).get<std::uint32_t>(),4);callback=false;}
    if(c.at("ambient").get<bool>())for(unsigned i=0;i<3;++i){callback=true;s.write(obj+0x500+i*0x100+0x18,i+1<c.at("child_count").get<unsigned>()?obj+0x600+i*0x100:0,4);callback=false;}
    _mm_setcsr(c.at("mxcsr").get<unsigned>());
    bool rejected=false;
    try{if(c.at("ambient").get<bool>())vista_widget_animation_ambient_update(obj,s);else vista_widget_animation_update(obj,c.at("now").get<std::uint32_t>(),s);}catch(const FormatError&){rejected=true;}
    const auto mxcsr=_mm_getcsr();_mm_setcsr(saved);
    if(!c.at("result").at("fault").is_null()) {
      if(!rejected)throw FormatError("Original animation fault was normalized in case "+std::to_string(cases+faults));
      if(c.at("result").at("fault")!="instruction-budget" && hex(slot(obj,0x78))!=c.at("result").at("bank").get<std::string>())throw FormatError("Original animation fault partial writes differ");
      ++faults;++checks;continue;
    }
    if(rejected)throw FormatError("Original finite animation was rejected in case "+std::to_string(cases+faults));
    if(hex(slot(obj,0x78))!=c.at("result").at("bank").get<std::string>())throw FormatError("Original animation bank differs in case "+std::to_string(cases+faults));
    if(actions!=c.at("result").at("actions"))throw FormatError("Original animation callbacks differ in case "+std::to_string(cases+faults));
    if(writes!=c.at("result").at("writes"))throw FormatError("Original animation write sequence differs in case "+std::to_string(cases+faults));
    if(clocks!=c.at("result").at("clocks").get<std::size_t>())throw FormatError("Original animation cached-clock order/count differs");
    // Unicorn does not model sticky SSE exception status bits. Control bits and
    // exact output bytes are checked; status-bit parity remains unproved.
    if((mxcsr&~63u)!=(c.at("result").at("mxcsr_out").get<unsigned>()&~63u))throw FormatError("Original animation MXCSR controls differ in case "+std::to_string(cases+faults));
    ++cases;checks+=5;
  }
  if(cases!=f.at("summary").at("complete").get<std::size_t>())throw FormatError("Animation fixture complete count changed");
  std::cout<<"PASS: "<<cases<<" original finite updates, "<<checks<<" bank/write/callback/MXCSR-control checks; "<<faults<<" original fault/limit admissions rejected explicitly\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
