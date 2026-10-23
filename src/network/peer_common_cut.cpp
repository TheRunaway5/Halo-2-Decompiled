#include "halo2/peer_common_cut.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <algorithm>
#include <mutex>
#include <stdexcept>

namespace halo2 {
namespace {
namespace bridge=online_protocol_bridge;
// Preserve invalid unsigned endpoints before crossing the fixed-width ABI.
std::uint8_t peer_index(unsigned peer){return std::uint8_t(peer<16?peer:16);}
bridge::HostAgreementContext to_bridge_context(const AuthorityTransferContext &value){
    bridge::HostAgreementContext result{};
    std::ranges::transform(value.session,result.session.begin(),[](auto byte){return std::to_integer<std::uint8_t>(byte);});
    result.epoch=value.epoch;result.membership_revision=value.membership_revision;
    result.peers=value.peers;result.host=peer_index(value.host);return result;
}
AuthorityTransferContext from_bridge_context(const bridge::HostAgreementContext &value){
    AuthorityTransferContext result;
    std::ranges::transform(value.session,result.session.begin(),[](auto byte){return std::byte(byte);});
    result.epoch=value.epoch;result.membership_revision=value.membership_revision;
    result.peers=value.peers;result.host=value.host;return result;
}
bridge::CommonCutValue cut(const PeerCommonCut &value){
    bridge::CommonCutValue result{};
    result.present=true;result.context=to_bridge_context(value.context);result.recipients=value.recipients;
    result.seal.present=true;result.seal.id=value.seal.id;result.seal.tick=value.seal.tick;result.seal.bytes=value.seal.bytes;
    std::ranges::transform(value.seal.digest,result.seal.digest.begin(),[](auto byte){return std::to_integer<std::uint8_t>(byte);});
    return result;
}
PeerCommonCut cut(const bridge::CommonCutValue &value){
    PeerCommonCut result{from_bridge_context(value.context),{value.seal.id,value.seal.tick,value.seal.bytes,{}},value.recipients};
    std::ranges::transform(value.seal.digest,result.seal.digest.begin(),[](auto byte){return std::byte(byte);});
    return result;
}
template<class F>auto checked(F &&operation){
    try{return operation();}
    catch(const rust::Error &error){throw std::invalid_argument(error.what());}
}
auto make_agreement(unsigned self,const AuthorityTransferContext &value){
    if(self>=16||value.host>=16)throw std::invalid_argument("Invalid peer common-cut association or packet");
    return checked([&]{return bridge::new_common_cut_agreement(peer_index(self),to_bridge_context(value),native_multiplayer_protocol);});
}
auto bytes(std::span<const std::byte> packet){
    return rust::Slice<const std::uint8_t>{reinterpret_cast<const std::uint8_t *>(packet.data()),packet.size()};
}
}
// The native owner supplies synchronization only. The bounded control codec,
// association phases, fixed membership and retry cursor belong to Rust.
struct PeerCommonCutAgreement::Impl {
    mutable std::mutex mutex;
    rust::Box<bridge::CommonCutAgreement> agreement;
    Impl(unsigned self,const AuthorityTransferContext &value):agreement(make_agreement(self,value)){}
    template<class F>bool action(const PeerCommonCut &value,F operation){
        std::lock_guard lock(mutex);
        return checked([&]{return operation(*agreement,cut(value));});
    }
};
PeerCommonCutAgreement::PeerCommonCutAgreement(unsigned self,AuthorityTransferContext value)
    :impl_(std::make_unique<Impl>(self,value)){}
PeerCommonCutAgreement::~PeerCommonCutAgreement()=default;
AuthorityTransferContext PeerCommonCutAgreement::context()const{
    const auto &s=*impl_;std::lock_guard lock(s.mutex);
    return from_bridge_context(checked([&]{return bridge::common_cut_context(*s.agreement);}));
}
bool PeerCommonCutAgreement::offer(const PeerCommonCut &value){return impl_->action(value,bridge::common_cut_offer);}
bool PeerCommonCutAgreement::confirm_native(const PeerCommonCut &value){return impl_->action(value,bridge::common_cut_confirm_native);}
bool PeerCommonCutAgreement::associate(const PeerCommonCut &value){return impl_->action(value,bridge::common_cut_associate);}
bool PeerCommonCutAgreement::apply(const PeerCommonCut &value){return impl_->action(value,bridge::common_cut_apply);}
bool PeerCommonCutAgreement::release(const PeerCommonCut &value){return impl_->action(value,bridge::common_cut_release);}
bool PeerCommonCutAgreement::reject(const PeerCommonCut &value){return impl_->action(value,bridge::common_cut_reject);}
bool PeerCommonCutAgreement::abort(const PeerCommonCut &value){return impl_->action(value,bridge::common_cut_abort);}
bool PeerCommonCutAgreement::acknowledge_abort(const PeerCommonCut &value){return impl_->action(value,bridge::common_cut_acknowledge_abort);}
std::vector<AuthorityTransferPacket> PeerCommonCutAgreement::poll(std::uint64_t now,unsigned budget){
    auto &s=*impl_;std::lock_guard lock(s.mutex);
    const auto packets=checked([&]{return bridge::common_cut_poll(*s.agreement,now,budget);});
    std::vector<AuthorityTransferPacket> result;result.reserve(packets.size());
    for(const auto &packet:packets){
        AuthorityTransferPacket value{packet.peer,{}};value.bytes.reserve(packet.bytes.size());
        for(const auto byte:packet.bytes)value.bytes.push_back(std::byte(byte));
        result.push_back(std::move(value));
    }
    return result;
}
bool PeerCommonCutAgreement::is_packet(std::span<const std::byte> packet)noexcept{
    return bridge::common_cut_is_packet(bytes(packet));
}
bool PeerCommonCutAgreement::receive(unsigned peer,std::span<const std::byte> packet,std::uint64_t now){
    if(!is_packet(packet))return false;
    auto &s=*impl_;std::lock_guard lock(s.mutex);
    return checked([&]{return bridge::common_cut_receive(*s.agreement,peer_index(peer),bytes(packet),now);});
}
PeerCommonCutState PeerCommonCutAgreement::state()const{
    const auto &s=*impl_;std::lock_guard lock(s.mutex);
    const auto value=checked([&]{return bridge::common_cut_state(*s.agreement);});
    PeerCommonCutState result{from_bridge_context(value.context),value.recipients,{},{},value.associated,value.applied,
        value.released,value.aborted,value.native_all_validated,value.commit,value.release,value.abort,value.rejected};
    if(value.committed.present)result.committed=cut(value.committed);
    if(value.pending.present)result.pending=cut(value.pending);
    return result;
}
}
