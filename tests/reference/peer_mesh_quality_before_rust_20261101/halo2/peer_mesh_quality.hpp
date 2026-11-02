#pragma once
#include "halo2/peer_authority_transfer.hpp"
#include <array>
#include <optional>

namespace halo2 {
struct PeerLinkQuality {
    std::uint32_t rtt_ms=UINT32_MAX,upload_bytes_per_second{};
    std::uint16_t loss_per_mille=1000;
    bool measured{};
};
struct PeerQualityRow {
    std::array<PeerLinkQuality,16> links;
    std::uint64_t received_ms{};
};
struct PeerHostQuality {
    unsigned peer=16;
    std::uint32_t mean_rtt_ms{},minimum_upload_bytes_per_second{};
    std::uint16_t worst_loss_per_mille{};
};
// Measurement only; never elects, certifies or disconnects a player. Context
// and authenticated_peer must come from the accepted direct DTLS connection.
// Upload is acknowledged application delivery under this bounded offered load,
// not an ISP line-speed claim. NAT type is deliberately not inferred from RTT
// or ICE candidate names. Every public operation is synchronized.
class PeerMeshQuality {
public:
    PeerMeshQuality(unsigned self,AuthorityTransferContext);
    ~PeerMeshQuality();
    AuthorityTransferContext context()const;
    std::vector<AuthorityTransferPacket> poll(std::uint16_t direct_ready,std::uint64_t now_ms,unsigned budget=4);
    bool receive(unsigned authenticated_peer,std::span<const std::byte>,std::uint64_t now_ms);
    std::array<std::optional<PeerQualityRow>,16> matrix(std::uint64_t now_ms)const;
    std::optional<PeerQualityRow> local(std::uint64_t now_ms)const;
    // Require fresh, measured paths in both directions across the physical
    // membership. Sort by measured loss, upload bottleneck, mean RTT, peer ID.
    // Callers still require a fully validated native world for promotion.
    std::vector<PeerHostQuality> candidates(std::uint64_t now_ms)const;
    // Score the fixed successor membership: every admitted PC except this
    // context's predecessor host. Never remove another peer on a local timeout.
    // This is measurement only; the host/cut quorum still certifies succession.
    // A sole survivor has no inter-PC measurement and returns no quality row.
    std::vector<PeerHostQuality> migration_candidates(std::uint64_t now_ms)const;
    static bool is_packet(std::span<const std::byte>)noexcept;
private:
    std::vector<PeerHostQuality> rank(std::uint64_t,std::uint16_t)const;
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
