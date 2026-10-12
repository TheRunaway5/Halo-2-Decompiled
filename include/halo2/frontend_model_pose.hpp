#pragma once
#include "halo2/frontend_model_ik.hpp"
#include "halo2/frontend_model_locals.hpp"
#include "halo2/source_shadow.hpp"
namespace halo2 {
struct FrontendModelPose {
  std::vector<Transform> body,weapon;
};
// B7B40's default/unarmed startup pose precedes equipping. B8EE0/B93B0
// resolves and stores these parent-local fields once; later animation and
// UI primary requests do not realign the attached object's local frame.
ObjectAttachmentFrame frontend_model_initialize_weapon_attachment(
    const RenderModel&,const Transform&,const AnimationClip& startup,
    const FrontendModelPrimary&,const WeaponAttachmentMarkers&);
// Actual menu137800/BD610 banks: initial complement-mask sample, held
// weapon world IK, deferred local sample/grip, then deferred hierarchy.
// Parent attachment and every IK chain node must belong to the initial
// bank, as proved for the disc's menu Spartans/Elites. No clocks advance.
FrontendModelPose frontend_model_pose(const RenderModel&,const Transform&,
    const AnimationClip&,float retained_frame,
    std::span<const std::uint32_t> deferred_mask,const FrontendModelGrip*,
    const RenderModel*weapon_model,const AnimationClip*weapon_clip,
    const WeaponAttachmentMarkers*,const FrontendModelWeaponIk*,float aim_weight,
    const FrontendModelPrimary*source_primary=nullptr,
    const ObjectAttachmentFrame*retained_weapon_local=nullptr);
// BD610's ascending model-order1421F0/142A60 rebuild. Existing unmasked
// parents retain their world callback result; no global hierarchy rebuild.
void frontend_model_deferred_world(const RenderModel&,std::span<Transform>,
    std::span<const BonePose>,std::span<const std::uint32_t> deferred_mask);
}
