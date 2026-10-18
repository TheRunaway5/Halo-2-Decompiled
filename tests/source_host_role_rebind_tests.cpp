#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/source_host_continuation_codec.hpp"
#include "halo2/source_authority_binding.hpp"
#include <algorithm>
#include <deque>
#include <iostream>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
template<class F>void rejects(F run){bool rejected{};try{run();}catch(const std::exception &){rejected=true;}
  check(rejected,"Invalid host role transition was accepted");}
struct FixedClock final:PhysicsLiveClockSource {
  std::uint32_t milliseconds()const noexcept override{return 1000;}
  std::uint64_t cycles()const noexcept override{return 7000000;}
  std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
// Sequential logical adapter, with the accepted generation supplied separately
// by the fixture. This does not authenticate an election or prove WAN delivery.
struct Bus {struct Packet {unsigned from;std::vector<std::byte> bytes;};std::array<std::deque<Packet>,16> queues;};
struct Endpoint final:MultiplayerDatagrams {
  std::shared_ptr<Bus> bus;unsigned self;
  Endpoint(std::shared_ptr<Bus> b,unsigned s):bus(std::move(b)),self(s){}
  void send(std::span<const std::byte> p,unsigned to)override{check(to<16,"Invalid peer endpoint");bus->queues[to].push_back({self,{p.begin(),p.end()}});}
  bool receive(std::vector<std::byte> &p,unsigned &from)override{auto &q=bus->queues[self];if(q.empty())return false;
    from=q.front().from;p=std::move(q.front().bytes);q.pop_front();return true;}
};
SourcePlayerConfiguration profile(std::string_view name){auto defaults=source_default_player_configuration();std::array<std::uint8_t,9> appearance;
  std::copy_n(defaults.begin()+0x40,appearance.size(),appearance.begin());return native_selected_player_configuration(name,appearance,-1);}
}
int main(int argc,char **argv)try{
  std::cout<<std::unitbuf;if(argc!=3||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
  MultiplayerOptions options;options.gameplay_xbe=argv[1];options.map=argv[2];options.starting_weapon=5;
  options.local_players=1;options.source_object_world=true;options.variant={"Slayer",0,{{0x59,5},{10,0},{2,0}}};
  MultiplayerPreparedWorld producer(options,std::nullopt,std::make_shared<FixedClock>());producer.bind_source_local_views(producer.local_slots);
  SourceMatchAuthority authority(producer.match,*producer.source_objects);
  constexpr std::uint64_t fingerprint=0x15c071500;
  std::vector<OnlineAdmission> roster;for(unsigned i=0;i<8;++i)
    roster.push_back({i,"Role seat "+std::to_string(i),-1,{},i==0?0u:i<5?1u:5u,i!=0&&i!=1&&i!=5});
  auto bus=std::make_shared<Bus>();auto host=std::make_unique<LanHost>(producer.match,std::make_shared<Endpoint>(bus,0),fingerprint,roster);
  const std::array profiles{profile("Local 1"),profile("Local 2"),profile("Local 3"),profile("Local 4")};
  const std::array remote_profiles{profile("Remote 1"),profile("Remote 2"),profile("Remote 3")};
  auto promoted=std::make_unique<LanClient>(std::make_shared<Endpoint>(bus,1),0,fingerprint,std::span<const std::int8_t>{},profiles);
  LanClient remote(std::make_shared<Endpoint>(bus,5),0,fingerprint,std::span<const std::int8_t>{},remote_profiles);
  std::array<Controls,4> local{};std::array<Controls,3> other{};std::array<Controls,16> commands{};
  for(unsigned n=0;n<32&&!(promoted->ready()&&remote.ready());++n){promoted->send(local);remote.send(other);host->receive(commands);host->broadcast();promoted->receive();remote.receive();}
  check(host->peers()==2&&promoted->ready()&&remote.ready(),"Eight-player native admission did not complete");
  AuthorityTransferContext before;before.session.front()=std::byte{0x71};before.epoch=3;before.membership_revision=7;
  before.peers=(1u<<0)|(1u<<1)|(1u<<5);before.host=0;
  auto after=before;++after.epoch;++after.membership_revision;after.peers&=~1u;after.host=1;
  const auto old_binding=source_authority_binding(before),new_binding=source_authority_binding(after);
  rejects([&]{remote.rebind_host(before,after,{1,0,1,{std::byte{1}}});});
  auto invalid_roster=roster;invalid_roster.back().slot=15;
  rejects([&]{remote.bind_continuity(before,5,invalid_roster);});
  remote.bind_continuity(before,5,roster);rejects([&]{remote.bind_continuity(before,5,roster);});
  for(unsigned i=0;i<4;++i)local[i].yaw=producer.match.unit(i+1).yaw;
  for(unsigned i=0;i<3;++i)other[i].yaw=producer.match.unit(i+5).yaw;
  for(unsigned packet=0;packet<3;++packet){for(auto &c:local){c.grenade=c.melee=true;c.pickup=packet<2;c.pickup_milliseconds=250;}
    promoted->send(local);remote.send(other);}
  commands={};commands[0].yaw=producer.match.unit(0).yaw;host->receive(commands);
  check(commands[1].grenade&&commands[1].melee&&commands[1].pickup,"Predecessor did not consume the first queued pulse");
  producer.match.step(commands);host->broadcast();promoted->receive();remote.receive();
  const auto captured=capture_source_host_continuation(*host,before,1);const auto native=encode_source_authority(captured.world);
  const auto admitted=authority.player_admissions(std::array{0u,1u,2u,3u,4u,5u,6u,7u});
  const auto wire=encode_source_host_continuation(captured);
  for(unsigned tick=0;tick<3;++tick){commands={};for(unsigned i=0;i<8;++i)commands[i].yaw=producer.match.unit(i).yaw;
    producer.match.step(commands);host->broadcast();promoted->receive();remote.receive();}
  check(remote.state().tick>captured.world.match.simulation.ticks,"Client rollback precondition was not reached");
  // Save old-host snapshots as stale traffic. They must not publish after the
  // independently accepted host endpoint changes.
  host->broadcast();auto stale=bus->queues[5];bus->queues[5].clear();
  promoted.reset();host.reset();bus->queues[0].clear();bus->queues[1].clear();
  std::exception_ptr failure;
  {std::jthread executor([&]{try{
    MultiplayerPreparedWorld target(options,std::nullopt,std::make_shared<FixedClock>());target.bind_source_local_views(target.local_slots);
    SourceMatchAuthority imported(target.match,*target.source_objects);
    const auto received=decode_source_host_continuation(wire,before,fingerprint,roster,admitted);
    auto prepared=imported.prepare(received.world,old_binding,admitted);check(imported.commit(std::move(prepared)),"Cannot import the complete native predecessor world");
    const auto endpoint=std::make_shared<Endpoint>(bus,1);
    rejects([&]{LanHost::resume_after_host_loss(target.match,endpoint,fingerprint,roster,received.host,before,after,admitted);});
    target.bind_source_local_views(std::array{1u,2u,3u,4u});
    const auto rebound=encode_source_authority(imported.checkpoint(old_binding));
    auto invalid=after;invalid.epoch=before.epoch;
    rejects([&]{LanHost::resume_after_host_loss(target.match,endpoint,fingerprint,roster,received.host,before,invalid,admitted);});
    invalid=after;invalid.session.back()^=std::byte{1};
    rejects([&]{LanHost::resume_after_host_loss(target.match,endpoint,fingerprint,roster,received.host,before,invalid,admitted);});
    invalid=after;invalid.peers=before.peers;
    rejects([&]{LanHost::resume_after_host_loss(target.match,endpoint,fingerprint,roster,received.host,before,invalid,admitted);});
    auto bad=admitted;bad[1].identity.front()^=0x80;
    rejects([&]{LanHost::resume_after_host_loss(target.match,endpoint,fingerprint,roster,received.host,before,after,bad);});
    auto changed=roster;changed[5].name="Unadmitted";
    rejects([&]{LanHost::resume_after_host_loss(target.match,endpoint,fingerprint,changed,received.host,before,after,admitted);});
    auto migrated=LanHost::resume_after_host_loss(target.match,endpoint,fingerprint,roster,received.host,before,after,admitted);
    check(migrated.host->peers()==1&&migrated.local.slots==std::vector<unsigned>({1,2,3,4}),"Role transition did not transfer the promoted four-seat bank");
    check(encode_source_authority(imported.checkpoint(old_binding))==rebound,"Host role transition mutated native simulation or local view state");
    check(migrated.local.camera.has_value(),"Promoted couch camera owner was lost");
    NativeVehicleCamera camera(*target.source_objects,*target.content.original_data);
    auto cp=camera.prepare(*migrated.local.camera,new_binding);check(camera.commit(std::move(cp)),"Promoted local native camera failed to restore");
    rejects([&]{camera.prepare(*migrated.local.camera,old_binding);});
    const auto receipt=AuthorityCutSeal{1,captured.world.match.simulation.ticks,native.size(),{std::byte{1}}};
    auto wrong=after;wrong.host=5;rejects([&]{remote.rebind_host(before,wrong,receipt);});
    rejects([&]{remote.rebind_host(before,after,{});});remote.rebind_host(before,after,receipt);
    rejects([&]{remote.rebind_host(before,after,receipt);});
    bus->queues[5]=stale;remote.receive();check(remote.received_states().empty(),"Old-host snapshots survived endpoint rebind");
    const auto old_deaths=target.match.player(0).deaths;const auto old_player=target.match.player(0).source_datum;
    target.match.depart(0);check(target.match.player(0).deaths==old_deaths&&target.match.player(0).source_datum==old_player,"Host loss fabricated a death or replaced the original result identity");
    const auto start=target.match.simulation().ticks();
    for(unsigned tick=0;tick<12;++tick){std::array<Controls,16> input{};
      LanHost::receive_local_continuation(migrated.local,input);
      for(unsigned slot=1;slot<5;++slot)check(input[slot].grenade==(tick<2)&&input[slot].melee==(tick<2)&&!input[slot].pickup,
          "Promoted local controls lost/repeated queued pulses or a completed interaction");
      remote.send(other);migrated.host->receive(input);target.match.step(input);migrated.host->broadcast();remote.receive();
      check(remote.error().empty()&&remote.ready()&&remote.state().tick==target.match.simulation().ticks(),"Surviving native client stopped following the replacement host");
      check(std::ranges::equal(remote.slots(),std::array{5u,6u,7u}),"Surviving couch connection changed accepted slots");
      for(unsigned slot=0;slot<8;++slot)check(target.match.player(slot).source_datum==admitted[slot].player,"Migration replaced a full native player allocation");
    }
    check(target.match.simulation().ticks()==start+12&&!target.match.finished(),"Migrated player-hosted match did not survive");
    const auto renewed=migrated.host->checkpoint(new_binding);const auto world=imported.checkpoint(new_binding);
    check(!encode_source_host_continuation({after,2,world,renewed}).empty(),"New host cannot export its successor world while retaining the departed host PCR row");
  }catch(...){failure=std::current_exception();}});}
  if(failure)std::rethrow_exception(failure);
  std::cout<<"PASS native host-loss role rebind: eight admitted players, four promoted local seats, three surviving client seats, preserved input pulses/cookies/snapshots/cameras, old endpoint rejection, rollback to accepted cut, native departure/results and twelve subsequent connected ticks\n";
  std::cout<<"LIMIT sequential logical transport and explicitly supplied generation decision; no election, live DTLS epoch fence, runtime controller/headset or automatic migration acceptance\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
