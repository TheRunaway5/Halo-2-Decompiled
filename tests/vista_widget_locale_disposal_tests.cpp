#include "halo2/vista_widget_locale.hpp"
#include "halo2/vista_widget_disposal.hpp"
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
  const auto digit=[](char c)->unsigned {if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid original locale bank hex");};
  if(s.size()%2)throw FormatError("Odd original locale bank hex");
  std::vector<std::byte> b(s.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(std::uint8_t(16*digit(s[2*i])+digit(s[2*i+1])));return b;
}
std::string hex(std::span<const std::byte> b) {constexpr char d[]="0123456789abcdef";std::string out;for(auto c:b){const auto v=std::to_integer<unsigned>(c);out+=d[v>>4];out+=d[v&15];}return out;}
std::uint32_t get(std::span<const std::byte> b,std::size_t n) {if(n>b.size())throw FormatError("Controlled locale read exceeds raw bank");std::uint32_t v{};for(std::size_t i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[i]))<<(8*i);return v;}
void put(std::span<std::byte> b,std::uint32_t v,std::size_t n=4) {if(n>b.size())throw FormatError("Controlled locale write exceeds raw bank");for(std::size_t i=0;i<n;++i)b[i]=std::byte(std::uint8_t(v>>(8*i)));}
template<class F>void rejects(F&& f) {try{f();}catch(const FormatError&){return;}throw FormatError("Missing genuine locale/disposal service admitted");}
constexpr std::uint32_t obj=0x1a00000,entry=0x1918000;
}
int main(int argc,char** argv)try {
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);if(!input)return 77;
  Json fixture;input>>fixture;
  const auto state=[&](const Json& index) {Json out=Json::object();for(const auto& [address,bank]:fixture.at("states").at(index.get<std::size_t>()).items())out[address]=fixture.at("banks").at(bank.get<std::size_t>());return out;};
  std::size_t count{},actual{},failures{},mutations{},reentries{},read_count{},write_count{},service_count{};
  for(const auto& r:fixture.at("cases")) {
    Json expected_events=r.at("events");for(auto& event:expected_events)event["state"]=state(event.at("state"));const auto expected_final=state(r.at("final"));
    const auto& q=r.at("input");const auto op=q.at("op").get<std::string>();std::map<std::uint32_t,std::vector<std::byte>> regions;
    const auto initial=state(r.at("initial"));for(const auto& [key,value]:initial.items())regions.emplace(std::stoul(key),unhex(value));
    const auto view=[&](std::uint32_t address,std::size_t size)->std::span<std::byte> {auto it=regions.upper_bound(address);if(it==regions.begin())throw FormatError("Unbound original locale address");--it;const auto offset=std::size_t(address-it->first);if(offset>it->second.size()||size>it->second.size()-offset)throw FormatError("Unbound original locale address");return std::span(it->second).subspan(offset,size);};
    bool table_visible{};const auto table=r.at("table").is_null()?0u:r.at("table").get<std::uint32_t>();
    const auto snapshot=[&] {Json b=Json::object();for(const auto& [id,bytes]:regions)if((id!=table||table_visible)&&std::find(r.at("private").begin(),r.at("private").end(),id)==r.at("private").end())b[std::to_string(id)]=hex(bytes);return b;};
    bool applied{},reentered{};Json trace=Json::array(),events=Json::array();std::map<std::string,std::size_t> counters;
    const auto mutate=[&](const std::string& name,std::uint32_t address) {if(q.contains("mutation")&&!applied){const auto& m=q.at("mutation");if(m.at("at")==name&&m.value("call",std::size_t{0})==counters[name]&&(name!="read"||m.at("address")==address)){applied=true;const auto bytes=unhex(m.at("hex"));auto target=view(m.at("write"),bytes.size());std::copy(bytes.begin(),bytes.end(),target.begin());}}};
    const auto tracked=[&](std::uint32_t address) {for(const auto& id:r.at("private"))if(address>=id.get<std::uint32_t>()&&address<id.get<std::uint32_t>()+8)return false;return true;};
    const auto read=[&](std::uint32_t address,std::size_t size) {std::span<std::byte> bytes;try{bytes=view(address,size);}catch(const FormatError&){trace.push_back(Json::array({"fault","r",address,size}));throw;}mutate("read",address);const auto value=get(bytes,size);if(tracked(address))trace.push_back(Json::array({"r",address,size,value}));return value;};
    const auto write=[&](std::uint32_t address,std::uint32_t value,std::size_t size) {std::span<std::byte> bytes;try{bytes=view(address,size);}catch(const FormatError&){trace.push_back(Json::array({"fault","w",address,size}));throw;}if(table&&address>=table&&address<table+0xd0)table_visible=true;const auto masked=size==4?value:value&((1u<<(size*8))-1);if(tracked(address))trace.push_back(Json::array({"w",address,size,masked}));put(bytes,value,size);};
    const auto provider=[&](const std::string& name,Json args) {events.push_back({{"name",name},{"args",args},{"state",snapshot()}});mutate(name,0);if(q.contains("fail")&&q.at("fail")==name)throw FormatError("Declared original locale/disposal provider failure");return counters[name]++;};
    VistaWidgetLocaleServices s;s.read=read;s.write=write;
    s.interface=[&] {const auto i=provider("interface",Json::array());const auto values=q.value("interfaces",Json::array({obj+0x800,obj+0x800}));return values.at(std::min(i,values.size()-1)).get<std::uint32_t>();};
    s.globals=[&](std::uint32_t datum) {const auto i=provider("globals",Json::array({datum}));const auto values=q.value("globals",Json::array({obj+0x1000,obj+0x1000}));return values.at(std::min(i,values.size()-1)).get<std::uint32_t>();};
    s.definition=[&](auto screen) {provider("definition",Json::array({screen}));return q.value("definition",0x4321u);};
    s.screen_tag=[&](auto datum) {provider("screen_tag",Json::array({datum}));return q.value("tag",obj+0x1400);};
    s.localized=[&](auto list,auto id,auto output) {provider("localized",Json::array({list,id,output}));if(!q.value("unchanged",false)){const auto bytes=unhex(q.value("text",std::string("4100a9033dd800de0000")));auto target=view(output,bytes.size());std::copy(bytes.begin(),bytes.end(),target.begin());}};
    s.style=[&](auto widget,auto target) {provider("style",Json::array({widget.value,target}));return q.value("style",obj+0x1600);};
    s.assign=[&](auto target,auto style,auto text) {provider("assign",Json::array({target,style,text}));};
    VistaWidgetDisposalServices d;d.read=read;d.write=write;
    d.shutdown=[&](auto widget,auto target) {provider("shutdown",Json::array({widget.value,target}));if(q.value("reenter",false)&&!reentered){reentered=true;put(view(widget.value+0x6c,1),0,1);vista_widget_dispose_kinds({obj},0,0,d);}};
    d.destructor=[&](auto widget,auto target,auto argument) {provider("destructor",Json::array({widget.value,target,argument}));};
    d.free=[&](auto widget) {provider("free",Json::array({widget.value}));};
    const auto scratch=r.at("scratch").get<std::uint32_t>();const auto frame=r.at("frame").get<std::uint32_t>();bool failed{};std::uint32_t returned{};
    try {
      if(op=="header")vista_widget_header_locale({obj},q.value("id",0x11000777u),scratch,s);
      else if(op=="footer")vista_widget_footer_locale({obj},entry+4,scratch,frame,s);
      else if(op=="lookup")vista_widget_footer_locale_lookup(frame,s);
      else if(op=="screen")vista_widget_screen_locale({obj},q.value("id",0x11000777u),scratch,s);
      else if(op=="context")returned=vista_widget_locale_context(s);
      else if(op=="root")returned=vista_widget_locale_root({obj},s).value;
      else if(op=="detach")vista_widget_disposal_detach({obj},{q.value("child",obj+0x2100)},d);
      else if(op=="dispose")vista_widget_dispose_kinds({obj},static_cast<std::uint32_t>(q.value("first",std::int64_t{0})),static_cast<std::uint32_t>(q.value("last",std::int64_t{2})),d);
      else throw FormatError("Unknown original locale owner");
    }catch(const FormatError&){if(r.at("failure").is_null())throw;failed=true;}
    const bool return_ok=r.at("returned").is_null()||returned==r.at("returned");
    if(trace!=r.at("trace")||events!=expected_events||snapshot()!=expected_final||applied!=r.at("mutation")||reentered!=r.at("reentered")||failed!=!r.at("failure").is_null()||!return_ok) {
      std::cerr<<"locale/disposal case "<<count<<" "<<op<<" trace "<<(trace==r.at("trace"))<<" services "<<(events==expected_events)<<" bank "<<(snapshot()==expected_final)<<" failure "<<failed<<" expected "<<r.at("failure")<<'\n';
      Json debug={{"actual_trace",trace},{"expected_trace",r.at("trace")},{"actual_events",events},{"expected_events",expected_events},{"actual_final",snapshot()},{"expected_final",expected_final}};std::ofstream("/tmp/vista-locale-disposal-mismatch.json")<<debug.dump(2)<<'\n';
      throw FormatError("Original locale/disposal differential mismatch");
    }
    ++count;if(q.contains("actual"))++actual;if(failed)++failures;if(applied)++mutations;if(reentered)++reentries;for(const auto& t:trace){if(t.at(0)=="r")++read_count;if(t.at(0)=="w")++write_count;}service_count+=events.size();
  }
  VistaWidgetLocaleServices missing;vista_widget_header_locale({1},0xffffffffu,2,missing);
  rejects([&]{vista_widget_header_locale({1},0,2,missing);});rejects([&]{vista_widget_footer_locale({1},2,3,4,missing);});rejects([&]{vista_widget_footer_locale_lookup(1,missing);});
  VistaWidgetDisposalServices empty;vista_widget_dispose_kinds({1},1,0,empty);rejects([&]{vista_widget_dispose_kinds({1},0,0,empty);});
  // Genuine lazy boundaries: unset/absent branches avoid unused providers;
  // every reached locale, virtual method and deallocator remains mandatory.
  missing.interface=[] {return 0u;};vista_widget_footer_locale({1},2,3,4,missing);
  missing.interface=[] {return 1u;};rejects([&]{vista_widget_locale_context(missing);});
  missing.read=[](auto,auto) {return 0xffffffffu;};if(vista_widget_locale_context(missing))throw FormatError("NONE global datum reached a provider");
  missing.read=[](auto,auto) {return 0u;};rejects([&]{vista_widget_locale_context(missing);});
  missing.globals=[](auto) {return 0u;};if(vista_widget_locale_context(missing))throw FormatError("Missing original global result changed");
  rejects([&]{vista_widget_header_locale({1},0,2,missing);});
  missing.write=[](auto,auto,auto) {};rejects([&]{vista_widget_header_locale({1},0,2,missing);});
  missing.definition=[](auto) {return 0xffffffffu;};rejects([&]{vista_widget_header_locale({1},0,2,missing);});
  missing.style=[](auto,auto) {return 1u;};rejects([&]{vista_widget_header_locale({1},0,2,missing);});
  missing.assign=[](auto,auto,auto) {};vista_widget_header_locale({1},0,2,missing);
  missing.definition=[](auto) {return 0u;};rejects([&]{vista_widget_header_locale({1},0,2,missing);});
  missing.screen_tag=[](auto) {return 0u;};vista_widget_header_locale({1},0,2,missing);
  missing.screen_tag=[](auto) {return 1u;};rejects([&]{vista_widget_header_locale({1},0,2,missing);});
  missing.localized=[](auto,auto,auto) {};vista_widget_header_locale({1},0,2,missing);
  empty.read=[](auto,auto) {return 0u;};vista_widget_dispose_kinds({1},0,0,empty);
  empty.read=[](auto at,auto) {return at==1u+0xa04u?2u:0u;};rejects([&]{vista_widget_dispose_kinds({1},0,0,empty);});
  empty.write=[](auto,auto,auto) {};vista_widget_dispose_kinds({1},0,0,empty);
  empty.read=[](auto at,auto) {return at==1u+0xa04u?2u:at==2u+0x10u?0u:1u;};rejects([&]{vista_widget_dispose_kinds({1},0,0,empty);});
  empty.shutdown=[](auto,auto) {};rejects([&]{vista_widget_dispose_kinds({1},0,0,empty);});
  empty.destructor=[](auto,auto,auto) {};rejects([&]{vista_widget_dispose_kinds({1},0,0,empty);});
  empty.free=[](auto) {};vista_widget_dispose_kinds({1},0,0,empty);
  std::cout<<"PASS "<<count<<" original locale/disposal cases, "<<actual<<" actual PC wgit inputs, "<<read_count<<" exact reads, "<<write_count<<" exact writes, "<<service_count<<" exact provider calls, "<<failures<<" partial failures, "<<mutations<<" callback mutations, "<<reentries<<" actual reentrant disposal\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
