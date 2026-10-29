#pragma once
#include "halo2/multiplayer_snapshot_origin.hpp"
#include <memory>

namespace halo2 {
struct MultiplayerOptions;
class MultiplayerPreparedWorld;
class MultiplayerObjectRuntime;
class MultiplayerMatch;
class LanClient;
class AcceptedShadowCut;
// A recipient-local foreground native renderer. The transport supplies an
// authenticated owning cut paired to the exact gameplay body; only actual
// native preparation enables a borrow. This owner never becomes gameplay,
// replay, recovery or host-migration authority.
class SourceShadowPresentationOwner {
public:
    SourceShadowPresentationOwner(const MultiplayerOptions&,const MultiplayerPreparedWorld &resident);
    ~SourceShadowPresentationOwner();
    SourceShadowPresentationOwner(const SourceShadowPresentationOwner &)=delete;
    SourceShadowPresentationOwner &operator=(const SourceShadowPresentationOwner &)=delete;
    bool adopt(const LanClient&,const AcceptedShadowCut&,const MultiplayerSnapshotOrigin &current);
    MultiplayerObjectRuntime *world(const LanClient&,const MultiplayerSnapshotOrigin &current)const;
    // Read-only scene/device replica from that same closed native cut. This
    // never substitutes for the ordinary gameplay/prediction Match.
    const MultiplayerMatch *match(const LanClient&,const MultiplayerSnapshotOrigin &current)const;
    const MultiplayerSnapshotOrigin &origin()const;
    void reset();
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
