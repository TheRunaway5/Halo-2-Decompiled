#pragma once
#include "halo2/cache.hpp"
#include "halo2/model.hpp"
#include "halo2/biped_landing.hpp"
#include <array>
#include <functional>
#include <memory>
namespace halo2 {
struct NativeLocomotionFrame {
  const AnimationClip *clip{};
  float frame{};
  std::uint32_t activity=0xffffffffu;
};
struct NativeFootstepEvent {unsigned timing{},kind{},marker{};float frame{};};
// Explicit native mover/presentation adaptation. It uses the source classifier,
// authored gait binding and complete channel clock; it does not manufacture a
// retail biped physical-state record or claim Havok admission.
class NativeLocomotion {
public:
  NativeLocomotion(const Cache &,std::uint32_t graph,std::uint32_t weapon,
                   std::array<std::uint32_t,2> native_presentation_rng,bool dual=false);
  void bind_facing(std::uint32_t biped,float yaw);
  ~NativeLocomotion();
  void tick(float forward,float right,bool native_walking,bool crouching,
            Vec3 velocity,const std::function<void(const NativeFootstepEvent&)> &,
            const SourceLandingState *landing=nullptr,const float *aim_yaw=nullptr);
  NativeLocomotionFrame frame() const;
  NativeLocomotionFrame sample(float fraction) const;
  std::span<const std::byte> manager() const;
  float facing(float fraction=1) const;
private:
  struct State;
  std::unique_ptr<State> state_;
};
// 1883E0 sound outputs with actual resident matg/foot metadata. Distance/PVS,
// contact and marker ownership are explicit native caller inputs, not guessed.
std::array<std::uint32_t,3> authored_footstep_sounds(
    const Cache &,std::uint32_t foot,std::int16_t global_material,
    unsigned kind,int variant);
} // namespace halo2
