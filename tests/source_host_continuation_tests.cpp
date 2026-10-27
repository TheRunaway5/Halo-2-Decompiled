#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/source_host_continuation_codec.hpp"
#include <openssl/sha.h>
#include <algorithm>
#include <deque>
#include <iostream>
#include <thread>

using namespace halo2;
namespace {
void check(bool yes,const char *why){if(!yes)throw FormatError(why);}
template<class F>void rejects(F f){bool caught{};try{f();}catch(const FormatError &){caught=true;}
  check(caught,"Invalid host continuation was accepted");}
struct FixedClock final:PhysicsLiveClockSource {
  std::uint32_t milliseconds()const noexcept override{return 1000;}
  std::uint64_t cycles()const noexcept override{return 7000000;}
  std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
// Test adapter for the authenticated logical-endpoint interface. This does not
// assert encryption, NAT traversal, election or actual WAN delivery.
struct Bus {
  struct Packet {unsigned from{};std::vector<std::byte> bytes;bool operator==(const Packet &)const=default;};
  std::array<std::deque<Packet>,16> incoming;
};
struct Endpoint final:MultiplayerDatagrams {
  std::shared_ptr<Bus> bus;unsigned self;
  Endpoint(std::shared_ptr<Bus> b,unsigned s):bus(std::move(b)),self(s){}
  void send(std::span<const std::byte> bytes,unsigned peer)override {
    check(peer<16,"Fixture endpoint exceeds the physical peer bank");
    bus->incoming[peer].push_back({self,{bytes.begin(),bytes.end()}});
  }
  bool receive(std::vector<std::byte> &bytes,unsigned &peer)override {
    auto &q=bus->incoming[self];if(q.empty())return false;
    peer=q.front().from;bytes=std::move(q.front().bytes);q.pop_front();return true;
  }
};
SourcePlayerConfiguration profile(std::string_view name) {
  const auto defaults=source_default_player_configuration();std::array<std::uint8_t,9> appearance;
  std::copy_n(defaults.begin()+0x40,appearance.size(),appearance.begin());
  return native_selected_player_configuration(name,appearance,-1);
}
struct Frame {
  std::uint64_t before{},after{};
  SourceExecutionPhase phase;
  LanHost::Continuation host;
  std::deque<Bus::Packet> inputs;
  std::array<std::deque<Bus::Packet>,2> outputs;
};
auto hash(std::span<const std::byte> bytes){std::array<std::byte,32> result{};
  check(SHA256(reinterpret_cast<const unsigned char *>(bytes.data()),bytes.size(),reinterpret_cast<unsigned char *>(result.data())),"Cannot hash fixture cut");return result;}
void rehash(std::vector<std::byte> &bytes){const auto digest=hash(std::span<const std::byte>(bytes).subspan(44));
  std::copy(digest.begin(),digest.end(),bytes.begin()+12);}
std::size_t first_peer_idle(std::span<const std::byte> bytes){
  std::size_t cursor=44+2+47+56+32+8+8;
  const auto u=[&](unsigned width){check(cursor+width<=bytes.size(),"Fixture host field exceeds bytes");std::uint64_t v{};
    for(unsigned i=0;i<width;++i)v|=std::uint64_t(std::to_integer<unsigned>(bytes[cursor++]))<<(8*i);return v;};
  const auto configuration=u(4);cursor+=configuration+16;const auto seats=u(1);
  for(unsigned i=0;i<seats;++i){cursor+=1;const auto name=u(4);cursor+=name+4+8+4+1;if(u(1))cursor+=1;}
  const auto roster=u(1);cursor+=roster*(1+4+12);check(u(1)>0,"Fixture host has no peer");
  cursor+=1+8+8;const auto views=u(1);cursor+=views*2;
  for(unsigned i=0;i<views;++i)if(u(1))cursor+=std::tuple_size_v<SourcePlayerConfiguration>;
  cursor+=4+4;if(u(1))cursor+=68;cursor+=1;
  check(cursor+8<=bytes.size(),"Fixture host has no idle field");return cursor;
}
}
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;if(argc!=3||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
  // CTest selects H2_NET_PROFILE=complete for this exact complete-packet
  // contract. Adaptive delta sender ACK/baseline/bandwidth history belongs to
  // its separate transport owner and is not contained in the native sidecar.
  // Keep every packet-byte and complete source-world comparison strict.
  MultiplayerOptions options;options.gameplay_xbe=argv[1];options.map=argv[2];options.starting_weapon=5;
  options.local_players=1;options.source_object_world=true;
  MultiplayerPreparedWorld producer(options,std::nullopt,std::make_shared<FixedClock>());
  producer.bind_source_local_views(producer.local_slots);
  auto &match=producer.match;SourceMatchAuthority authority(match,*producer.source_objects);
  auto bus=std::make_shared<Bus>();auto wire=std::make_shared<Endpoint>(bus,0);
  constexpr std::uint64_t fingerprint=0x15c071000;
  std::vector<OnlineAdmission> roster;
  for(unsigned slot=0;slot<6;++slot)roster.push_back({slot,"Couch "+std::to_string(slot),-1,{},slot==0?0u:slot<5?1u:5u,slot>1&&slot<5});
  roster[0].matchmaking_level=1;roster[5].matchmaking_level=50;
  auto host=std::make_unique<LanHost>(match,wire,fingerprint,roster);
  const std::array profiles{profile("Remote one"),profile("Remote two"),profile("Remote three"),profile("Remote four")};
  LanClient couch(std::make_shared<Endpoint>(bus,1),0,fingerprint,{},profiles);
  LanClient other(std::make_shared<Endpoint>(bus,5),0,fingerprint);
  std::array<Controls,4> local{};Controls remote{};
  std::array<Controls,multiplayer_capacity> commands{};
  for(unsigned i=0;i<32&&!(couch.ready()&&other.ready());++i) {
    couch.send(local);other.send(std::span(&remote,1));host->receive(commands);host->broadcast();couch.receive();other.receive();
  }
  check(couch.ready()&&other.ready()&&host->peers()==2,"Native logical transport failed actual source admission");
  check(std::ranges::equal(couch.slots(),std::array{1u,2u,3u,4u})&&other.slots()[0]==5,
      "Host fixture did not preserve its admitted four-view remote ownership");
  for(unsigned view=0;view<4;++view)local[view].yaw=match.unit(couch.slots()[view]).yaw;
  remote.yaw=match.unit(5).yaw;
  // Three input datagrams coalesce before one native phase. Their pulse
  // backlog must survive, while consumed completions must never execute again.
  for(unsigned packet=0;packet<3;++packet) {
    for(unsigned view=0;view<4;++view) {
      local[view].jump=local[view].fire=local[view].reload=true;
      local[view].grenade=local[view].melee=local[view].switch_grenade=true;
      local[view].pickup=packet<2;local[view].pickup_milliseconds=std::uint16_t(150+packet*100);
    }
    couch.send(local);
  }
  commands={};commands[0].yaw=match.unit(0).yaw;host->receive(commands);
  check(commands[1].grenade&&commands[1].melee&&commands[1].pickup&&commands[1].pickup_milliseconds==250,
      "Real host did not latch completed interactions and queued pulse inputs");
  match.step(commands);host->broadcast();couch.receive();other.receive();
  AuthorityTransferContext context;context.session.front()=std::byte{0x71};context.epoch=3;context.membership_revision=7;
  context.peers=(1u<<0)|(1u<<1)|(1u<<5);context.host=0;const auto binding=source_authority_binding(context);
  const auto slots=std::array{0u,1u,2u,3u,4u,5u};
  const auto admitted=authority.player_admissions(slots);
  const auto captured=capture_source_host_continuation(*host,context,1);
  const auto &world=captured.world;const auto &sidecar=captured.host;
  const auto cut_bytes=encode_source_authority(world);
  const auto bundled=encode_source_host_continuation(captured);
  for(auto prefix:{std::size_t(0),std::size_t(43),std::size_t(151),bundled.size()-1})
    rejects([&]{decode_source_host_continuation(std::span(bundled).first(prefix),context,fingerprint,roster,admitted);});
  auto wrong_context=context;++wrong_context.membership_revision;
  rejects([&]{decode_source_host_continuation(bundled,wrong_context,fingerprint,roster,admitted);});
  auto changed_bundle=bundled;changed_bundle.at(151+80)^=std::byte{1};rehash(changed_bundle);
  rejects([&]{decode_source_host_continuation(changed_bundle,context,fingerprint,roster,admitted);});
  // Even a recomputed outer checksum/world hash cannot substitute a different
  // world's bytes beside the original host sidecar's content binding.
  std::uint32_t native_bytes{};for(unsigned i=0;i<4;++i)native_bytes|=std::uint32_t(std::to_integer<unsigned>(bundled.at(147+i)))<<(i*8);
  const auto substituted_hash=hash(std::span<const std::byte>(changed_bundle).subspan(151,native_bytes));
  std::copy(substituted_hash.begin(),substituted_hash.end(),changed_bundle.begin()+115);rehash(changed_bundle);
  rejects([&]{decode_source_host_continuation(changed_bundle,context,fingerprint,roster,admitted);});
  SourceAuthorityCodecLimits tight;tight.encoded_bytes=bundled.size()-1;
  rejects([&]{decode_source_host_continuation(bundled,context,fingerprint,roster,admitted,tight);});
  auto staging=source_authority_network_codec_limits();staging.staged_bytes=1;
  rejects([&]{decode_source_host_continuation(bundled,context,fingerprint,roster,admitted,staging);});
  rejects([&]{encode_source_host_continuation(captured,tight);});
  const LanHost::ContinuationBinding bound{context,{1,authority.execution_tick(),cut_bytes.size(),hash(cut_bytes)}};
  const auto wire_bytes=LanHost::encode_continuation(sidecar,bound);
  check(LanHost::encode_continuation(LanHost::decode_continuation(wire_bytes,bound,fingerprint,roster,admitted),bound)==wire_bytes,
      "Host continuation is not a canonical value round trip");
  for(unsigned prefix=0;prefix<wire_bytes.size();++prefix)
    rejects([&]{LanHost::decode_continuation(std::span(wire_bytes).first(prefix),bound,fingerprint,roster,admitted);});
  auto malformed=wire_bytes;malformed.back()^=std::byte{1};
  rejects([&]{LanHost::decode_continuation(malformed,bound,fingerprint,roster,admitted);});
  malformed=wire_bytes;malformed.push_back(std::byte{});
  rejects([&]{LanHost::decode_continuation(malformed,bound,fingerprint,roster,admitted);});
  malformed.resize(512*1024+1);
  rejects([&]{LanHost::decode_continuation(malformed,bound,fingerprint,roster,admitted);});
  auto other_binding=bound;++other_binding.context.epoch;
  rejects([&]{LanHost::decode_continuation(wire_bytes,other_binding,fingerprint,roster,admitted);});
  other_binding=bound;++other_binding.context.membership_revision;
  rejects([&]{LanHost::decode_continuation(wire_bytes,other_binding,fingerprint,roster,admitted);});
  other_binding=bound;other_binding.world.digest.back()^=std::byte{1};
  rejects([&]{LanHost::decode_continuation(wire_bytes,other_binding,fingerprint,roster,admitted);});
  rejects([&]{LanHost::decode_continuation(wire_bytes,bound,fingerprint+1,roster,admitted);});
  auto wrong_admission=admitted;wrong_admission.back().identity.front()^=0x80;
  rejects([&]{LanHost::decode_continuation(wire_bytes,bound,fingerprint,roster,wrong_admission);});
  auto changed_rank=roster;changed_rank.back().matchmaking_level=49;
  rejects([&]{LanHost::decode_continuation(wire_bytes,bound,fingerprint,changed_rank,admitted);});
  changed_rank=roster;changed_rank.back().matchmaking_level.reset();
  rejects([&]{LanHost::decode_continuation(wire_bytes,bound,fingerprint,changed_rank,admitted);});
  // Named protocol extents, independent of private C++ structure layout.
  constexpr unsigned configuration_at=44+2+47+56+32+8+8;
  std::uint32_t configuration_bytes{};for(unsigned i=0;i<4;++i)configuration_bytes|=std::uint32_t(std::to_integer<unsigned>(wire_bytes.at(configuration_at+i)))<<(i*8);
  const auto locals_at=configuration_at+4+configuration_bytes;
  malformed=wire_bytes;malformed.at(locals_at+16)=std::byte{17};rehash(malformed);
  rejects([&]{LanHost::decode_continuation(malformed,bound,fingerprint,roster,admitted);});
  malformed=wire_bytes;std::copy_n(malformed.begin()+locals_at,4,malformed.begin()+locals_at+4);rehash(malformed);
  rejects([&]{LanHost::decode_continuation(malformed,bound,fingerprint,roster,admitted);});
  const auto idle_at=first_peer_idle(wire_bytes);
  malformed=wire_bytes;std::fill_n(malformed.begin()+idle_at,8,std::byte{0xff});rehash(malformed);
  rejects([&]{LanHost::decode_continuation(malformed,bound,fingerprint,roster,admitted);});
  auto maximum_idle=wire_bytes;std::fill_n(maximum_idle.begin()+idle_at,7,std::byte{0xff});maximum_idle.at(idle_at+7)=std::byte{0x7f};rehash(maximum_idle);
  rejects([&]{host->checkpoint({});});
  {LanHost raw(match,0,fingerprint);rejects([&]{raw.checkpoint(binding);});}
  authority.capture_execution_phase([&]{rejects([&]{host->checkpoint(binding);});
    rejects([&]{capture_source_host_continuation(*host,context,1);});});
  std::exception_ptr thread_error;
  {std::jthread wrong([&]{try{
    rejects([&]{host->checkpoint(binding);});rejects([&]{host->receive(commands);});
    rejects([&]{host->broadcast();});rejects([&]{host->peers();});
    rejects([&]{capture_source_host_continuation(*host,context,1);});
  }catch(...){thread_error=std::current_exception();}});}
  if(thread_error)std::rethrow_exception(thread_error);
  std::vector<Frame> frames;
  for(unsigned phase=0;phase<8;++phase) {
    // Leave two queued grenade/melee edges to be consumed from the saved bank.
    // A later release and press exercise the restored receive/sequence baseline.
    if(phase==3||phase==4) {
      for(auto &c:local){c={};if(phase==4){c.fire=true;c.switch_weapon=true;c.dual_wield=true;c.dual_wield_milliseconds=400;}}
    }
    // Fresh keepalives keep this input-restoration case independent of test
    // machine speed and the separate real-time idle/departure policy.
    for(auto &c:local)c.grenade=c.melee=c.switch_grenade=false;
    couch.send(local);other.send(std::span(&remote,1));
    Frame f;f.inputs=bus->incoming[0];f.before=authority.execution_tick();
    f.phase=authority.capture_execution_phase([&] {
      commands={};commands[0].yaw=match.unit(0).yaw;host->receive(commands);
      check(commands[1].grenade==(phase<2)&&commands[1].melee==(phase<2),"Queued pulse was lost or dispatched twice");
      check(!commands[1].pickup,"Consumed interaction was repeated after the native cut");
      match.step(commands);host->broadcast();
    });
    f.after=authority.execution_tick();f.host=host->checkpoint(binding);
    f.outputs={bus->incoming[1],bus->incoming[5]};frames.push_back(std::move(f));
    couch.receive();other.receive();
  }
  SourceReplayBatch local_tape{{context,bound.world,bound.world,1},{}};
  bool saw_local_camera{};
  for(const auto &f:frames){local_tape.steps.push_back({f.before,f.after,f.phase});
    for(const auto &call:f.phase.calls)saw_local_camera|=call.kind==SourceExecutionCallKind::PlayerCamera;}
  check(saw_local_camera,"Actual host broadcast no longer exercises its local camera capability");
  rejects([&]{encode_source_host_replay({local_tape,frames.back().host});});
  // The complete checkpoint and trusted local diagnostic replay below retain
  // every real camera operation. They are not a transferable network journal.
  // Exercise paired-envelope grammar with a separate actual no-op phase at
  // the final complete cut, without filtering calls from the gameplay tape.
  const auto expected=encode_source_authority(authority.checkpoint(binding));
  const AuthorityCutSeal codec_base{1,authority.execution_tick(),expected.size(),hash(expected)};
  SourceReplayBatch batch{{context,codec_base,codec_base,1},{}};
  auto observation=authority.capture_execution_phase([]{});
  check(observation.calls.empty()&&observation.inputs.empty(),"No-op codec observation unexpectedly supplied gameplay");
  batch.steps.push_back({codec_base.tick,codec_base.tick,std::move(observation)});
  const auto paired_replay=encode_source_host_replay({batch,frames.back().host});
  check(source_host_replay_payload(paired_replay)&&!source_host_continuation_payload(paired_replay)&&
      !source_replay_batch_payload(paired_replay),"Paired replay envelope was mistaken for another stream format");
  const auto decoded_replay=decode_source_host_replay(paired_replay,batch.binding,fingerprint,roster,admitted);
  check(encode_source_host_replay(decoded_replay)==paired_replay,"Paired replay did not round trip every host/native field");
  for(auto prefix:{std::size_t(0),std::size_t(7),std::size_t(43),std::size_t(48),paired_replay.size()-1})
    rejects([&]{decode_source_host_replay(std::span(paired_replay).first(prefix),batch.binding,fingerprint,roster,admitted);});
  auto wrong_replay=batch.binding;++wrong_replay.context.epoch;
  rejects([&]{decode_source_host_replay(paired_replay,wrong_replay,fingerprint,roster,admitted);});
  wrong_replay=batch.binding;++wrong_replay.context.membership_revision;
  rejects([&]{decode_source_host_replay(paired_replay,wrong_replay,fingerprint,roster,admitted);});
  wrong_replay=batch.binding;wrong_replay.previous.digest.back()^=std::byte{1};
  rejects([&]{decode_source_host_replay(paired_replay,wrong_replay,fingerprint,roster,admitted);});
  wrong_replay=batch.binding;++wrong_replay.first_sequence;
  rejects([&]{decode_source_host_replay(paired_replay,wrong_replay,fingerprint,roster,admitted);});
  rejects([&]{decode_source_host_replay(paired_replay,batch.binding,fingerprint+1,roster,admitted);});
  rejects([&]{decode_source_host_replay(paired_replay,batch.binding,fingerprint,roster,wrong_admission);});
  rejects([&]{encode_source_host_replay({batch,sidecar});}); // Initial host state cannot describe the final native tick.
  auto corrupt_replay=paired_replay;corrupt_replay.back()^=std::byte{1};rehash(corrupt_replay);
  rejects([&]{decode_source_host_replay(corrupt_replay,batch.binding,fingerprint,roster,admitted);});
  corrupt_replay=paired_replay;corrupt_replay.push_back(std::byte{});
  rejects([&]{decode_source_host_replay(corrupt_replay,batch.binding,fingerprint,roster,admitted);});
  SourceReplayLimits replay_limits;replay_limits.encoded_bytes=paired_replay.size()-1;
  rejects([&]{encode_source_host_replay({batch,frames.back().host},replay_limits);});
  rejects([&]{decode_source_host_replay(paired_replay,batch.binding,fingerprint,roster,admitted,replay_limits);});
  // Alter the independently chained replay bytes while preserving their valid
  // phase encodings and recomputing the outer checksum. The original host
  // sidecar must still reject the different replay digest before native use.
  corrupt_replay=paired_replay;constexpr unsigned replay_previous_digest=48+16+48+56+24;
  corrupt_replay.at(replay_previous_digest)^=std::byte{1};rehash(corrupt_replay);
  auto changed_parent=batch.binding;changed_parent.previous.digest.front()^=std::byte{1};
  // Both base and previous refer to the same initial cut, so retain that
  // invariant while changing its independently supplied content proof.
  corrupt_replay.at(48+16+48+24)^=std::byte{1};rehash(corrupt_replay);
  changed_parent.base.digest.front()^=std::byte{1};
  rejects([&]{decode_source_host_replay(corrupt_replay,changed_parent,fingerprint,roster,admitted);});
  host.reset(); // The continuation must own every peer/camera value after this.
  std::exception_ptr imported_error;
  {std::jthread executor([&]{try {
    MultiplayerPreparedWorld target(options,std::nullopt,std::make_shared<FixedClock>());
    target.bind_source_local_views(target.local_slots);SourceMatchAuthority imported(target.match,*target.source_objects);
    const auto received=decode_source_host_continuation(bundled,context,fingerprint,roster,admitted);
    const auto &received_world=received.world;const auto &sidecar=received.host;
    auto next=imported.prepare(received_world,binding,admitted);check(imported.commit(std::move(next)),"Cannot import the complete native cut");
    auto target_bus=std::make_shared<Bus>();auto target_wire=std::make_shared<Endpoint>(target_bus,0);
    const auto create=[&](auto cut,auto b,auto fp,auto admissions,auto accepted) {
      return LanHost::resume(target.match,target_wire,fp,std::move(admissions),cut,b,accepted);
    };
    auto wrong=binding;wrong.back()=std::byte{1};
    rejects([&]{create(sidecar,wrong,fingerprint,roster,admitted);});
    rejects([&]{create(sidecar,binding,fingerprint+1,roster,admitted);});
    rejects([&]{create(LanHost::Continuation{},binding,fingerprint,roster,admitted);});
    auto bad=admitted;bad.back().identity.front()^=0x80;
    rejects([&]{create(sidecar,binding,fingerprint,roster,bad);});
    bad=admitted;bad.pop_back();rejects([&]{create(sidecar,binding,fingerprint,roster,bad);});
    auto changed=roster;changed.back().name="Unadmitted";
    rejects([&]{create(sidecar,binding,fingerprint,changed,admitted);});
    changed=roster;changed.back().matchmaking_level=49;
    rejects([&]{create(sidecar,binding,fingerprint,changed,admitted);});
    changed=roster;changed.back().matchmaking_level.reset();
    rejects([&]{create(sidecar,binding,fingerprint,changed,admitted);});
    imported.capture_execution_phase([&]{rejects([&]{create(sidecar,binding,fingerprint,roster,admitted);});});
    const auto original_locals=target.local_slots;
    target.bind_source_local_views(std::array{1u});
    rejects([&]{create(sidecar,binding,fingerprint,roster,admitted);});
    target.bind_source_local_views(original_locals);
    auto repaired=imported.prepare(received_world,binding,admitted);
    check(imported.commit(std::move(repaired)),"Cannot repair the complete cut after rejecting a different host view owner");
    check(encode_source_authority(imported.checkpoint(binding))==cut_bytes,"Rejected host continuation mutated the complete native cut");
    auto accepted=admitted;std::ranges::reverse(accepted);
    auto resumed=create(sidecar,binding,fingerprint,roster,accepted);
    check(resumed->peers()==2,"Host continuation lost an admitted remote connection");
    check(encode_source_authority(imported.checkpoint(binding))==cut_bytes,"Host continuation joined/respawned or otherwise mutated a native player");
    for(unsigned phase=0;phase<frames.size();++phase) {
      const auto &f=frames[phase];
      target_bus->incoming[0]=f.inputs;std::array<Controls,multiplayer_capacity> delivered{};
      try {
        imported.replay_execution_phase(f.phase,[&]{
          delivered[0].yaw=target.match.unit(0).yaw;resumed->receive(delivered);target.match.step(delivered);resumed->broadcast();
        });
      }catch(const std::exception &e){throw FormatError("Host replay phase "+std::to_string(phase)+": "+e.what());}
      for(unsigned peer=0;peer<2;++peer) {
        const auto &actual=target_bus->incoming[peer?5:1],&expected_packets=f.outputs[peer];
        if(actual!=expected_packets) {
          std::cerr<<"Host continuation snapshot mismatch phase="<<phase<<" peer="<<(peer?5:1)
            <<" actual_packets="<<actual.size()<<" expected_packets="<<expected_packets.size()
            <<" actual_first_kind="<<(!actual.empty()&&actual.front().bytes.size()>6?std::to_integer<unsigned>(actual.front().bytes[6]):0)
            <<" expected_first_kind="<<(!expected_packets.empty()&&expected_packets.front().bytes.size()>6?std::to_integer<unsigned>(expected_packets.front().bytes[6]):0)<<'\n';
          for(unsigned packet=0;packet<std::min(actual.size(),expected_packets.size());++packet)
            if(actual[packet]!=expected_packets[packet]) {
              const auto &a=actual[packet].bytes,&b=expected_packets[packet].bytes;std::size_t at{};
              while(at<std::min(a.size(),b.size())&&a[at]==b[at])++at;
              std::cerr<<" first_packet="<<packet<<" actual_bytes="<<a.size()<<" expected_bytes="<<b.size()
                <<" actual_kind="<<(a.size()>6?std::to_integer<unsigned>(a[6]):0)
                <<" expected_kind="<<(b.size()>6?std::to_integer<unsigned>(b[6]):0)<<" first_difference="<<at<<'\n';break;
            }
        }
      }
      check(target_bus->incoming[1]==f.outputs[0]&&target_bus->incoming[5]==f.outputs[1],
          "Restored host changed an actual complete snapshot, sequence, camera or connection binding");
      target_bus->incoming[1].clear();target_bus->incoming[5].clear();
    }
    check(encode_source_authority(imported.checkpoint(binding))==expected,"Restored host changed the complete replayed native world");
    rejects([&]{create(sidecar,binding,fingerprint,roster,admitted);}); // Different completed tick.
    resumed.reset();auto original=imported.prepare(received_world,binding,admitted);
    check(imported.commit(std::move(original)),"Cannot restore native cut for saturating idle timeout");
    const auto expired_cut=LanHost::decode_continuation(maximum_idle,bound,fingerprint,roster,admitted);
    auto expired=create(expired_cut,binding,fingerprint,roster,admitted);
    const auto deaths=target.match.player(1).deaths;std::array<Controls,multiplayer_capacity> idle_controls{};
    expired->receive(idle_controls);
    check(expired->peers()==1&&target.match.player(1).deaths==deaths,
        "Extreme received idle age overflowed or fabricated a death instead of native peer expiry");
    for(unsigned slot=1;slot<5;++slot)check(target.match.player(slot).source_flags&2u,"Expired couch peer lost its original departure path");
  }catch(...){imported_error=std::current_exception();}});}
  if(imported_error)std::rethrow_exception(imported_error);
  std::cout<<"PASS encoded host continuation across native executors: bounded canonical bytes, exact generation/membership/world-seal/roster binding, malformed-prefix/count/local-view rejection, two logical peers/four couch views, complete native world/cameras/snapshots, consumed input serials and no rebirth\n";
  std::cout<<"LIMIT complete snapshot profile, logical adapter and same host/local ownership; no default delta-stream continuation, election, generation handoff, controller/voice/WAN or automatic migration acceptance\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
