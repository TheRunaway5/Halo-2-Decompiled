#include "halo2/vista_session_map_change.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>

using namespace halo2;
using J=nlohmann::json;
namespace {
void require(bool b,const char* why){if(!b)throw std::runtime_error(why);}
void put(std::span<std::byte> b,unsigned at,std::uint32_t v,unsigned n=4){for(unsigned i=0;i<n;++i)b[at+i]=std::byte(v>>(i*8));}
std::vector<std::byte> unhex(const std::string& s) {
  require(s.size()%2==0,"Invalid original hex buffer");std::vector<std::byte> b;
  for(unsigned i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
  return b;
}
std::string hex(std::span<const std::byte> b) {
  constexpr char digits[]="0123456789abcdef";std::string out;
  for(auto v:b){auto x=std::to_integer<unsigned>(v);out+=digits[x>>4];out+=digits[x&15];}return out;
}
std::uint32_t value(const J& q,const char* name,std::uint32_t fallback){return q.contains(name)?std::uint32_t(q.at(name).get<std::int64_t>()):fallback;}
std::array<std::byte,vista_session_source_size> initial(const J& q) {
  std::array<std::byte,vista_session_source_size> b{};
  put(b,0x73a0,value(q,"state",5));put(b,0x4dd4,value(q,"connection",0));put(b,0x4dc8,0x76543210);put(b,0x4dcc,0xfedcba98);
  put(b,0x4dd0,0x1badb002);put(b,0x4cb8,2);put(b,0x4dd8,value(q,"map_id",123));
  const auto path=q.value("path",std::string("levels\\test\\actual"));
  for(unsigned i=0;i<std::min<std::size_t>(path.size(),260);++i)put(b,0x4cbc+i*2,static_cast<unsigned char>(path[i]),2);
  put(b,0x5101,value(q,"prepared",1),1);put(b,0x5104,value(q,"machines",1));
  const auto machine=unhex("123456789abc"),identity=unhex("1032547698badcfe");std::copy(machine.begin(),machine.end(),b.begin()+0x5108);
  put(b,0x5eb8,value(q,"campaign",0xffff),2);put(b,0x70+0x11e4,value(q,"peer_count",1));put(b,0x70+0x14,value(q,"role",0));
  put(b,0x64,value(q,"local",0));put(b,0x71c0,value(q,"host",0));put(b,0x1c,value(q,"type",0));put(b,0x4c74,value(q,"local_flag",0),1);put(b,0x4c78,value(q,"ui_mode",0));
  const auto variant=source_game_variant_default(0);for(unsigned i=0;i<variant.bytes.size();++i)b[0x4e3c+i]=std::byte(variant.bytes[i]);
  for(unsigned i=0;i<17;++i)for(unsigned j=0;j<6;++j)b[0x70+i*0x10c+0x22+j]=std::byte(std::to_integer<unsigned>(machine[j])+i);
  for(unsigned i=0;i<96;++i)b[0x4ddc+i]=std::byte(i*13+7);
  put(b,0x5178,value(q,"occupied",1),1);put(b,0x5179,value(q,"guest",0),1);put(b,0x517a,value(q,"controller",0),2);
  std::copy(machine.begin(),machine.end(),b.begin()+0x5180);std::copy(identity.begin(),identity.end(),b.begin()+0x5186);
  for(unsigned i=0;i<0x84;++i)b[0x5190+i]=std::byte(i*17+29);
  return b;
}
const char* intent(VistaSessionMapChangeIntent v) {
  switch(v){case VistaSessionMapChangeIntent::select_map:return "select_map";case VistaSessionMapChangeIntent::select_scenario:return "select_scenario";
    case VistaSessionMapChangeIntent::select_variant:return "select_variant";case VistaSessionMapChangeIntent::ui_reason:return "ui_reason";
    case VistaSessionMapChangeIntent::phase_write:return "phase_write";case VistaSessionMapChangeIntent::remote_close:return "remote_close";}
  throw std::runtime_error("Unknown map-change intent");
}
}
int main(int argc,char** argv){try{
  require(argc==2,"Original map-change fixture argument");std::ifstream file(argv[1]);require(bool(file),"Open original fixture");std::string line;unsigned builders{},validators{},presence{};
  while(std::getline(file,line)) {
    const auto r=J::parse(line);
    if(r.at("kind")=="validator") {const auto options=unhex(r.at("options"));require(vista_session_multiplayer_options_valid(options)==bool(r.at("accepted").get<unsigned>()),"Original multiplayer validator mismatch");++validators;continue;}
    if(r.at("kind")=="presence") {
      const auto operation=r.at("operation").get<std::string>();const auto v=std::uint32_t(r.at("value").get<std::int64_t>());VistaWorldOptions options{};put(options,0x2e4,v);
      const auto call=VistaSessionMapChangeCall{operation=="select_map"?VistaSessionMapChangeIntent::select_map:operation=="select_variant"?VistaSessionMapChangeIntent::select_variant:operation=="select_scenario"?VistaSessionMapChangeIntent::select_scenario:VistaSessionMapChangeIntent::ui_reason,v};
      const auto context=vista_session_map_change_presence(call,options);J events=J::array();if(context)events.push_back({"context",context->user,context->context,context->value});
      require(events==r.at("events"),"Original imported rich-presence arguments mismatch");++presence;continue;
    }
    const auto& q=r.at("input");auto bank=initial(q);const auto before=bank;VistaSessionTransitionState state{{0xdeadbeef,0xfeed1234}};
    J events=J::array();VistaWorldOptions loaded{};VistaSessionMapChangeServices s;
    s.constructor_byte=[&]{const auto v=value(q,"ctor",0);events.push_back({"constructor_byte",v});return std::uint8_t(v);};
    s.configured_tick_rate=[&]{events.push_back({"tick_rate"});return std::bit_cast<std::int32_t>(value(q,"tick",60));};
    s.random_identity=[&]{events.push_back({"random"});std::array<std::byte,8> out;out.fill(std::byte{0x5a});return out;};
    s.pause=[&](bool v){events.push_back({"pause",unsigned(v)});};
    s.validate_options=[&](auto options){events.push_back({"validate"});return std::uint8_t(q.contains("validator")?value(q,"validator",0):vista_session_multiplayer_options_valid(options));};
    s.request_load=[&](auto options){events.push_back({"request"});std::copy(options.begin(),options.end(),loaded.begin());const auto identity=unhex("8899aabbccddeeff");std::copy(identity.begin(),identity.end(),loaded.begin()+0x10);return std::uint8_t(value(q,"load",1));};
    s.loaded_options=[&]{events.push_back({"loaded_options"});return std::span<const std::byte>(loaded);};
    const auto result=vista_session_apply_map_change(state,bank,s);
    for(unsigned i=0;i<result.call_count;++i) {
      const auto& c=result.calls[i];J event=J::array({intent(c.intent)});
      if(c.intent!=VistaSessionMapChangeIntent::select_scenario && c.intent!=VistaSessionMapChangeIntent::select_variant)event.push_back(c.argument);
      events.push_back(event);
    }
    std::array<std::byte,8> identity;put(identity,0,state.world_identity[0]);put(identity,4,state.world_identity[1]);
    const bool options_match=result.options?J(hex(*result.options))==r.at("options"):r.at("options").is_null();
    if(bank!=before || !options_match || result.accepted!=r.at("accepted").get<unsigned>() || hex(identity)!=r.at("identity").get<std::string>() || events!=r.at("events")) {
      std::cerr<<"builder="<<builders<<" input="<<q.dump()<<"\nexpected events="<<r.at("events").dump()<<"\nactual events="<<events.dump()<<" identity="<<hex(identity)<<'\n';
      if(result.options && !r.at("options").is_null()) {const auto expected=unhex(r.at("options"));for(unsigned i=0;i<expected.size();++i)if(expected[i]!=(*result.options)[i])std::cerr<<"option offset="<<i<<" expected="<<std::to_integer<unsigned>(expected[i])<<" actual="<<std::to_integer<unsigned>((*result.options)[i])<<'\n';}
      throw std::runtime_error("Original complete map-change mismatch");
    }
    require(bool(result.loaded)==bool(result.accepted),"Current options copied only after accepted fulfillment");++builders;
  }
  require(builders==101 && validators==45 && presence==1831,"Complete original map-change corpus");
  bool rejected=false;try{(void)vista_session_multiplayer_options_valid({});}catch(const FormatError&){rejected=true;}require(rejected,"Truncated world options rejected");
  rejected=false;try{(void)vista_session_build_map_change({},{});}catch(const FormatError&){rejected=true;}require(rejected,"Truncated session rejected");
  std::cout<<"Vista map-change original builders="<<builders<<" validators="<<validators<<" presence="<<presence<<" PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
