#include "halo2/peer_host_agreement.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>

namespace halo2 {
namespace {
namespace bridge=online_protocol_bridge;
constexpr unsigned invalid_peer=16;
// Public native indices are unsigned. Preserve their invalidity before the
// fixed-width ABI: e.g. 256 must never become the admitted physical peer zero.
std::uint8_t peer_index(unsigned peer){return std::uint8_t(peer<16?peer:invalid_peer);}
bridge::HostAgreementContext context(const AuthorityTransferContext &value){
    bridge::HostAgreementContext result{};
    std::ranges::transform(value.session,result.session.begin(),[](auto byte){return std::to_integer<std::uint8_t>(byte);});
    result.epoch=value.epoch;result.membership_revision=value.membership_revision;
    result.peers=value.peers;result.host=peer_index(value.host);return result;
}
AuthorityTransferContext context(const bridge::HostAgreementContext &value){
    AuthorityTransferContext result;
    std::ranges::transform(value.session,result.session.begin(),[](auto byte){return std::byte(byte);});
    result.epoch=value.epoch;result.membership_revision=value.membership_revision;
    result.peers=value.peers;result.host=value.host;return result;
}
bridge::MigrationPreparationCut cut(const AuthorityCutSeal &value){
    bridge::MigrationPreparationCut result{};
    result.present=true;result.id=value.id;result.tick=value.tick;result.bytes=value.bytes;
    std::ranges::transform(value.digest,result.digest.begin(),[](auto byte){return std::to_integer<std::uint8_t>(byte);});
    return result;
}
AuthorityCutSeal cut(const bridge::MigrationPreparationCut &value){
    AuthorityCutSeal result{value.id,value.tick,value.bytes,{}};
    std::ranges::transform(value.digest,result.digest.begin(),[](auto byte){return std::byte(byte);});
    return result;
}
template<class F>auto checked(F &&operation){
    try{return operation();}
    catch(const rust::Error &error){throw std::invalid_argument(error.what());}
}
auto make_agreement(unsigned self,const AuthorityTransferContext &value,std::uint64_t now){
    if(self>=16||value.host>=16)throw std::invalid_argument("Invalid peer host agreement packet or context");
    return checked([&]{return bridge::new_host_agreement(peer_index(self),context(value),native_multiplayer_protocol,now);});
}
auto bytes(std::span<const std::byte> packet){
    return rust::Slice<const std::uint8_t>{reinterpret_cast<const std::uint8_t *>(packet.data()),packet.size()};
}
}
// Synchronization belongs to the native session owner. Wire parsing, retries,
// ballots, promises, votes and fixed-membership election policy live in Rust.
struct PeerHostAgreement::Impl {
    mutable std::shared_mutex mutex;
    rust::Box<bridge::HostAgreement> agreement;
    Impl(unsigned self,const AuthorityTransferContext &value,std::uint64_t now)
        :agreement(make_agreement(self,value,now)){}
};
PeerHostAgreement::PeerHostAgreement(unsigned self,AuthorityTransferContext value,std::uint64_t now)
    :impl_(std::make_unique<Impl>(self,value,now)){}
PeerHostAgreement::~PeerHostAgreement()=default;
void PeerHostAgreement::observe_host(std::uint64_t now){
    auto &s=*impl_;std::unique_lock lock(s.mutex);
    checked([&]{bridge::host_agreement_observe_host(*s.agreement,now);});
}
bool PeerHostAgreement::publish_ready(const ValidatedAuthorityCut &value){
    auto &s=*impl_;std::unique_lock lock(s.mutex);
    return checked([&]{return bridge::host_agreement_publish_ready(*s.agreement,peer_index(value.peer),context(value.context),cut(value.seal));});
}
bool PeerHostAgreement::propose(unsigned candidate,std::uint64_t now){
    auto &s=*impl_;std::unique_lock lock(s.mutex);
    return checked([&]{return bridge::host_agreement_propose(*s.agreement,peer_index(candidate),now);});
}
bool PeerHostAgreement::drive(unsigned candidate,std::uint64_t now){
    auto &s=*impl_;std::unique_lock lock(s.mutex);
    return checked([&]{return bridge::host_agreement_drive(*s.agreement,peer_index(candidate),now);});
}
std::vector<AuthorityTransferPacket> PeerHostAgreement::poll(std::uint64_t now,unsigned budget){
    auto &s=*impl_;std::unique_lock lock(s.mutex);
    const auto packets=checked([&]{return bridge::host_agreement_poll(*s.agreement,now,budget);});
    std::vector<AuthorityTransferPacket> result;result.reserve(packets.size());
    for(const auto &packet:packets){
        AuthorityTransferPacket value{packet.peer,{}};value.bytes.reserve(packet.bytes.size());
        for(const auto byte:packet.bytes)value.bytes.push_back(std::byte(byte));
        result.push_back(std::move(value));
    }
    return result;
}
bool PeerHostAgreement::is_packet(std::span<const std::byte> packet)noexcept{
    return bridge::host_agreement_is_packet(bytes(packet));
}
bool PeerHostAgreement::receive(unsigned peer,std::span<const std::byte> packet,std::uint64_t now){
    if(!is_packet(packet))return false;
    auto &s=*impl_;std::unique_lock lock(s.mutex);
    return checked([&]{return bridge::host_agreement_receive(*s.agreement,peer_index(peer),bytes(packet),now);});
}
PeerHostAgreementState PeerHostAgreement::state(std::uint64_t now)const{
    const auto &s=*impl_;std::shared_lock lock(s.mutex);
    const auto value=checked([&]{return bridge::host_agreement_state(*s.agreement,now);});
    PeerHostAgreementState result{value.suspected,value.fenced,value.electorate,value.silent,value.majority,{},{},value.rejected};
    for(unsigned peer=0;peer<result.ready.size();++peer)
        if(value.ready[peer].present)result.ready[peer]=cut(value.ready[peer]);
    if(value.decision.present){const auto &d=value.decision;
        result.decision=PeerMigrationDecision{context(d.previous),context(d.next),
            {d.ballot.round,d.ballot.proposer},{d.value.host,cut(d.value.cut)},d.voters};
    }
    return result;
}
}
