#pragma once
#include "halo2/transform.hpp"

namespace halo2 {
struct FirstPersonIkConstraint {
    std::uint32_t arms_marker{}, weapon_marker{};
    unsigned arms_node{}, weapon_node{};
    Transform arms_local, weapon_local;
};
struct FirstPersonIkDefinition {
    std::vector<FirstPersonIkConstraint> constraints;
};

// Retail 168644/1D90E0: the input nodes have already received the observer
// camera and camera-control adjustment. Only the arms are changed.
void apply_first_person_ik(const FirstPersonIkDefinition &,
                          std::span<const int> arms_parents,
                          std::span<Transform> arms_nodes,
                          std::span<const Transform> weapon_nodes,
                          float aiming_weight);

// Retail 120220, also used by third-person marker attachment. Returns false
// for the source's degenerate-chain branch, preserving every input transform.
bool source_two_bone_ik(Transform &root, Transform &middle,
                       Transform &end, Transform target);
Transform source_marker_transform(Vec3 position,
                                 const std::array<float, 4> &quaternion);

// Unit aiming slot at (unit + signed short +33E) +80. 113870 initializes
// its curve to 1; 1130F0 selects its direction and 113910 advances one tick.
struct FirstPersonAimBlend {
    std::int8_t value{}, count{}, curve=1;
    std::uint8_t flags{};
};
void source_aim_blend_select(FirstPersonAimBlend &, bool weapon_eligible,
                           unsigned ticks_per_second);
bool source_aim_blend_advance(FirstPersonAimBlend &);
float source_aim_blend_weight(const FirstPersonAimBlend &);

// 1112F0/111590 and CD660. The two flags are from the actual base and
// secondary animations' jmad records (+18), not their resource channels.
struct FirstPersonAimEligibility {
    std::uint32_t base_state=0xffffffffu, secondary_action=0xffffffffu;
    std::uint16_t base_animation_flags{}, secondary_animation_flags{};
    bool dual_wield{};
};
bool source_weapon_aim_eligible(const FirstPersonAimEligibility &);
} // namespace halo2
