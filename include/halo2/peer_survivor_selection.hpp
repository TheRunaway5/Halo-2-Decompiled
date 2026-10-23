#pragma once
#include "halo2/peer_authority_transfer.hpp"
#include "halo2/peer_common_cut.hpp"

namespace halo2 {
inline constexpr std::uint32_t authority_feature_survivor_selection=2048u;
struct PeerSurvivorInventoryEntry {
    AuthorityCutSeal cut;
    // An actually observed irreversible CommonCommit/Release for this exact
    // seal. Native import, byte delivery and Associated alone do not qualify.
    bool irreversible_common{};
};
struct PeerSurvivorInventory {
    std::uint64_t lease{};
    std::array<std::optional<PeerSurvivorInventoryEntry>,3> cuts;
};
enum class PeerSurvivorInventoryAdmission { Admit,Refuse,Invalidate };
struct PeerSurvivorInventoryPreparation {
    PeerSurvivorInventory inventory;
    PeerSurvivorInventoryAdmission admission;
};
// Rust metadata policy only. Session admission and genuine retained native /
// foreground leases remain caller obligations. Neither function publishes Ready.
PeerSurvivorInventoryAdmission classify_survivor_inventory(const AuthorityTransferContext &,
    const PeerSurvivorInventory &,const std::optional<PeerCommonCutState> &);
PeerSurvivorInventoryPreparation make_retained_survivor_inventory(const AuthorityTransferContext &,
    std::uint64_t lease,const std::array<std::optional<AuthorityCutSeal>,3> &retained,
    const std::optional<PeerCommonCutState> &);
struct PeerSurvivorSelectionState {
    AuthorityTransferContext context;
    std::uint16_t survivors{},inventory_peers{},accepted{};
    std::optional<AuthorityCutSeal> selected;
    AuthorityTransferDigest transcript{};
    bool selection_complete{},failed{};
    std::uint16_t ready_observed{};
    std::uint64_t rejected{};
};
// Metadata only, under the existing crash/partition model. The caller freezes
// and retains the actual native/foreground owners before publishing a lease.
// Every fixed survivor supplies its own authenticated inventory; an Accept
// never supplies another peer's missing row. The single attempt fails closed
// permanently, with no timeout membership removal, restart or native readiness.
class PeerSurvivorSelection {
public:
    PeerSurvivorSelection(unsigned self,AuthorityTransferContext);
    ~PeerSurvivorSelection();
    PeerSurvivorSelection(const PeerSurvivorSelection &)=delete;
    PeerSurvivorSelection &operator=(const PeerSurvivorSelection &)=delete;
    bool publish_inventory(const PeerSurvivorInventory &);
    bool invalidate(std::optional<AuthorityCutSeal> = {});
    bool allows_ready(unsigned peer,const AuthorityCutSeal &)const;
    // Invoke only after an actual HostAgreement Ready was published/learned.
    bool observe_ready(unsigned peer,const AuthorityCutSeal &);
    std::vector<AuthorityTransferPacket> poll(std::uint64_t now_ms,unsigned budget=4);
    bool receive(unsigned authenticated_peer,std::span<const std::byte>,std::uint64_t now_ms);
    PeerSurvivorSelectionState state()const;
    static bool is_packet(std::span<const std::byte>)noexcept;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
