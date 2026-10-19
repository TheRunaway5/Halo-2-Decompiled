#pragma once
#include "halo2/native_grenade.hpp"
#include "halo2/source_object_view.hpp"
#include "halo2/object_lighting.hpp"
#include <memory>

namespace halo2 {
class CollisionWorld;
// Native high-refresh presentation between completed simulation snapshots.
// Only the same released full owner/definition/type may share a position
// window. New, reattached or recycled owners use their current position.
// This does not advance physics, timers, contacts or detonation callbacks.
Vec3 grenade_render_position(std::span<const NativeGrenadeProjectile> previous,
                             const NativeGrenadeProjectile& current,float fraction);
struct SourceGrenadePreparedBank {
    std::uint32_t definition=0xffffffffu;
    unsigned type{},first_material{},material_count{};
    std::array<std::size_t,6> lod_vertices{};
};
struct SourceGrenadeDraw {
    std::uint32_t owner=0xffffffffu,definition=0xffffffffu;
    unsigned type{},first_material{},material_count{};
    SourceObjectViewSelection selection;
    std::vector<Vertex> vertices;
    ObjectLightingState lighting;
    bool ordinary_lighting{};
};
// Two authored matg projectile models, with complete per-owner region/LOD,
// default-node pose and ordinary lighting state for the verified released
// branch. Current WORLD poses/velocity are native sandbox inputs. Attached
// inputs are rejected: this does not infer an original parent-to-hide rule.
// Released angular integration is not implemented.
//
// Resident texture/material templates are shared, but each returned draw owns
// an independent lighting bank. A renderer must allocate distinct uniform
// aliases (or submit separate banks) before merging owners. Appending these
// vertices to one view with one shared material-range binding is incorrect.
class SourceGrenadeScene {
public:
    // The caller supplies the real native allocator capacity and imported
    // constructor-one operand; no smaller rendering/gameplay cap is invented.
    SourceGrenadeScene(const Cache&,std::size_t bsp,Mesh&,
                      const CollisionWorld&,std::size_t native_object_capacity,
                      std::uint32_t constructor_one_bits);
    ~SourceGrenadeScene();
    SourceGrenadeScene(SourceGrenadeScene&&) noexcept;
    SourceGrenadeScene&operator=(SourceGrenadeScene&&) noexcept;
    SourceGrenadeScene(const SourceGrenadeScene&)=delete;
    SourceGrenadeScene&operator=(const SourceGrenadeScene&)=delete;
    // Completed-frame world adoption: retire owner light/LOD/pose/draw caches
    // while keeping both authored material/geometry banks resident. The caller
    // quiesces borrowers and synchronizes actual incoming released projectiles.
    void reset_lifetimes() noexcept;
    std::span<const SourceGrenadePreparedBank> banks()const;
    void synchronize(std::span<const NativeGrenadeProjectile>);
    // Actual admitted view/distance/shader producers remain required. This
    // does not manufacture source PVS/registration or full Xbox physics.
    const std::vector<SourceGrenadeDraw>& draw_view(std::int32_t render_frame,
                                                  const SourceObjectViewLod&);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace halo2
