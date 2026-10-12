#pragma once
#include "halo2/first_person_ik.hpp"
#include "halo2/model.hpp"

namespace halo2 {
struct FrontendModelWeaponIk {
  bool world_callback{};
  FirstPersonIkDefinition definition;
};
// Original113BA0/113980 admits the supplied positive slot0 weight and
// applies each valid model marker node once, in original class-pair order.
void apply_frontend_model_weapon_ik(const FrontendModelWeaponIk&,
    const RenderModel&,std::span<Transform> body,
    std::span<const Transform> weapon,float weight);
}
