#include "halo2/peer_migration_preparation.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <algorithm>
#include <stdexcept>

namespace halo2 {
namespace {
namespace bridge=online_protocol_bridge;
std::uint8_t endpoint(unsigned value){return std::uint8_t(value<16?value:16);}
auto context(const AuthorityTransferContext &value){
    bridge::HostAgreementContext out{};
    std::ranges::transform(value.session,out.session.begin(),[](auto v){return std::to_integer<std::uint8_t>(v);});
    out.epoch=value.epoch;out.membership_revision=value.membership_revision;out.peers=value.peers;out.host=endpoint(value.host);return out;
}
auto context(const bridge::HostAgreementContext &value){
    AuthorityTransferContext out;
    std::ranges::transform(value.session,out.session.begin(),[](auto v){return std::byte(v);});
    out.epoch=value.epoch;out.membership_revision=value.membership_revision;out.peers=value.peers;out.host=value.host;return out;
}
auto cut(const AuthorityCutSeal &value){
    bridge::MigrationPreparationCut out{};out.present=true;out.id=value.id;out.tick=value.tick;out.bytes=value.bytes;
    std::ranges::transform(value.digest,out.digest.begin(),[](auto v){return std::to_integer<std::uint8_t>(v);});return out;
}
auto ticket(const PeerMigrationPreparationTicket &value){
    bridge::MigrationDriverTicket out{};out.present=true;out.context=context(value.context);out.self_peer=endpoint(value.self);out.id=value.id;return out;
}
auto ticket(const bridge::MigrationDriverTicket &value){
    return PeerMigrationPreparationTicket{context(value.context),value.self_peer,value.id};
}
auto inventory(const PeerSurvivorInventory &value){
    bridge::SurvivorInventory out{};out.lease=value.lease;
    for(unsigned i=0;i<value.cuts.size();++i)if(value.cuts[i]){
        out.cuts[i].cut=cut(value.cuts[i]->cut);out.cuts[i].irreversible_common=value.cuts[i]->irreversible_common;
    }
    return out;
}
auto common(const std::optional<PeerCommonCutState> &value){
    bridge::CommonCutState out{};if(!value)return out;
    out.context=context(value->context);out.recipients=value->recipients;
    const auto convert=[](const std::optional<PeerCommonCut> &v){bridge::CommonCutValue out{};
        if(v){out.present=true;out.context=context(v->context);out.seal=cut(v->seal);out.recipients=v->recipients;}return out;};
    out.committed=convert(value->committed);out.pending=convert(value->pending);
    out.associated=value->associated;out.applied=value->applied;out.released=value->released;out.aborted=value->aborted;
    out.native_all_validated=value->native_all_validated;out.commit=value->commit;out.release=value->release;
    out.abort=value->abort;out.rejected=value->rejected;return out;
}
auto observation(const PeerMigrationPreparationObservation &value){
    bridge::MigrationDriverObservation out{};out.self_peer=endpoint(value.self);out.phase=value.phase;
    out.supported=value.supported;out.selection_started=value.selection_started;
    if(value.context){out.context_present=true;out.context=context(*value.context);}
    if(value.admitted){out.admitted_present=true;out.admitted=context(*value.admitted);}
    if(value.agreement){out.agreement_present=true;const auto &v=*value.agreement;auto &a=out.agreement;
        a.suspected=v.suspected;a.fenced=v.fenced;a.electorate=v.electorate;a.silent=v.silent;
        a.majority=v.majority;a.rejected=v.rejected;
        for(unsigned i=0;i<v.ready.size();++i)if(v.ready[i])a.ready[i]=cut(*v.ready[i]);
        if(v.decision){const auto &d=*v.decision;auto &p=a.decision;p.present=true;
            p.previous=context(d.previous);p.next=context(d.next);p.ballot.round=d.ballot.round;
            p.ballot.proposer=endpoint(d.ballot.proposer);p.value.host=endpoint(d.value.host);
            p.value.cut=cut(d.value.cut);p.voters=d.voters;}
    }
    if(value.selection){out.selection_present=true;const auto &v=*value.selection;auto &s=out.selection;
        s.context=context(v.context);s.survivors=v.survivors;s.inventory_peers=v.inventory_peers;s.accepted=v.accepted;
        if(v.selected)s.selected=cut(*v.selected);
        std::ranges::transform(v.transcript,s.transcript.begin(),[](auto b){return std::to_integer<std::uint8_t>(b);});
        s.selection_complete=v.selection_complete;s.failed=v.failed;s.ready_observed=v.ready_observed;s.rejected=v.rejected;
    }
    return out;
}
PeerMigrationPreparationFreeze freeze(const bridge::MigrationDriverFreeze &v){
    return {static_cast<PeerMigrationPreparationFreezeKind>(v.kind),static_cast<PeerMigrationPreparationReason>(v.reason)};
}
template<class F>auto protect(F &&work){
    try{return work();}catch(const rust::Error &error){throw std::invalid_argument(error.what());}
}
}
struct PeerMigrationPreparation::Impl {
    rust::Box<bridge::MigrationDriver> driver;
    Impl(unsigned self,const AuthorityTransferContext &value):driver(bridge::new_migration_driver(endpoint(self),context(value))){}
};
PeerMigrationPreparation::PeerMigrationPreparation(unsigned self,const AuthorityTransferContext &context)
    :impl_(protect([&]{return std::make_unique<Impl>(self,context);})){}
PeerMigrationPreparation::~PeerMigrationPreparation()=default;
PeerMigrationPreparationAction PeerMigrationPreparation::step(const PeerMigrationPreparationObservation &value){
    return protect([&]{const auto result=bridge::migration_driver_step(*impl_->driver,observation(value));
        return PeerMigrationPreparationAction{static_cast<PeerMigrationPreparationActionKind>(result.kind),
            result.ticket.present?std::optional{ticket(result.ticket)}:std::nullopt,
            static_cast<PeerMigrationPreparationReason>(result.reason)};});
}
bool PeerMigrationPreparation::retained(const PeerMigrationPreparationTicket &value,std::uint64_t lease){
    return protect([&]{return bridge::migration_driver_retained(*impl_->driver,ticket(value),lease);});
}
PeerMigrationPreparationFreeze PeerMigrationPreparation::admit_freeze(const PeerMigrationPreparationObservation &observed,
    const PeerMigrationPreparationTicket &value,const PeerSurvivorInventory &retained,const std::optional<PeerCommonCutState> &state){
    return protect([&]{return freeze(bridge::migration_driver_admit_freeze(*impl_->driver,observation(observed),ticket(value),inventory(retained),bool(state),common(state)));});
}
PeerMigrationPreparationFreeze PeerMigrationPreparation::admit_external_freeze(const PeerMigrationPreparationObservation &observed,
    const PeerSurvivorInventory &retained,const std::optional<PeerCommonCutState> &state){
    return protect([&]{return freeze(bridge::migration_driver_admit_external_freeze(*impl_->driver,observation(observed),inventory(retained),bool(state),common(state)));});
}
bool PeerMigrationPreparation::reserve_retirement(const PeerMigrationPreparationObservation &observed,const PeerMigrationPreparationTicket &value){
    return protect([&]{return bridge::migration_driver_reserve_retirement(*impl_->driver,observation(observed),ticket(value));});
}
PeerMigrationPreparationRetirement PeerMigrationPreparation::retired(const PeerMigrationPreparationObservation &observed,const PeerMigrationPreparationTicket &value){
    return protect([&]{return static_cast<PeerMigrationPreparationRetirement>(bridge::migration_driver_retired(*impl_->driver,observation(observed),ticket(value)));});
}
bool PeerMigrationPreparation::blocks_publication()const{return bridge::migration_driver_blocks_publication(*impl_->driver);}
bool PeerMigrationPreparation::retirement_pending()const{return bridge::migration_driver_retirement_pending(*impl_->driver);}
}
