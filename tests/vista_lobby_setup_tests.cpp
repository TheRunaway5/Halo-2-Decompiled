#include "halo2/vista_lobby_setup.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr VistaLobbySetupObject object=0x1a00000,text=object+0x9000,bitmap=object+0x8000,
    definition=object+0xa000,image=object+0xb000,event=object+0xc000,party=object+0xc200,created=object+0xe000;
std::string hex(std::uint32_t value) {std::ostringstream s;s<<std::uppercase<<std::hex<<std::setw(8)<<std::setfill('0')<<value;return s.str();}
std::string pointer(VistaLobbySetupObject value) {std::ostringstream s;s<<"0x"<<std::hex<<value;return s.str();}
void require(bool value,const char* message) {if(!value)throw FormatError(message);}
template<class F> void rejects(F f,const char* message) {
  bool failed{};try{f();}catch(const FormatError&){failed=true;}require(failed,message);
}
}
int main(int argc,char** argv)try {
  if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
  std::ifstream input(argv[1]);Json cases;input>>cases;unsigned tested{};
  for(const auto& recorded:cases) {
    const auto& q=recorded.at("input");const auto op=q.at("op").get<std::string>();
    const auto integer=[&](const char* key,std::int32_t fallback=0) {return q.value(key,fallback);};
    const auto byte=[&](const char* key,std::uint8_t fallback=1) {return q.value(key,fallback);};
    const auto handle=q.value("handle",std::uint32_t{});
    Json events=Json::array();unsigned allocated{};std::int32_t quick_key{};
    std::array<std::int16_t,8> bank{};
    VistaLobbySetupList list;VistaLobbySetupState state{byte("removed",0)};
    VistaLobbySetupServices s;
    s.base_list=[&](auto controller){events.push_back({"base_list",controller});};
    s.child_array=[&](auto offset,auto stride,auto count,auto ctor,auto dtor) {
      events.push_back({"child_array",object+offset,stride,count,ctor,dtor});
    };
    s.callback_owner=[&]{events.push_back({"callback_owner"});};
    s.session_enum=[&]{events.push_back({"enum",integer("enum",3)});return integer("enum",3);};
    s.row_bank=[&](auto name,auto capacity,auto stride){events.push_back({"row_bank",name,capacity,stride});};
    s.bank_initialize=[&]{events.push_back({"bank_initialize"});};
    s.allocate_row=[&]{
      events.push_back({"allocate_row",allocated});
      return q.contains("handles")?q.at("handles").at(allocated++).get<std::uint32_t>():0x120000u+allocated++;
    };
    s.store_row=[&](auto index,auto semantic){require(index<bank.size(),"Wrong original row-bank index");bank[index]=semantic;};
    s.host=[&]{events.push_back({"host",byte("host")});return byte("host");};
    s.count=[&]{events.push_back({"count",integer("count",1)});return integer("count",1);};
    s.callback_link=[&](auto callback){events.push_back({"callback_link",callback==0x64fa19});};
    s.text=[&](auto i){events.push_back({"text_lookup",i,byte("texts")});return byte("texts")?text+i*0x100:0;};
    s.selected_row=[&](auto actual_handle)->std::optional<std::int16_t> {
      require(actual_handle==handle,"Wrong source selected-row handle");
      events.push_back({"selected_row",byte("row_exists")});
      if(!byte("row_exists"))return std::nullopt;
      return std::int16_t(integer("row"));
    };
    s.set_string=[&](auto target,auto id){events.push_back({"string_id",(target-text)/0x100,hex(id)});};
    s.screen_tag=[&](auto id){events.push_back({"screen_tag",id,byte("descriptor")});return byte("descriptor")?0xe5dc03bau:0xffffffffu;};
    s.register_list=[&](auto datum,const auto& r) {
      require(r.field14==0 && std::all_of(r.tail.begin(),r.tail.end(),[](auto b){return b==0;}),"Wrong original registration tail");
      events.push_back({"registration",hex(datum),r.fields[0],r.fields[1],r.fields[2],r.fields[3],r.list_offset==0xa5c});
    };
    s.activate_list=[&]{events.push_back({"activate_list",true});};
    s.initial_focus=[&]{events.push_back({"screen_initial_focus"});};
    s.selection=[&]{events.push_back({"selection",handle});return handle;};
    s.child=[&](auto kind,auto i,auto recursive) {
      const auto exists=kind==8?byte("bitmap"):byte("texts");
      events.push_back({"child_lookup",kind,i,recursive,exists});return exists?(kind==8?bitmap:text+i*0x100):0;
    };
    s.map_query=[&](auto& map) {
      map={33,177,byte("custom",0)?definition:0};
      events.push_back({"map_query",byte("map_valid"),33,177,byte("custom",0)});return byte("map_valid");
    };
    s.image=[&](auto target,auto value) {
      require(target==bitmap,"Wrong source image widget");events.push_back({"image",value==image,pointer(value)});
    };
    s.multiplayer_definition=[&](auto map)->std::optional<std::uint32_t> {
      events.push_back({"multiplayer_definition",map,byte("definition")});
      if(!byte("definition"))return std::nullopt;
      return byte("ref")?0x12345678u:0u;
    };
    s.resolve_image=[&](auto reference,auto frame) {
      events.push_back({"resolve_image",reference,frame});return byte("image")?image:0;
    };
    s.identifier_image=[&](auto identifier,auto& output) {
      //459369 is a declared service of64F0EB. Its independently executed
      //original body clears output0 even on lookup failure; it never returns
      //an image in this supplied PE, including the provider-true branch.
      events.push_back({"map_info_by_identifier",identifier==definition});
      events.push_back({"identifier_lookup",identifier==definition,byte("custom_valid")});output=0;
      if(!byte("custom_valid"))return std::uint8_t{};
      events.push_back({"provider61AA42",byte("provider",0)});return std::uint8_t{1};
    };
    s.variant_engine=[&]()->std::optional<std::int32_t> {
      events.push_back({"variant",byte("variant")});if(!byte("variant"))return std::nullopt;return integer("engine",2);
    };
    s.resource_frame=[&](auto target,auto resource,auto frame) {
      require(target==bitmap,"Wrong source resource widget");events.push_back({"bitmap_resource_frame",resource,frame});
    };
    s.authored_frame=[&](auto target,auto frame) {
      require(target==bitmap,"Wrong source authored frame widget");events.push_back({"bitmap_frame",frame});
    };
    s.find_row=[&](auto semantic){events.push_back({"find_row",semantic,byte("party_row",0)});return byte("party_row",0)?party:0;};
    s.reset_list=[&]{events.push_back({"reset_list"});};
    s.set_selection=[&](auto selection){events.push_back({"set_selection",selection});};
    s.remove_row=[&](auto row){events.push_back({"remove_row",row==party});};
    s.base_update=[&]{events.push_back({"base_update"});};
    s.dispatch_row=[&](auto index) {
      //64FA19 performs this direct array read without a service call.
      require(index==std::uint16_t(handle),"Wrong source row-bank handle mask");return std::int16_t(integer("row"));
    };
    s.invoke_callback=[&](auto callback,auto passed_event){events.push_back({"callback",hex(callback),passed_event==event});};
    s.request_context_initialize=[&]{events.push_back({"request_context_initialize"});};
    s.screen_request=[&](const auto& r) {
      events.push_back({"screen_request",hex(r.controllers),hex(r.argument2),hex(r.argument3),hex(r.factory)});return created;
    };
    s.request_context_release=[&]{events.push_back({"request_context_release"});return created;};
    s.set_quick_key=[&](auto screen,auto key){require(screen==created,"Wrong source post-release publication target");quick_key=key;};
    if(op=="registration")vista_lobby_setup_construct(list,4,s);
    else if(op=="labels")require(vista_lobby_setup_label(handle,s)==(recorded.at("label_result").get<unsigned>()!=0),"Wrong original label return");
    else if(op=="details")vista_lobby_setup_details(state,s);
    else if(op=="setup_initialize")vista_lobby_setup_initialize(s);
    else if(op=="dispatch")vista_lobby_setup_dispatch(handle,event,s);
    else if(op=="open_map")vista_lobby_setup_open_map(integer("controller"),s);
    else if(op=="open_rules")vista_lobby_setup_open_rules(integer("controller"),s);
    else if(op=="open_quick")vista_lobby_setup_open_quick(integer("controller"),s);
    else if(op=="quick_request")vista_lobby_setup_quick_request(integer("key"),q.value("argument2",4u),q.value("argument3",4u),q.at("mask"),s);
    else throw FormatError("Unexpected original setup operation");
    Json rows=Json::array();for(unsigned i=0;i<list.count;++i)rows.push_back(list.rows[i]);
    const auto& expected=recorded.at("state");
    if(events!=recorded.at("events") || rows!=recorded.at("rows") || list.field464!=expected.at("flag464") ||
        state.party_removed!=expected.at("removed") || quick_key!=expected.at("quick_key")) {
      std::cerr<<"case="<<tested<<" input="<<q.dump()<<"\nexpected="<<recorded.dump()<<"\nactual events="<<events.dump()
          <<" rows="<<rows.dump()<<" flag464="<<unsigned(list.field464)<<" removed="<<unsigned(state.party_removed)<<" key="<<quick_key<<'\n';
      throw FormatError("Original Vista lobby setup mismatch");
    }
    if(op=="registration") {
      require(list.callback==0x64fa19,"Missing original constructor callback registration");
      require(Json(bank)==recorded.at("row_bank_slots"),"Wrong original allocation-handle row-bank writes");
      for(unsigned i=0;i<list.count;++i)require(list.handles[i]==(q.contains("handles")?q.at("handles").at(i).get<std::uint32_t>():0x120000u+i),"Lost original allocated handle");
    }
    ++tested;
  }
  require(tested==939,"Incomplete original Vista setup corpus");
  //Failure and publication seams that a scalar selection-only test misses.
  require(!vista_lobby_setup_label(std::nullopt,{}),"Null row widget consulted services");
  vista_lobby_setup_dispatch(0xffffffffu,event,{});
  rejects([&]{vista_lobby_setup_open_map(0,{});},"Missing consumed setup service accepted");
  VistaLobbySetupServices null_variant;null_variant.variant_engine=[]()->std::optional<std::int32_t>{return std::nullopt;};
  rejects([&]{vista_lobby_setup_open_quick(0,null_variant);},"Null source variant guessed");
  VistaLobbySetupServices requests;unsigned stage{};VistaLobbySetupObject published{};
  requests.request_context_initialize=[&]{require(stage++==0,"Wrong request initialization order");};
  requests.screen_request=[&](const VistaLobbySetupRequest& r){
    require(stage++==1 && r.controllers==8 && r.argument2==9 && r.argument3==12 && r.factory==0x658896,"Wrong request tuple");return VistaLobbySetupObject{11};
  };
  requests.request_context_release=[&]{require(stage++==2,"Wrong release order");return VistaLobbySetupObject{22};};
  requests.set_quick_key=[&](auto target,auto key){require(stage++==3 && key==26,"Wrong quick publication order/key");published=target;};
  vista_lobby_setup_quick_request(26,9,12,8,requests);
  require(stage==4 && published==22,"Quick key published on request result instead of release result");
  stage=0;requests.request_context_release=[&]{require(stage++==2,"Wrong null release order");return VistaLobbySetupObject{};};
  rejects([&]{vista_lobby_setup_quick_request(26,9,12,8,requests);},"Null original publication target accepted");
  require(stage==3,"Null publication wrote a quick key");
  std::cout<<"PASS original Vista lobby setup cases="<<tested<<"; failure and publication seams passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
