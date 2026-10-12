#include "halo2/vista_frontend_readiness.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <string_view>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void require(bool value,const std::string& what) {if(!value)throw FormatError(what);}
std::int32_t signed_word(const Json& j) {
  return std::bit_cast<std::int32_t>(j.get<std::uint32_t>());
}
constexpr std::array<std::string_view,18> names{
  "lifecycle_context","lifecycle_active","lifecycle_slot","current_session",
  "session_admission","mode","map_readiness","local_map_readiness","player_present",
  "player_eligible","variant","map_selection","builtin_map_metadata",
  "custom_metadata_clear","custom_map_metadata","player_configuration",
  "countdown","host_permission"};
VistaFrontendReadinessQueries queries(const Json& q) {
  VistaFrontendReadinessQueries v;
  v.lifecycle_active=q.at("active");v.lifecycle_slot=q.at("slot");v.current_session=q.at("current");
  if(!q.at("session").is_null())v.session_state=q.at("session");
  v.mode=q.at("mode");v.map_status=q.at("status");v.map_progress=q.at("progress");v.local_map_status=q.at("local");
  for(unsigned i=0;i<v.players.size();++i) {
    const auto& p=q.at("players").at(i);v.players[i].present=p.at(0);v.players[i].eligible=p.at(1);
    if(!p.at(2).is_null())v.players[i].team=p.at(2).get<std::int8_t>();
  }
  if(!q.at("variant").is_null())v.variant=VistaFrontendVariantView{q.at("variant").at(0),q.at("variant").at(1)};
  if(!q.at("map").is_null())v.map=VistaFrontendMapView{q.at("map").at(0),q.at("map").at(1)};
  if(!q.at("builtin").is_null())v.builtin_metadata=q.at("builtin").get<VistaFrontendMapCapacities>();
  v.custom_metadata_loaded=q.at("customloaded");
  if(!q.at("custom").is_null())v.custom_metadata=q.at("custom").get<VistaFrontendMapCapacities>();
  v.countdown=q.at("countdown");v.host_permission=q.at("host");v.mxcsr=q.at("mxcsr");return v;
}
template<class F>void rejects(F&& action,const char* message) {
  bool rejected{};try {action();}catch(const FormatError&) {rejected=true;}require(rejected,message);
}
}
int main(int argc,char** argv) try {
  if(argc!=2)throw FormatError("original Vista frontend readiness fixture required");
  std::ifstream input(argv[1]);require(bool(input),"original Vista frontend readiness fixture unavailable");
  Json fixture;input>>fixture;
  require(fixture.at("version")=="H2V_FRONTEND_READINESS_ORIGINAL_V1","unexpected frontend readiness fixture");
  require(fixture.at("source_sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109","unexpected Vista source authority");
  std::size_t producers{},leaves{};
  for(const auto& row:fixture.at("cases")) {
    const std::string label=row.at("label");const auto& expected=row.at("result");
    const auto result=vista_frontend_readiness(queries(row.at("query")),row.at("query").at("output"));
    require(result.state==signed_word(expected.at("state")),label+": original UI state differs");
    if(expected.at("progress").is_null())require(!result.progress_bits,label+": null progress output materialized");
    else require(result.progress_bits && *result.progress_bits==expected.at("progress").get<std::uint32_t>(),label+": original progress bits differ");
    const auto& calls=expected.at("calls");require(result.call_count==calls.size(),label+": service count differs");
    for(unsigned i=0;i<result.call_count;++i) {
      const auto& actual=result.calls[i];
      require(names.at(unsigned(actual.service))==calls.at(i).at(0).get<std::string>() &&
        actual.argument==calls.at(i).at(1).get<std::uint32_t>(),label+": original service order/argument differs at"+std::to_string(i));
    }
    ++producers;
  }
  for(const auto& row:fixture.at("player_leaves")) {
    const std::uint8_t query=row.at("query");const std::uint32_t mask=row.at("mask"),datum=row.at("datum"),leaf=row.at("leaf");
    if(leaf==0x6156e1)require(vista_frontend_player_present(query,mask,datum)==(row.at("result").get<unsigned>()!=0),"original player presence differs");
    else if(leaf==0x615820) {
      const auto peers=row.at("peers").get<std::vector<std::int32_t>>();
      require(vista_frontend_player_eligible(query,mask,datum,row.at("local"),peers)==(row.at("result").get<unsigned>()!=0),"original player eligibility differs");
    } else if(leaf==0x6157ca) {
      const auto teams=row.at("teams").get<std::vector<std::int8_t>>();
      const auto actual=vista_frontend_player_team(query,mask,datum,teams);
      if(row.at("result").is_null())require(!actual,"original null player configuration differs");
      else require(actual && *actual==row.at("result").get<std::int8_t>(),"original player team byte differs");
    } else throw FormatError("unknown original player leaf");
    ++leaves;
  }
  require(producers>900 && leaves>500,"incomplete original frontend readiness coverage");
  VistaFrontendReadinessQueries q;q.lifecycle_active=1;q.lifecycle_slot=1;q.current_session=1;
  rejects([&]{vista_frontend_readiness(q,true);},"missing current-session fields admitted");
  q.session_state=5;
  rejects([&]{vista_frontend_readiness(q,true);},"unmasked SSE context admitted");
  q.mxcsr=0x1f80;q.mode=-1;q.variant=VistaFrontendVariantView{1,1};
  rejects([&]{vista_frontend_readiness(q,true);},"missing consumed map selection admitted");
  q.map=VistaFrontendMapView{17,1};q.custom_metadata_loaded=1;
  rejects([&]{vista_frontend_readiness(q,true);},"missing consumed custom metadata admitted");
  rejects([&]{vista_frontend_player_eligible(1,1,0,0,{});},"missing consumed player peer record admitted");
  rejects([&]{vista_frontend_player_team(1,1,0,{});},"missing consumed player configuration admitted");
  require(!vista_frontend_player_eligible(0,1,0,0,{}) && !vista_frontend_player_team(0,1,0,{}),"unconsumed player banks rejected");
  std::cout<<"PASS "<<producers<<" unchanged Vista frontend readiness producer cases, "<<leaves
    <<" actual player getter controls; exact progress bits, branch results and ordered service intents\n";
  return 0;
} catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
