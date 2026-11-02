// Actual v16 native queue/descriptor/settings handoff. Provider handoff is a
// private fixture; this probe does not construct or certify a played world.
#include "halo2/online_matchmaking.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <thread>

using namespace halo2;
using Json=nlohmann::json;
using namespace std::chrono_literals;
namespace {
void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
std::string input(const char *name){const auto *s=std::getenv(name);require(s&&*s,"Private handoff input missing");return s;}
std::string encode(const std::string &s){
  auto *p=curl_easy_escape(nullptr,s.c_str(),int(s.size()));require(p,"Private handoff encoding failed");
  std::string result(p);curl_free(p);return result;
}
}
int main(){
  Json report={{"full_settings_validated",false},{"native_session_taken",false},
    {"native_loading_reached",false},{"played_world",false},{"real_provider",false},
    {"sdk_available",online_protocol_bridge::eos_sdk_available()}};
  const char *stage="initialize";
  try {
    require(curl_global_init(CURL_GLOBAL_DEFAULT)==CURLE_OK,"Native curl initialization failed");
    const auto base=input("H2_HANDOFF_URL"),token=input("H2_HANDOFF_TOKEN"),subject=input("H2_HANDOFF_SUBJECT");
    const auto expected=Json::parse(input("H2_HANDOFF_GAMES"));
    const auto playlist=input("H2_HANDOFF_PLAYLIST");
    const std::string device(43,'d'),ns="http://specs.openid.net/auth/2.0",op="https://steamcommunity.com/openid/login";
    const auto url=op+"?openid.ns="+encode(ns)+"&openid.mode=checkid_setup&openid.realm="+encode(base+"/")+"&openid.return_to="+
      encode(base+"/v1/auth/steam/callback/"+device)+"&openid.identity="+encode(ns+"/identifier_select")+"&openid.claimed_id="+encode(ns+"/identifier_select");
    auto login=DiscordLogin::steam([](const auto&){return true;},[&](const DiscordHttpRequest &r,std::stop_token){
      if(r.url==base+"/v1/auth/steam/start")return DiscordHttpResponse{201,Json{{"device",device},{"url",url},{"expires_in",300},{"interval",1}}.dump()};
      require(r.url==base+"/v1/auth/steam/poll","Unexpected provider handoff request");
      return DiscordHttpResponse{200,Json{{"state","signed_in"},{"token",token},{"expires_in",28800},
        {"identity",{{"id",subject},{"username","Native fixture"},{"display_name","Native fixture"}}}}.dump()};
    });
    OnlineMatchmakingOptions options;options.endpoint=base;options.allow_loopback_http=true;
    options.compatibility="halo2-native-queue-v16";options.poll_interval=500ms;
    std::istringstream maps(input("H2_HANDOFF_MAPS"));
    for(std::string map;std::getline(maps,map,',');)options.maps.push_back(map);
    OnlineMatchmaking matchmaking(login,options);login->start();
    bool refreshed=false,queued=false;std::shared_ptr<OnlineMatchSession> session;
    const auto deadline=std::chrono::steady_clock::now()+75s;
    stage="native provider handoff";
    while(std::chrono::steady_clock::now()<deadline){
      const auto identity=login->snapshot();
      if(identity.status==DiscordLoginStatus::Failed){report["provider_message"]=identity.message;throw std::runtime_error("Native provider handoff refused");}
      if(identity.identity&&!refreshed){matchmaking.refresh(input("H2_HANDOFF_NAME"));refreshed=true;stage="native session/catalog";}
      const auto state=matchmaking.snapshot();report["queue_status"]=unsigned(state.status);
      if(state.match){
        const auto &match=*state.match;
        require(match.settings.has_value(),"Native handoff omitted complete authored settings");
        const auto row=std::find_if(expected.begin(),expected.end(),[&](const auto &r){return r.at("id")==match.game_id;});
        require(row!=expected.end(),"Native handoff selected an unknown source game");
        require(row->at("map")==match.map&&row->at("variant")==match.variant,"Native handoff changed map/variant identity");
        const auto raw=row->at("settings").template get<std::string>();require(raw.size()==608,"Expected settings extent changed");
        for(unsigned i=0;i<304;++i)require(match.settings->bytes[i]==std::stoul(raw.substr(2*i,2),nullptr,16),"Native handoff changed authored settings bytes");
        report["full_settings_validated"]=true;report["settings_bytes"]=304;
        report["game_id"]=match.game_id;report["map"]=match.map;report["variant"]=match.variant;
      }
      if(state.status==OnlineMatchmakingStatus::Failed){report["native_error"]=state.message;throw std::runtime_error("Native queue/handoff failed");}
      if(state.status==OnlineMatchmakingStatus::Ready&&!queued){
        const auto row=std::find_if(state.playlists.begin(),state.playlists.end(),[&](const auto &p){return p.id==playlist;});
        require(row!=state.playlists.end()&&row->eligible,"Source playlist is unavailable to the native v16 client");
        matchmaking.join(playlist);queued=true;stage="native search/QoS/descriptor";
      }
      if(!session)session=matchmaking.take_launch();
      if(session){
        report["native_session_taken"]=true;report["transport_phase"]=unsigned(session->phase());
        require(session->direct_udp(),"Full-settings handoff changed its admitted transport");
        stage="native EOS/loading";
        if(session->phase()==OnlineMatchSession::Phase::Failed){report["native_error"]=session->error();throw std::runtime_error("Native admitted transport failed");}
        if(session->phase()==OnlineMatchSession::Phase::Loading){
          require(report["full_settings_validated"]==true,"Native loading omitted immutable settings validation");
          report["native_loading_reached"]=true;report["stage"]=stage;
          std::cout<<report.dump()<<'\n';return 0;
        }
      }
      std::this_thread::sleep_for(10ms);
    }
    throw std::runtime_error("Native handoff deadline expired");
  }catch(const std::exception &error){report["stage"]=stage;report["error"]=error.what();std::cout<<report.dump()<<'\n';return 1;}
}
