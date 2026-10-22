#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "helpers/multiplayer_snapshot_checks.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
struct Files {
  std::filesystem::path path;
  explicit Files(unsigned index) {
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Engine network flow requires owned scratch");
    path=std::filesystem::path(scratch)/("engine-network-"+std::to_string(index));std::filesystem::create_directories(path);
  }
  ~Files(){std::error_code error;std::filesystem::remove_all(path,error);}
};
}
int main(int argc,char** argv)try {
  std::cout<<std::unitbuf;
  check(argc==4 || argc==5,"Usage: multiplayer_engine_network_tu15_tests MCC_MAP VARIANTS REPORT [PRESET]");
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  Json fixture;std::ifstream(argv[2])>>fixture;
  Json report={{"passed",false},{"cases",Json::array()},{"protocol",native_multiplayer_protocol},
    {"scope","Actual canonical TU1.5 authority with two source-born local players and a real loopback-UDP participant. Complete received snapshots, original engine entity/relationship outputs and identities must match authority through join, ordinary frames, departure and a new late join. Authored variants remain unchanged. This is bounded native host/client integration, not Xbox wire interoperability, objective actions in network matches, visual/audio delivery or complete cross-machine matches."}};
  const auto save=[&]{std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot retain engine network evidence");};
  unsigned index{},failures{};
  for(const auto& preset:fixture.at("variants")) {
    const auto name=preset.at("name").get<std::string>();if(argc==5 && name!=argv[4])continue;
    Json row={{"name",name},{"passed",false},{"stage","construct"},{"joins",Json::array()}};
    try {
      Files files(index++);MultiplayerOptions options;options.map=argv[1];options.source_object_world=true;
      options.source_object_savegame_directory=files.path;options.local_players=2;
      options.variant={name,preset.at("frontend_engine").get<unsigned>(),{}};
      for(const auto& w:preset.at("writes"))options.variant.values.append(w[0].get<unsigned>(),w[1].get<int>());
      const auto variant=source_game_variant(options.variant);const bool teams=source_game_variant_get(variant,91)!=0;
      if(teams)options.selected_teams={0,1};
      MultiplayerPreparedWorld prepared(options);auto& match=prepared.match;
      check(match.source_engine_owned(),"Engine network selected a bounded authority");
      const auto fingerprint=0x7475313500000000ull+index;
      LanHost host(match,0,fingerprint,options.variant);
      const auto address="127.0.0.1:"+std::to_string(host.port());
      std::array<Controls,multiplayer_capacity> controls{};
      for(const auto slot:prepared.local_slots){controls[slot].yaw=match.unit(slot).yaw;controls[slot].pitch=match.unit(slot).pitch;}
      const std::array<std::int8_t,1> selected_team{0};
      const auto selected=teams?std::span<const std::int8_t>(selected_team):std::span<const std::int8_t>{};
      unsigned frames{},first_slot{};std::uint32_t first_player=null_datum,first_unit=null_datum;
      const auto advance=[&] {host.receive(controls);match.step(controls);++frames;host.broadcast();};
      const auto verify=[&](LanClient& client) {
        check(client.ready() && client.error().empty(),"Canonical engine UDP participant is not ready");
        const auto expected=match.state();const auto& actual=client.state();
        check(actual.source_engine_entity.has_value() && expected.source_engine_entity==actual.source_engine_entity,
            "Original engine entity/relationship output changed over UDP");
        check(!actual.defusal && !actual.defusal_rules,"Original Assault was decoded as bounded custom Defusal");
        test::verify_multiplayer_snapshot(expected,actual);
      };
      const auto connect=[&](LanClient& client,unsigned number) {
        std::array<Controls,1> local{};unsigned polls{};
        row["stage"]="join-"+std::to_string(number);
        while(polls<80 && !client.ready() && client.error().empty()) {
          client.send(local);advance();client.receive();++polls;
          if(!client.ready())std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if(!client.error().empty())throw FormatError("Canonical engine UDP admission: "+client.error());
        verify(client);check(client.slots().size()==1,"Canonical engine UDP admitted the wrong local roster size");
        const auto slot=client.slots()[0];
        check(slot<multiplayer_capacity && match.player(slot).connected,"UDP peer has no actual native/source roster record");
        for(unsigned n=0;n<6;++n) {
          client.send(local);advance();client.receive();verify(client);
        }
        unsigned factory_frames{};Json waits=Json::array();
        while(factory_frames<240 && match.player(slot).source_current_object==null_datum && !match.finished()) {
          const auto& p=match.player(slot);
          if(factory_frames<3 || factory_frames%30==0)waits.push_back({{"frame",factory_frames},{"tick",match.state().tick},
              {"flags",p.source_flags},{"lives",p.source_lives_remaining},{"respawn_countdown",p.respawn.countdown}});
          client.send(local);advance();client.receive();verify(client);++factory_frames;
        }
        row["last_factory_wait"]={{"slot",slot},{"frames",factory_frames},{"samples",std::move(waits)}};
        const auto& player=match.player(slot);
        check(player.source_current_object!=null_datum && !client.state().players[slot].dead,
            "Admitted engine participant did not acquire a genuine source current unit");
        row["joins"].push_back({{"number",number},{"slot",slot},{"polls",polls},{"tick",match.state().tick},
            {"source_player",player.source_datum},{"source_unit",player.source_current_object},
            {"factory_frames",factory_frames},{"entity_bytes",client.state().source_engine_entity->bytes.size()}});
        return slot;
      };
      {
        LanClient client(address,fingerprint,1,selected);first_slot=connect(client,1);
        first_player=match.player(first_slot).source_datum;first_unit=match.player(first_slot).source_current_object;
      }
      row["stage"]="ordinary-peer-departure";unsigned polls{};
      while(polls<80 && !(match.player(first_slot).source_flags&2u)) {
        advance();++polls;if(!(match.player(first_slot).source_flags&2u))std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      const auto removed=host.last_peer_removal();
      check(removed && removed->reason==LanHost::PeerRemovalReason::authenticated_leave &&
          (removed->slot_mask&(1u<<first_slot)) && host.peers()==0,
          "Canonical engine UDP leave did not complete its authenticated peer removal");
      check(match.player(first_slot).source_datum==first_player && (match.player(first_slot).source_flags&2u) &&
          match.player(first_slot).source_current_object==null_datum,
          "Canonical engine UDP leave lost its original retained row/current-unit release");
      row["departure"]={{"slot",first_slot},{"retained_player",first_player},{"flags",match.player(first_slot).source_flags},
          {"source_unit",match.player(first_slot).source_current_object},{"removal_ordinal",removed->ordinal}};
      {
        LanClient client(address,fingerprint,1,selected);const auto slot=connect(client,2);
        check(match.player(slot).source_datum!=first_player && match.player(slot).source_current_object!=first_unit,
            "Late engine join aliased its retired player/current-unit generation");
      }
      for(unsigned n=0;n<4;++n)advance();
      row["ordinary_frames"]=frames;row["passed"]=true;row["stage"]="complete";
    }catch(const std::exception& error){row["error"]=error.what();++failures;}
    std::cout<<row.dump()<<'\n';report["cases"].push_back(std::move(row));save();
  }
  check(index==(argc==5?1u:39u),"Engine network preset coverage changed");report["passed"]=failures==0;save();return failures?1:0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
