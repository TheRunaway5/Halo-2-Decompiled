#pragma once
#include "halo2/cache.hpp"
#include <functional>

namespace halo2 {
struct ItemPointDefinition;
struct ItemPointState;
struct RawCollisionBsp;
struct PickupCapsuleDefinition {
  std::uint32_t flags{};
  float standing{},crouching{},radius{};
};
struct PickupCapsule { Vec3 bottom;float extent{},radius{}; };
// E0C20 arithmetic after the original BACC0 root producer. Current crouch and
// authored biped fields are inputs, not inferred from the root or capsule.
PickupCapsule pickup_capsule(Vec3 source_root,const PickupCapsuleDefinition &,float crouch);
struct PickupNormalizer {
  Vec3 input,output;
  double length{};
  float length_spill{},returned{},inverse{};
};
// 30BD0 under the measured fixture's FPCW027F/MXCSR1F80. The x87 length
// operations round at53 bits before the float32 spill and SSE reciprocal.
PickupNormalizer pickup_side_normalizer(Vec3);
struct PickupVisibilityCast {
  Vec3 start,delta;
  std::uint32_t mask{},unit{},item{};
};
struct PickupVisibilityHit { bool hit{};Vec3 point; };
using PickupVisibilityQuery=std::function<PickupVisibilityHit(const PickupVisibilityCast &)>;
struct PickupVisibilityResult {
  bool visible=true;
  std::optional<PickupNormalizer> normalizer;
  Vec3 side,positive_target,negative_target;
  std::vector<PickupVisibilityCast> casts;
};
// C57A0 consumer only. FBFA0 point/current capsule and the outer dispatcher
// are explicit services; this does not manufacture item settling or poses.
PickupVisibilityResult pickup_visibility(const PickupCapsule &,Vec3 item_point,
    std::uint32_t unit,std::uint32_t item,std::uint8_t unit_type,std::uint8_t item_type,
    const PickupVisibilityQuery &);
// C57A0 with its original FBFA0 point producer. The current node/contact
// state, capsule root/crouch and collision dispatcher still have explicit owners.
PickupVisibilityResult source_item_pickup_visibility(const PickupCapsule &,
    const ItemPointDefinition &,const ItemPointState &,const RawCollisionBsp &active_collision,
    std::uint32_t unit,std::uint8_t unit_type,std::uint8_t item_type,
    const PickupVisibilityQuery &);
}
