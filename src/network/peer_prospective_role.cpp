#include "halo2/peer_prospective_role.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <algorithm>
#include <stdexcept>

namespace halo2 {
namespace {
namespace bridge=online_protocol_bridge;
auto context(const AuthorityTransferContext &value){
    bridge::HostAgreementContext out{};
    std::ranges::transform(value.session,out.session.begin(),[](auto byte){return std::to_integer<std::uint8_t>(byte);});
    out.epoch=value.epoch;out.membership_revision=value.membership_revision;out.peers=value.peers;out.host=std::uint8_t(value.host);return out;
}
auto context(const bridge::HostAgreementContext &value){
    AuthorityTransferContext out;
    std::ranges::transform(value.session,out.session.begin(),[](auto byte){return std::byte(byte);});
    out.epoch=value.epoch;out.membership_revision=value.membership_revision;out.peers=value.peers;out.host=value.host;return out;
}
auto cut(const AuthorityCutSeal &value){
    bridge::MigrationPreparationCut out{};out.present=true;out.id=value.id;out.tick=value.tick;out.bytes=value.bytes;
    std::ranges::transform(value.digest,out.digest.begin(),[](auto byte){return std::to_integer<std::uint8_t>(byte);});return out;
}
auto cut(const bridge::MigrationPreparationCut &value){
    AuthorityCutSeal out{value.id,value.tick,value.bytes,{}};
    std::ranges::transform(value.digest,out.digest.begin(),[](auto byte){return std::byte(byte);});return out;
}
}
PeerProspectiveHostRole derive_peer_prospective_host_role(
    const AuthorityTransferContext &previous,const AuthorityCutSeal &selected,unsigned self){
    // Validate public unsigned endpoints before the fixed-width ABI. All
    // membership, checked successor arithmetic and seal policy stays in Rust.
    if(self>=16||previous.host>=16)throw std::invalid_argument("Invalid prospective-role physical endpoint");
    try{
        const auto value=bridge::derive_prospective_host_role(context(previous),cut(selected),std::uint8_t(self));
        return {context(value.previous),context(value.next),cut(value.cut)};
    }catch(const rust::Error &error){throw std::invalid_argument(error.what());}
}
PeerProspectiveClientRole derive_peer_prospective_client_role(
    const AuthorityTransferContext &previous,const AuthorityCutSeal &selected,unsigned self){
    if(self>=16||previous.host>=16)throw std::invalid_argument("Invalid prospective client physical endpoint");
    try{
        const auto value=bridge::derive_prospective_client_role(context(previous),cut(selected),std::uint8_t(self));
        return {context(value.previous),cut(value.cut),value.self_peer,value.next_epoch,value.next_membership_revision,value.survivors};
    }catch(const rust::Error &error){throw std::invalid_argument(error.what());}
}
AuthorityTransferContext bind_peer_prospective_client_role(
    const PeerProspectiveClientRole &role,const PeerMigrationDecision &decision){
    // Guard every public unsigned endpoint that crosses the fixed-width ABI.
    // Ballot/voters are intentionally left to Runtime's actual certificate
    // comparison; no placeholder Decision or election evidence is constructed.
    if(role.self>=16||role.previous.host>=16||decision.previous.host>=16||decision.next.host>=16||decision.value.host>=16)
        throw std::invalid_argument("Invalid prospective client certificate endpoint");
    bridge::ProspectiveClientRoleDescriptor value{};value.previous=context(role.previous);value.cut=cut(role.cut);
    value.self_peer=std::uint8_t(role.self);value.next_epoch=role.next_epoch;
    value.next_membership_revision=role.next_membership_revision;value.survivors=role.survivors;
    bridge::HostAgreementValue endpoint{};endpoint.host=std::uint8_t(decision.value.host);endpoint.cut=cut(decision.value.cut);
    try{return context(bridge::bind_prospective_client_role(value,context(decision.previous),context(decision.next),endpoint));}
    catch(const rust::Error &error){throw std::invalid_argument(error.what());}
}
}
