#include "halo2/vista_lobby_factory.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr std::uint32_t screen0=0x1a00000;
std::vector<std::byte> unhex(const std::string& text) {
  if(text.size()%2)throw FormatError("Odd original factory bank hex");
  const auto digit=[](char c)->unsigned {
    if(c>='0'&&c<='9')return unsigned(c-'0');
    if(c>='a'&&c<='f')return unsigned(c-'a')+10;
    throw FormatError("Invalid original factory bank hex");
  };
  std::vector<std::byte> out(text.size()/2);
  for(std::size_t i=0;i<out.size();++i)out[i]=std::byte(std::uint8_t(16*digit(text[2*i])+digit(text[2*i+1])));
  return out;
}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string out;
  for(auto b:bytes){const auto v=std::to_integer<unsigned>(b);out+=digits[v>>4];out+=digits[v&15];}return out;
}
void put(std::span<std::byte> b,std::size_t o,std::uint32_t v) {
  if(o>b.size()||4>b.size()-o)throw FormatError("Controlled factory write exceeds input bank");
  for(unsigned i=0;i<4;++i)b[o+i]=std::byte(std::uint8_t(v>>(8*i)));
}
std::uint32_t get(std::span<const std::byte> b,std::size_t o) {
  if(o>b.size()||4>b.size()-o)throw FormatError("Controlled factory read exceeds input bank");
  std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[o+i]))<<(8*i);return v;
}
template<class F> void rejects(F&& f){try{f();}catch(const FormatError&){return;}throw FormatError("Missing genuine factory service was admitted");}
} //namespace

int main(int argc,char** argv)try {
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);if(!input)return 77;
  Json fixture;input>>fixture;std::size_t count{},faults{},reentrant_faults{},clocks{},mutations{};
  for(const auto& row:fixture.at("cases")) {
    const auto& q=row.at("input");const auto& initial=row.at("initial");
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    std::vector<std::uint32_t> managers;
    for(const auto& [key,value]:initial.at("screens").items())banks.emplace(std::stoul(key),unhex(value));
    for(const auto& [key,value]:initial.at("state").at("managers").items()) {
      const auto identity=std::uint32_t(std::stoul(key));managers.push_back(identity);banks.emplace(identity,unhex(value));
    }
    auto request=unhex(initial.at("state").at("descriptor"));auto secondary=unhex(initial.at("secondary"));
    const auto descriptor=[&](std::vector<std::byte>& b){return VistaLobbyFactoryDescriptor(b.data(),32);};
    VistaLobbyFactoryGlobals globals;
    globals.current={initial.at("state").at("current")};globals.alternate=std::to_integer<std::uint8_t>(unhex(initial.at("state").at("alternate"))[0]);
    globals.construction.chat_owner_count=initial.at("state").at("chat_count");globals.construction.last_screen_id=initial.at("state").at("last_screen");
    const auto bounds=unhex(initial.at("bounds"));std::copy(bounds.begin(),bounds.end(),globals.construction.screen_bounds.begin());
    VistaLobbyManagerGraphics graphics{initial.at("state").at("graphics"),std::to_integer<std::uint8_t>(unhex(initial.at("state").at("graphics_failed"))[0])};
    Json events=Json::array();std::map<std::string,unsigned> occurrences;std::set<std::size_t> applied;std::size_t clock{};
    const auto manager=VistaWidgetIdentity{q.at("manager")};VistaLobbyFactoryServices services;
    // Use actual bytes for all source fields; no native session state is supplied.
    const auto state_snapshot=[&] {
      const std::array<std::byte,1> alternate{std::byte{globals.alternate}},failed{std::byte{graphics.failed}};
      Json state={{"current",globals.current.value},{"alternate",hex(alternate)},
        {"chat_count",globals.construction.chat_owner_count},{"last_screen",globals.construction.last_screen_id},
        {"graphics",graphics.identity},{"graphics_failed",hex(failed)},{"descriptor",hex(request)},{"managers",Json::object()}};
      for(auto identity:managers)state["managers"][std::to_string(identity)]=hex(banks.at(identity));
      return state;
    };
    std::function<void(const std::string&,Json)> event;
    event=[&](const std::string& name,Json args) {
      events.push_back({{"name",name},{"args",std::move(args)},{"state",state_snapshot()}});
      const auto occurrence=++occurrences[name];
      const auto changes=q.value("mutations",Json::array());
      for(std::size_t i=0;i<changes.size();++i) {
        const auto& m=changes[i];if(applied.contains(i)||m.at("at")!=name||m.value("occurrence",1u)!=occurrence)continue;
        applied.insert(i);const auto kind=m.at("kind").get<std::string>();
        if(kind=="manager")put(banks.at(m.value("identity",manager.value)),m.at("offset"),m.at("value"));
        else if(kind=="request")put(request,4*m.at("index").get<unsigned>(),m.at("value"));
        else if(kind=="screen")put(banks.at(m.at("identity")),m.at("offset"),m.at("value"));
        else if(kind=="alternate")globals.alternate=m.at("value");
        else if(kind=="graphics")graphics.identity=m.at("value");
        else if(kind=="pending")services.manager.pending(manager,VistaWidgetIdentity{m.at("screen")},descriptor(secondary));
        else if(kind=="activate")vista_lobby_manager_activate(manager,graphics,services.manager);
        else throw FormatError("Unknown controlled original factory mutation");
      }
    };
    auto& s=services.manager;
    s.bank=[&](auto id)->std::span<std::byte>{return banks.at(id.value);};
    s.window=[&](auto id){event("window",Json::array({id.value}));return get(banks.at(id.value),0x78);};
    s.sound=[&](auto value){event("sound",Json::array({value}));};
    s.pending=[&](auto id,auto screen,auto desc){event("pending",Json::array({id.value,screen.value,hex(desc)}));vista_lobby_manager_pending(id,screen,desc,s);};
    s.destructor=[&](auto id,auto value){event("destructor",Json::array({id.value,value}));};
    s.free=[&](auto id){event("free",Json::array({id.value}));};
    s.dispose=[&](auto id){event("dispose",Json::array({id.value}));};
    s.graphics=[&](auto id){event("graphics",Json::array({id}));return std::bit_cast<std::int32_t>(q.at("hresult").get<std::uint32_t>());};
    s.prepare=[&](auto id){event("prepare",Json::array({id.value}));};
    s.focus=[&](auto id,auto a,auto b){event("focus",Json::array({id.value,a,b}));};
    s.initialize=[&](auto id,auto desc){event("initialize",Json::array({id.value,hex(desc)}));};
    s.child_layout=[&](auto id){event("child_layout",Json::array({id.value}));};
    s.base_layout=[&](auto id){event("base_layout",Json::array({id.value}));};
    s.animate=[&](auto id,auto reason){event("animate",Json::array({id.value,reason}));};
    s.parameters=[&](auto id,auto desc){event("parameters",Json::array({id.value,hex(desc)}));};
    s.focused=[&](auto id,auto value){event("focused",Json::array({id.value,value}));return q.at("focused").get<std::uint8_t>();};
    s.update=[&](auto id){event("update",Json::array({id.value}));};
    services.allocate=[&](auto size,auto flags){event("allocate",Json::array({size,flags}));return VistaWidgetIdentity{q.at("allocation").get<bool>()?screen0:0};};
    services.read_cached_ui_clock=[&]{const auto& values=q.at("clocks");const auto value=values.at(clock++%values.size()).get<std::uint32_t>();event("clock",Json::array({value}));return value;};
    const auto mode=q.at("mode").get<std::string>();bool fault{};Json result=nullptr;
    const auto actual_descriptor=[&] {
      if(!q.contains("alias")||q.at("alias").is_null())return descriptor(request);
      return VistaLobbyFactoryDescriptor(banks.at(manager.value).data()+q.at("alias").get<std::size_t>(),32);
    };
    try {
      if(mode=="factory"||mode=="composition")result=vista_lobby_factory(q.at("alternate"),actual_descriptor(),globals,services).value;
      else if(mode=="register")vista_lobby_register_screen({screen0},actual_descriptor(),s);
      else if(mode=="pending")s.pending(manager,{screen0},actual_descriptor());
      else if(mode=="activate")vista_lobby_manager_activate(manager,graphics,s);
      else throw FormatError("Unknown original factory mode");
    }catch(const FormatError&){if(row.at("fault").is_null())throw;fault=true;}
    if(mode=="composition"&&!fault)vista_lobby_manager_activate(manager,graphics,s);
    Json final={{"state",state_snapshot()},{"screens",Json::object()},{"secondary",hex(secondary)},{"bounds",hex(globals.construction.screen_bounds)}};
    for(const auto& [key,value]:initial.at("screens").items()){(void)value;final["screens"][key]=hex(banks.at(std::stoul(key)));}
    if(events!=row.at("events")||final!=row.at("final")||result!=row.at("result")||fault!=!row.at("fault").is_null()||clock!=row.at("clock_reads")||applied.size()!=row.at("applied_mutations")) {
      std::cerr<<"factory case "<<count<<" mode "<<mode<<" events "<<(events==row.at("events"))<<" banks "<<(final==row.at("final"))<<'\n';
      if(events!=row.at("events"))for(std::size_t i=0;i<std::max(events.size(),row.at("events").size());++i) {
        if(i>=events.size()||i>=row.at("events").size()||events[i]!=row.at("events")[i]){std::cerr<<"first event "<<i<<" native "<<(i<events.size()?events[i].dump():"NONE")<<" original "<<(i<row.at("events").size()?row.at("events")[i].dump():"NONE")<<'\n';break;}
      }
      throw FormatError("Original PC14 factory/manager differential mismatch");
    }
    ++count;
    if(fault){const auto pc=row.at("fault").get<std::string>();if(pc=="006467C2"||pc=="00646852")++faults;else ++reentrant_faults;}
    clocks+=clock;mutations+=applied.size();
  }
  VistaLobbyManagerGraphics graphics;VistaLobbyManagerServices missing;
  rejects([&]{vista_lobby_manager_activate({1},graphics,missing);});
  std::array<std::byte,32> descriptor{};
  rejects([&]{vista_lobby_register_screen({1},descriptor,missing);});
  rejects([&]{vista_lobby_manager_pending({1},{2},descriptor,missing);});
  VistaLobbyFactoryGlobals globals;VistaLobbyFactoryServices no_allocation;
  rejects([&]{(void)vista_lobby_factory(false,descriptor,globals,no_allocation);});
  //637670's genuine renderer failure returns before any manager-bank access.
  VistaLobbyManagerGraphics unavailable_graphics{0x12345678,0x80};
  missing.graphics=[](auto identity){if(identity!=0x12345678)throw FormatError("Renderer identity changed");return std::int32_t{-1};};
  vista_lobby_manager_activate({0},unavailable_graphics,missing);
  if(unavailable_graphics.failed!=1)throw FormatError("Renderer failure did not retain the source global write");
  std::cout<<"PASS "<<count<<" original PC14 factory/manager cases, "<<clocks<<" actual constructor clock peeks, "<<faults<<" allocation-fault prefixes, "<<reentrant_faults<<" reentrant-fault prefixes, "<<mutations<<" applied mutation/reentry inputs; exact banks and call order\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
