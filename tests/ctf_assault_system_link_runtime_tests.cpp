#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/system_link_session.hpp"
#include "halo2/frontend_session.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <future>
#include <iostream>
#include <thread>
using namespace halo2;
using namespace std::chrono_literals;
namespace {void need(bool v,const char* why){if(!v)throw FormatError(why);}}
int main(int argc,char** argv)try {
  need(argc==4,"Supply MCC_MAP ORIGINAL_PRESETS REPORT");
  const std::filesystem::path map=argv[1];if(!std::filesystem::is_regular_file(map))return 77;
  const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Native runtime return needs owned scratch");
  nlohmann::json fixture;std::ifstream(argv[2])>>fixture;
  const auto& preset=*std::ranges::find_if(fixture.at("variants"),[](const auto& row){return row.at("name")=="Multi Flag CTF";});
  UiVariant variant;variant.name=preset.at("name");variant.engine=preset.at("frontend_engine");
  for(const auto& write:preset.at("writes"))variant.values.append(write.at(0),write.at(1));
  FrontendAppearance appearance;const auto bytes=std::span<const std::uint8_t,9>(reinterpret_cast<const std::uint8_t*>(&appearance),9);
  const auto local=native_selected_player_configuration("Runtime Host",bytes,0),remote=native_selected_player_configuration("Runtime Peer",bytes,1);
  MultiplayerOptions options;options.map=map;options.shared_maps=map.parent_path();options.variant=variant;options.ticks=1800;options.mute=true;
  options.local_players=1;options.selected_profiles={local};options.selected_teams={0};options.source_object_world=true;
  options.source_object_savegame_directory=std::filesystem::path(scratch)/"runtime-return";
  auto session=std::make_shared<SystemLinkSession>(true);options.system_link_session=session;
  auto world=std::make_shared<MultiplayerPreparedWorld>(options);options.prepared_world=world;
  const auto fingerprint=custom_slayer_fingerprint(multiplayer_fingerprint(map,map.parent_path(),0,world->network_starting_weapon_identity()),variant);
  std::promise<std::string> address;auto ready=address.get_future();std::exception_ptr client_error;std::shared_ptr<SystemLinkSession> peer;
  std::jthread worker([&](std::stop_token stop){try {
    need(ready.wait_for(30s)==std::future_status::ready,"Runtime did not publish its real LAN host key");
    peer=std::make_shared<SystemLinkSession>(false);auto client=peer->enter_client(ready.get(),fingerprint,1,std::array<std::int8_t,1>{1},std::array<SourcePlayerConfiguration,1>{remote});
    const auto start=std::chrono::steady_clock::now();
    while(!stop.stop_requested()) {
      need(std::chrono::steady_clock::now()-start<90s,"Runtime peer exceeded finite native match budget");
      std::array<Controls,1> input{};
      const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
      input[0].pickup=(ms/500)%2==0;input[0].pickup_milliseconds=unsigned(ms%500);
      client->send(input);client->receive();if(client->ready())peer->world_update(client->state());
      if(peer->state() && peer->state()->phase==SystemLinkPhase::postgame){peer->retain({},std::move(client));return;}
      if(!client->error().empty())throw FormatError(client->error());std::this_thread::sleep_for(5ms);
    }
  }catch(...){client_error=std::current_exception();}});
  options.session_lan_host_ready=[&](const auto& key){address.set_value(key);};
  // The normal runtime alone advances the match. These are declared approach
  // and goal position inputs, read from genuine registry/mapping owners.
  options.session_world_update=[&](const MultiplayerMatch& match) {
    if(match.state().finished || !match.player(1).connected || match.player(1).source_current_object==null_datum)return false;
    auto& objects=*world->source_objects;auto& source=objects.tu15_world();auto& memory=source.memory();
    const auto word=[&](auto p){return Bytes(memory.view(p,4)).u32(0);};
    auto query=source.prepare_source_call(std::array{1u});query.gp[0]=match.player(1).source_datum;source.run_actual(0x1a8fd0u,query);
    Vec3 position{};bool found{};
    if(query.gp[0]==null_datum) {
      const auto engine=word(0x4f63b8u);const auto count=word(engine+0xc18u);need(count<=8,"Runtime objective registry exceeds capacity");
      for(unsigned i=0;i<count;++i) {
        const auto item=word(engine+0xc1cu+i*16u);if(item==null_datum)continue;
        if(Bytes(memory.view(objects.object_body(item)+0x17eu,2)).u16(0)!=0)continue;
        position=objects.object_transform(item).position;position.x-=.2f;found=true;break;
      }
    } else {
      auto mapping=source.prepare_source_call(std::array{1u});source.run_actual(0x15b3b0u,mapping);
      const auto mapped=std::bit_cast<std::int32_t>(mapping.gp[0]);need(mapped>=0 && mapped<9,"Runtime original team mapping absent");
      const auto goal=std::bit_cast<std::int16_t>(Bytes(memory.view(word(0x52d710u)+0x14u+unsigned(mapped)*2u,2)).u16(0));
      const auto scenario=word(0x4ecc20u);need(goal>=0 && unsigned(goal)<word(scenario+0x118u),"Runtime original goal absent");
      const auto marker=Bytes(memory.view(word(scenario+0x11cu)+unsigned(goal)*32u,12));position={marker.f32(0),marker.f32(4),marker.f32(8)};found=true;
    }
    if(found)source.set_object_position(match.player(1).source_current_object,position);
    return false;
  };
  MultiplayerRuntimeOutcome outcome;const auto result=run_multiplayer(options,outcome);worker.request_stop();worker.join();if(client_error)std::rethrow_exception(client_error);
  need(result==0 && outcome.reason==MultiplayerExitReason::system_link_postgame && outcome.system_link_session==session && outcome.authoritative_banks && outcome.state && outcome.state->game_over_complete(),"Ordinary native runtime did not hand off completed System Link ownership");
  const std::weak_ptr<MultiplayerPreparedWorld> departed=world;options.prepared_world.reset();world.reset();need(departed.expired(),"Ordinary runtime handoff retained its native world");
  need(peer && peer->state()->phase==SystemLinkPhase::postgame,"Ordinary runtime peer lost its confirmed session");
  session->done();const auto end=std::chrono::steady_clock::now()+5s;
  while(peer->state()->phase!=SystemLinkPhase::lobby){session->pump();peer->pump();need(std::chrono::steady_clock::now()<end,"Ordinary runtime lobby continuation timed out");std::this_thread::sleep_for(2ms);}
  std::ofstream out(argv[3]);out<<nlohmann::json{{"passed",true},{"native_runtime",true},{"reason","system_link_postgame"},{"source_tick",outcome.state->tick},{"world_released",true},{"same_retained_transport",true},{"peer_followed_done",true},{"scope","Ordinary headless runtime, genuine native source match/UDP/controller inputs and goal positions; runtime alone advances ticks and closes its quiescent authority frame. No score/timer/winner/completion output supplied."}}.dump(2)<<'\n';
  session->close();peer->close();return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
