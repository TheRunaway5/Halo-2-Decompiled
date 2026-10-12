#include "halo2/vista_lobby_update.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace halo2;
using Json=nlohmann::json;
namespace {
unsigned checks{};
void check(bool condition,const std::string& why) {
  ++checks;if(!condition)throw std::runtime_error(why);
}
template<std::size_t N> std::array<std::byte,N> bytes(const std::string& input) {
  if(input.size()!=N*2)throw std::runtime_error("fixture byte extent");
  std::array<std::byte,N> output;
  for(std::size_t i=0;i<N;++i)output[i]=std::byte(std::stoul(input.substr(i*2,2),nullptr,16));
  return output;
}
std::string hex(std::span<const std::byte> input) {
  constexpr char digits[]="0123456789abcdef";std::string output;
  for(auto b:input){auto v=std::to_integer<unsigned>(b);output+=digits[v>>4];output+=digits[v&15];}
  return output;
}
std::uint32_t u32(const Json& j) {return std::uint32_t(j.get<std::int64_t>());}
template<class R> Json array(const R& values) {
  auto result=Json::array();for(const auto value:values)result.push_back(value);return result;
}
struct Run {
  const Json& q;
  VistaLobbyUpdateState state;
  VistaLobbyUpdateGlobals globals;
  VistaLobbyUpdateScratch scratch;
  std::array<std::byte,0x130> variant{};
  std::array<std::byte,0x60> identifier;
  Json events=Json::array();
  unsigned enums{},hosts{};
  explicit Run(const Json& input):q(input) {
    state.field08=std::uint16_t(u32(q.at("field08")));
    state.field3486=std::uint8_t(u32(q.at("retry_byte")));
    const auto team=bytes<16>(q.at("team_records"));
    for(unsigned i=0;i<4;++i)std::copy_n(team.begin()+i*4,4,state.field1c6c[i].begin());
    state.field3514=q.at("previous_map");state.field3518=bytes<0x60>(q.at("previous_identifier"));
    state.field3578=std::uint8_t(u32(q.at("changed")));
    globals.fieldd78cb8=bytes<0x130>(q.at("pending_data"));
    globals.fieldd78de8=u32(q.at("pending_map_id"));
    globals.fieldd78df0=u32(q.at("pending_map_state"));
    globals.fieldd78df4=u32(q.at("pending_variant_state"));
    globals.fieldd78dec=std::uint8_t(u32(q.at("pending_variant_present")));
    globals.fieldd78ded=std::uint8_t(u32(q.at("pending_map_present")));
    globals.fieldd78df8=std::uint8_t(u32(q.at("pending_gate")));
    scratch.local_player_ids=q.at("local_seed").get<std::array<std::uint32_t,4>>();
    scratch.host_word=u32(q.at("host_seed"));scratch.map_first_output=u32(q.at("map_seed"));
    const auto engine=u32(q.at("engine"));
    for(unsigned i=0;i<4;++i)variant[0x44+i]=std::byte(engine>>(i*8));
    variant[0x48]=std::byte(u32(q.at("flags")));identifier=bytes<0x60>(q.at("identifier_data"));
  }
  template<class... A> void event(const std::string& name,A&&... a) {
    auto item=Json::array();item.push_back(name);(item.push_back(std::forward<A>(a)),...);
    events.push_back(std::move(item));
  }
  std::uint32_t value(const char* name,unsigned& count) {
    const auto& list=q.at(name);return u32(list.at(std::min(count++,unsigned(list.size()-1))));
  }
  void mutate(const std::string& name) {
    if(q.value("mutation",std::string{})==name) {
      variant[0x48]=std::byte{1};globals.fieldd78df8=1;identifier[0x40]=std::byte{0x5c};
    }
  }
  std::uint8_t query(const char* name) {auto result=u32(q.at(name));event(name,result);return std::uint8_t(result);}
  VistaLobbyUpdateServices services() {
    VistaLobbyUpdateServices s;
    s.session_enum=[this]{auto v=value("enums",enums);event("enum",std::bit_cast<std::int32_t>(v));return std::bit_cast<std::int32_t>(v);};
    s.variant=[this]{const bool present=q.at("variant");event("variant",present);return VistaLobbyUpdateVariant{present?1u:0u,variant};};
    s.host=[this]{const auto v=value("hosts",hosts);event("host",v);mutate("host");return std::uint8_t(v);};
    s.campaign=[this]{return query("campaign");};
    s.retry_map=[this]{return query("retry");};
    s.pending_map_ready=[this]{return query("pending_map_ready");};
    s.pending_variant_ready=[this]{return query("pending_variant_ready");};
    s.pending_map=[this](auto v){event("pending_map",v);};
    s.pending_variant=[this](auto b){event("pending_variant",hex(b));};
    s.clear_map=[this]{event("clear_map");};s.reset_variant=[this]{event("reset_variant");};
    s.notification=[this](const auto& v){auto row=array(v);row.insert(row.begin(),"notification");events.push_back(row);};
    s.controller_mask=[this]{auto v=u32(q.at("controller_mask"));event("controller_mask",v);return v;};
    s.register_controllers=[this](auto v){event("register_controllers",v);};
    s.multiplayer=[this]{return query("multiplayer");};
    s.next_controller=[this](auto previous) {
      auto cs=q.at("controllers").get<std::vector<std::int32_t>>();std::int32_t next=-1;
      if(previous==-1){if(!cs.empty())next=cs.front();}
      else {auto found=std::find(cs.begin(),cs.end(),previous);if(found!=cs.end() && ++found!=cs.end())next=*found;}
      event("next_controller",previous,next);return next;
    };
    s.local_player=[this](auto c){auto v=u32(q.at("local_players").at(c));event("local_player",c,v);return v;};
    s.occupied=[this](auto id){auto v=u32(q.at("occupied").at(id));event("occupied",id,v);return std::uint8_t(v);};
    s.sort_players=[this](auto ids,auto stride,auto compare){event("sort",array(ids),stride,compare);std::reverse(ids.begin(),ids.end());};
    s.dialogs=[this]{event("dialogs");mutate("dialogs");};
    s.countdown=[this](auto host){event("countdown",host);};
    s.pane=[this]{event("pane");mutate("pane");};
    s.visibility=[this]{event("visibility");mutate("visibility");};
    s.chat=[this]{event("chat");mutate("chat");};
    s.display=[this](auto owner,const auto& tuple) {
      const auto name=owner==VistaLobbyUpdateDisplay::caption?"caption":
          owner==VistaLobbyUpdateDisplay::status?"status":
          owner==VistaLobbyUpdateDisplay::privacy?"privacy":
          owner==VistaLobbyUpdateDisplay::campaign_details?"campaign_details":"roster";
      event(name,tuple.session_enum,unsigned(tuple.variant.identity!=0),array(tuple.players),
          array(tuple.local_players),tuple.count,tuple.host_word);
      if(q.value("mutation",std::string{})==name){tuple.players[0]=0x12345678;tuple.local_players[3]=0x87654321;}
      mutate(name);
    };
    s.details=[this](const auto& v,const auto& indices){auto row=array(indices);row.insert(row.begin(),unsigned(v.identity!=0));row.insert(row.begin(),"details");events.push_back(row);};
    s.map_bitmap=[this]{event("map_bitmap");mutate("map_bitmap");};
    s.help=[this]{event("help");mutate("help");};
    s.state9=[this]{return query("state9");};
    s.channel_in_use=[this](auto a,auto b){auto v=u32(q.at("channel"));event("channel",a,b,v);return std::uint8_t(v);};
    s.settings_change=[this](auto v){event("settings",v);};
    s.selected_map=[this](auto& map) {
      event("selected_map",map.first,map.map_id,unsigned(map.identifier.has_value()));
      map.first=u32(q.at("map_first"));map.map_id=q.at("map_id");
      if(q.at("identifier").get<bool>()) {
        if(q.value("identifier_alias",false))map.identifier=state.field3518;
        else map.identifier=identifier;
      } else map.identifier.reset();
      return std::uint8_t(u32(q.at("map_return")));
    };
    s.ui_context=[this]{event("ui_context");return std::uintptr_t(0x23450000);};
    s.identifier_provider=[this](auto c){auto v=u32(q.at("provider"));event("identifier_provider",c,v);return std::uint8_t(v);};
    s.unavailable_map=[this]{event("unavailable_map");mutate("unavailable_map");};
    s.peer_count=[this]{auto v=q.at("peer_count").get<std::int32_t>();event("peer_count",v);return v;};
    s.allow_single_peer=[this](auto v){event("allow_single",v);};
    s.unavailable_map_text=[this]{event("unavailable_map_text");return 0x98765432u;};
    s.question=[this](const auto& v){auto row=array(v);row.insert(row.begin(),"question");events.push_back(row);};
    s.browser=[this]{event("browser");mutate("browser");};
    s.teams=[this](const auto& v){event("teams",unsigned(v.identity!=0));};
    s.favorite=[this]{return query("favorite");};
    s.favorite_context=[this]{event("favorite_context");return std::uintptr_t(0x34560000);};
    s.identity=[this](auto& id){event("identity",array(id));id=q.at("identity").get<std::array<std::uint32_t,2>>();return std::uint8_t(u32(q.at("identity_return")));};
    s.favorite_identity=[this](auto c,const auto& id){auto v=u32(q.at("favorite_identity"));event("favorite_identity",c,array(id),v);return std::uint8_t(v);};
    s.screen_a0=[this](auto v){event("screen_a0",v);};
    s.spinner=[this](auto ids,auto count){event("spinner",array(ids),count);};
    s.base_update=[this]{event("base");mutate("base");};
    return s;
  }
  Json output() const {
    std::array<std::byte,16> team;
    for(unsigned i=0;i<4;++i)std::copy(state.field1c6c[i].begin(),state.field1c6c[i].end(),team.begin()+i*4);
    return {{"field08",state.field08},{"retry_byte",state.field3486},{"team_records",hex(team)},
      {"previous_map",state.field3514},{"previous_identifier",hex(state.field3518)},
      {"changed",state.field3578},{"pending_data",hex(globals.fieldd78cb8)},
      {"pending_map_id",globals.fieldd78de8},{"pending_map_state",globals.fieldd78df0},
      {"pending_variant_state",globals.fieldd78df4},{"pending_variant_present",globals.fieldd78dec},
      {"pending_map_present",globals.fieldd78ded},{"pending_gate",globals.fieldd78df8}};
  }
};
template<class F> void rejects(F&& f,const char* why) {
  bool rejected=false;try {f();}catch(const FormatError&){rejected=true;}check(rejected,why);
}
}
int main(int argc,char** argv) {
  try {
    if(argc!=2)throw std::runtime_error("usage: vista_lobby_update_tests original.json");
    Json fixture;std::ifstream input(argv[1]);input>>fixture;
    check(fixture.at("schema")=="H2V_PC14_UPDATE_ORIGINAL_V1","fixture schema");
    check(fixture.at("source").at("sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109","original executable pin");
    check(fixture.at("source_span").at("sha256")=="83f6d7443827b694e457aa69c326e4f1c68aed7435badf5698efd529bdecc950","complete original update span");
    unsigned index{};
    for(const auto& item:fixture.at("cases")) {
      Run r(item.at("input"));vista_lobby_update(r.state,r.globals,r.scratch,r.services());
      check(r.output()==item.at("output"),"state mismatch at case "+std::to_string(index));
      if(r.events!=item.at("events")) {
        for(std::size_t i=0;i<std::max(r.events.size(),item.at("events").size());++i)
          if(i>=r.events.size() || i>=item.at("events").size() || r.events[i]!=item.at("events")[i]) {
            std::cerr<<"case "<<index<<" event "<<i<<"\nactual "<<(i<r.events.size()?r.events[i].dump():"END")
                <<"\nsource "<<(i<item.at("events").size()?item.at("events")[i].dump():"END")<<'\n';break;
          }
      }
      check(r.events==item.at("events"),"ordered service trace mismatch at case "+std::to_string(index));++index;
    }
    const auto base=fixture.at("cases").front().at("input");
    {Run r(base);auto s=r.services();s.base_update={};rejects([&]{vista_lobby_update(r.state,r.globals,r.scratch,s);},"missing consumed base rejected");}
    {Run r(base);auto s=r.services();s.pending_variant={};s.sort_players={};s.screen_a0={};vista_lobby_update(r.state,r.globals,r.scratch,s);check(r.events.back()==Json::array({"base"}),"unconsumed services not required");}
    {auto q=base;q["map_return"]=1;q["map_id"]=-1;q["identifier"]=false;Run r(q);rejects([&]{vista_lobby_update(r.state,r.globals,r.scratch,r.services());},"original null identifier dereference kept unowned");}
    {auto q=base;q["controllers"]=Json::array({4});Run r(q);rejects([&]{vista_lobby_update(r.state,r.globals,r.scratch,r.services());},"out-of-owner controller rejected");}
    {Run r(base);auto s=r.services();s.variant=[] {return VistaLobbyUpdateVariant{1,{}};};rejects([&]{vista_lobby_update(r.state,r.globals,r.scratch,s);},"consumed missing variant bytes rejected");}
    {Run r(base);auto s=r.services();s.occupied=[](auto)->std::uint8_t {throw std::runtime_error("service exception");};bool stopped=false;
      try{vista_lobby_update(r.state,r.globals,r.scratch,s);}catch(const std::runtime_error&){stopped=true;}
      check(stopped && std::none_of(r.events.begin(),r.events.end(),[](const auto& e){return e[0]=="base";}),"service failure does not run later owners");}
    std::cout<<"Vista lobby update: "<<index<<" original cases, "<<checks<<" checks passed\n";
    return 0;
  } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
