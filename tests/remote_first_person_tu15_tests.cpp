#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_player_profile.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
struct Files {
  std::filesystem::path path;
  Files(){auto name=(std::filesystem::temp_directory_path()/"halo2-remote-first-person-XXXXXX").string();
    check(::mkdtemp(name.data()),"Cannot create private source files");path=name;}
  ~Files(){std::error_code error;std::filesystem::remove_all(path,error);}
};
}
// A remote TU1.5 player's first person exists on no native world of the host.
// The host's native clock kernels animate its replicated view from the
// melee/fire activities the TU1.5 world raised, and every snapshot carries
// that clock, so a non-host viewmodel animates like an offline one.
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;if(argc!=3||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
  Files files;MultiplayerOptions options;options.gameplay_xbe=argv[1];options.map=argv[2];options.starting_weapon=5;
  options.local_players=1;options.source_object_world=true;options.source_object_savegame_directory=files.path;
  MultiplayerPreparedWorld prepared(options);auto &match=prepared.match;
  const auto fp=prepared.cache->load_first_person_binding(prepared.rules.weapon,2);
  match.configure_replicated_first_person(fp);match.configure_local_views(prepared.local_slots,fp);
  constexpr std::uint64_t fingerprint=0x15de0000f;
  LanHost host(match,0,fingerprint,options.variant);
  const auto defaults=source_default_player_configuration();std::array<std::uint8_t,9> appearance;
  std::copy_n(defaults.begin()+0x40,appearance.size(),appearance.begin());
  const std::array<SourcePlayerConfiguration,1> profile{native_selected_player_configuration("Remote",appearance,-1)};
  LanClient client(host.join_address(),fingerprint,1,{},profile);
  std::array<Controls,16> controls{};Controls remote{};
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(8);
  while(!client.ready()&&std::chrono::steady_clock::now()<deadline) {
    client.send(std::span(&remote,1));host.receive(controls);host.broadcast();client.receive();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  check(client.ready(),"Remote TU1.5 client did not join");
  const auto slot=client.slots()[0];
  std::vector<std::uint32_t> activities;bool clock_seen{};
  const auto tick=[&](Controls input) {
    client.send(std::span(&input,1));host.receive(controls);
    for(unsigned i=0;i<16;++i)if(i!=slot)controls[i].yaw=match.unit(i).yaw;
    match.step(controls);host.broadcast();client.receive();
    const auto &player=client.state().players[slot];
    if(player.first_person_clock.present) {
      clock_seen=true;
      const auto activity=player.first_person_clock.current.activity;
      if(activities.empty()||activities.back()!=activity)activities.push_back(activity);
    }
  };
  for(unsigned n=0;n<60;++n)tick(remote);
  check(clock_seen,"Remote TU1.5 view published no first-person clock");
  Controls melee=remote;melee.melee=true;tick(melee);
  std::uint32_t melee_activity=null_datum;
  for(unsigned n=0;n<45;++n) {
    tick(remote);
    if(match.unit(slot).melee_action.active())melee_activity=match.unit(slot).melee_action.activity;
  }
  check(melee_activity!=null_datum,"Remote melee did not run in the TU1.5 world");
  check(std::ranges::find(activities,melee_activity)!=activities.end(),"Remote first-person clock never played the melee");
  Controls fire=remote;fire.fire=true;
  for(unsigned n=0;n<20;++n)tick(fire);
  for(unsigned n=0;n<10;++n)tick(remote);
  check(std::ranges::find(activities,0x06000006u)!=activities.end(),"Remote first-person clock never played firing");
  std::cout<<"PASS remote TU1.5 first person: native clock kernels drive "<<activities.size()
      <<" replicated activity changes (melee "<<std::hex<<melee_activity<<std::dec<<", fire) over actual UDP\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
