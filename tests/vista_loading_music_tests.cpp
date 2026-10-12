#include "halo2/vista_loading_music.hpp"

#include <nlohmann/json.hpp>
#include <array>
#include <bit>
#include <cfenv>
#include <fstream>
#include <iostream>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const std::string& message) {
  if(!value)throw std::runtime_error(message);
}
VistaLoadingMusicState state(const Json& j) {
  return {j[0],j[1],j[2],j[3],j[4],j[5]};
}
std::array<std::uint32_t,6> words(const VistaLoadingMusicState& s) {
  return {s.applied,s.requested,s.desired,s.previous_ui_clock,s.sound_handle,s.stopped_network_clock};
}
}
int main(int argc,char** argv) {
  try {
    check(argc==2,"Supply unchanged original Vista loading music fixture");
    std::ifstream input(argv[1]);
    const auto fixture=Json::parse(input);
    check(fixture.at("schema")=="H2V_LOADING_MUSIC_ORIGINAL_V1","Wrong original fixture schema");
    check(fixture.at("source_sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109","Wrong Vista PE identity");
    unsigned count{};
    for(const auto& row:fixture.at("cases")) {
      auto actual=state(row.at("before"));
      const auto& query=row.at("supplied");
      Json events=Json::array();
      auto record=[&](auto&&... fields) { events.push_back(Json::array({fields...})); };
      VistaLoadingMusicServices services;
      services.current_screen=[&] { const std::uint32_t v=query.at("screen");record("screen",v);return v; };
      services.audio_allowed=[&] { const unsigned v=query.at("permit_audio");record("permit_audio",v);return v!=0; };
      services.ui_clock=[&] { const std::uint32_t v=query.at("ui_clock");record("ui_clock",v);return v; };
      services.network_clock=[&] { const std::uint32_t v=query.at("network_clock");record("network_clock",v);return v; };
      services.shared_ui=[&]()->std::optional<VistaLoadingMusicSharedUi> {
        const bool present=query.at("shared");
        const std::uint32_t music=query.at("music"),duration=query.at("duration");
        record("shared_ui",present,music,duration);
        if(!present)return {};
        return VistaLoadingMusicSharedUi{music,duration};
      };
      services.play=[&](std::uint32_t music,std::uint32_t none,std::uint32_t gain) {
        const std::uint32_t handle=query.at("new_handle");
        record("play",music,none,gain,handle);return handle;
      };
      services.stop=[&](std::uint32_t handle) { record("stop",handle); };
      const std::u16string wide_path=u"levels\\declared\\declared";
      const std::string path="levels\\declared\\declared";
      services.loading_request=[&]()->std::optional<std::u16string> {
        const bool present=query.at("request");record("request",present);
        if(present)return wide_path;
        return {};
      };
      services.path_conversion=[&](std::u16string_view value) {
        check(value==wide_path,"Wrong native conversion input");record("path_conversion");return path;
      };
      services.map_state=[&](std::string_view value) {
        check(value==path,"Wrong native map state input");
        const std::uint32_t result=query.at("map_state");record("map_state",result);return result;
      };
      services.estimate=[&](std::u16string_view value) {
        check(value==wide_path,"Wrong native estimate input");
        const std::uint32_t result=query.at("estimate");record("estimate",result);
        return std::bit_cast<std::int32_t>(result);
      };
      std::optional<std::uint16_t> kind;
      if(query.at("scenario").get<bool>())kind=query.at("scenario_kind");
      const std::string operation=row.at("kind");
      std::optional<std::uint32_t> result;
      if(operation=="initialize")vista_loading_music_initialize(actual);
      else if(operation=="desired")vista_loading_music_desired(actual,kind,services);
      else if(operation=="update")vista_loading_music_update(actual,kind,services);
      else if(operation=="ready")result=vista_loading_music_ready(actual,services);
      else if(operation=="secondary") {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        std::uint16_t cw;
        asm volatile("fnstcw %0":"=m"(cw));
        const auto mxcsr=_mm_getcsr();
        const auto exceptions=std::fetestexcept(FE_ALL_EXCEPT);
#endif
        result=vista_loading_music_secondary_bits(actual,services,{0x027f,0x1f80});
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        std::uint16_t after_cw;
        asm volatile("fnstcw %0":"=m"(after_cw));
        check(cw==after_cw && mxcsr==_mm_getcsr() && exceptions==std::fetestexcept(FE_ALL_EXCEPT),"Secondary progress leaked FP state");
#endif
      } else throw std::runtime_error("Unknown original operation");
      const std::string label="original case "+std::to_string(count)+" "+operation;
      check(Json(words(actual))==row.at("after"),label+": owner mutation mismatch");
      if(result)check(*result==row.at("result").get<std::uint32_t>(),label+": result mismatch");
      check(events==row.at("events"),label+": service ordering mismatch\n"+events.dump()+"\n"+row.at("events").dump());
      ++count;
    }
    check(count==6505,"Original fixture coverage changed");
    std::cout<<"PASS Vista loading music owner original cases="<<count<<"\n";
  } catch(const std::exception& e) { std::cerr<<e.what()<<"\n";return 1; }
}
