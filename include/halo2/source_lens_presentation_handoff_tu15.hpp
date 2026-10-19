#pragma once
#include "halo2/source_lens_query_frame_tu15.hpp"
#include "halo2/source_lens_draw_frame_tu15.hpp"

namespace halo2 {
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
        SourceLensQueryFrameTu15::PreparedRebind queries_;
        SourceLensDrawFrameTu15::PreparedRebind draws_;
    };
    // All allocation happens here. Partial preparation or token destruction
    // preserves both live owners and their actual pending GPU results.
    static Prepared prepare(SourceLensQueryFrameTu15 &,SourceLensDrawFrameTu15 &,
        const SourceRuntimeMemory &);
    static bool ready(const SourceLensQueryFrameTu15 &,const SourceLensDrawFrameTu15 &,
        const Prepared &,const SourceRuntimeMemory &) noexcept;
    // Recheck both before publication. No allocation, native callback, Vulkan
    // call or owner destruction occurs. The caller's token loses eligibility
    // and holds all retired owners. Retain it across the native commit, then
    // destroy/move-assign it outside the session lock on this executor, while
    // the SDL window is alive. That later destruction may wait for actual GPU
    // work; this is not a bounded-latency or seamless-migration claim.
    static bool commit(SourceLensQueryFrameTu15 &,SourceLensDrawFrameTu15 &,
        Prepared &&,const SourceRuntimeMemory &) noexcept;
};
}
