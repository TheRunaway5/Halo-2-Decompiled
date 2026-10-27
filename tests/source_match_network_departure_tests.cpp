#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_player_profile.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
struct Files {
  std::filesystem::path path;
  Files(){auto name=(std::filesystem::temp_directory_path()/"halo2-native-network-departure-XXXXXX").string();
    check(::mkdtemp(name.data()),"Cannot create private source network files");path=name;}
  ~Files(){std::error_code error;std::filesystem::remove_all(path,error);}
};
SourcePlayerConfiguration profile(std::string_view name) {
  const auto defaults=source_default_player_configuration();std::array<std::uint8_t,9> appearance;
  std::copy_n(defaults.begin()+0x40,appearance.size(),appearance.begin());
  return native_selected_player_configuration(name,appearance,-1);
}
std::uint32_t count(MultiplayerObjectRuntime &world) {
  auto &bank=world.memory();std::uint32_t census{},active{};
  bank.read(0x4f54f0u,std::as_writable_bytes(std::span(&census,1)));
  bank.read(census,std::as_writable_bytes(std::span(&active,1)));return active;
}
}
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;if(argc!=3||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
  Files files;MultiplayerOptions options;options.gameplay_xbe=argv[1];options.map=argv[2];options.starting_weapon=5;
  options.local_players=2;options.source_object_world=true;options.source_object_savegame_directory=files.path;
  MultiplayerPreparedWorld prepared(options);auto &match=prepared.match;auto &world=*prepared.source_objects;
  const auto fp=prepared.cache->load_first_person_binding(prepared.rules.weapon,2);
  match.configure_replicated_first_person(fp);match.configure_local_views(prepared.local_slots,fp);
  // Declared native-protocol fingerprint, not a retail packet or live ONLINE
  // admission. Every simulated remote consumes this one actual source world.
  constexpr std::uint64_t fingerprint=0x15de00004;
  LanHost host(match,0,fingerprint,options.variant);
  const auto address=host.join_address();
  const std::span<const std::int8_t> teams{};
  const std::array<SourcePlayerConfiguration,1> leaving_profile{profile("Leaves normally")},
      silent_profile{profile("Goes silent")},survivor_profile{profile("Still playing")};
  auto leaving=std::make_unique<LanClient>(address,fingerprint,1,teams,leaving_profile);
  auto silent=std::make_unique<LanClient>(address,fingerprint,1,teams,silent_profile);
  LanClient survivor(address,fingerprint,1,teams,survivor_profile);
  std::array<Controls,16> controls{};Controls remote{};
  const auto ready_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(8);
  while(!(leaving->ready()&&silent->ready()&&survivor.ready())&&std::chrono::steady_clock::now()<ready_deadline) {
    leaving->send(std::span(&remote,1));silent->send(std::span(&remote,1));survivor.send(std::span(&remote,1));
    host.receive(controls);leaving->receive();silent->receive();survivor.receive();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  if(!(leaving->ready()&&silent->ready()&&survivor.ready()))
    throw FormatError("Source UDP admission failed: "+leaving->error()+"; "+silent->error()+"; "+survivor.error());
  check(host.peers()==3&&count(world)==5,"Real source UDP roster did not admit five actual players");
  const auto leaving_slot=leaving->slots()[0],silent_slot=silent->slots()[0],survivor_slot=survivor.slots()[0];
  // Native admission does not manufacture the original deferred biped birth.
  // Execute ordinary real frames and preserve every actual UDP participant
  // until the departing player owns its genuine source unit/body.
  const auto birth_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(8);
  while((!match.unit(leaving_slot).exists || match.unit(leaving_slot).source_datum==null_datum ||
      match.player(leaving_slot).source_current_object!=match.unit(leaving_slot).source_datum) &&
      std::chrono::steady_clock::now()<birth_deadline) {
    for(unsigned i=0;i<5;++i)controls[i].yaw=match.unit(i).yaw;
    leaving->send(std::span(&remote,1));silent->send(std::span(&remote,1));survivor.send(std::span(&remote,1));
    host.receive(controls);match.step(controls);host.broadcast();
    leaving->receive();silent->receive();survivor.receive();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  const auto left=match.player(leaving_slot),timed_out=match.player(silent_slot);
  const auto source_unit=match.unit(leaving_slot).source_datum;
  check(match.unit(leaving_slot).exists && source_unit!=null_datum && left.source_current_object==source_unit,
      "Original ordinary frames did not publish the departing participant's actual source unit");
  check(!world.object_body(source_unit).is_null(),"Departing participant lacks its actual original body owner");
  leaving.reset(); // A real authenticated native leave datagram.
  const auto leave_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
  while(host.peers()!=2&&std::chrono::steady_clock::now()<leave_deadline) {
    survivor.send(std::span(&remote,1));host.receive(controls);survivor.receive();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  check(host.peers()==2&&(match.player(leaving_slot).source_flags&2u)&&count(world)==4,
      "Real leave datagram deleted/retained the wrong active source participant");
  const auto start=match.state().tick;const auto timeout_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(7);
  unsigned ticks{};
  do {
    controls={};for(unsigned i=0;i<5;++i)controls[i].yaw=match.unit(i).yaw;
    survivor.send(std::span(&remote,1));host.receive(controls);match.step(controls);host.broadcast();survivor.receive();++ticks;
    std::this_thread::sleep_for(std::chrono::milliseconds(33));
  }while(host.peers()!=1&&std::chrono::steady_clock::now()<timeout_deadline);
  check(host.peers()==1&&count(world)==3&&(match.player(silent_slot).source_flags&2u),
      "A silent UDP peer did not take the original retained departure path at the actual timeout");
  for(unsigned i=0;i<4;++i) {
    survivor.send(std::span(&remote,1));host.receive(controls);match.step(controls);host.broadcast();survivor.receive();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  check(!match.finished()&&survivor.ready()&&survivor.error().empty()&&survivor.state().tick>start+10&&ticks>10,
      "An ordinary source departure stopped the remaining real native UDP match");
  for(const auto [slot,before]:{std::pair{leaving_slot,left},std::pair{silent_slot,timed_out}}) {
    const auto &p=match.player(slot);const auto &row=survivor.state().players[slot];
    check(p.connected&&p.source_datum==before.source_datum&&p.name==before.name&&
        p.source_profile_request==before.source_profile_request&&p.deaths==before.deaths&&p.score==before.score&&
        row.connected&&row.dead&&(row.source_flags&2u)&&row.source_player==p.source_datum&&
        row.name==p.name&&row.deaths==p.deaths&&row.source_unit==null_datum&&row.source_weapon==null_datum,
        "The remaining UDP peer lost a retained departed identity/profile/result row");
  }
  const auto final_host=match.state();
  std::cerr<<"DEPARTURE_DIAGNOSTICS expected_unit="<<source_unit<<" expected_player="<<left.source_datum
      <<" host_tick="<<final_host.tick<<" peer_tick="<<survivor.state().tick
      <<" survivor_host_player="<<match.player(survivor_slot).source_datum
      <<" survivor_peer_player="<<survivor.state().players[survivor_slot].source_player
      <<" host_corpses="<<final_host.corpses.size()<<" peer_corpses="<<survivor.state().corpses.size()
      <<" host_expected_corpse="<<std::ranges::any_of(final_host.corpses,[&](const auto &c){return c.object==source_unit&&c.player==left.source_datum;})
      <<" peer_expected_corpse="<<std::ranges::any_of(survivor.state().corpses,[&](const auto &c){return c.object==source_unit&&c.player==left.source_datum;})<<'\n';
  check(survivor.state().players[survivor_slot].source_player==match.player(survivor_slot).source_datum&&
      std::ranges::any_of(survivor.state().corpses,[&](const auto &c){return c.object==source_unit&&c.player==left.source_datum;}),
      "Survivor changed identity or lost the original departing unit's corpse");
  std::cout<<"PASS actual TU1.5 native UDP departure: two local source players, three real UDP peers, graceful leave plus actual five-second silent-peer timeout, "
      <<ticks<<" live source ticks, retained original identities/profiles/results/corpse rows, no added numerical deaths and a surviving peer\n";
  std::cout<<"LIMIT native protocol localhost/source match; master room/report arbitration, custom native-owned modes, source cancellation and physical/WAN/voice acceptance remain separate\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
