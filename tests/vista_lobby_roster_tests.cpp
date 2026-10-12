#include "halo2/vista_lobby_roster.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const std::string& text){if(!value)throw std::runtime_error(text);}
std::string hex(std::span<const std::byte> b) {
  constexpr char digits[]="0123456789abcdef";std::string out;
  for(auto v:b){auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;
}
std::string narrow(std::u16string_view s) {
  std::string out;for(auto c:s){check(c<128,"fixture name requires ASCII");out+=char(c);}return out;
}
VistaLobbyRosterConfiguration config(const Json& row) {
  const auto s=row.at("configuration").get<std::string>();check(s.size()==0x108,"configuration extent");
  VistaLobbyRosterConfiguration b;
  for(unsigned i=0;i<b.size();++i)b[i]=std::byte(std::stoul(s.substr(i*2,2),nullptr,16));
  return b;
}
Json descriptor_json(const VistaLobbyRosterDescriptor& d) {
  return Json{{"flags",d.flags},{"name",narrow(d.name)},{"appearance",hex(d.appearance)},
      {"team",d.team},{"team_string",d.team_string},{"rank",d.rank_range},{"no_team",d.no_team},
      {"selector",std::bit_cast<std::uint32_t>(d.selector)}};
}
struct Services final:VistaLobbyRosterServices {
  const Json& row;const Json& q;const VistaPcrAppearanceAssets& assets;
  Json events=Json::array(),descriptors=Json::array();
  std::vector<VistaLobbyRosterSkinView> skins;
  std::array<unsigned,16> groups;
  std::array<unsigned,52> texts;
  std::array<unsigned,45> bitmaps,frames{};
  Services(const Json& r,const VistaPcrAppearanceAssets& a):row(r),q(row.at("input")),assets(a) {
    groups.fill(165);texts.fill(165);bitmaps.fill(165);
  }
  std::int32_t current_session_enum() override {
    auto v=q.at("enum").get<std::int32_t>();events.push_back({"session_enum",v});return v;
  }
  VistaLobbyRosterConfiguration player_configuration(std::uint32_t id) override {
    events.push_back({"player_configuration",id});check(id==0,"solo player identifier");return config(row);
  }
  void publish(std::span<const VistaLobbyRosterDescriptor> rows) override {
    events.push_back({"publish",rows.size()});
    for(const auto& d:rows){descriptors.push_back(descriptor_json(d));skins.push_back(vista_lobby_roster_skin(d,assets,{0x27f,0x1f80}));}
    groups.fill(0);std::fill_n(groups.begin(),rows.size(),1u);
  }
  void unused_colors(std::uint32_t team,std::uint8_t no_team,std::array<std::byte,16> appearance) override {
    events.push_back({"unused_colors",team,no_team,hex(appearance)});
  }
  std::uint32_t lookup_text(std::int16_t i) override {
    events.push_back({"lookup_text",i});return i<0 || i>=52?0u:std::uint32_t(i)+1;
  }
  std::uint32_t lookup_bitmap(std::int16_t i) override {
    events.push_back({"lookup_bitmap",i});return i<0 || i>=45?0u:std::uint32_t(i)+101;
  }
  Json label(std::uint32_t token) {return token<100?Json{"text",token-1}:Json{"bitmap",token-101};}
  void visible(std::uint32_t token,bool v) override {
    events.push_back({"visible",label(token),unsigned(v)});
    if(token<100)texts[token-1]=v;else bitmaps[token-101]=v;
  }
  bool voice_available(std::uint32_t id) override {
    auto v=q.at("voice_available").get<unsigned>();events.push_back({"voice_available",id,v});return (v&255)!=0;
  }
  bool voice_active(std::uint32_t id) override {
    auto v=q.at("voice_active").get<unsigned>();events.push_back({"voice_active",id,v});return (v&255)!=0;
  }
  void frame(std::uint32_t token,std::uint32_t v) override {
    events.push_back({"frame",label(token),v});frames[token-101]=v;
  }
  void hide_slot(std::uint32_t kind,std::int32_t i) override {
    events.push_back({"hide_slot",kind,i,0});
    if(kind==6 && i>=0 && i<52)visible(std::uint32_t(i)+1,false);
    if(kind==8 && i>=0 && i<45)visible(std::uint32_t(i)+101,false);
  }
};
Json normalize(const Json& events) {
  Json out=Json::array();
  for(const auto& e:events) {
    const auto name=e[0].get<std::string>();
    if(name=="current_session" || name=="text_written" || name=="text_id")continue;
    if((name=="visible" || name=="frame") && e[1][0]!="text" && e[1][0]!="bitmap")continue;
    out.push_back(e);
  }
  return out;
}
void check_skin(const VistaLobbyRosterSkinView& view,const Json& expected) {
  check(unsigned(view.name_visible)==expected[0]["visible"].get<unsigned>(),"skin name visibility");
  check(view.name_color==expected[0]["rgb"].get<VistaPcrRgb>(),"skin name RGB bits");
  check(unsigned(view.primary_visible)==expected[1]["visible"].get<unsigned>(),"skin primary visibility");
  check(view.primary_color==expected[1]["rgb"].get<VistaPcrRgb>(),"skin primary RGB bits");
  check(view.primary_frame==expected[1]["frame"],"skin primary frame retention");
  check(unsigned(view.secondary_visible)==expected[2]["visible"].get<unsigned>(),"skin secondary visibility");
  check(unsigned(view.rank_visible)==expected[3]["visible"].get<unsigned>(),"skin rank visibility");
  check(view.rank_frame==expected[3]["frame"],"skin rank frame retention");
  check(unsigned(view.emblem_visible)==expected[4]["visible"].get<unsigned>(),"skin emblem visibility");
}
} // namespace
int main(int argc,char** argv) {
  try {
    check(argc==3,"usage: vista_lobby_roster_tests evidence.json execution.json");
    Json evidence,rows;std::ifstream(argv[1])>>evidence;std::ifstream(argv[2])>>rows;
    const auto palette=evidence.at("appearance_palette").get<std::vector<VistaPcrRgb>>();
    const auto teams=evidence.at("team_palette").get<std::vector<VistaPcrRgb>>();
    std::vector<VistaPcrRankRange> ranges;
    for(const auto& r:evidence.at("rank_ranges"))ranges.push_back({r[0].get<std::int16_t>(),r[1].get<std::int16_t>()});
    unsigned cases=0;
    for(const auto& row:rows) {
      const auto& q=row.at("input");const auto pane=q.at("pane").get<int>();
      const auto& skin=evidence.at("skin_assets").at(pane==0?"E5D403B2":"E5D903B7");
      VistaPcrAppearanceAssets assets;assets.globals_present=assets.rank_owner_present=true;
      assets.appearance_palette=palette;assets.team_palette=teams;assets.rank_ranges=ranges;
      assets.primary_bitmap_present=assets.rank_bitmap_present=true;
      assets.primary_page_count=skin.at("bitmap_bindings")[0].at("page_count");
      assets.rank_page_count=skin.at("bitmap_bindings")[2].at("page_count");
      assets.previous_primary_frame=assets.previous_rank_frame=17;assets.emblem_definition_flags=skin.at("emblem_flags");
      std::array<std::uint32_t,1> ids{0};VistaLobbyRosterInput input;
      input.session_enum=q.at("enum");input.pane=std::int16_t(pane);input.variant_present=q.at("variant_present").get<unsigned>()!=0;
      input.variant_flags=q.at("variant_team");input.player_ids=std::span(ids).first(q.at("count").get<unsigned>());
      input.local_player_ids[0]=0;input.local_edits[0]={q.at("edit").get<unsigned>()!=0,q.at("edit_team").get<std::int16_t>()};
      Services services(row,assets);vista_lobby_roster_update(input,assets,services);
      check(services.descriptors==row.at("descriptor"),"descriptor bytes/flags at case "+std::to_string(cases));
      check(services.events==normalize(row.at("events")),"owner command order at case "+std::to_string(cases));
      check(services.groups==row.at("group_visibility").get<decltype(services.groups)>(),"group visibility");
      check(services.texts==row.at("main_text_visibility").get<decltype(services.texts)>(),"main text visibility");
      check(services.bitmaps==row.at("main_bitmap_visibility").get<decltype(services.bitmaps)>(),"main bitmap visibility");
      check(services.frames==row.at("main_bitmap_frames").get<decltype(services.frames)>(),"main bitmap frames");
      if(!services.skins.empty()) {
        const auto& view=services.skins[0];check_skin(view,row.at("skin"));
        bool text_found=false;
        for(const auto& event:row.at("events")) {
          if(event[0]=="text_id") {
            check(view.team_string==std::stoul(event[2].get<std::string>(),nullptr,16),"skin text-ID assignment");text_found=true;
          }else if(event[0]=="text_written") {
            check(view.team_string==0xffffffffu && narrow(view.name)==event[2].get<std::string>(),"skin UTF-16 name assignment");text_found=true;
          }
        }
        check(text_found,"original skin text assignment event absent");
      }
      ++cases;
    }
    check(cases==evidence.at("cases"),"fixture case total");
    std::cout<<"PASS "<<cases<<" original PC14 roster descriptor, skin, visibility and command cases\n";return 0;
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
