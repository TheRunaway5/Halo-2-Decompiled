#include "halo2/vista_lobby_widget_queries.hpp"
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
std::vector<std::byte> unhex(const std::string& s){
 if(s.size()%2)throw FormatError("Odd query bank hex");
 const auto d=[](char c)->unsigned{if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid query bank hex");};
 std::vector<std::byte>b(s.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(std::uint8_t(d(s[2*i])*16+d(s[2*i+1])));return b;
}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string s;for(auto x:b){const auto v=std::to_integer<unsigned>(x);s+=d[v>>4];s+=d[v&15];}return s;}
std::uint32_t get(std::span<const std::byte>b,std::size_t o){if(o>b.size()||4>b.size()-o)throw FormatError("Query fixture read exceeds bank");std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[o+i]))<<(i*8);return v;}
void put(std::span<std::byte>b,std::size_t o,std::uint32_t v){if(o>b.size()||4>b.size()-o)throw FormatError("Query fixture write exceeds bank");for(unsigned i=0;i<4;++i)b[o+i]=std::byte(std::uint8_t(v>>(i*8)));}
template<class F>void rejects(F&& f){try{f();}catch(const FormatError&){return;}throw FormatError("Missing query owner admitted");}
}
int main(int argc,char**argv)try{
 if(argc!=2)return 77;
 std::ifstream input(argv[1]);if(!input)return 77;Json fixture;input>>fixture;
 std::size_t cases{},faults{},authored{},mutations{};
 for(const auto&r:fixture.at("cases")){
  const auto&q=r.at("input");std::map<std::uint32_t,std::vector<std::byte>> regions;
  for(const auto&[key,value]:r.at("initial").items())regions.emplace(std::stoul(key),unhex(value));
  const auto view=[&](std::uint32_t address)->std::span<std::byte>{
   auto it=regions.upper_bound(address);if(it==regions.begin())throw FormatError("Unbound query source identity");--it;
   const auto offset=std::size_t(address-it->first);if(offset>=it->second.size())throw FormatError("Unbound query source identity");return std::span(it->second).subspan(offset);
  };
  std::uint32_t focus=r.at("initial_focus");bool applied{};Json events=Json::array();
  const auto snapshot=[&]{Json banks=Json::object();for(const auto&[id,b]:regions)banks[std::to_string(id)]=hex(b);return Json{{"focus",focus},{"banks",banks}};};
  const auto event=[&](const std::string&name,Json args){
   events.push_back({{"name",name},{"args",std::move(args)},{"state",snapshot()}});
   if(q.contains("mutation")&&!applied&&q.at("mutation").at("at")==name){const auto&m=q.at("mutation");applied=true;if(m.at("kind")=="focus")focus=m.at("value");else put(view(m.at("identity")),m.at("offset"),m.at("value"));}
  };
  VistaLobbyWidgetQueryServices s;
  s.bank=[&](auto id){return view(id.value);};
  s.root80=[&](auto id){event("root80",Json::array({id.value}));return vista_lobby_widget_root80(id,s);};
  s.root84=[&](auto id){event("root84",Json::array({id.value}));return vista_lobby_widget_root84(id,s);};
  s.virtual34=[&](auto id){return vista_lobby_widget_virtual34(id,s);};
  s.virtual38=[&](auto id){return vista_lobby_widget_virtual38(id,s);};
  s.focused=[&](auto argument,auto family,auto window){event("focused",Json::array({argument,family,window}));return VistaWidgetIdentity{focus};};
  s.set_focused=[&](auto argument,auto family,auto window,auto owner){event("set_focused",Json::array({argument,family,window,owner.value}));focus=owner.value;};
  const auto notify=[&](const std::string&name,VistaWidgetIdentity id,std::uint32_t argument){if(get(view(id.value),0)==0x7d3fcc)vista_lobby_widget_focus_notification(argument);else event(name,Json::array({id.value,argument}));};
  s.gained=[&](auto id,auto argument){notify("gained",id,argument);};s.lost=[&](auto id,auto argument){notify("lost",id,argument);};
  s.base_layout=[&](auto id){event("base_layout",Json::array({id.value}));};
  const VistaWidgetIdentity target{r.at("target")};const auto mode=q.at("mode").get<std::string>();Json result=nullptr;bool fault{};
  try{
   if(mode=="layout")vista_lobby_widget_child_layout(target,s);
   else if(mode=="text"||mode=="bitmap")result=vista_lobby_widget_child(target,q.at("ordinal"),mode=="text"?6:8,s).value;
   else if(mode=="get_bounds"){auto b=view(r.at("address"));vista_lobby_widget_get_bounds(target,std::span<std::byte,8>(b.data(),8),s);}
   else if(mode=="set_bounds"){auto b=view(r.at("address"));vista_lobby_widget_set_bounds(target,std::span<const std::byte,8>(b.data(),8),s);}
   else if(mode=="virtual34")result=vista_lobby_widget_virtual34(target,s);
   else if(mode=="virtual38")result=vista_lobby_widget_virtual38(target,s);
   else if(mode=="focused")result=std::uint32_t(vista_lobby_widget_focused(target,q.value("argument",0u),s));
   else if(mode=="focus")vista_lobby_widget_focus(target,q.value("argument",0u),std::uint8_t(q.value("acquire",1u)),s);
   else if(mode=="duration")result=vista_lobby_widget_duration();
   else if(mode=="prepare")vista_lobby_widget_prepare();
   else if(mode=="notification")vista_lobby_widget_focus_notification(q.value("argument",0u));
   else if(mode=="root80")result=s.root80(target);
   else if(mode=="root84")result=s.root84(target);
   else throw FormatError("Unknown original query mode");
  }catch(const FormatError&){if(r.at("fault").is_null())throw;fault=true;}
  if(events!=r.at("events")||snapshot()!=r.at("final")||fault!=!r.at("fault").is_null()||(!fault&&result!=r.at("result"))||applied!=r.at("applied_mutation")){
   std::cerr<<"query case "<<cases<<" mode "<<mode<<" events "<<(events==r.at("events"))<<" banks "<<(snapshot()==r.at("final"))<<" result "<<result<<" expected "<<r.at("result")<<'\n';
   if(events!=r.at("events"))for(std::size_t i=0;i<std::max(events.size(),r.at("events").size());++i)if(i>=events.size()||i>=r.at("events").size()||events[i]!=r.at("events")[i]){std::cerr<<"first event "<<i<<" native "<<(i<events.size()?events[i].dump():"NONE")<<" original "<<(i<r.at("events").size()?r.at("events")[i].dump():"NONE")<<'\n';break;}
   throw FormatError("Original PC14 widget query differential mismatch");
  }
  ++cases;if(fault)++faults;if(applied)++mutations;if(!r.at("authored").is_null())++authored;
 }
 VistaLobbyWidgetQueryServices missing;std::array<std::byte,8>b{};
 rejects([&]{(void)vista_lobby_widget_virtual34({1},missing);});
 rejects([&]{vista_lobby_widget_child_layout({1},missing);});
 rejects([&]{vista_lobby_widget_focus({1},0,1,missing);});
 rejects([&]{vista_lobby_widget_set_bounds({1},b,missing);});
 rejects([&]{(void)vista_lobby_widget_child({1},0,7,missing);});
 // Negative low16 ordinals return before the original reads the screen bank.
 if(vista_lobby_widget_child({0},0xffff,6,missing))throw FormatError("Negative original child ordinal admitted");
 std::cout<<"PASS "<<cases<<" original PC14 widget query/layout/focus cases, "<<authored<<" actual registration graphs, "<<faults<<" fault prefixes, "<<mutations<<" applied callbacks; exact banks and service order\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
