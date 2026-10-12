#include "halo2/vista_network_browser_back.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;using Json=nlohmann::json;
namespace {
void check(bool value,const char* message){if(!value)throw FormatError(message);}
}
int main(int argc,char**argv)try {
  if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
  std::ifstream file(argv[1]);Json rows;file>>rows;
  unsigned backs{},dispatches{},joined{},rejected{},local_dispatches{};
  for(const auto& row:rows) {
    const auto& q=row.at("input");const auto op=q.at("op").get<std::string>();
    Json events=Json::array();
    std::uint32_t mask{};
    for(unsigned i=0;i<4;++i)if(q.at("flags").at(i).get<std::uint32_t>()&8)mask|=1u<<i;
    VistaBrowserMainmenuReturnState state{q.at("initialized"),q.at("first_clock"),q.at("special")};
    VistaBrowserMainmenuReturnServices menu;
    menu.initialize_profiles=[&]{events.push_back({"initialize_profiles"});};
    menu.ui_clock=[&]{return q.at("clock").get<std::uint32_t>();};
    menu.controller_count=[&]{events.push_back({"count"});return std::uint32_t(std::popcount(mask));};
    menu.controller_mask=[&]{events.push_back({"mask"});return mask;};
    menu.create_screen=[&](const auto& descriptor){Json call=Json::array({"factory"});for(auto value:descriptor)call.push_back(value);events.push_back(std::move(call));};
    VistaBrowserBackServices back;
    back.channel=[&]{events.push_back({"channel",q.at("channel")});return q.at("channel").get<std::uint32_t>();};
    back.window=[&]{events.push_back({"window",q.at("window")});return q.at("window").get<std::uint32_t>();};
    back.screen_present=[&](auto channel,auto window,auto screen)->std::uint8_t {
      events.push_back({"present",channel,window,screen,q.at("present")});return q.at("present");
    };
    back.mainmenu_reason=[&](auto reason) {
      events.push_back({"reason",reason});
      if(op=="joined")vista_network_browser_mainmenu_return_single(state,menu);
    };
    back.close_channel=[&](auto channel,auto window){events.push_back({"close",channel,window});};
    back.base_input=[&]()->std::uint8_t{events.push_back({"base",q.at("base")});return q.at("base");};
    if(op=="dispatch") {
      events.push_back({"reason",2});
      if(std::popcount(mask)!=1) {
        bool threw{};try{vista_network_browser_mainmenu_return_single(state,menu);}catch(const FormatError&){threw=true;}
        check(threw,"Unbound controller-count factory silently succeeded");
        check(events.back()==Json::array({"count"}),"Unbound count reached an invented factory");
        ++rejected;
        if(std::popcount(mask)>1) {
          events=Json::array({Json::array({"reason",2})});
          state={q.at("initialized"),q.at("first_clock"),q.at("special")};
          vista_network_browser_mainmenu_return_local(state,menu);
          check(events==row.at("events") && state.initialized==row.at("initialized") &&
              state.initialization_clock==row.at("first_clock") && state.pending_special_menu==row.at("special"),
              "Original multi-user reason2 title factory descriptor/order/state differs");
          ++local_dispatches;
        }
        continue;
      }
      vista_network_browser_mainmenu_return_single(state,menu);++dispatches;
    } else {
      check(vista_network_browser_back_event(5,q.at("component")),"Original Back component table changed");
      const auto returned=vista_network_browser_back(q.at("modal"),back);
      check(returned==row.at("returned"),"Original browser Back AL differs");
      if(op=="back")++backs;else ++joined;
    }
    const auto& expected=row.at("events");
    if(op=="joined") {
      check(events.size()<expected.size(),"Joined factory proof omitted its external services");
      for(unsigned i=0;i<events.size();++i)check(events.at(i)==expected.at(i),"Original joined Back/dispatch order differs");
      check(row.at("menu_flag")==0 && row.at("input_flag")==0,"Original mainmenu factory failed its literal flag resets");
      check(row.at("report")==0,"Original optional report flag did not clear");
      check(row.at("report_argument")== (q.at("report").get<unsigned>()?0:0x83),"Original optional report argument changed outside its branch");
      bool constructed{};
      for(const auto& event:expected)if(event.at(0)=="screen_construct") {
        check(event==Json::array({"screen_construct",6,5,4,mask,0xa60}),"Original constructor did not create PC6 with the actual controller mask");
        constructed=true;
      }
      check(constructed,"Original PC6 constructor was bypassed");
    } else check(events==expected,"Original browser Back/dispatch service order differs");
    if(op!="back")check(state.initialized==row.at("initialized") &&
        state.initialization_clock==row.at("first_clock") && state.pending_special_menu==row.at("special"),
        "Original reason2 retained UI state differs");
  }
  for(std::uint32_t type=0;type<10;++type)for(std::uint32_t component=0;component<20;++component)
    check(vista_network_browser_back_event(type,component)==(type==5 && (component==1||component==13)),"Browser Back event admitted an unrelated original branch");
  check(!vista_network_browser_back_event(5,0xffffffffu) && !vista_network_browser_back_event(0xffffffffu,1),"Back table wrapping admission changed");
  bool missing{};try{(void)vista_network_browser_back(0,{});}catch(const FormatError&){missing=true;}
  check(missing,"Browser Back invented a missing widget channel");
  check(backs==144 && dispatches==72 && joined==72 && rejected==216 && local_dispatches==198,"Incomplete original browser Back corpus");
  const auto refresh_path=std::filesystem::path(argv[1]).parent_path()/"vista_browser_profile_refresh_original.json";
  std::ifstream refresh_file(refresh_path);check(bool(refresh_file),"Missing executed original profile refresh corpus");
  Json refresh_rows;refresh_file>>refresh_rows;unsigned refreshed{};
  for(const auto& row:refresh_rows) {
    const auto& q=row.at("input");std::vector<std::byte> controllers(4*0x127c);
    for(unsigned i=0;i<controllers.size();++i)controllers[i]=std::byte((i*37+11)&255);
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j) {
      controllers[i*0x127c+j]=std::byte(q.at("flags").at(i).get<std::uint32_t>()>>(j*8));
      controllers[i*0x127c+0x121c+j]=std::byte(q.at("indices").at(i).get<std::uint32_t>()>>(j*8));
    }
    auto expected=controllers;
    for(const auto& change:row.at("changes"))expected.at(change.at(0).get<unsigned>())=std::byte(change.at(1).get<unsigned>());
    Json events=Json::array();std::uint8_t pending=q.at("pending");
    VistaBrowserProfileRefreshServices services;
    services.refresh_catalog=[&]{events.push_back({"refresh_catalog"});};
    services.remap_index=[&](std::uint32_t index)->std::uint32_t {
      const auto value=q.at("remap").at(index-11).get<std::uint32_t>();
      events.push_back({"remap",index,value});return value;
    };
    services.controller_mask=[&] {
      events.push_back({"mask"});std::uint32_t mask{};
      for(unsigned i=0;i<4;++i)if(q.at("flags").at(i).get<std::uint32_t>()&8)mask|=1u<<i;
      return mask;
    };
    services.missing_profile_dialog=[&](const auto& descriptor) {
      Json event=Json::array({"dialog"});for(auto value:descriptor)event.push_back(value);events.push_back(std::move(event));
    };
    vista_network_browser_refresh_profiles(controllers,pending,services);
    check(controllers==expected && pending==row.at("pending") && events==row.at("events"),
        "Complete original608690 profile refresh bytes/order differ");++refreshed;
  }
  check(refreshed==432,"Incomplete original profile refresh corpus");
  std::cout<<"PASS original Vista PC13 Back/reason2 cases="<<backs+dispatches+joined
      <<" multi-user original title factories="<<local_dispatches<<" bounded single-factory refusals="<<rejected<<" profile-refresh cases="<<refreshed<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
