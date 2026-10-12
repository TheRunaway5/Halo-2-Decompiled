#include "halo2/vista_lobby_info.hpp"
#include <nlohmann/json.hpp>
#include <array>
#include <bit>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool v,const std::string& message) {if(!v)throw std::runtime_error(message);}
std::int32_t signed_word(const Json& v) {return std::bit_cast<std::int32_t>(v.get<std::uint32_t>());}
Json normalize(const Json& events) {
  Json out=Json::array();
  for(const auto& e:events) {
    const std::string name=e.at(0);
    if(name=="hide_gate" || name=="current_session" || name=="count_output" ||
        name=="membership_view" || name=="locale" || name=="text_interface" ||
        name=="text_written")continue;
    out.push_back(e);
  }
  return out;
}
}
int main(int argc,char** argv) {try {
  check(argc==3,"Supply PC14 original evidence.json and execution.json");
  std::ifstream report_input(argv[1]),fixture_input(argv[2]);
  check(bool(report_input) && bool(fixture_input),"Cannot read PC14 original fixtures");
  const auto report=Json::parse(report_input),fixture=Json::parse(fixture_input);
  check(report.at("source").at("sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109","Wrong original PE");
  check(report.at("assets").at("source").at("sha256")=="eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44","Wrong original map");
  check(report.at("cases")==fixture.size(),"Incomplete original fixture");
  std::vector<std::vector<std::uint32_t>> storage;
  for(const auto& list:report.at("assets").at("description_lists")) {
    std::vector<std::uint32_t> strings;
    for(const auto& e:list.at("entries"))strings.push_back(std::stoul(e.at("string_id").get<std::string>(),nullptr,16));
    storage.push_back(std::move(strings));
  }
  std::vector<VistaLobbyInfoDescriptionList> lists;
  for(const auto& s:storage)lists.emplace_back(s);
  unsigned privacy_cases=0,help_cases=0,query_cases=0,list_cases=0;
  std::size_t number=0;
  for(const auto& row:fixture) {
    const auto& q=row.at("input");
    const std::string kind=q.at("kind");
    const auto pane=static_cast<std::int16_t>(q.at("pane").get<int>());
    const VistaLobbyInfoSession session{q.at("current").get<unsigned>()!=0,q.at("state"),
      signed_word(q.at("class")),q.at("privacy"),signed_word(q.at("count"))};
    const auto enumeration=signed_word(q.at("enum"));
    const bool child=q.at("child").get<unsigned>()!=0,root=q.at("root").get<unsigned>()!=0,tag=q.at("tag").get<unsigned>()!=0;
    const VistaLobbyInfoDescriptions descriptions{root,tag,lists};
    if(kind=="class_gate") {check((vista_lobby_info_session_class(session)==2)==(row.at("returned").get<int>()!=0),"Class wrapper case "+std::to_string(number));++query_cases;}
    else if(kind=="privacy_query") {check(vista_lobby_info_session_privacy(session)==row.at("returned").get<std::uint32_t>(),"Privacy wrapper case "+std::to_string(number));++query_cases;}
    else if(kind=="count_query") {check(vista_lobby_info_session_count(session)==row.at("returned").get<std::int32_t>(),"Count wrapper case "+std::to_string(number));++query_cases;}
    else if(kind=="hide_gate") {check(vista_lobby_info_privacy_hidden(enumeration)==(row.at("returned").get<int>()!=0),"Hide wrapper case "+std::to_string(number));++query_cases;}
    else {
      Json events=Json::array();
      const bool valid=pane>=0 && pane<=2;
      std::array<std::uint8_t,52> visibility;
      visibility.fill(q.at("visible").get<std::uint8_t>());
      std::array<std::string,52> labels;
      labels.fill("retained label sentinel");
      std::optional<std::uint32_t> selected;
      auto record=[&](auto&&... values){events.push_back(Json::array({values...}));};
      VistaLobbyInfoServices api;
      api.lookup_text=[&](auto index) {record("lookup_text",index);return index>=0 && index<52 && child?std::uint32_t(index+1):0;};
      api.session_enum=[&] {record("session_enum",enumeration);return enumeration;};
      api.session_class=[&] {record("class_gate");return vista_lobby_info_session_class(session);};
      api.membership_count=[&] {record("count_query");return vista_lobby_info_session_count(session);};
      api.privacy=[&] {record("privacy_query");return vista_lobby_info_session_privacy(session);};
      api.visible=[&](auto text,auto value) {check(text && text<=52,"Bad child token");record("visible",text-1,value);visibility[text-1]=value;};
      api.string_id=[&](auto text,auto id) {
        check(text && text<=52,"Bad string child token");selected=id;
        char hex[9];std::snprintf(hex,sizeof hex,"%08X",id);
        record("set_string_id",text-1,hex);
        if(root)labels[text-1]=report.at("assets").at("locale").at(hex).at("text");
      };
      api.description_id=[&](auto text,auto li,auto entry) {
        record("description_entry",text-1,li,entry);
        if(root) {
          record("screen_tag",14,tag?1:0);
          if(tag)record("tag_metadata","E5CA03A8");
        }
        return vista_lobby_info_description_id(descriptions,li,entry);
      };
      int runtime=-1;
      if(kind=="privacy") {vista_lobby_info_privacy(pane,api);if(valid)runtime=pane==1?7:12;++privacy_cases;}
      else if(kind=="help") {vista_lobby_info_help(pane,api);if(valid)runtime=pane==0?48:pane==1?43:49;++help_cases;}
      else if(kind=="help_list") {runtime=48;vista_lobby_info_description(runtime+1,q.at("list"),q.at("entry"),api);++list_cases;}
      else throw std::runtime_error("Unexpected fixture kind "+kind);
      const auto expected=normalize(row.at("events"));
      check(events==expected,"Original command trace case "+std::to_string(number)+": "+events.dump()+" != "+expected.dump());
      if(runtime>=0) {
        check(visibility[runtime]==row.at("visible").get<std::uint8_t>(),"Original visible byte case "+std::to_string(number));
        check(labels[runtime]==row.at("text").get<std::string>(),"Original label case "+std::to_string(number));
      } else check(row.at("visible").is_null() && row.at("text").is_null(),"Original invalid pane state");
      if(selected) {char hex[9];std::snprintf(hex,sizeof hex,"%08X",*selected);check(row.at("selected_string_id")==hex,"Original selected ID");}
      else check(row.at("selected_string_id").is_null(),"Original skipped setter");
    }
    ++number;
  }
  // Required callbacks are only evaluated on their original admitted branches.
  VistaLobbyInfoServices empty;
  vista_lobby_info_help(-1,empty);
  empty.lookup_text=[](auto){return VistaLobbyInfoText{};};
  vista_lobby_info_privacy(-1,empty);
  vista_lobby_info_help(0,empty);
  vista_lobby_info_privacy(0,empty);
  std::cout<<"PASS PC14 original cases "<<number<<" privacy "<<privacy_cases<<" help "<<help_cases<<" query "<<query_cases<<" lists "<<list_cases<<"\n";
  return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
