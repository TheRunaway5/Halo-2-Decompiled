#pragma once
#include "halo2/cache.hpp"
#include "halo2/model.hpp"
#include "halo2/biped_landing.hpp"
#include <array>
#include <functional>
#include <memory>
namespace halo2 {
struct ObjectAttachmentMarker;
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
  // Borrowed-cache callers retain their existing lifetime contract. Owning
  // continuation is available only through the shared-cache overload.
  NativeLocomotion(const Cache &,std::uint32_t graph,std::uint32_t weapon,
                   std::array<std::uint32_t,2> native_presentation_rng,bool dual=false);
  NativeLocomotion(std::shared_ptr<const Cache>,std::uint32_t graph,std::uint32_t weapon,
                   std::array<std::uint32_t,2> native_presentation_rng,bool dual=false);
  NativeLocomotion(const NativeLocomotion &)=delete;
  NativeLocomotion &operator=(const NativeLocomotion &)=delete;
  void bind_facing(std::uint32_t biped,float yaw);
  ~NativeLocomotion();
  void tick(float forward,float right,bool native_walking,bool crouching,
            Vec3 velocity,const std::function<void(const NativeFootstepEvent&)> &,
            const SourceLandingState *landing=nullptr,const float *aim_yaw=nullptr);
  NativeLocomotionFrame frame() const;
  NativeLocomotionFrame sample(float fraction) const;
  std::span<const std::byte> manager() const;
  float facing(float fraction=1) const;
  class Checkpoint {
  public:
    Checkpoint()=default;
    // Owning payload availability, including after the original adapter dies;
    // this is not executor, prepared-restore or migration readiness.
    bool valid()const noexcept;
  private:
    friend class NativeLocomotion;
    struct Data;std::shared_ptr<const Data> data_;
  };
  class PreparedRestore {
  public:
    ~PreparedRestore();
    PreparedRestore(PreparedRestore &&)noexcept;
    PreparedRestore &operator=(PreparedRestore &&)noexcept;
  private:
    friend class NativeLocomotion;
    PreparedRestore();struct Data;std::unique_ptr<Data> data_;
  };
  // Exact channel/origin/RNG/facing values with the same retained resource
  // bank. Capture/preparation require a quiescent bank on its executor. A
  // borrowed-cache adapter cannot manufacture an owning checkpoint.
  Checkpoint checkpoint()const;
  bool owns_checkpoint(const Checkpoint &)const noexcept;
  PreparedRestore prepare_restore(const Checkpoint &)const;
  bool restore_ready(const PreparedRestore &)const noexcept;
  // Swap only. The token retains displaced values until its later destruction.
  bool commit_restore(PreparedRestore &)noexcept;
  class PreparedReplacement {
  public:
    ~PreparedReplacement();
    PreparedReplacement(PreparedReplacement &&)noexcept;
    PreparedReplacement &operator=(PreparedReplacement &&)noexcept;
  private:
    friend class NativeLocomotion;
    PreparedReplacement();struct Data;std::unique_ptr<Data> data_;
  };
  // Historical slots may be absent or have a different current adapter. Build
  // a small private adapter from the owned bank/values without initial gait
  // selection, decode, tick or events. The destination slot must outlive the
  // token; its address, current identity and revision are checked at commit.
  static PreparedReplacement prepare_replacement(const Checkpoint &,
      const std::unique_ptr<NativeLocomotion> &slot);
  static bool replacement_ready(const PreparedReplacement &,
      const std::unique_ptr<NativeLocomotion> &slot)noexcept;
  // Retains the displaced adapter, manager spans and clip bank in the consumed
  // token. Keep that token until all pre-commit frame borrowers have finished.
  static bool commit_replacement(PreparedReplacement &,
      std::unique_ptr<NativeLocomotion> &slot)noexcept;
private:
  struct State;
  explicit NativeLocomotion(std::unique_ptr<State>);
  std::unique_ptr<State> state_;
};
// 1883E0 sound outputs with actual resident matg/foot metadata. Distance/PVS,
// contact and marker ownership are explicit native caller inputs, not guessed.
std::array<std::uint32_t,3> authored_footstep_sounds(
    const Cache &,std::uint32_t foot,std::int16_t global_material,
    unsigned kind,int variant);
// E4D90 uses the unparented unit's world position when the marker ordinal is
// outside bipd+2F8 or the current posed marker lookup returns no result.
Vec3 authored_footstep_position(
    std::span<const std::vector<ObjectAttachmentMarker>> marker_bank,
    std::span<const Transform> current_nodes, unsigned marker, Vec3 unit_position);
} // namespace halo2
