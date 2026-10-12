#include "halo2/vista_lobby_child_layout.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <string>

using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr std::array<VistaLobbyChildHandle,2> arrow_handles{0x17000000,0x17000100};
constexpr VistaLobbyChildHandle text_handle=0x17000200;
constexpr std::array<VistaLobbyChildHandle,2> stale_handles{0x11111111,0x22222222};
void require(bool value,const char* message) {
  if(!value)throw FormatError(message);
}
template<class F> void rejects(F&& run) {
  bool rejected{};
  try{run();}catch(const FormatError&){rejected=true;}
  require(rejected,"Missing consumed child-layout binding accepted");
}
struct Driver {
  VistaLobbyChildLayoutState state{};
  std::array<std::uint8_t,2> visible{};
  VistaLobbyChildRect bounds{};
  unsigned arrows=3;
  bool has_text{},set{},base{};
  Json events=Json::array();
  VistaLobbyChildLayoutServices services() {
    VistaLobbyChildLayoutServices s;
    s.bitmap=[this](std::int16_t index) {
      require(index==39 || index==40,"Unexpected child-layout bitmap index");
      require(events.empty() || index==40,"Bitmap39 lookup was not first");
      if(index==40) {
        require(events==Json::array({{"bitmap",39}}),"Bitmap40 lookup order changed");
        require(state.arrows[0]==((arrows&1)?arrow_handles[0]:0),"Arrow39 was not retained before bitmap40 lookup");
        require(state.arrows[1]==stale_handles[1],"Arrow40 was retained before its lookup");
      }
      events.push_back({"bitmap",index});
      const auto i=unsigned(index-39);
      return (arrows&(1u<<i))?arrow_handles[i]:VistaLobbyChildHandle{};
    };
    s.text=[this](std::int16_t index) {
      require(index==51,"Unexpected child-layout text index");
      check_retained();
      if(!state.chat_enabled)require(visible==std::array<std::uint8_t,2>{},"Text lookup preceded chat arrow stores");
      events.push_back({"text",index});
      return has_text?text_handle:VistaLobbyChildHandle{};
    };
    s.store_visible=[this](VistaLobbyChildHandle handle,std::uint8_t value) {
      check_retained();
      const auto i=handle==arrow_handles[0]?0u:1u;
      require(handle==arrow_handles[i],"Visibility store used a substituted child");
      require(value==0,"Chat-disabled arrow store was not zero");
      events.push_back({"visible",i,value});visible[i]=value;
    };
    s.get_bounds=[this](VistaLobbyChildHandle handle) {
      require(handle==text_handle,"Bounds read used a substituted text child");
      events.push_back({"get_bounds"});return bounds;
    };
    s.set_bounds=[this](VistaLobbyChildHandle handle,const VistaLobbyChildRect& rect) {
      require(handle==text_handle,"Bounds write used a substituted text child");
      require(!set,"Text bounds were written more than once");set=true;
      require(rect[0]==bounds[0] && rect[1]==bounds[1] && rect[3]==bounds[3],"Child layout changed a retained coordinate");
      events.push_back({"set_bounds",rect[0],rect[1],rect[2],rect[3]});bounds=rect;
    };
    s.base_layout=[this] {
      check_retained();require(set==has_text,"Base layout preceded the text bounds write");
      require(!base,"Base layout was called more than once");base=true;
      events.push_back({"base_layout"});
    };
    return s;
  }
  void check_retained() const {
    for(unsigned i=0;i<2;++i)
      require(state.arrows[i]==((arrows&(1u<<i))?arrow_handles[i]:0),"Child-layout retained arrow identity changed");
  }
  void reset(std::uint8_t chat,unsigned mask,bool text) {
    state={chat,stale_handles};arrows=mask;has_text=text;set=false;base=false;events=Json::array();
  }
};
void binding_checks() {
  Driver d;d.reset(1,0,false);
  auto s=d.services();s.store_visible={};s.get_bounds={};s.set_bounds={};
  //NULL arrows and absent unused services are valid when chat is nonzero and
  //text51 is NULL. The required base call still executes after both lookups.
  vista_lobby_child_layout(d.state,s);
  require(d.base,"Base layout skipped for NULL text and arrows");
  for(unsigned missing=0;missing<6;++missing) {
    d.reset(0,3,true);s=d.services();
    switch(missing) {
      case 0:s.bitmap={};break;
      case 1:s.text={};break;
      case 2:s.store_visible={};break;
      case 3:s.get_bounds={};break;
      case 4:s.set_bounds={};break;
      case 5:s.base_layout={};break;
    }
    rejects([&]{vista_lobby_child_layout(d.state,s);});
  }
  //The source would dereference NULL on these invalid constructed layouts.
  //The adapter reports the binding defect at the corresponding store; it
  //neither skips a required arrow nor substitutes a different child.
  for(unsigned mask=0;mask<3;++mask) {
    d.reset(0,mask,false);d.visible={0x80,0xff};s=d.services();
    rejects([&]{vista_lobby_child_layout(d.state,s);});d.check_retained();
    Json expected=Json::array({{"bitmap",39},{"bitmap",40}});
    if(mask==1)expected.push_back({"visible",0,0});
    require(d.events==expected,"Invalid child binding changed prior source operation order");
    require(!d.base,"Invalid child binding continued to base layout");
  }
  //The chat byte is read after the bitmap lookups, including a service which
  //publishes it while returning the second child. No cached initial flag.
  d.reset(0,3,false);d.visible={0x80,0xff};s=d.services();
  const auto lookup=s.bitmap;
  s.bitmap=[&](auto index){const auto result=lookup(index);if(index==40)d.state.chat_enabled=0x80;return result;};
  vista_lobby_child_layout(d.state,s);
  require(d.visible==std::array<std::uint8_t,2>{0x80,0xff},"Chat was read before the final bitmap lookup");
}
}
int main(int argc,char** argv)try {
  if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
  std::ifstream input(argv[1]);Json rows;input>>rows;
  require(rows.is_array(),"Invalid child-layout original corpus");
  unsigned count{};std::uint64_t instructions{};
  Driver d;
  for(const auto& row:rows) {
    const auto& q=row.at("input");
    d.reset(q.at("chat").get<std::uint8_t>(),q.at("arrows").get<unsigned>(),q.at("text").get<unsigned>()!=0);
    d.visible=q.at("visible").get<decltype(d.visible)>();d.bounds=q.at("bounds").get<VistaLobbyChildRect>();
    vista_lobby_child_layout(d.state,d.services());
    unsigned retained{};for(unsigned i=0;i<2;++i)if(d.state.arrows[i])retained|=1u<<i;
    if(d.events!=row.at("events") || Json(d.visible)!=row.at("visible") ||
        retained!=row.at("retained_arrows").get<unsigned>()) {
      std::cerr<<"case="<<count<<" input="<<q.dump()<<"\nexpected="<<row.dump()
          <<"\nactual_events="<<d.events.dump()<<" visible="<<Json(d.visible).dump()<<" retained="<<retained<<'\n';
      throw FormatError("Original Vista lobby child-layout mismatch");
    }
    d.check_retained();require(d.base,"Original child-layout case did not call base");
    instructions+=row.at("instructions").get<unsigned>();++count;
  }
  require(count==468 && instructions==20736,"Incomplete original child-layout corpus");
  binding_checks();
  std::cout<<"PASS original Vista lobby child-layout cases="<<count<<" original_instructions="<<instructions
      <<" exact_service_and_store_order=true retained_identity=true binding_checks=true\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
