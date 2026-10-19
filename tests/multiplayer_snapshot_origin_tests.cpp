#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/effect_presentation_continuity.hpp"
#include <openssl/sha.h>
#include <algorithm>
#include <deque>
#include <iostream>

using namespace halo2;
namespace {
void check(bool yes,const char *why){if(!yes)throw FormatError(why);}
template<class F>void rejects(F run){bool rejected=false;try{run();}catch(const FormatError&){rejected=true;}
    check(rejected,"Unmatched accepted snapshot was admitted");}
struct FixedClock final:PhysicsLiveClockSource {
    std::uint32_t milliseconds()const noexcept override{return 1000;}
    std::uint64_t cycles()const noexcept override{return 7000000;}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
// Sequential logical-endpoint adapter, not encryption or WAN evidence.
struct Bus {struct Packet {unsigned from{};std::vector<std::byte> bytes;};std::array<std::deque<Packet>,16> queues;};
struct Endpoint final:MultiplayerDatagrams {
    std::shared_ptr<Bus> bus;unsigned self;
    Endpoint(std::shared_ptr<Bus> b,unsigned s):bus(std::move(b)),self(s){}
    void send(std::span<const std::byte> bytes,unsigned to)override{check(to<16,"Bad fixture endpoint");bus->queues[to].push_back({self,{bytes.begin(),bytes.end()}});}
    bool receive(std::vector<std::byte> &bytes,unsigned &from)override{auto &q=bus->queues[self];if(q.empty())return false;
        from=q.front().from;bytes=std::move(q.front().bytes);q.pop_front();return true;}
};
SourcePlayerConfiguration profile(std::string_view name){const auto defaults=source_default_player_configuration();std::array<std::uint8_t,9> colors;
    std::copy_n(defaults.begin()+0x40,colors.size(),colors.begin());return native_selected_player_configuration(name,colors,-1);}
AuthorityTransferDigest hash(std::span<const std::byte> bytes){AuthorityTransferDigest out{};
    check(SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),reinterpret_cast<unsigned char*>(out.data())),"Cannot hash native cut");return out;}
std::uint32_t sequence(const Bus::Packet &p){check(p.bytes.size()>=27,"Snapshot header is truncated");return Bytes(p.bytes).u32(23);}
}
int main(int argc,char **argv)try {
    if(argc!=3 || !std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    MultiplayerOptions options;options.gameplay_xbe=argv[1];options.map=argv[2];options.starting_weapon=5;
    options.local_players=1;options.source_object_world=true;
    MultiplayerPreparedWorld world(options,std::nullopt,std::make_shared<FixedClock>());world.bind_source_local_views(world.local_slots);
    SourceMatchAuthority authority(world.match,*world.source_objects);
    auto bus=std::make_shared<Bus>();constexpr std::uint64_t fingerprint=0x15c071900;
    std::vector<OnlineAdmission> roster;
    for(unsigned slot=0;slot<6;++slot)roster.push_back({slot,"Receipt "+std::to_string(slot),-1,{},slot==0?0u:slot<5?1u:5u,slot>1&&slot<5});
    LanHost host(world.match,std::make_shared<Endpoint>(bus,0),fingerprint,roster);
    const std::array profiles{profile("One"),profile("Two"),profile("Three"),profile("Four")};
    LanClient couch(std::make_shared<Endpoint>(bus,1),0,fingerprint,{},profiles);
    check(!couch.state_origin().tick(),"Unadmitted client invented a state receipt");
    const MultiplayerSnapshotOrigin missing;
    rejects([&]{(void)missing.accepted_state();});
    check(missing.local_slots().empty()&&!missing.same_connection(missing),"Missing receipt invented owned admission");
    std::array<Controls,4> local{};std::array<Controls,16> controls{};
    for(unsigned i=0;i<32&&!couch.ready();++i){couch.send(local);host.receive(controls);host.broadcast();couch.receive();}
    check(couch.ready() && couch.slots().size()==4 && !couch.received_origins().empty(),"Four-seat receipt admission failed");
    const auto unbound=couch.received_origins().back();check(!unbound.context(),"Pre-admission receipt invented a context");
    check(couch.state_origin().same(unbound)&&unbound.local_slots().empty()&&!unbound.same_connection(unbound),
        "Pre-admission receipt invented retained decoded state or a bound connection");
    rejects([&]{(void)unbound.accepted_state();});
    AuthorityTransferContext context;context.host=0;context.peers=(1u<<0)|(1u<<1)|(1u<<5);
    context.epoch=3;context.membership_revision=7;context.session[0]=std::byte{0x71};
    couch.bind_continuity(context,1,roster);const auto binding=source_authority_binding(context);
    check(!couch.state_origin().tick()&&couch.received_origins().back().same(unbound)&&
        !couch.received_origins().back().context(),
        "Continuity binding retroactively certified a pre-admission state");
    world.match.step(controls);host.broadcast();const auto first_packets=bus->queues[1];couch.receive();
    check(couch.received_states().size()==couch.received_origins().size(),"Accepted state/receipt order differs");
    const auto early=couch.received_origins().back();const auto tick=couch.state().tick;
    check(early.tick()==tick && early.context()==context && !couch.state().players[5].connected,"Initial receipt has wrong state");
    const std::array couch_slots{1u,2u,3u,4u};
    const auto *retained_early=&early.accepted_state();
    check(couch.state_origin().same(early)&&early.same_connection(early)&&
        std::ranges::equal(early.local_slots(),couch_slots)&&retained_early!=&couch.state()&&
        retained_early->tick==tick&&!retained_early->players[5].connected&&
        retained_early->players[1].source_configuration==couch.state().players[1].source_configuration&&
        retained_early->players[4].source_configuration==couch.state().players[4].source_configuration,
        "Bound receipt did not retain its actual decoded state and four couch slots");
    const auto native_cut=[&](std::uint64_t ordinal){
        const auto native=authority.checkpoint(binding);const auto bytes=encode_source_authority(native);
        return ValidatedAuthorityCut{1,context,{ordinal,authority.execution_tick(),bytes.size(),hash(bytes)}};
    };
    const auto early_cut=native_cut(1);const auto early_host=host.checkpoint(binding);
    const auto early_proof=couch.prove_snapshot(early_host,early_cut,early);
    rejects([&]{(void)couch.prove_snapshot(early_host,early_cut,unbound);});
    rejects([&]{(void)couch.prove_snapshot(early_host,early_cut,MultiplayerSnapshotOrigin{});});
    const auto encode_host=LanHost::encode_continuation(early_host,{context,early_cut.seal});
    const auto admitted=authority.player_admissions(std::array{0u,1u,2u,3u,4u});
    const auto decoded=LanHost::decode_continuation(encode_host,{context,early_cut.seal},fingerprint,roster,admitted);
    check(couch.prove_snapshot(decoded,early_cut,early).matches(early),"Codec lost exact snapshot origin");
    auto obsolete=encode_host;obsolete[7]=std::byte{'1'};
    rejects([&]{(void)LanHost::decode_continuation(obsolete,{context,early_cut.seal},fingerprint,roster,admitted);});

    std::uint32_t primary=0x10203040,auxiliary=0x12345678;const SourceEffectRandom random{primary,auxiliary};Mesh mesh;
    NativeWorldEffects effects(*world.cache,mesh,world.match.simulation().collision(),random);
    NativeMuzzleEffects muzzle(*world.cache,random);SourceMapEffects map(*world.cache,world.content.objects,effects);
    SourceCombatAttachments combat(*world.cache,effects,{});EffectPresentationContinuity presentation(muzzle,effects,map,combat,random);
    effects.advance(tick,tick_seconds);muzzle.advance(tick,tick_seconds);
    rejects([&]{(void)presentation.capture(unbound);});
    const auto early_presentation=presentation.capture(early);
    rejects([&]{(void)presentation.bind(early_presentation,early_cut);});
    const auto early_bound=presentation.bind(early_presentation,early_proof);
    check(early_bound.cut().seal==early_cut.seal,"Origin proof did not retain its exact native cut");

    auto other=std::make_unique<LanClient>(std::make_shared<Endpoint>(bus,5),0,fingerprint);Controls neutral{};
    for(unsigned i=0;i<32&&!other->ready();++i){other->send(std::span(&neutral,1));host.receive(controls);other->receive();}
    check(other->ready(),"Same-tick late join failed");other->bind_continuity(context,5,roster);
    host.broadcast();const auto genuine=bus->queues[1];bus->queues[1].clear();
    check(!genuine.empty() && !first_packets.empty(),"Missing body/sequence mismatch controls");
    const auto late_cut=native_cut(2);const auto late_host=host.checkpoint(binding);
    // Reuse an older valid body under the newer sequence in this fixture's
    // logical transport. DTLS would reject network tampering; this exercises
    // accidental/malicious body association after a caller claims acceptance.
    for(auto packet:first_packets){const auto next=sequence(genuine.front());
        for(unsigned i=0;i<4;++i)packet.bytes[23+i]=std::byte(next>>(8*i));bus->queues[1].push_back(std::move(packet));}
    couch.receive();check(couch.state().tick==tick && !couch.state().players[5].connected && !couch.received_origins().empty(),
        "Reused valid-body control did not reach the ordinary decoder");
    const auto wrong_body=couch.received_origins().back();
    rejects([&]{(void)couch.prove_snapshot(late_host,late_cut,wrong_body);});
    bus->queues[1]=genuine;couch.receive();check(couch.received_origins().empty(),"Duplicate sequence created another origin receipt");
    host.broadcast();const auto replacement_packets=bus->queues[5];couch.receive();other->receive();
    check(couch.state().tick==tick && couch.state().players[5].connected && couch.received_states().size()==couch.received_origins().size(),
        "Newer same-tick roster was not accepted with its receipt");
    const auto latest=couch.received_origins().back(),other_origin=other->received_origins().back();
    check(latest.sequence()!=early.sequence(),"Same-tick states lost their distinct sequence");
    check(early.same_connection(latest)&&!early.same(latest)&&!early.same_connection(other_origin)&&
        !early.same_connection(unbound)&& &early.accepted_state()==retained_early&&
        !retained_early->players[5].connected&&latest.accepted_state().players[5].connected,
        "Later same-tick adoption mutated an earlier owning state or mixed connections");
    const auto latest_cut=native_cut(3);const auto latest_host=host.checkpoint(binding);
    const auto proof=couch.prove_snapshot(latest_host,latest_cut,latest);
    rejects([&]{(void)couch.prove_snapshot(latest_host,latest_cut,early);});
    rejects([&]{(void)couch.prove_snapshot(latest_host,latest_cut,other_origin);});
    rejects([&]{(void)presentation.bind(early_presentation,proof);});
    check(couch.prove_snapshot(early_host,early_cut,early).matches(early),"Later receives destroyed an older owning receipt");
    const auto latest_presentation=presentation.capture(latest);const auto bound=presentation.bind(latest_presentation,proof);
    check(bound.cut().seal==latest_cut.seal,"Same-tick receipt bound the wrong native cut");
    host.broadcast();host.broadcast();couch.receive();
    check(couch.received_states().size()==2&&couch.received_origins().size()==2&&
        couch.state_origin().same(couch.received_origins().back())&&
        early.same_connection(couch.state_origin())&& &early.accepted_state()==retained_early&&
        std::ranges::equal(early.local_slots(),couch_slots)&&!retained_early->players[5].connected,
        "A later receive batch discarded or changed the retained decoded receipt");
    const auto last_adopted=couch.state_origin();
    couch.receive();check(couch.received_states().empty() && couch.received_origins().empty()&&couch.state_origin().same(last_adopted),
        "Empty receive changed the last adopted receipt or retained per-poll rows");
    auto next=context;++next.epoch;++next.membership_revision;next.peers&=~1u;next.host=1;
    auto prepared=other->prepare_rebind_host(context,next,latest_cut.seal);
    check(other->commit_rebind_host(std::move(prepared)),"Cannot exercise source-generation retirement");
    check(!other->state_origin().tick()&&other->received_origins().empty(),"Rebinding retained an old-generation adopted receipt");
    rejects([&]{(void)other->prove_snapshot(latest_host,latest_cut,other_origin);});
    // Reuse real decoded snapshot bytes through the new logical endpoint.
    // This checks generation ownership, not replacement-host native proof.
    check(!replacement_packets.empty(),"Missing real snapshot for the new-generation decoder control");
    const auto replacement_sequence=sequence(replacement_packets.back());
    bus->queues[5].clear();
    for(auto packet:replacement_packets)if(sequence(packet)==replacement_sequence){
        packet.from=1;bus->queues[5].push_back(std::move(packet));}
    other->receive();
    const auto rebound=other->state_origin();const std::array other_slots{5u};
    check(rebound.context()==next&&rebound.tick()==tick&&rebound.same_connection(rebound)&&
        !other_origin.same_connection(rebound)&&!rebound.same_connection(other_origin)&&
        std::ranges::equal(rebound.local_slots(),other_slots)&&rebound.accepted_state().players[5].connected,
        "New-generation receipt inherited the retired connection identity");
    const auto *retained_rebound=&rebound.accepted_state();
    other.reset();
    check(&rebound.accepted_state()==retained_rebound&&retained_rebound->tick==tick&&
        retained_rebound->players[5].connected&&std::ranges::equal(rebound.local_slots(),other_slots)&&
        other_origin.accepted_state().players[5].connected&&std::ranges::equal(other_origin.local_slots(),other_slots),
        "Client destruction discarded the receipt's owned decoded state or slots");
    std::cout<<"PASS exact native snapshot receipt/sidecar proof, owned decoded state/four couch slots through later batches and client destruction, same-tick late join, wrong body under matching sequence, stale/duplicate/foreign/unbound receipts, codec roundtrip/version and epoch retirement\n";
    std::cout<<"LIMIT logical endpoint adapter and locally validated native cuts; no DTLS/WAN/quorum or runtime/GPU migration; effect presenters are empty\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
