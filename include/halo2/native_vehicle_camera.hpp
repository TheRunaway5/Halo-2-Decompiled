#pragma once
#include "halo2/native_death_camera.hpp"
#include "halo2/source_vehicle_director_tu15.hpp"
#include <cstddef>
#include <memory>
#include "halo2/source_player_admission.hpp"
#include <span>
#include <vector>

namespace halo2 {
class MultiplayerObjectRuntime;
struct OriginalData;
// Native view binding for the actual TU1.5 camera scheduler and complete
// observer motion/world path. Includes the authored handoff after seat exit.
class NativeVehicleCamera {
public:
  NativeVehicleCamera(MultiplayerObjectRuntime &,const OriginalData &);
  std::optional<NativeDeathCameraView> update(unsigned view,std::uint32_t unit,
      std::uint16_t player_flags,float seconds);
  std::optional<NativeDeathCameraView> update_player(unsigned view,std::uint32_t player,
      std::uint16_t player_flags,float seconds,bool cycle_pressed=false);
  bool death_view(unsigned view) const {return views_.at(view).controller[0xc/4]==0x247140u;}
  using AuthorityBinding=std::array<std::byte,32>;
  class Checkpoint {
  public:
    Checkpoint()=default;
  private:
    friend class NativeVehicleCamera;
    struct Data;std::shared_ptr<const Data> data_;
  };
  class Prepared {
  public:
    ~Prepared();
    Prepared(Prepared &&)noexcept;
    Prepared &operator=(Prepared &&)noexcept;
  private:
    friend class NativeVehicleCamera;
    Prepared();struct Data;std::unique_ptr<Data> data_;
  };
  // Owning in-process observer/controller/cluster/RNG values. The caller must pair
  // this with the same completed native world and its exact cut binding.
  // Neither these values nor their low32 source tick authorize host promotion.
  Checkpoint checkpoint(AuthorityBinding)const;
  // Restore the complete native world first, on this camera's executor. Native
  // player salts and full identities must still agree. Preparation is private;
  // commit requires the prepared world stamp and the same current camera state.
  // Active source recording/replay is excluded. This target and its world
  // must outlive preparation; an owning Checkpoint outlives its producer.
  Prepared prepare(const Checkpoint &,AuthorityBinding)const;
  bool commit(Prepared &&)noexcept;
  // Explicit bounded value codec, provided by halo2_authority_codec. A
  // checksum detects corruption; the caller authenticates the enclosing cut.
  // Ordered view admissions come independently from the accepted couch roster.
  static std::vector<std::byte> encode_checkpoint(const Checkpoint &);
  static std::uint64_t checkpoint_allocation(const Checkpoint &);
  // Rebind owning camera values only after an independently authorized role
  // transition. Ordered salted views remain identical; no world is changed.
  static Checkpoint rebind_checkpoint(const Checkpoint &,AuthorityBinding previous,
      AuthorityBinding next,std::span<const SourcePlayerAdmission> views);
  static Checkpoint decode_checkpoint(std::span<const std::byte>,AuthorityBinding,
      std::uint32_t native_tick,std::span<const SourcePlayerAdmission> views);
private:
  struct View {
    SourceObserverState observer{};
    SourceCameraControllerState controller{};
    std::uint32_t player=0xffffffffu;
    bool seated{},handing_off{};
    bool operator==(const View &)const=default;
  };
  MultiplayerObjectRuntime &runtime_;
  const OriginalData &data_;
  std::array<View,4> views_{};
  DeathCameraSavedClusters saved_clusters_;
  std::uint32_t random_{};
};
} // namespace halo2
