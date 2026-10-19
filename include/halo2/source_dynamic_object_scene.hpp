#pragma once
#include "halo2/source_dynamic_object.hpp"
#include "halo2/source_object_view.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/appearance.hpp"
#include "halo2/source_prt.hpp"
#include <memory>

namespace halo2 {
class CollisionWorld;
struct SourceDynamicObjectDraw {
    std::uint32_t object{},definition{};
    std::vector<Vertex> vertices;
    ObjectLightingState lighting;
    bool ordinary_lighting{};
    ObjectColors colors;
    std::vector<SourcePrtMaterialBank> prt;
};
class SourceDynamicObjectScene {
public:
    SourceDynamicObjectScene(const Cache &,std::size_t bsp,Mesh &,const CollisionWorld &);
    ~SourceDynamicObjectScene();
    // Completed-frame world adoption, including a rewind with unchanged full
    // handles. Retire pose/light/LOD/draw history; keep authored resident banks.
    // The enclosing adopter owns quiescence; synchronize actual incoming WORLD
    // inputs before drawing. This does not validate or promote a native world.
    void reset_lifetimes() noexcept;
    void synchronize(std::span<const SourceDynamicObject>,std::span<const SourceWorldPose>);
    const SourceDynamicObject &get(std::uint32_t) const;
    std::span<const Transform> nodes(std::uint32_t) const;
    const std::vector<SourceDynamicObjectDraw> &draw_view(std::int32_t,const SourceObjectViewLod &,
        const std::function<std::vector<Transform>(std::uint32_t)> &sample={});
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
