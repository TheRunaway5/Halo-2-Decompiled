#pragma once
#include "halo2/peer_host_agreement.hpp"
#include "halo2/peer_survivor_selection.hpp"
#include <memory>

namespace halo2 {
// Actual Session metadata, captured together under its generation mutex.
// Phase ordinals match OnlineMatchSession::Phase and are checked in Rust.
struct PeerMigrationPreparationObservation {
    std::optional<AuthorityTransferContext> context,admitted;
    unsigned self{};
    std::uint8_t phase{};
    bool supported{},selection_started{};
    std::optional<PeerHostAgreementState> agreement;
    std::optional<PeerSurvivorSelectionState> selection;
};
struct PeerMigrationPreparationTicket {
    AuthorityTransferContext context;
    unsigned self{};
    std::uint64_t id{};
    bool operator==(const PeerMigrationPreparationTicket &)const=default;
};
enum class PeerMigrationPreparationActionKind { Wait,Prepare,Retire,Published,Terminal };
enum class PeerMigrationPreparationReason {
    None,Healthy,Unavailable,Stale,Provenance,Aborted,Published,Fenced,Exhausted,NativeFailure
};
struct PeerMigrationPreparationAction {
    PeerMigrationPreparationActionKind kind;
    std::optional<PeerMigrationPreparationTicket> ticket;
    PeerMigrationPreparationReason reason;
};
enum class PeerMigrationPreparationFreezeKind { Publish,Retire,Refuse,Invalidate };
struct PeerMigrationPreparationFreeze {
    PeerMigrationPreparationFreezeKind kind;
    PeerMigrationPreparationReason reason;
};
enum class PeerMigrationPreparationRetirement { Retryable,RetiredButFenced,Stale };
// Session-owned Rust scheduling/admission. Requests never certify real native
// leases, quiescence, resource readiness or an election decision. The caller
// serializes each observation/admission and actual publication under its lock.
class PeerMigrationPreparation {
public:
    PeerMigrationPreparation(unsigned self,const AuthorityTransferContext &);
    ~PeerMigrationPreparation();
    PeerMigrationPreparation(const PeerMigrationPreparation &)=delete;
    PeerMigrationPreparation &operator=(const PeerMigrationPreparation &)=delete;
    PeerMigrationPreparationAction step(const PeerMigrationPreparationObservation &);
    bool retained(const PeerMigrationPreparationTicket &,std::uint64_t lease);
    PeerMigrationPreparationFreeze admit_freeze(const PeerMigrationPreparationObservation &,
        const PeerMigrationPreparationTicket &,const PeerSurvivorInventory &,const std::optional<PeerCommonCutState> &);
    PeerMigrationPreparationFreeze admit_external_freeze(const PeerMigrationPreparationObservation &,
        const PeerSurvivorInventory &,const std::optional<PeerCommonCutState> &);
    bool reserve_retirement(const PeerMigrationPreparationObservation &,const PeerMigrationPreparationTicket &);
    PeerMigrationPreparationRetirement retired(const PeerMigrationPreparationObservation &,const PeerMigrationPreparationTicket &);
    bool blocks_publication()const;
    bool retirement_pending()const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
