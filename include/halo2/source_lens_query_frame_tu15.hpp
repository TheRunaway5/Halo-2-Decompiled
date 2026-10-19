#pragma once
#include "halo2/source_lens_query_gpu.hpp"
#include "halo2/source_view_projection.hpp"
#include <memory>

namespace halo2 {
class Renderer;
class PhysicsLiveWorldTu15;
class SourceRuntimeMemory;
class SourceLensPresentationHandoffTu15;
// Native presentation owner for the original request/query ring. The source
// kernels own selection, projected bounds, completion and visibility. Only
// the actual renderer supplies hardware samples and submission events.
class SourceLensQueryFrameTu15 {
    struct PreparedData;
public:
    explicit SourceLensQueryFrameTu15(Renderer &);
    ~SourceLensQueryFrameTu15();
    // At a completed frame, bind a separately prepared incoming world whose
    // original CPU query ring has already been retired. Allocate all new
    // private device-ABI state before retiring old Vulkan query work. No GPU
    // result/generation or old source pointer survives this explicit handoff.
    // Rejection leaves the old presentation owner intact.
    // The read-only inspection scope need not remain alive: only the expected
    // world's identity is retained. begin() acquires its real native borrower.
    void rebind(const SourceRuntimeMemory &);
    class PreparedRebind {
    public:
        ~PreparedRebind();
        PreparedRebind(PreparedRebind &&) noexcept;
        PreparedRebind &operator=(PreparedRebind &&) noexcept;
    private:
        friend class SourceLensQueryFrameTu15;
        friend class SourceLensPresentationHandoffTu15;
        PreparedRebind();
        std::unique_ptr<PreparedData> data_;
    };
    // Staging does not retire GPU work or borrow incoming source pages. The
    // caller keeps both worlds/renderer alive and quiescent through commit,
    // and validates its complete native adoption boundary independently.
    PreparedRebind prepare_rebind(const SourceRuntimeMemory &) const;
    bool rebind_ready(const PreparedRebind &,const SourceRuntimeMemory &) const noexcept;
    // Publication performs no Vulkan call or destruction. The caller's token
    // loses commit eligibility but retains old GPU/CPU owners until destroyed
    // outside the enclosing commit/session lock, on this executor, while the
    // renderer's SDL window is still alive. Retirement can then wait for GPU.
    bool commit_rebind(PreparedRebind &&,const SourceRuntimeMemory &) noexcept;
    void begin(PhysicsLiveWorldTu15 &);
    // Original2B68C runs before this view's object/effect request producers.
    // Query issue/completion stays at the later world-render boundary.
    void prepare_view(PhysicsLiveWorldTu15 &,unsigned);
    std::vector<SourceLensQueryDraw> view(PhysicsLiveWorldTu15 &,unsigned,
        Vec3 position,Vec3 forward,Vec3 up,float angle,
        const SourceViewProjectionBank &,std::array<std::int16_t,4> rectangle,
        unsigned logical_width,unsigned logical_height);
    std::vector<SourceLensQueryDraw> view(PhysicsLiveWorldTu15 &,unsigned,
        Vec3 position,Vec3 forward,Vec3 up,float angle,const SourceViewProjectionBank &,
        std::array<std::int16_t,4> rectangle,unsigned logical_width,unsigned logical_height,Vec2 clip_planes);
    void finish_view(PhysicsLiveWorldTu15 &,unsigned);
    void finish(PhysicsLiveWorldTu15 &,bool submitted);
    std::span<const SourceLensQueryResult> completed() const;
private:
    friend class SourceLensPresentationHandoffTu15;
    Renderer *renderer_owner() const noexcept;
    void publish_rebind(PreparedRebind &) noexcept;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
