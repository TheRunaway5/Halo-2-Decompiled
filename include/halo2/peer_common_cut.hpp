#pragma once
#include "halo2/peer_authority_transfer.hpp"

namespace halo2 {
// Negotiated through the authority transfer Hello. A legacy endpoint may
// decode native cuts without supporting this independent metadata barrier.
inline constexpr std::uint32_t authority_feature_source_common_cut=512u;
struct PeerCommonCut {
    AuthorityTransferContext context;
    AuthorityCutSeal seal;
    std::uint16_t recipients{};
    bool operator==(const PeerCommonCut &)const=default;
};
struct PeerCommonCutState {
    AuthorityTransferContext context;
    std::uint16_t recipients{};
    std::optional<PeerCommonCut> committed,pending;
    std::uint16_t associated{},applied{},released{},aborted{};
    // Native-all validation is an explicit host input; Commit carries that
    // host assertion to recipients. None of these flags certifies full native,
    // presentation, device, input or playing-audio owner readiness.
    bool native_all_validated{},commit{},release{},abort{};
    std::uint64_t rejected{};
};
// One fixed physical membership/generation. Metadata only: callers retain the
// actual native/frontier associations and publish each local result explicitly.
// Associated is an irrevocable promise to retain that exact pair until Abort or
// Release. All calls are synchronized; no owner, callback, timeout-based member
// removal, host election or migration activation is provided here. Preserve
// this object throughout its generation, including a host crash/partition.
class PeerCommonCutAgreement {
public:
    PeerCommonCutAgreement(unsigned self,AuthorityTransferContext);
    ~PeerCommonCutAgreement();
    PeerCommonCutAgreement(const PeerCommonCutAgreement &)=delete;
    PeerCommonCutAgreement &operator=(const PeerCommonCutAgreement &)=delete;
    AuthorityTransferContext context()const;
    // Host only; recipients must be the full context membership minus host.
    // A successor cannot replace an incompletely Released/Aborted round.
    bool offer(const PeerCommonCut &);
    bool confirm_native(const PeerCommonCut &);
    // Recipient only, after owning native/frontier pin, prepared recovery-pin
    // publication, and old-pin retirement respectively. These operations never
    // apply a simulation or activate a role. Apply/Release also require the
    // complete local Associated/Applied matrices. Host controls may be latched
    // while peer broadcasts catch up; receipt alone never fabricates a bit.
    bool associate(const PeerCommonCut &);
    bool apply(const PeerCommonCut &);
    bool release(const PeerCommonCut &);
    // Missing/stale association may reject only before local Associated.
    // Host Abort is pre-Commit only. Each recipient must then retire its pending
    // pair and explicitly acknowledge Abort before the host can offer again.
    bool reject(const PeerCommonCut &);
    bool abort(const PeerCommonCut &);
    bool acknowledge_abort(const PeerCommonCut &);
    std::vector<AuthorityTransferPacket> poll(std::uint64_t now_ms,unsigned budget=4);
    bool receive(unsigned authenticated_peer,std::span<const std::byte>,std::uint64_t now_ms);
    PeerCommonCutState state()const;
    static bool is_packet(std::span<const std::byte>)noexcept;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
