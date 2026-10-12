#pragma once
#include "halo2/observer_world.hpp"

namespace halo2 {
class MultiplayerObjectRuntime;
struct OriginalData;
struct NativeDeathCameraView {
  Vec3 position{},forward{},up{};
  float horizontal_fov{};
};
// Native local-view binding for the original multiplayer death packet and
// complete observer adoption, motion and world owners. Player and corpse
// identities are read from the retained live source pools on every frame.
class NativeDeathCamera {
public:
  NativeDeathCamera(MultiplayerObjectRuntime &,const OriginalData &,
                   std::uint32_t &auxiliary_random);
  std::optional<NativeDeathCameraView> update(unsigned view,
      std::uint32_t player,std::uint16_t player_flags,bool team_only,
      float seconds,bool game_in_progress,bool clock_running,bool cycle_pressed,
      const NativeDeathCameraView &ordinary);
private:
  struct View {
    SourceObserverState observer{};
    std::optional<DeathCameraState> death;
    std::optional<NativeDeathCameraView> presented;
    std::uint32_t player=0xffffffffu;
  };
  MultiplayerObjectRuntime &runtime_;
  const OriginalData &data_;
  std::uint32_t &random_;
  std::array<View,4> views_{};
  DeathCameraSavedClusters saved_clusters_;
};
} // namespace halo2
