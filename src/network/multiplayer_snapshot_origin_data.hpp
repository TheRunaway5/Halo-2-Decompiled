#pragma once
#include "halo2/multiplayer_snapshot_origin.hpp"
#include "halo2/multiplayer.hpp"
namespace halo2 {
// Local values and explicit wire fields; never memcpy this C++ layout.
struct MultiplayerSnapshotStamp {
    std::uint64_t fingerprint{},nonce{},cookie{};
    std::uint32_t sequence{},tick{},bytes{};
    AuthorityTransferDigest digest{};
    bool operator==(const MultiplayerSnapshotStamp &)const=default;
};
struct MultiplayerSnapshotOrigin::Data {
    struct Decoded {
        MultiplayerState state;
        std::vector<unsigned> slots;
    };
    std::shared_ptr<const int> owner;
    std::optional<AuthorityTransferContext> context;
    unsigned self=16;
    std::uint64_t revision{};
    MultiplayerSnapshotStamp stamp;
    // Retained by continuity or the native presentation lane. Legacy standalone
    // receipts keep no decoded state or slot allocation.
    std::unique_ptr<const Decoded> decoded;
    // A native presentation body is an exact authenticated historical value,
    // not an ordinary gameplay/candidate/recovery receipt.
    bool presentation_only{};
    bool gameplay_repair{};
};
struct MultiplayerSnapshotProof::Data {
    MultiplayerSnapshotOrigin origin;
    ValidatedAuthorityCut cut;
};
}
