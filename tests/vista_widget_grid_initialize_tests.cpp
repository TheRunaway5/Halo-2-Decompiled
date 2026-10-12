#include "halo2/vista_widget_grid_initialize.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace{
std::vector<std::byte> unhex(const std::string&s){const auto digit=[](char c)->unsigned{if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid original grid bank hex");};if(s.size()%2)throw FormatError("Odd grid bank hex");std::vector<std::byte>b(s.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(std::uint8_t(16*digit(s[2*i])+digit(s[2*i+1])));return b;}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string out;for(auto c:b){const auto v=std::to_integer<unsigned>(c);out+=d[v>>4];out+=d[v&15];}return out;}
std::uint32_t get(std::span<const std::byte>b,std::size_t n){if(n>b.size())throw FormatError("Controlled grid read exceeds raw bank");std::uint32_t v{};for(std::size_t i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[i]))<<(8*i);return v;}
void put(std::span<std::byte>b,std::uint32_t v,std::size_t n=4){if(n>b.size())throw FormatError("Controlled grid write exceeds raw bank");for(std::size_t i=0;i<n;++i)b[i]=std::byte(std::uint8_t(v>>(8*i)));}
template<class F>void rejects(F&&f){try{f();}catch(const FormatError&){return;}throw FormatError("Missing genuine grid owner admitted");}
}
int main(int argc,char**argv)try{
 if(argc!=2)return 77;
 std::ifstream input(argv[1]);if(!input)return 77;Json fixture;input>>fixture;std::size_t count{},actual{},failures{},mutations{},read_count{};
 for(const auto&r:fixture.at("cases")){
  const auto&q=r.at("input");std::map<std::uint32_t,std::vector<std::byte>> regions;
  for(const auto&[key,value]:r.at("initial").items())regions.emplace(std::stoul(key),unhex(value));
  const auto view=[&](std::uint32_t address)->std::span<std::byte>{auto it=regions.upper_bound(address);if(it==regions.begin())throw FormatError("Unbound original raw grid address");--it;const auto offset=std::size_t(address-it->first);if(offset>=it->second.size())throw FormatError("Unbound original raw grid address");return std::span(it->second).subspan(offset);};
  const auto snapshot=[&]{Json b=Json::object();for(const auto&[id,bytes]:regions)b[std::to_string(id)]=hex(bytes);return b;};
  bool applied{};Json reads=Json::array(),events=Json::array();
  const auto mutate=[&](const std::string&name,std::uint32_t address){if(q.contains("mutation")&&!applied){const auto&m=q.at("mutation");if(m.at("at")==name&&(name!="read"||m.at("address")==address)){applied=true;const auto bytes=unhex(m.at("hex"));auto target=view(m.at("write"));if(bytes.size()>target.size())throw FormatError("Controlled callback write exceeds raw grid bank");std::copy(bytes.begin(),bytes.end(),target.begin());}}};
  const auto event=[&](const std::string&name,Json args){events.push_back({{"name",name},{"args",std::move(args)},{"state",snapshot()}});mutate(name,0);};
  VistaWidgetGridInitializeServices s;s.bank=[&](auto id){return view(id.value);};
  s.read=[&](auto address,auto size){const auto before=snapshot();mutate("read",address);const auto value=get(view(address),size);reads.push_back({{"address",address},{"size",size},{"value",value},{"state",before}});return value;};
  s.skin_children=[&](auto id,auto point,auto skin){event("skin_children",Json::array({id.value,hex(point),skin}));put(view(id.value+0x14),q.value("child_identity",0x1a01000u));put(view(id.value+0x6e),0x97,1);if(q.value("fail",Json())=="skin_children")throw FormatError("Declared original skin provider failure");};
  s.child_bounds=[&](auto id){event("child_bounds",Json::array({id.value}));put(view(id.value+0x30),0x12345678);if(q.value("fail",Json())=="child_bounds")throw FormatError("Declared original bounds provider failure");const auto bytes=unhex(q.value("bounds",std::string("0080007fff7f0080")));VistaWidgetGridBounds result;std::copy(bytes.begin(),bytes.end(),result.begin());return result;};
  auto position=view(r.at("position"));bool failed{};
  try{vista_widget_grid_initialize({0x1a00000},std::span<const std::byte,4>(position.data(),4),s);}catch(const FormatError&){if(r.at("failure").is_null())throw;failed=true;}
  if(reads!=r.at("reads")||events!=r.at("events")||snapshot()!=r.at("final")||applied!=r.at("applied_mutation")||failed!=!r.at("failure").is_null()){
   std::cerr<<"grid case "<<count<<" read order "<<(reads==r.at("reads"))<<" services "<<(events==r.at("events"))<<" bank "<<(snapshot()==r.at("final"))<<'\n';
   if(reads!=r.at("reads"))for(std::size_t i=0;i<std::max(reads.size(),r.at("reads").size());++i)if(i>=reads.size()||i>=r.at("reads").size()||reads[i]!=r.at("reads")[i]){std::cerr<<"first read "<<i<<" native "<<(i<reads.size()?reads[i].dump():"NONE")<<" original "<<(i<r.at("reads").size()?r.at("reads")[i].dump():"NONE")<<'\n';break;}
   throw FormatError("Original620493 grid differential mismatch");
  }
  ++count;if(!r.at("actual").is_null())++actual;if(failed)++failures;if(applied)++mutations;read_count+=reads.size();
 }
 VistaWidgetGridInitializeServices missing;std::array<std::byte,4> position{};
 rejects([&]{vista_widget_grid_initialize({1},position,missing);});
 std::array<std::byte,0x80> bank{};missing.bank=[&](auto){return std::span(bank);};
 rejects([&]{vista_widget_grid_initialize({1},position,missing);});
 std::size_t reads{};missing.read=[&](std::uint32_t,std::size_t){++reads;return 0xffffffffu;};
 vista_widget_grid_initialize({1},position,missing);if(reads!=1)throw FormatError("NONE grid skin reached unconsumed raw/service owners");
 // A valid raw record requires both real lower owners; neither can default.
 missing.read=[](std::uint32_t address,std::size_t){return address==0x12||address==0x13?1u:0u;};
 rejects([&]{vista_widget_grid_initialize({1},position,missing);});
 missing.skin_children=[](auto,auto,auto){};
 rejects([&]{vista_widget_grid_initialize({1},position,missing);});
 std::cout<<"PASS "<<count<<" original620493 cases, "<<actual<<" actual cache records, "<<read_count<<" exact raw reads, "<<failures<<" partial provider failures, "<<mutations<<" callback mutations\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
