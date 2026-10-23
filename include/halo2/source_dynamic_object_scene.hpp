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
    float camouflage{};
};
class SourceDynamicObjectScene {
    struct PreparedData;
public:
    // Exact local owner/lighting/LOD/pose and all four completed draw banks.
    // Resident definitions/resources and collision producers remain unchanged;
    // this token does not certify a native cut or complete migration readiness.
    class Checkpoint {
    public:
        Checkpoint()=default;
        bool valid()const noexcept;
    private:
        friend class SourceDynamicObjectScene;
        struct Data;std::shared_ptr<const Data> data_;
    };
    class PreparedRestore {
    public:
        ~PreparedRestore();
        PreparedRestore(PreparedRestore &&)noexcept;
        PreparedRestore &operator=(PreparedRestore &&)noexcept;
    private:
        friend class SourceDynamicObjectScene;
        PreparedRestore();std::unique_ptr<PreparedData> data_;
    };
    Checkpoint checkpoint()const;
    bool owns_checkpoint(const Checkpoint &)const noexcept;
    PreparedRestore prepare_restore(const Checkpoint &)const;
    bool restore_ready(const PreparedRestore &)const noexcept;
    // Executor-local swaps only; caller retains the consumed token until old
    // draw/element/node spans and the enclosing publication lock have retired.
    // References to per-view vector objects remain stable, while their former
    // elements belong to that token. No cold reconstruction or callback runs.
    bool commit_restore(PreparedRestore &)noexcept;
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
