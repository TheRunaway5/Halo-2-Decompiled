#pragma once
#include "halo2/source_lens_query_frame_tu15.hpp"
#include "halo2/source_lens_draw_frame_tu15.hpp"
#include "halo2/source_native_publication.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/peer_authority_transfer.hpp"
#include <optional>

namespace halo2 {
class MultiplayerPreparedWorld;
class MultiplayerRuntimeAuthority;
class SourcePeerHostTakeover;
// Completed-frame transaction for the two lens owners. This is a local
// presentation subtransaction, not native-world validation or host election.
// Callers retain/quiesce both owners, their common renderer/window and the
// exact incoming world until commit; the enclosing adopter validates its seal.
class SourceLensPresentationHandoffTu15 {
public:
    class Prepared {
    public:
        Prepared(Prepared &&) noexcept=default;
        Prepared &operator=(Prepared &&) noexcept=default;
    private:
        friend class SourceLensPresentationHandoffTu15;
        Prepared(SourceLensQueryFrameTu15::PreparedRebind &&,
            SourceLensDrawFrameTu15::PreparedRebind &&);
        MultiplayerPreparedWorld *native_target_{};
        // Keep the exact Cache alive until old renderer/query tokens retire.
        std::shared_ptr<const Cache> native_cache_;
        ValidatedAuthorityCut native_cut_;
        SourceMatchAuthority::ExecutionBoundary native_boundary_;
        const SourceRuntimeMemory *native_memory_{};
        std::optional<SourceNativePublication::Boundary> publication_;
        bool consumed_{};
        SourceLensQueryFrameTu15::PreparedRebind queries_;
        SourceLensDrawFrameTu15::PreparedRebind draws_;
    };
    // All allocation happens here. Partial preparation or token destruction
    // preserves both live owners and their actual pending GPU results.
    static Prepared prepare(SourceLensQueryFrameTu15 &,SourceLensDrawFrameTu15 &,
        const SourceRuntimeMemory &);
    // Trusted actual legacy host target before its native publication hold.
    // A prospective common target is reachable only through prepare_common;
    // neither API exposes the private target or creates native readiness.
    static Prepared prepare_host(SourceLensQueryFrameTu15 &,SourceLensDrawFrameTu15 &,
        SourcePeerHostTakeover &);
    static Prepared prepare_common(MultiplayerRuntimeAuthority &,const ValidatedAuthorityCut &,
        SourceLensQueryFrameTu15 &,SourceLensDrawFrameTu15 &);
    // Bind once to the genuine creator-held full target/cut/boundary scope.
    // The token retains only a weak witness. An expired or replacement scope
    // cannot renew it, and a default/foreign scope never creates eligibility.
    static bool bind_publication(Prepared &,SourceNativePublication::Boundary) noexcept;
    static bool ready(const SourceLensQueryFrameTu15 &,const SourceLensDrawFrameTu15 &,
        const Prepared &,const SourceRuntimeMemory &) noexcept;
    // Bound-native only: no caller-visible raw memory identity is required.
    static bool ready(const SourceLensQueryFrameTu15 &,const SourceLensDrawFrameTu15 &,
        const Prepared &) noexcept;
    // Recheck both before publication. No allocation, native callback, Vulkan
    // call or owner destruction occurs. The caller's token loses eligibility
    // and holds all retired owners. Retain it across the native commit, then
    // destroy/move-assign it outside the session lock on this executor, while
    // the SDL window is alive. That later destruction may wait for actual GPU
    // work; this is not a bounded-latency or seamless-migration claim.
    static bool commit(SourceLensQueryFrameTu15 &,SourceLensDrawFrameTu15 &,
        Prepared &&,const SourceRuntimeMemory &) noexcept;
    static bool commit(SourceLensQueryFrameTu15 &,SourceLensDrawFrameTu15 &,Prepared &&) noexcept;
private:
    friend class MultiplayerRuntimeAuthority;
    static Prepared prepare_native(SourceLensQueryFrameTu15 &,SourceLensDrawFrameTu15 &,
        MultiplayerPreparedWorld &,const ValidatedAuthorityCut &);
};
}
