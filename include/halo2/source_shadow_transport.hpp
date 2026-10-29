#pragma once
#include "halo2/multiplayer_snapshot_origin.hpp"
#include "halo2/multiplayer_datagrams.hpp"
#include "halo2/source_player_admission.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>

namespace halo2 {
class LanHost;
class LanClient;
class OnlineMatchSession;
class SourceMatchAuthority;
class AuthorityArchiveImage;

// An authenticated presentation payload paired to its actual decoded historical
// body. Receiving it does not replace current gameplay state. This is NOT native
// validation, recovery readiness or host authority.
// Only LanClient creates it after the independent admission/control and complete
// payload have both matched this connection's exact snapshot stamp.
class AcceptedShadowCut {
public:
    // Complete encoded envelope is materialized only for diagnostic callers.
    std::span<const std::byte> bytes()const;
    std::span<const std::byte> raw_bytes()const noexcept;
    std::array<std::byte,32> raw_digest()const;
    const AuthorityArchiveImage& archive_image()const;
    std::uint64_t serial()const noexcept;
    std::uint64_t generation()const noexcept;
    const std::array<std::byte,32>& binding()const;
    std::span<const SourcePlayerAdmission> admissions()const noexcept;
    std::span<const OnlineAdmission> roster()const noexcept;
    std::span<const unsigned> producer_views()const noexcept;
    std::span<const unsigned> local_views()const noexcept;
    const MultiplayerSnapshotOrigin& origin()const;
    bool same_generation(const AcceptedShadowCut&)const noexcept;
    bool same_world(const AcceptedShadowCut&)const noexcept;
private:
    friend class LanClient;
    struct Data;
    std::shared_ptr<const Data> data_;
};

struct SourceShadowTransportStatistics {
    std::uint64_t captures{},encoded{},published{},received{},acknowledged{};
    std::uint64_t full_bytes{},wire_bytes{},capture_us{},encode_us{};
    std::uint64_t backpressure{},coalesced{},rejected{},generation{};
    std::uint64_t latest_tick{},latest_capture_at_ms{},largest_capture_gap_ms{};
    std::uint64_t raw_bytes{},delta_bytes{},full_cuts{},delta_cuts{},latest_acknowledged_sequence{};
    struct Cut {
        std::uint64_t generation{},serial{},publication{},tick{},sequence{},base_serial{};
        std::uint64_t raw_bytes{},native_bytes{},wire_bytes{},capture_us{},encode_us{},archive_decode_us{},native_ready_us{};
        bool delta{};
    };
    std::vector<Cut> cuts; // Most recent sixteen actual native publications.
    bool encoding{},awaiting_native{};
    std::string error;
};

// Internal socket boundary. Its caller has already authenticated the physical
// endpoint and the native nonce/cookie. Nothing in these values elects a host.
struct SourceShadowPeer {
    std::uint64_t key{},fingerprint{},nonce{},cookie{},revision{};
    std::vector<unsigned> slots;
    bool operator==(const SourceShadowPeer&)const=default;
};
struct SourceShadowSnapshotStamp {
    std::uint64_t fingerprint{},nonce{},cookie{};
    std::uint32_t sequence{},tick{},bytes{};
    std::array<std::byte,32> digest{};
    bool operator==(const SourceShadowSnapshotStamp&)const=default;
};
struct SourceShadowPayload {
    SourceShadowSnapshotStamp snapshot;
    std::uint64_t serial{},generation{};
    std::array<std::byte,32> binding{},admission_digest{},world_identity{};
    std::vector<SourcePlayerAdmission> admissions;
    std::vector<OnlineAdmission> roster;
    std::vector<unsigned> producer_views,local_views;
    std::shared_ptr<const std::vector<std::byte>> bytes;
    std::shared_ptr<const AuthorityArchiveImage> archive;
    // Exact ordinary complete snapshot bytes captured by LanHost's encoder,
    // not state manufactured from the native checkpoint or packet metadata.
    std::shared_ptr<const std::vector<std::byte>> snapshot_body;
};

// Keep the network/socket library independent of the larger native codec.
// Implementations own only immutable values on workers; capture/native ACKs
// stay on the caller's actual foreground executor.
class SourceShadowHostLane {
public:
    virtual ~SourceShadowHostLane()=default;
    virtual void connection(const SourceShadowPeer&)=0;
    virtual void retire(std::uint64_t)=0;
    virtual void update_roster(std::span<const OnlineAdmission>)=0;
    virtual bool ready(std::uint64_t)=0;
    virtual void emitted(const SourceShadowPeer&,const SourceShadowSnapshotStamp&,
        std::span<const std::byte> actual_body)=0;
    virtual bool pending()const noexcept=0;
    virtual bool capture()=0;
    virtual void receive(std::uint64_t,std::span<const std::byte>,std::uint64_t)=0;
    virtual std::vector<std::vector<std::byte>> poll(std::uint64_t,std::uint64_t)=0;
    virtual SourceShadowTransportStatistics statistics()const=0;
};
class SourceShadowClientLane {
public:
    virtual ~SourceShadowClientLane()=default;
    // Publication policy follows the actual ONLINE owner, independently of
    // whether its authenticated native datagrams use a relay wrapper/socket.
    virtual bool requires_continuity()const noexcept{return false;}
    virtual void connection(const SourceShadowPeer&)=0;
    virtual void receive(std::span<const std::byte>,std::uint64_t)=0;
    virtual std::vector<std::vector<std::byte>> poll(std::uint64_t)=0;
    virtual std::optional<SourceShadowPayload> completed()=0;
    // Called only after the foreground owner imported/validated this exact cut.
    virtual bool acknowledge(const SourceShadowPayload&)=0;
    virtual bool current(const SourceShadowPayload&)const noexcept=0;
    virtual SourceShadowTransportStatistics statistics()const=0;
};
}
