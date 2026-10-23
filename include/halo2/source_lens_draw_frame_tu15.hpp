#pragma once
#include "halo2/source_lens_draw_geometry_tu15.hpp"
#include "halo2/source_view_projection.hpp"
#include <memory>
namespace halo2 {
class Renderer;
class PhysicsLiveWorldTu15;
class SourceRuntimeMemory;
class SourceLensPresentationHandoffTu15;
// Retains authored bitmap metadata before renderer construction, then binds
// only actual resident images. Original kernels own gate, request selection,
// visibility, animation, color, geometry and bitmap selection.
class SourceLensDrawFrameTu15 {
    struct PreparedData;
public:
    SourceLensDrawFrameTu15(const Cache &,Mesh &);
    ~SourceLensDrawFrameTu15();
    void bind(Renderer &);
    // Completed-frame handoff to a separately prepared, retired query bank.
    // Keep resident bitmap/material identities; replace all private SDK state,
    // headers, FIFO, source borrower, camera and frame counters transactionally.
    // The read-only incoming identity can outlive its inspection scope. The
    // next view acquires the actual native borrower and initializes its SDK.
    void rebind(const SourceRuntimeMemory &);
    class PreparedRebind {
    public:
        ~PreparedRebind();
        PreparedRebind(PreparedRebind &&) noexcept;
        PreparedRebind &operator=(PreparedRebind &&) noexcept;
    private:
        friend class SourceLensDrawFrameTu15;
        friend class SourceLensPresentationHandoffTu15;
        PreparedRebind();
        std::unique_ptr<PreparedData> data_;
    };
    // Prepare private SDK/header state without changing the old owner. The
    // enclosing transaction retains/quiesces the exact incoming native world;
    // pointer identity here is not an authentication or native validity seal.
    PreparedRebind prepare_rebind(const SourceRuntimeMemory &) const;
    bool rebind_ready(const PreparedRebind &,const SourceRuntimeMemory &) const noexcept;
    // Consumes eligibility; the caller's token retains retired private state
    // until destruction outside the enclosing commit. No Vulkan calls occur.
    bool commit_rebind(PreparedRebind &&,const SourceRuntimeMemory &) noexcept;
    void begin();
    std::vector<SourceLensDraw> view(PhysicsLiveWorldTu15 &,unsigned,
        Vec3 position,Vec3 forward,Vec3 up,float angle,
        const SourceViewProjectionBank &,std::array<std::int16_t,4> rectangle,
        unsigned logical_width,unsigned logical_height);
    std::vector<SourceLensDraw> view(PhysicsLiveWorldTu15 &,unsigned,
        Vec3 position,Vec3 forward,Vec3 up,float angle,const SourceViewProjectionBank &,
        std::array<std::int16_t,4> rectangle,unsigned logical_width,unsigned logical_height,Vec2 clip_planes);
    void finish(bool submitted);
private:
    friend class SourceLensPresentationHandoffTu15;
    Renderer *renderer_owner() const noexcept;
    // Private paired-owner path after authentic full native Boundary binding;
    // validates local presentation identity only, never native ring storage.
    bool rebind_ready_held(const PreparedRebind &,const SourceRuntimeMemory &) const noexcept;
    void publish_rebind(PreparedRebind &) noexcept;
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
