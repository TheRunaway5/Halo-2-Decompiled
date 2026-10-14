#pragma once
#include "halo2/native_weapon_projectile.hpp"
#include "halo2/source_object_view.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/appearance.hpp"
#include "halo2/source_light_volume_draw.hpp"
#include <memory>

namespace halo2 {
class CollisionWorld;
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
public:
    NativeWeaponProjectileScene(const Cache&,std::size_t bsp,Mesh&,
        const CollisionWorld&,std::span<const std::uint32_t> definitions,std::size_t capacity);
    ~NativeWeaponProjectileScene();
    void synchronize(std::span<const NativeWeaponProjectile>);
    const std::vector<NativeWeaponProjectileDraw>& draw_view(std::int32_t,const SourceObjectViewLod&);
    const std::vector<SourceLightVolumeDraw>& light_volume_view(unsigned,Vec3 actual_camera_forward,
        const SourceViewProjectionBank &,RenderViewport,float actual_w_distance);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
