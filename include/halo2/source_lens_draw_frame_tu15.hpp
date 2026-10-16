#pragma once
#include "halo2/source_lens_draw_geometry_tu15.hpp"
#include "halo2/source_view_projection.hpp"
#include <memory>
namespace halo2 {
class Renderer;
class PhysicsLiveWorldTu15;
// Retains authored bitmap metadata before renderer construction, then binds
// only actual resident images. Original kernels own gate, request selection,
// visibility, animation, color, geometry and bitmap selection.
class SourceLensDrawFrameTu15 {
public:
    SourceLensDrawFrameTu15(const Cache &,Mesh &);
    ~SourceLensDrawFrameTu15();
    void bind(Renderer &);
    void begin();
    std::vector<SourceLensDraw> view(PhysicsLiveWorldTu15 &,unsigned,
        Vec3 position,Vec3 forward,Vec3 up,float angle,
        const SourceViewProjectionBank &,std::array<std::int16_t,4> rectangle,
        unsigned logical_width,unsigned logical_height);
    void finish(bool submitted);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
