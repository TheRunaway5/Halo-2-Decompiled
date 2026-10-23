#pragma once
#include "halo2/native_weapon_projectile.hpp"
#include "halo2/source_object_view.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/appearance.hpp"
#include "halo2/source_light_volume_draw.hpp"
#include <memory>

namespace halo2 {
class CollisionWorld;
// Completed first WORLD node of an attached source projectile. Its parent
// transform and animation have already been evaluated by the source owner.
struct SourceProjectileGlowOwner {
    std::uint32_t object{},definition{};
    Transform first_world_node;
};
struct NativeWeaponProjectileDraw {
    std::uint32_t owner{},definition{};
    std::vector<Vertex> vertices;
    ObjectLightingState lighting;
    bool ordinary_lighting{};
    ObjectColors colors;
};
// Authored singleton/default-node projectile models on actual native WORLD
// snapshots. Geometry/LOD/shaders use the existing source consumers. Full
// original birth, guidance, attachment, contrail and physics remain separate.
class NativeWeaponProjectileScene {
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
        friend class NativeWeaponProjectileScene;
        struct Data;std::shared_ptr<const Data> data_;
    };
    class PreparedRestore {
    public:
        ~PreparedRestore();
        PreparedRestore(PreparedRestore &&)noexcept;
        PreparedRestore &operator=(PreparedRestore &&)noexcept;
    private:
        friend class NativeWeaponProjectileScene;
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
    NativeWeaponProjectileScene(const Cache&,std::size_t bsp,Mesh&,
        const CollisionWorld&,std::span<const std::uint32_t> definitions,std::size_t capacity);
    ~NativeWeaponProjectileScene();
    // Completed-frame adoption: discard full-owner light/LOD/pose and model/
    // glow draw history, retaining resident authored resources. Re-synchronize
    // actual incoming projectiles before drawing. Caller owns world quiescence.
    void reset_lifetimes() noexcept;
    void synchronize(std::span<const NativeWeaponProjectile>);
    const std::vector<NativeWeaponProjectileDraw>& draw_view(std::int32_t,const SourceObjectViewLod&);
    const std::vector<SourceLightVolumeDraw>& light_volume_view(unsigned,Vec3 actual_camera_forward,
        const SourceViewProjectionBank &,RenderViewport,float actual_w_distance,
        std::span<const SourceProjectileGlowOwner> attached={});
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
