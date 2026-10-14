#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace halo2 {
using AuthorityTransferDigest=std::array<std::byte,32>;
struct AuthorityTransferContext {
    AuthorityTransferDigest session{};
    std::uint32_t epoch{};
    std::uint64_t membership_revision{};
    std::uint16_t peers{}; // Physical connection owners, never couch seats.
    unsigned host=16;
    bool operator==(const AuthorityTransferContext &)const=default;
};
struct AuthorityCutSeal {
    std::uint64_t id{},tick{},bytes{};
    AuthorityTransferDigest digest{};
    bool operator==(const AuthorityCutSeal &)const=default;
};
struct AuthorityTransferPacket {unsigned peer{};std::vector<std::byte> bytes;};
struct ReceivedAuthorityCut {
    unsigned peer{};
    AuthorityTransferContext context;
    AuthorityCutSeal seal;
    std::shared_ptr<const std::vector<std::byte>> bytes;
};
struct ValidatedAuthorityCut {
    unsigned peer{};
    AuthorityTransferContext context;
    AuthorityCutSeal seal;
};
struct AuthorityTransferLimits {
    std::uint64_t maximum_bytes=64ull*1024*1024;
    std::uint64_t bytes_per_second=512ull*1024;
    std::uint64_t burst_bytes=16*1024;
    std::uint64_t retry_ms=250,offer_ms=500,idle_ms=15'000,lifetime_ms=300'000;
    unsigned window_chunks=64;
};
// Reliable bounded application fragments over the room's existing authenticated
// DTLS datagrams. receive() must get the transport-verified physical endpoint;
// a packet's own fields cannot authenticate its sender. No master relay is used.
// All state/queues are synchronized; large publication/completion hashes run
// outside the state mutex. Context changes require an externally committed
// authority/membership decision, not a local timeout or a packet claim.
class PeerAuthorityTransfer {
public:
    PeerAuthorityTransfer(unsigned self,AuthorityTransferContext,AuthorityTransferLimits = {});
    ~PeerAuthorityTransfer();
    AuthorityTransferContext context()const;
    bool replace_context(const AuthorityTransferContext &expected,AuthorityTransferContext next);
    bool supported(unsigned peer)const;
    AuthorityCutSeal publish(const AuthorityTransferContext &expected,std::uint64_t tick,
        std::vector<std::byte> archive,std::span<const unsigned> targets,std::uint64_t now_ms);
    std::vector<AuthorityTransferPacket> poll(std::uint64_t now_ms,unsigned packet_budget=4);
    // True means an authority packet was consumed, including a rejected one.
    bool receive(unsigned authenticated_peer,std::span<const std::byte>,std::uint64_t now_ms);
    std::optional<ReceivedAuthorityCut> take_completed();
    // Only the owning executor may call this, after decoding and complete world
    // preparation. Byte receipt/checksum completion never acknowledges a world.
    bool acknowledge_validated(const AuthorityTransferContext &,const AuthorityCutSeal &);
    std::vector<ValidatedAuthorityCut> validated_cuts();
    static bool is_packet(std::span<const std::byte>);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
