#pragma once
#include "halo2/model.hpp"
#include <map>
#include <optional>
#include "halo2/unit_local_transition.hpp"

namespace halo2 {
// Native presentation of decoded retail body channels. Gameplay/action
// ownership remains with the unit; this adapter never starts an action.
class NativePlayerAnimation {
public:
  NativePlayerAnimation(const Cache &,std::uint32_t graph,std::uint32_t weapon);
  const AnimationClip *action(std::uint32_t activity,bool crouch,bool dual);
  std::vector<BonePose> pose(const RenderModel &,const AnimationClip &primary,
      float frame,float pitch,bool crouch,bool dual,bool moving,
      const AnimationClip *secondary=nullptr,float secondary_frame=0,
      const AnimationClip *offhand=nullptr,float offhand_frame=0,float relative_yaw=0,
      float lowered_weight=0,NativePlayerAnimation *offhand_animation=nullptr,
      std::uint32_t primary_activity=0xffffffffu,float aim_weight=1,
      bool support_hand_grip=true);
private:
  const Cache &cache_;
  std::uint32_t graph_;
  std::array<std::uint32_t,2> identifiers_;
  std::array<std::vector<std::uint32_t>,2> hand_nodes_;
  std::map<std::array<std::uint32_t,3>,std::optional<AnimationClip>> clips_;
  struct AimScreen {int yaw_negative{},width{},pitch_negative{},pitch_positive{};float negative_step{},positive_step{},yaw_negative_step{},yaw_positive_step{};};
  std::map<std::pair<std::uint32_t,std::uint32_t>,AimScreen> screens_;
};
// 287B90/28B6B0 additive arithmetic and type2 component replacement. Source
// masks select components, so an arm action cannot replace untouched legs.
void apply_native_player_overlay(const RenderModel &,std::span<BonePose>,
    const AnimationClip &,float frame,float weight=1);
// 1DCE60 shortest-sign normalized local transition arithmetic. Kept separate
// from packed-key interpolation, which has different quaternion rules.
void blend_native_player_locals(std::span<BonePose>,std::span<const BonePose>,float);
class NativePlayerTransition {
public:
  void tick(std::uint32_t activity,bool crouch,std::span<const BonePose>,
      const UnitConstructorTailConstants &);
  void apply(std::span<BonePose>,float fraction=1) const;
private:
  std::array<std::byte,0x90> manager_{};
  std::vector<BonePose> current_,saved_;
  std::uint32_t activity_=0xffffffffu;
  bool crouch_{};
};
} // namespace halo2
