#include "halo2/peer_snapshot_candidate.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <algorithm>
#include <stdexcept>

namespace halo2 {
namespace {
namespace bridge=online_protocol_bridge;
template<class F>auto checked(F &&call){try{return call();}catch(const rust::Error &e){throw std::invalid_argument(e.what());}}
bridge::SourceAdmissionsContext wire(const AuthorityTransferContext &c){
    bridge::SourceAdmissionsContext out;std::ranges::transform(c.session,out.session.begin(),[](auto b){return std::to_integer<std::uint8_t>(b);});
    out.epoch=c.epoch;out.membership_revision=c.membership_revision;out.peers=c.peers;out.host=c.host;return out;
}
AuthorityTransferContext native(const bridge::SourceAdmissionsContext &c){
    AuthorityTransferContext out;std::ranges::transform(c.session,out.session.begin(),[](auto b){return std::byte(b);});
    out.epoch=c.epoch;out.membership_revision=c.membership_revision;out.peers=c.peers;out.host=c.host;return out;
}
bridge::NativeSnapshotToken wire(const PeerSnapshotToken &t){bridge::NativeSnapshotToken out;out.present=true;out.context=wire(t.context);out.recipients=t.recipients;out.nonce=t.nonce;return out;}
PeerSnapshotToken native(const bridge::NativeSnapshotToken &t){return {native(t.context),t.recipients,t.nonce};}
bridge::NativeSnapshotLimits wire(const PeerSnapshotLimits &l){bridge::NativeSnapshotLimits out;out.maximum_archive_bytes=l.maximum_archive_bytes;out.retry_ms=l.retry_ms;out.idle_ms=l.idle_ms;out.window_chunks=l.window_chunks;return out;}
rust::Slice<const std::uint8_t> wire(std::span<const std::byte> bytes){return {reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size()};}
std::vector<std::byte> native(const rust::Vec<std::uint8_t> &bytes){std::vector<std::byte> out;out.reserve(bytes.size());for(auto b:bytes)out.push_back(std::byte(b));return out;}
}
struct PeerSnapshotCandidate::Impl {
    const unsigned self;
    const std::shared_ptr<const void> identity=std::make_shared<const unsigned char>(0);
    rust::Box<bridge::PeerSnapshotCandidate> owner;
    Impl(unsigned p,const AuthorityTransferContext &c,const PeerSnapshotLimits &l):self(p),owner(checked([&]{return bridge::new_peer_snapshot_candidate(p,native_multiplayer_protocol,wire(c),wire(l));})){}
    // Only native storage and private identity facts. No owner pointer/callback
    // is retained here, so these calls cannot reenter the Rust state mutex.
    struct LeaseIo final:bridge::NativeSnapshotLeaseIo {
        const std::shared_ptr<const void> identity;const unsigned self;
        Delivery *captured{};std::optional<Delivery> *returned{};const std::optional<Delivery> *observed{};mutable std::optional<Delivery> issued;
        LeaseIo(const std::shared_ptr<const void> &identity,unsigned self):identity(identity),self(self){}
        const Delivery *delivery()const noexcept {if(captured)return captured;if(returned)return *returned?&**returned:nullptr;if(observed)return *observed?&**observed:nullptr;return nullptr;}
        bridge::NativeSnapshotWitness snapshot_witness()const noexcept override{
            bridge::NativeSnapshotWitness out;const auto *d=delivery();out.present=d!=nullptr;
            if(d){out.owner_matches=d->identity_==identity;out.recipient=d->recipient_;out.token=wire(d->token_);out.bytes_present=bool(d->bytes_);}return out;
        }
        void snapshot_issue(const bridge::NativeSnapshotToken &t,std::unique_ptr<bridge::NativeSnapshotBuffer> bytes)const noexcept override{
            issued=Delivery{native(t),self,std::move(bytes),identity};
        }
        void snapshot_destroy()const noexcept override{
            if(captured){captured->bytes_.reset();captured->identity_.reset();}
            else if(returned&&*returned){(*returned)->bytes_.reset();(*returned)->identity_.reset();returned->reset();}
        }
    };
};
PeerSnapshotCandidate::Delivery::Delivery(PeerSnapshotToken t,unsigned peer,std::unique_ptr<bridge::NativeSnapshotBuffer> bytes,std::shared_ptr<const void> identity)noexcept
    :token_(std::move(t)),recipient_(peer),bytes_(std::move(bytes)),identity_(std::move(identity)){}
PeerSnapshotCandidate::Delivery::Delivery(Delivery &&)noexcept=default;
PeerSnapshotCandidate::Delivery &PeerSnapshotCandidate::Delivery::operator=(Delivery &&)noexcept=default;
PeerSnapshotCandidate::Delivery::~Delivery()=default;
std::span<const std::byte> PeerSnapshotCandidate::Delivery::bytes()const noexcept{return bytes_?bytes_->bytes():std::span<const std::byte>{};}
PeerSnapshotCandidate::PeerSnapshotCandidate(unsigned self,AuthorityTransferContext c,PeerSnapshotLimits l):impl_(std::make_unique<Impl>(self,c,l)){static_assert(native_multiplayer_protocol<=0xffffu);}
PeerSnapshotCandidate::~PeerSnapshotCandidate()=default;
unsigned PeerSnapshotCandidate::self()const noexcept{return bridge::peer_snapshot_self(*impl_->owner);}
std::optional<PeerSnapshotToken> PeerSnapshotCandidate::begin(std::uint64_t now){return checked([&]()->std::optional<PeerSnapshotToken>{const auto t=bridge::peer_snapshot_begin(*impl_->owner,now);return t.present?std::optional(native(t)):std::nullopt;});}
bool PeerSnapshotCandidate::acknowledge_arm(const PeerSnapshotToken &t){return checked([&]{return bridge::peer_snapshot_arm(*impl_->owner,wire(t));});}
bool PeerSnapshotCandidate::publish(const PeerSnapshotToken &t,std::vector<PeerSnapshotArchive> archives,std::uint64_t now){
    rust::Vec<bridge::NativeSnapshotArchive> rows;rows.reserve(archives.size());
    for(auto &a:archives){bridge::NativeSnapshotArchive out;out.peer=a.peer;out.bytes=std::make_unique<bridge::NativeSnapshotBuffer>(std::move(a.bytes));rows.push_back(std::move(out));}
    return checked([&]{return bridge::peer_snapshot_publish(*impl_->owner,wire(t),std::move(rows),now);});
}
std::optional<PeerSnapshotCandidate::Delivery> PeerSnapshotCandidate::take_delivery(){Impl::LeaseIo io(impl_->identity,impl_->self);checked([&]{return bridge::peer_snapshot_take(*impl_->owner,io);});return std::move(io.issued);}
bool PeerSnapshotCandidate::acknowledge_captured(Delivery &&d){Impl::LeaseIo io(impl_->identity,impl_->self);io.captured=&d;return checked([&]{return bridge::peer_snapshot_capture(*impl_->owner,io);});}
bool PeerSnapshotCandidate::close(const PeerSnapshotToken &t,PeerSnapshotClose mode){return checked([&]{return bridge::peer_snapshot_close(*impl_->owner,wire(t),std::uint32_t(mode));});}
bool PeerSnapshotCandidate::reject(const PeerSnapshotToken &t){return checked([&]{return bridge::peer_snapshot_reject(*impl_->owner,wire(t));});}
bool PeerSnapshotCandidate::can_acknowledge_retired(const PeerSnapshotToken &t,const std::optional<Delivery> &d)const{Impl::LeaseIo io(impl_->identity,impl_->self);io.observed=&d;return checked([&]{return bridge::peer_snapshot_can_retire(*impl_->owner,wire(t),io);});}
bool PeerSnapshotCandidate::acknowledge_retired(const PeerSnapshotToken &t,std::optional<Delivery> &&d){Impl::LeaseIo io(impl_->identity,impl_->self);io.returned=&d;return checked([&]{return bridge::peer_snapshot_retire(*impl_->owner,wire(t),io);});}
PeerSnapshotState PeerSnapshotCandidate::state()const{return checked([&]{const auto v=bridge::peer_snapshot_state(*impl_->owner);PeerSnapshotState out;
    out.context=native(v.context);out.recipients=v.recipients;if(v.pending.present)out.pending=native(v.pending);out.armed=v.armed;out.delivered=v.delivered;out.captured=v.captured;out.retired=v.retired;out.rejected=v.rejected;
    if(v.closing_present)out.closing=PeerSnapshotClose(v.closing);out.published=v.published;out.delivery_outstanding=v.delivery_outstanding;out.stalled=v.stalled;out.buffered_bytes=v.buffered_bytes;out.maximum_buffered_bytes=v.maximum_buffered_bytes;out.rejected_packets=v.rejected_packets;return out;});}
std::vector<AuthorityTransferPacket> PeerSnapshotCandidate::poll(std::uint64_t now,unsigned budget){return checked([&]{std::vector<AuthorityTransferPacket> out;for(const auto &p:bridge::peer_snapshot_poll(*impl_->owner,now,budget))out.push_back({p.peer,native(p.bytes)});return out;});}
bool PeerSnapshotCandidate::receive(unsigned peer,std::span<const std::byte> bytes,std::uint64_t now){return checked([&]{return bridge::peer_snapshot_receive(*impl_->owner,peer,wire(bytes),now);});}
bool PeerSnapshotCandidate::is_packet(std::span<const std::byte> bytes)noexcept{return bridge::peer_snapshot_is_packet(wire(bytes));}
}
