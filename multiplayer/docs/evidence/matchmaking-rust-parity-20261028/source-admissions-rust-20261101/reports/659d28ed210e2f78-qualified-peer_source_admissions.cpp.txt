#include "halo2/peer_source_admissions.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <algorithm>
#include <mutex>
#include <stdexcept>

namespace halo2 {
namespace {
namespace bridge=online_protocol_bridge;
bridge::SourceAdmissionsContext context(const AuthorityTransferContext &v){
    bridge::SourceAdmissionsContext out;std::ranges::transform(v.session,out.session.begin(),[](auto b){return std::to_integer<std::uint8_t>(b);});
    out.epoch=v.epoch;out.membership_revision=v.membership_revision;out.peers=v.peers;out.host=v.host;return out;
}
AuthorityTransferContext context(const bridge::SourceAdmissionsContext &v){
    AuthorityTransferContext out;std::ranges::transform(v.session,out.session.begin(),[](auto b){return std::byte(b);});
    out.epoch=v.epoch;out.membership_revision=v.membership_revision;out.peers=v.peers;out.host=v.host;return out;
}
std::vector<bridge::SourceAdmissionsSeat> seats(std::span<const OnlineAdmission> values){
    std::vector<bridge::SourceAdmissionsSeat> out;
    for(const auto &v:values){bridge::SourceAdmissionsSeat s;s.slot=v.slot;s.peer=v.peer;s.guest=v.guest;
        std::ranges::transform(v.identity,s.account.begin(),[](auto b){return std::to_integer<std::uint8_t>(b);});out.push_back(s);}
    return out;
}
rust::Vec<bridge::SourceAdmissionsPlayer> players(std::span<const SourcePlayerAdmission> values){
    rust::Vec<bridge::SourceAdmissionsPlayer> out;
    for(const auto &v:values){bridge::SourceAdmissionsPlayer p;p.slot=v.slot;p.player=v.player;p.identity=v.identity;out.push_back(p);}return out;
}
bridge::SourceAdmissionsAccepted accepted(const AcceptedSourcePlayers &v){
    bridge::SourceAdmissionsAccepted out;out.present=true;out.context=context(v.context);
    std::ranges::transform(v.digest,out.digest.begin(),[](auto b){return std::to_integer<std::uint8_t>(b);});out.players=players(v.players);return out;
}
std::optional<AcceptedSourcePlayers> accepted(const bridge::SourceAdmissionsAccepted &v){
    if(!v.present)return {};AcceptedSourcePlayers out;out.context=context(v.context);
    std::ranges::transform(v.digest,out.digest.begin(),[](auto b){return std::byte(b);});
    for(const auto &p:v.players)out.players.push_back({p.slot,p.player,p.identity});return out;
}
template<class T>auto slice(const rust::Vec<T> &v){return rust::Slice<const T>{v.data(),v.size()};}
template<class T>auto slice(const std::vector<T> &v){return rust::Slice<const T>{v.data(),v.size()};}
auto bytes(std::span<const std::byte> v){return rust::Slice<const std::uint8_t>{reinterpret_cast<const std::uint8_t*>(v.data()),v.size()};}
template<class F>auto checked(F &&f){try{return f();}catch(const rust::Error &e){throw std::invalid_argument(e.what());}}
}
// Synchronization and owned typed conversions only. Rust holds all admission,
// roster/identity validation, hashing, wire, retry and successor-resumption state.
struct PeerSourceAdmissions::Impl {
    mutable std::mutex mutex;
    rust::Box<bridge::PeerSourceAdmissions> owner;
    explicit Impl(rust::Box<bridge::PeerSourceAdmissions> value):owner(std::move(value)){}
};
PeerSourceAdmissions::PeerSourceAdmissions(unsigned self,AuthorityTransferContext c,std::span<const OnlineAdmission> roster)
    :impl_([&]{const auto rows=seats(roster);return std::make_unique<Impl>(checked([&]{return bridge::new_peer_source_admissions(self,native_multiplayer_protocol,halo2::context(c),slice(rows));}));}()){}
PeerSourceAdmissions::PeerSourceAdmissions(std::unique_ptr<Impl> value):impl_(std::move(value)){}
PeerSourceAdmissions::~PeerSourceAdmissions()=default;
AuthorityTransferContext PeerSourceAdmissions::context()const{
    std::lock_guard lock(impl_->mutex);return halo2::context(checked([&]{return bridge::peer_source_admissions_context(*impl_->owner);}));
}
bool PeerSourceAdmissions::replace_context(const AuthorityTransferContext &expected,AuthorityTransferContext next){
    std::lock_guard lock(impl_->mutex);return checked([&]{return bridge::peer_source_admissions_replace(*impl_->owner,halo2::context(expected),halo2::context(next));});
}
std::unique_ptr<PeerSourceAdmissions> PeerSourceAdmissions::resume_after_host_loss(unsigned self,AuthorityTransferContext next,
    std::span<const OnlineAdmission> roster,const AcceptedSourcePlayers &previous){
    const auto rows=seats(roster);const auto before=halo2::accepted(previous);
    auto owner=checked([&]{return bridge::peer_source_admissions_resume(self,native_multiplayer_protocol,halo2::context(next),slice(rows),before);});
    return std::unique_ptr<PeerSourceAdmissions>(new PeerSourceAdmissions(std::make_unique<Impl>(std::move(owner))));
}
bool PeerSourceAdmissions::publish(const AuthorityTransferContext &c,std::span<const SourcePlayerAdmission> values){
    const auto rows=players(values);std::lock_guard lock(impl_->mutex);
    return checked([&]{return bridge::peer_source_admissions_publish(*impl_->owner,halo2::context(c),slice(rows));});
}
std::optional<AcceptedSourcePlayers> PeerSourceAdmissions::accepted()const{
    std::lock_guard lock(impl_->mutex);return halo2::accepted(checked([&]{return bridge::peer_source_admissions_accepted(*impl_->owner);}));
}
bool PeerSourceAdmissions::confirmed(unsigned peer)const{
    std::lock_guard lock(impl_->mutex);return checked([&]{return bridge::peer_source_admissions_confirmed(*impl_->owner,peer);});
}
std::vector<AuthorityTransferPacket> PeerSourceAdmissions::poll(std::uint64_t now,unsigned budget){
    std::lock_guard lock(impl_->mutex);const auto values=checked([&]{return bridge::peer_source_admissions_poll(*impl_->owner,now,budget);});
    std::vector<AuthorityTransferPacket> out;
    for(const auto &v:values){AuthorityTransferPacket p{v.peer,{}};for(auto b:v.bytes)p.bytes.push_back(std::byte(b));out.push_back(std::move(p));}return out;
}
bool PeerSourceAdmissions::receive(unsigned peer,std::span<const std::byte> packet){
    if(!is_packet(packet))return false;std::lock_guard lock(impl_->mutex);
    return checked([&]{return bridge::peer_source_admissions_receive(*impl_->owner,peer,bytes(packet));});
}
bool PeerSourceAdmissions::is_packet(std::span<const std::byte> packet){return bridge::peer_source_admissions_is_packet(bytes(packet));}
}
