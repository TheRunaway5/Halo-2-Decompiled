#pragma once
#include "halo2/peer_authority_transfer.hpp"

namespace halo2 {
inline constexpr std::uint32_t authority_feature_source_snapshot_candidate=1024u;
struct PeerSnapshotToken {
    AuthorityTransferContext context;
    std::uint16_t recipients{};
    std::uint64_t nonce{};
    bool operator==(const PeerSnapshotToken &)const=default;
};
enum class PeerSnapshotClose { Release,Abort };
struct PeerSnapshotArchive { unsigned peer{};std::vector<std::byte> bytes; };
struct PeerSnapshotLimits {
    // Native body plus unchanged packet headers and archive framing. At most
    // fifteen physical recipients: host archive storage is bounded by 135 MiB.
    std::uint64_t maximum_archive_bytes=9ull*1024*1024;
    std::uint64_t retry_ms=100,idle_ms=15'000;
    unsigned window_chunks=32;
};
struct PeerSnapshotState {
    AuthorityTransferContext context;
    std::uint16_t recipients{};
    std::optional<PeerSnapshotToken> pending;
    std::uint16_t armed{},delivered{},captured{},retired{},rejected{};
    std::optional<PeerSnapshotClose> closing;
    bool published{},delivery_outstanding{},stalled{};
    std::uint64_t buffered_bytes{},maximum_buffered_bytes{},rejected_packets{};
};
// Opaque, bounded direct-peer delivery with an explicit foreground lifetime.
// An Arm request is not an adoption fence. Only the owning foreground executor
// may acknowledge Arm, after draining its already accepted batch and applying
// the ordinary adoption fence. Captured and Retired likewise assert actual
// foreground work, never native proof, world readiness or host eligibility.
// All methods are synchronized. No native owners, callbacks, role changes,
// master relay, automatic member removal or timeout-generated ACKs are used.
class PeerSnapshotCandidate {
public:
    class Delivery {
    public:
        Delivery(Delivery &&)noexcept;
        Delivery &operator=(Delivery &&)noexcept;
        ~Delivery();
        Delivery(const Delivery &)=delete;
        Delivery &operator=(const Delivery &)=delete;
        const PeerSnapshotToken &token()const noexcept{return token_;}
        unsigned source()const noexcept{return token_.context.host;}
        unsigned recipient()const noexcept{return recipient_;}
        std::span<const std::byte> bytes()const noexcept;
    private:
        friend class PeerSnapshotCandidate;
        Delivery(PeerSnapshotToken,unsigned,std::unique_ptr<const std::vector<std::byte>>,
                 std::shared_ptr<const void>);
        PeerSnapshotToken token_;
        unsigned recipient_{};
        std::unique_ptr<const std::vector<std::byte>> bytes_;
        std::shared_ptr<const void> identity_;
    };
    PeerSnapshotCandidate(unsigned self,AuthorityTransferContext,PeerSnapshotLimits={});
    ~PeerSnapshotCandidate();
    PeerSnapshotCandidate(const PeerSnapshotCandidate &)=delete;
    PeerSnapshotCandidate &operator=(const PeerSnapshotCandidate &)=delete;
    unsigned self()const noexcept;
    // Host only. A successor requires every original physical retirement ACK.
    std::optional<PeerSnapshotToken> begin(std::uint64_t now_ms);
    bool acknowledge_arm(const PeerSnapshotToken &);
    // Host only, atomic exact-recipient batch after all Armed ACKs. Archive
    // bytes are opaque and unchanged; each recipient has its own archive.
    bool publish(const PeerSnapshotToken &,std::vector<PeerSnapshotArchive>,std::uint64_t now_ms);
    std::optional<Delivery> take_delivery();
    // On false, the caller retains its Delivery. On true, consumes its exact
    // issued lease. Native capture/adoption remains the executor's obligation.
    bool acknowledge_captured(Delivery &&);
    bool close(const PeerSnapshotToken &,PeerSnapshotClose);
    // Recipient request for host Abort; never manufactures Captured/Retired.
    bool reject(const PeerSnapshotToken &);
    // Synchronized observation only; does not consume bytes or reserve a later
    // acknowledgment. The actual acknowledgment rechecks these same guards.
    bool can_acknowledge_retired(const PeerSnapshotToken &,const std::optional<Delivery> &)const;
    // Any checked-out Delivery must be returned here for Abort retirement.
    // On false the optional and its bytes remain owned by the caller.
    bool acknowledge_retired(const PeerSnapshotToken &,std::optional<Delivery> &&);
    PeerSnapshotState state()const;
    std::vector<AuthorityTransferPacket> poll(std::uint64_t now_ms,unsigned packet_budget=4);
    bool receive(unsigned authenticated_peer,std::span<const std::byte>,std::uint64_t now_ms);
    static bool is_packet(std::span<const std::byte>)noexcept;
private:
    bool can_acknowledge_retired_locked(const PeerSnapshotToken &,const std::optional<Delivery> &)const;
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
