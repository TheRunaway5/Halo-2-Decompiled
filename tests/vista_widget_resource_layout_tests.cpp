#include "halo2/vista_widget_resource_layout.hpp"
#include "halo2/vista_widget_cache.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
std::vector<std::byte> unhex(const std::string& raw) {
  const auto digit=[](char c){return c<='9'?c-'0':c-'a'+10;};std::vector<std::byte>b(raw.size()/2);
  for(std::size_t i=0;i<b.size();++i)b[i]=std::byte{std::uint8_t(digit(raw[i*2])*16+digit(raw[i*2+1]))};
  return b;
}
std::string hex(std::span<const std::byte>b) {
  constexpr char d[]="0123456789abcdef";std::string out(b.size()*2,'0');
  for(std::size_t i=0;i<b.size();++i){const auto v=std::to_integer<unsigned>(b[i]);out[2*i]=d[v>>4];out[2*i+1]=d[v&15];}return out;
}
std::uint32_t scalar(std::span<const std::byte>b) {std::uint32_t v{};for(std::size_t i=0;i<b.size();++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);return v;}
std::vector<std::byte> load(const char*path) {
  std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in||in.tellg()<0)throw FormatError("Actual Vista source cache missing");
  std::vector<std::byte>b(std::size_t(in.tellg()));in.seekg(0);if(!in.read(reinterpret_cast<char*>(b.data()),std::streamsize(b.size())))throw FormatError("Short actual Vista cache");return b;
}
struct Segment {std::uint32_t base{};std::vector<std::byte>bytes;};
}
int main(int argc,char**argv)try {
  if(argc!=3)return 77;
  std::ifstream in(argv[2]);if(!in)return 77;Json f;in>>f;
  if(f.at("schema")!="H2_VISTA_WIDGET_RESOURCE_LAYOUT_ORIGINAL_V1" || f.at("map").at("sha256")!="eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44")throw FormatError("Resource-layout source authority changed");
  const auto input=load(argv[1]);std::uint64_t fingerprint=0xcbf29ce484222325ull;
  for(const auto byte:input)fingerprint=(fingerprint^std::to_integer<std::uint8_t>(byte))*0x100000001b3ull;
  if(input.size()!=f.at("map").at("bytes").get<std::size_t>() || fingerprint!=std::stoull(f.at("map").at("fnv64").get<std::string>(),nullptr,16))throw FormatError("Actual resource-layout cache bytes changed");
  const auto cache=std::make_shared<const VistaWidgetCacheBank>(input);
  const auto obj=f.at("object").get<std::uint32_t>();std::size_t cases{},checks{},faults{};
  for(const auto&q:f.at("cases")) {
    std::vector<Segment> segments{{obj,std::vector<std::byte>(0x20000)}};
    for(const auto&a:f.at("globals"))segments.push_back({a.get<std::uint32_t>(),std::vector<std::byte>(4)});
    if(q.at("null_page").get<bool>())segments.push_back({0,std::vector<std::byte>(4096)});
    const auto writable=[&](std::uint32_t at,std::size_t n)->std::span<std::byte>{
      for(auto&g:segments)if(at>=g.base && std::uint64_t(at-g.base)+n<=g.bytes.size())return std::span<std::byte>(g.bytes).subspan(at-g.base,n);
      throw FormatError("Unmapped mutable resource-layout registry");
    };
    const auto bytes=[&](std::uint32_t at,std::size_t n)->std::span<const std::byte>{
      for(auto&g:segments)if(at>=g.base && std::uint64_t(at-g.base)+n<=g.bytes.size())return std::span<const std::byte>(g.bytes).subspan(at-g.base,n);
      if(q.at("actual").get<bool>())return cache->bytes(q.at("base").get<std::uint32_t>(),at,n);
      throw FormatError("Unmapped resource-layout registry");
    };
    const auto apply=[&](const Json& patches){for(const auto&p:patches){const auto b=unhex(p.at(1).get<std::string>());auto dest=writable(p.at(0).get<std::uint32_t>(),b.size());std::copy(b.begin(),b.end(),dest.begin());}};
    apply(q.at("patches"));Json events=Json::array(),writes=Json::array();std::size_t requests{};std::string fault;
    VistaWidgetResourceLayoutServices s;
    s.read=[&](std::uint32_t at,std::size_t n){try{return scalar(bytes(at,n));}catch(const FormatError&){events.push_back(Json::array({"memory-fault",at,n}));fault="memory";throw;}};
    s.write=[&](std::uint32_t at,std::uint32_t v,std::size_t n){auto b=writable(at,n);for(std::size_t i=0;i<n;++i)b[i]=std::byte{std::uint8_t(v>>(i*8))};writes.push_back(Json::array({at,n,v}));};
    const auto mutate=[&](const char*owner,std::size_t index){for(const auto&a:q.at("mutations"))if(a.at("owner")==owner&&a.at("index")==index)apply(a.at("patches"));};
    s.request_bitmap=[&](std::uint32_t datum){++requests;events.push_back(Json::array({"bitmap",datum,scalar(bytes(obj+0x9fa,1)),scalar(bytes(obj+0x9fe,4))}));mutate("bitmap",requests);if(q.at("fail_bitmap")==requests){fault="service-bitmap";throw FormatError(fault);}};
    s.apply_cursor_resource=[&](std::uint32_t manager,std::uint32_t resource){events.push_back(Json::array({"cursor",manager,resource,scalar(bytes(obj+0x9fa,1)),scalar(bytes(obj+0x9fe,4))}));mutate("cursor",1);if(q.at("fail_cursor").get<bool>()){fault="service-cursor";throw FormatError(fault);}};
    bool rejected=false;try{vista_widget_resource_layout(obj,s);}catch(const FormatError&){rejected=true;}
    const auto&expected=q.at("result");const bool original_fault=!expected.at("fault").is_null();
    if(original_fault!=rejected)throw FormatError("Original resource-layout completion differs: "+q.at("name").get<std::string>());
    if(rejected){++faults;const auto label=expected.at("fault").get<std::string>();if((label.starts_with("service-")&&fault!=label)||(!label.starts_with("service-")&&fault!="memory"))throw FormatError("Original resource-layout fault boundary differs");}
    if(events!=expected.at("events"))throw FormatError("Original resource provider order/arguments differ: "+q.at("name").get<std::string>());
    if(writes!=expected.at("writes"))throw FormatError("Original resource-layout write prefix differs: "+q.at("name").get<std::string>());
    for(const auto&v:expected.at("snapshots")){const auto raw=unhex(v.at(1).get<std::string>());if(hex(bytes(v.at(0).get<std::uint32_t>(),raw.size()))!=v.at(1).get<std::string>())throw FormatError("Original resource-layout bank differs: "+q.at("name").get<std::string>());++checks;}
    ++cases;checks+=3;
  }
  std::cout<<"PASS: "<<cases<<" original resource-layout cases, "<<checks<<" provider/write/bank checks, "<<faults<<" matched fault/service prefixes\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
