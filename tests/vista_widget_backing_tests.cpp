#include "halo2/vista_widget_backing.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
std::vector<std::byte> unhex(const std::string& text) {
  const auto digit=[](char c)->unsigned {if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid backing fixture hex");};
  if(text.size()%2)throw FormatError("Odd backing fixture hex");
  std::vector<std::byte> bytes(text.size()/2);
  for(std::size_t i=0;i<bytes.size();++i)bytes[i]=std::byte(std::uint8_t(16*digit(text[2*i])+digit(text[2*i+1])));
  return bytes;
}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string text;
  for(const auto b:bytes){const auto v=std::to_integer<unsigned>(b);text+=digits[v>>4];text+=digits[v&15];}return text;
}
std::uint32_t get(std::span<const std::byte> bytes) {std::uint32_t v{};for(std::size_t i=0;i<bytes.size();++i)v|=std::uint32_t(std::to_integer<unsigned>(bytes[i]))<<(8*i);return v;}
void put(std::span<std::byte> bytes,std::uint32_t v) {for(std::size_t i=0;i<bytes.size();++i)bytes[i]=std::byte(std::uint8_t(v>>(8*i)));}
template<class F>void rejects(F&& f){try{f();}catch(const FormatError&){return;}throw FormatError("Unbound aligned-backing service admitted");}
} //namespace
int main(int argc,char** argv)try {
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);if(!input)return 77;Json fixture;input>>fixture;
  const auto base=fixture.at("base").get<std::uint32_t>();std::size_t cases{},checks{},reads{},writes{},calls{},failures{},mutations{},reentries{};
  for(const auto& row:fixture.at("cases")) {
    const auto& q=row.at("input");auto bytes=unhex(fixture.at("banks").at(row.at("initial").get<std::size_t>()));Json trace=Json::array(),events=Json::array();std::string failure;bool failed{},mutated{},reentered{};std::map<std::string,std::size_t> occurrences;
    const auto view=[&](std::uint32_t address,std::size_t width,char access)->std::span<std::byte> {
      if(address<base||std::uint64_t(address)-base>bytes.size()||width>bytes.size()-(std::uint64_t(address)-base)) {
        trace.push_back(Json::array({"fault",std::string(1,access),address,width}));failure="memory";throw FormatError("Unmapped source backing address");
      }
      return std::span(bytes).subspan(address-base,width);
    };
    VistaWidgetBackingServices s;
    const auto provider=[&](const std::string& name,const Json& args) {
      const auto occurrence=occurrences[name]++;events.push_back({{"name",name},{"args",args},{"bank",hex(bytes)}});
      if(q.contains("mutation")) {
        const auto& m=q.at("mutation");
        if(m.at("at")==name&&m.value("occurrence",std::size_t{0})==occurrence) {
          mutated=true;for(const auto& w:m.at("writes"))put(view(w.at(0),w.at(2),'w'),w.at(1));
        }
      }
      if(q.contains("fail")&&q.at("fail")==Json::array({name,occurrence})){failure="provider";throw FormatError("Declared genuine platform provider failure");}
      if(q.contains("reentry")) {
        const auto& r=q.at("reentry");
        if(!reentered&&r.at("at")==name){reentered=true;(void)vista_widget_aligned_backing_allocate(r.at("size"),r.at("exponent"),s);}
      }
    };
    s.read=[&](std::uint32_t address,std::size_t width){const auto v=get(view(address,width,'r'));trace.push_back(Json::array({"r",address,width,v}));return v;};
    s.write=[&](std::uint32_t address,std::uint32_t v,std::size_t width){auto dest=view(address,width,'w');trace.push_back(Json::array({"w",address,width,v}));put(dest,v);};
    s.process_heap=[&] {provider("process_heap",Json::array());return q.value("heap",0x12345678u);};
    s.allocate=[&](auto heap,auto flags,auto size){provider("allocate",Json::array({heap,flags,size}));return VistaWidgetIdentity{q.value("raw",base+0x123u)};};
    s.release=[&](auto heap,auto flags,auto original){provider("release",Json::array({heap,flags,original.value}));return q.value("released",1u);};
    std::uint32_t result{};
    try {
      switch(q.at("entry").get<std::uint32_t>()) {
      case 0x43ec8d:result=vista_widget_backing_allocate(q.value("size",0u),s).value;break;
      case 0x437e69:result=vista_widget_aligned_backing_allocate(q.value("size",0u),q.value("exponent",0u),s).value;break;
      case 0x43ec9f:case 0x437ec3:result=vista_widget_backing_release({q.at("data")},s);break;
      default:throw FormatError("Unknown backing source entry");
      }
    }catch(const FormatError&){if(row.at("failure").is_null())throw;failed=true;}
    auto expected_events=row.at("events");for(auto& event:expected_events)event["bank"]=fixture.at("banks").at(event.at("bank").get<std::size_t>());
    const auto final=fixture.at("banks").at(row.at("final").get<std::size_t>()).get<std::string>();
    if(trace!=row.at("trace")||events!=expected_events||hex(bytes)!=final||(!failed&&row.at("returned")!=result)||mutated!=row.at("mutation")||reentered!=row.at("reentry")||failed!=!row.at("failure").is_null()||(failed&&failure!=row.at("failure").get<std::string>())) {
      std::cerr<<"Backing case "<<cases<<" trace "<<(trace==row.at("trace"))<<" events "<<(events==expected_events)<<" bank "<<(hex(bytes)==final)<<'\n';
      std::ofstream("/tmp/vista-backing-mismatch.json")<<Json({{"q",q},{"trace",trace},{"expected",row.at("trace")},{"events",events},{"expected_events",expected_events},{"return",result},{"expected_return",row.at("returned")}}).dump(2)<<'\n';throw FormatError("Original backing mismatch");
    }
    ++cases;checks+=8+trace.size()+events.size();calls+=events.size();failures+=failed;mutations+=mutated;reentries+=reentered;
    for(const auto& t:trace){reads+=t.at(0)=="r";writes+=t.at(0)=="w";}
  }
  VistaWidgetBackingServices empty;
  rejects([&]{vista_widget_backing_allocate(1,empty);});rejects([&]{vista_widget_backing_release({1},empty);});
  empty.process_heap=[] {return 0u;};rejects([&]{vista_widget_backing_allocate(1,empty);});
  empty.allocate=[](auto,auto,auto){return VistaWidgetIdentity{};};if(vista_widget_backing_allocate(1,empty))throw FormatError("Platform allocation failure changed");
  empty.allocate=[](auto,auto,auto){return VistaWidgetIdentity{0x1000};};rejects([&]{vista_widget_backing_allocate(1,empty);});
  empty.read=[](auto,auto){return 0u;};rejects([&]{vista_widget_backing_release({1},empty);});
  std::cout<<"PASS "<<cases<<" original backing cases, "<<checks<<" bank/trace/order checks, "<<reads<<" reads, "<<writes<<" writes, "<<calls<<" declared genuine platform calls, "<<failures<<" fault prefixes, "<<mutations<<" mutations, "<<reentries<<" nested source reentries; 5 unbound service boundaries\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
