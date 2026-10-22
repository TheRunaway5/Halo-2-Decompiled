#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
namespace halo2 {
void MultiplayerMatch::publish_source_projectile_contact(const SourceProjectileDetonationTu15 &event) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
    if(!simulation_.source_physics_movement_ || !simulation_.grenade_sandbox_)
        throw FormatError("Source projectile contact requires the original effect journal owner");
    // This historical transport bank already retains generic101330 effects.
    // Publishing adds no damage, projectile, random draw or native contact.
    simulation_.grenade_sandbox_->adopt_source_detonations(simulation_.ticks(),std::span(&event,1));
}
}
