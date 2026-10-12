#include "halo2/vista_widget_child_bounds.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
std::vector<std::byte> unhex(const std::string& s) {
  const auto digit=[](char c)->unsigned {if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid original bounds bank hex");};
  if(s.size()%2)throw FormatError("Odd bounds bank hex");
  std::vector<std::byte> b(s.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(std::uint8_t(16*digit(s[2*i])+digit(s[2*i+1])));return b;
}
std::string hex(std::span<const std::byte> b) {constexpr char d[]="0123456789abcdef";std::string out;for(auto c:b){const auto v=std::to_integer<unsigned>(c);out+=d[v>>4];out+=d[v&15];}return out;}
std::uint32_t get(std::span<const std::byte> b,std::size_t n) {if(n>b.size())throw FormatError("Controlled bounds read exceeds raw bank");std::uint32_t v{};for(std::size_t i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[i]))<<(8*i);return v;}
void put(std::span<std::byte> b,std::uint32_t v,std::size_t n=4) {if(n>b.size())throw FormatError("Controlled bounds write exceeds raw bank");for(std::size_t i=0;i<n;++i)b[i]=std::byte(std::uint8_t(v>>(8*i)));}
template<class F>void rejects(F&& f) {try{f();}catch(const FormatError&){return;}throw FormatError("Missing genuine child bounds owner admitted");}
}
int main(int argc,char** argv)try {
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);if(!input)return 77;
  Json fixture;input>>fixture;std::size_t count{},actual{},failures{},mutations{},read_count{},bounds_count{};
  for(const auto& r:fixture.at("cases")) {
    const auto& q=r.at("input");std::map<std::uint32_t,std::vector<std::byte>> regions;
    for(const auto& [key,value]:r.at("initial").items())regions.emplace(std::stoul(key),unhex(value));
    const auto view=[&](std::uint32_t address)->std::span<std::byte> {auto it=regions.upper_bound(address);if(it==regions.begin())throw FormatError("Unbound original child bounds address");--it;const auto offset=std::size_t(address-it->first);if(offset>=it->second.size())throw FormatError("Unbound original child bounds address");return std::span(it->second).subspan(offset);};
    const auto snapshot=[&] {Json b=Json::object();for(const auto& [id,bytes]:regions)b[std::to_string(id)]=hex(bytes);return b;};
    bool applied{};Json reads=Json::array(),events=Json::array();
    const auto mutate=[&](const std::string& name,std::uint32_t address) {if(q.contains("mutation")&&!applied){const auto& m=q.at("mutation");if(m.at("at")==name&&(name!="read"||m.at("address")==address)){applied=true;const auto bytes=unhex(m.at("hex"));auto target=view(m.at("write"));if(bytes.size()>target.size())throw FormatError("Controlled callback exceeds raw bounds bank");std::copy(bytes.begin(),bytes.end(),target.begin());}}};
    VistaWidgetChildBoundsServices s;s.bank=[&](auto id){return view(id.value);};
    s.read=[&](auto address,auto size) {const auto before=snapshot();mutate("read",address);const auto value=get(view(address),size);reads.push_back({{"address",address},{"size",size},{"value",value},{"state",before}});return value;};
    s.bounds=[&](auto id) {
      events.push_back({{"name","bounds"},{"args",Json::array({id.value})},{"state",snapshot()}});mutate("bounds",0);
      if(q.value("fail_identity",0u)==id.value){put(view(id.value+0x30),0x12345678);throw FormatError("Declared original bounds provider failure");}
      VistaWidgetChildBounds result{};
      if(q.value("getter",false)) {
        // Genuine6116C0 reads the first DWORD, copies it, then reads/copies the second.
        put(std::span(result),get(view(id.value+0x20),4));put(std::span(result).subspan(4),get(view(id.value+0x24),4));
      } else {
        const std::size_t index=(id.value-0x1a01000u)/0x100u;
        const auto rect=q.value("bounds",Json::array({Json::array({300,-400,-200,400}),Json::array({300,-400,-200,400}),Json::array({300,-400,-200,400}),Json::array({300,-400,-200,400})})).at(index);
        for(std::size_t i=0;i<4;++i)put(std::span(result).subspan(i*2),std::uint32_t(rect.at(i).get<std::int32_t>()),2);
      }
      return result;
    };
    auto output=view(r.at("output"));bool failed{};
    try{vista_widget_child_bounds({0x1a00000},std::span<std::byte,8>(output.data(),8),s);}catch(const FormatError&){if(r.at("failure").is_null())throw;failed=true;}
    if(reads!=r.at("reads")||events!=r.at("events")||snapshot()!=r.at("final")||applied!=r.at("mutation")||failed!=!r.at("failure").is_null()) {
      std::cerr<<"bounds case "<<count<<" reads "<<(reads==r.at("reads"))<<" services "<<(events==r.at("events"))<<" bank "<<(snapshot()==r.at("final"))<<'\n';
      throw FormatError("Original6425BF child bounds differential mismatch");
    }
    ++count;if(q.contains("actual"))++actual;if(failed)++failures;if(applied)++mutations;read_count+=reads.size();bounds_count+=events.size();
  }
  VistaWidgetChildBoundsServices missing;std::array<std::byte,8> output{};
  rejects([&]{vista_widget_child_bounds({1},output,missing);});
  std::array<std::byte,0x80> bank{};missing.bank=[&](auto){return std::span(bank);};
  vista_widget_child_bounds({1},output,missing);
  if(hex(output)!="0080ff7fff7f0080")throw FormatError("Empty child sentinel changed");
  put(std::span(bank).subspan(0x14),1);put(std::span(bank).subspan(4),8);put(std::span(bank).subspan(0x70),2);
  rejects([&]{vista_widget_child_bounds({1},output,missing);});
  missing.read=[](auto,auto){return 0u;};
  rejects([&]{vista_widget_child_bounds({1},output,missing);});
  put(std::span(bank).subspan(0x14),0);put(std::span(bank).subspan(0x70),0);missing.read={};
  if(vista_widget_bitmap_bounds_excluded({1},missing)||vista_widget_kind9_bounds_excluded({1},missing))throw FormatError("Null raw record reached unconsumed flags");
  std::cout<<"PASS "<<count<<" original6425BF cases, "<<actual<<" actual PC14 skin flag records, "<<read_count<<" exact flag reads, "<<bounds_count<<" exact bounds calls, "<<failures<<" partial provider/fault prefixes, "<<mutations<<" callback mutations\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
