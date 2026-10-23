#include "halo2/peer_survivor_selection.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <algorithm>
#include <mutex>
#include <stdexcept>

namespace halo2 {
namespace {
namespace bridge=online_protocol_bridge;
std::uint8_t index(unsigned peer){return std::uint8_t(peer<16?peer:16);}
bridge::HostAgreementContext context(const AuthorityTransferContext &value){
    bridge::HostAgreementContext out{};
    std::ranges::transform(value.session,out.session.begin(),[](auto b){return std::to_integer<std::uint8_t>(b);});
    out.epoch=value.epoch;out.membership_revision=value.membership_revision;out.peers=value.peers;out.host=index(value.host);return out;
}
AuthorityTransferContext context(const bridge::HostAgreementContext &value){
    AuthorityTransferContext out;std::ranges::transform(value.session,out.session.begin(),[](auto b){return std::byte(b);});
    out.epoch=value.epoch;out.membership_revision=value.membership_revision;out.peers=value.peers;out.host=value.host;return out;
}
bridge::MigrationPreparationCut cut(const std::optional<AuthorityCutSeal> &value){
    bridge::MigrationPreparationCut out{};if(!value)return out;
    out.present=true;out.id=value->id;out.tick=value->tick;out.bytes=value->bytes;
    std::ranges::transform(value->digest,out.digest.begin(),[](auto b){return std::to_integer<std::uint8_t>(b);});return out;
}
AuthorityCutSeal cut(const bridge::MigrationPreparationCut &value){
    AuthorityCutSeal out{value.id,value.tick,value.bytes,{}};
    std::ranges::transform(value.digest,out.digest.begin(),[](auto b){return std::byte(b);});return out;
}
template<class F>auto checked(F &&operation){
    try{return operation();}catch(const rust::Error &error){throw std::invalid_argument(error.what());}
}
auto make(unsigned self,const AuthorityTransferContext &value){
    if(self>=16||value.host>=16)throw std::invalid_argument("Invalid survivor cut selection");
    return checked([&]{return bridge::new_survivor_selection(index(self),context(value),native_multiplayer_protocol);});
}
auto bytes(std::span<const std::byte> packet){return rust::Slice<const std::uint8_t>{reinterpret_cast<const std::uint8_t *>(packet.data()),packet.size()};}
auto inventory(const PeerSurvivorInventory &value){
    bridge::SurvivorInventory out{};out.lease=value.lease;
    for(unsigned i=0;i<value.cuts.size();++i)if(value.cuts[i]){
        out.cuts[i].cut=cut(value.cuts[i]->cut);out.cuts[i].irreversible_common=value.cuts[i]->irreversible_common;
    }
    return out;
}
PeerSurvivorInventory inventory(const bridge::SurvivorInventory &value){
    PeerSurvivorInventory out;out.lease=value.lease;
    for(unsigned i=0;i<out.cuts.size();++i)if(value.cuts[i].cut.present)
        out.cuts[i]=PeerSurvivorInventoryEntry{cut(value.cuts[i].cut),value.cuts[i].irreversible_common};
    return out;
}
auto metadata_context(const AuthorityTransferContext &value){
    if(value.host>=16)throw std::invalid_argument("Invalid survivor inventory physical endpoint");
    return context(value);
}
auto common_value(const std::optional<PeerCommonCut> &value){
    bridge::CommonCutValue out{};if(!value)return out;
    out.present=true;out.context=metadata_context(value->context);out.seal=cut(value->seal);out.recipients=value->recipients;return out;
}
auto common_state(const std::optional<PeerCommonCutState> &value){
    bridge::CommonCutState out{};if(!value)return out;
    out.context=metadata_context(value->context);out.recipients=value->recipients;
    out.committed=common_value(value->committed);out.pending=common_value(value->pending);
    out.associated=value->associated;out.applied=value->applied;out.released=value->released;out.aborted=value->aborted;
    out.native_all_validated=value->native_all_validated;out.commit=value->commit;out.release=value->release;
    out.abort=value->abort;out.rejected=value->rejected;return out;
}
PeerSurvivorInventoryAdmission admission(bridge::SurvivorInventoryAdmission value){
    switch(value){
    case bridge::SurvivorInventoryAdmission::Admit:return PeerSurvivorInventoryAdmission::Admit;
    case bridge::SurvivorInventoryAdmission::Refuse:return PeerSurvivorInventoryAdmission::Refuse;
    case bridge::SurvivorInventoryAdmission::Invalidate:return PeerSurvivorInventoryAdmission::Invalidate;
    }
    throw std::invalid_argument("Unknown Rust survivor inventory admission");
}
}
PeerSurvivorInventoryAdmission classify_survivor_inventory(const AuthorityTransferContext &binding,
    const PeerSurvivorInventory &value,const std::optional<PeerCommonCutState> &common){
    const auto context_value=metadata_context(binding);const auto common_snapshot=common_state(common);
    return checked([&]{return admission(bridge::survivor_inventory_admission(context_value,inventory(value),bool(common),common_snapshot));});
}
PeerSurvivorInventoryPreparation make_retained_survivor_inventory(const AuthorityTransferContext &binding,
    std::uint64_t lease,const std::array<std::optional<AuthorityCutSeal>,3> &retained,
    const std::optional<PeerCommonCutState> &common){
    const auto context_value=metadata_context(binding);const auto common_snapshot=common_state(common);
    std::array<bridge::MigrationPreparationCut,3> cuts{};
    for(unsigned i=0;i<retained.size();++i)cuts[i]=cut(retained[i]);
    return checked([&]{const auto value=bridge::survivor_retained_inventory(context_value,lease,
        {cuts.data(),cuts.size()},bool(common),common_snapshot);
        return PeerSurvivorInventoryPreparation{inventory(value.inventory),admission(value.admission)};});
}
struct PeerSurvivorSelection::Impl {
    mutable std::mutex mutex;
    rust::Box<bridge::SurvivorSelection> owner;
    Impl(unsigned self,const AuthorityTransferContext &binding):owner(make(self,binding)){}
};
PeerSurvivorSelection::PeerSurvivorSelection(unsigned self,AuthorityTransferContext binding):impl_(std::make_unique<Impl>(self,binding)){}
PeerSurvivorSelection::~PeerSurvivorSelection()=default;
bool PeerSurvivorSelection::publish_inventory(const PeerSurvivorInventory &input){
    const auto value=inventory(input);
    auto &s=*impl_;std::lock_guard lock(s.mutex);return checked([&]{return bridge::survivor_selection_publish_inventory(*s.owner,value);});
}
bool PeerSurvivorSelection::invalidate(std::optional<AuthorityCutSeal> value){
    auto &s=*impl_;std::lock_guard lock(s.mutex);return checked([&]{return bridge::survivor_selection_invalidate(*s.owner,cut(value));});
}
bool PeerSurvivorSelection::allows_ready(unsigned peer,const AuthorityCutSeal &value)const{
    const auto &s=*impl_;std::lock_guard lock(s.mutex);return checked([&]{return bridge::survivor_selection_allows_ready(*s.owner,index(peer),cut(value));});
}
bool PeerSurvivorSelection::observe_ready(unsigned peer,const AuthorityCutSeal &value){
    auto &s=*impl_;std::lock_guard lock(s.mutex);return checked([&]{return bridge::survivor_selection_observe_ready(*s.owner,index(peer),cut(value));});
}
std::vector<AuthorityTransferPacket> PeerSurvivorSelection::poll(std::uint64_t now,unsigned budget){
    auto &s=*impl_;std::lock_guard lock(s.mutex);const auto packets=checked([&]{return bridge::survivor_selection_poll(*s.owner,now,budget);});
    std::vector<AuthorityTransferPacket> out;out.reserve(packets.size());
    for(const auto &packet:packets){AuthorityTransferPacket value{packet.peer,{}};value.bytes.reserve(packet.bytes.size());
        for(auto byte:packet.bytes)value.bytes.push_back(std::byte(byte));out.push_back(std::move(value));}return out;
}
bool PeerSurvivorSelection::receive(unsigned peer,std::span<const std::byte> packet,std::uint64_t now){
    if(!is_packet(packet))return false;
    auto &s=*impl_;std::lock_guard lock(s.mutex);return checked([&]{return bridge::survivor_selection_receive(*s.owner,index(peer),bytes(packet),now);});
}
bool PeerSurvivorSelection::is_packet(std::span<const std::byte> packet)noexcept{return bridge::survivor_selection_is_packet(bytes(packet));}
PeerSurvivorSelectionState PeerSurvivorSelection::state()const{
    const auto &s=*impl_;std::lock_guard lock(s.mutex);const auto value=checked([&]{return bridge::survivor_selection_state(*s.owner);});
    PeerSurvivorSelectionState out{context(value.context),value.survivors,value.inventory_peers,value.accepted,{}, {},
        value.selection_complete,value.failed,value.ready_observed,value.rejected};
    if(value.selected.present)out.selected=cut(value.selected);
    std::ranges::transform(value.transcript,out.transcript.begin(),[](auto b){return std::byte(b);});return out;
}
}
