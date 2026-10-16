#pragma once
#include "halo2/source_lens_query_gpu.hpp"
#include "halo2/source_view_projection.hpp"
#include <memory>

namespace halo2 {
class Renderer;
class PhysicsLiveWorldTu15;
// Native presentation owner for the original request/query ring. The source
// kernels own selection, projected bounds, completion and visibility. Only
// the actual renderer supplies hardware samples and submission events.
class SourceLensQueryFrameTu15 {
public:
    explicit SourceLensQueryFrameTu15(Renderer &);
    ~SourceLensQueryFrameTu15();
    void begin(PhysicsLiveWorldTu15 &);
    // Original2B68C runs before this view's object/effect request producers.
    // Query issue/completion stays at the later world-render boundary.
    void prepare_view(PhysicsLiveWorldTu15 &,unsigned);
    std::vector<SourceLensQueryDraw> view(PhysicsLiveWorldTu15 &,unsigned,
        Vec3 position,Vec3 forward,Vec3 up,float angle,
        const SourceViewProjectionBank &,std::array<std::int16_t,4> rectangle,
        unsigned logical_width,unsigned logical_height);
    void finish_view(PhysicsLiveWorldTu15 &,unsigned);
    void finish(PhysicsLiveWorldTu15 &,bool submitted);
    std::span<const SourceLensQueryResult> completed() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
