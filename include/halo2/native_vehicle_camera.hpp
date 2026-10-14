#pragma once
#include "halo2/native_death_camera.hpp"

namespace halo2 {
class MultiplayerObjectRuntime;
struct OriginalData;
// Native view binding for the actual TU1.5 seated camera producer245EA0 and
// the complete observer motion/world path. Physics owns the body and seat.
class NativeVehicleCamera {
public:
  NativeVehicleCamera(MultiplayerObjectRuntime &,const OriginalData &);
  std::optional<NativeDeathCameraView> update(unsigned view,std::uint32_t unit,
      std::uint16_t player_flags,float yaw,float pitch,float seconds,
      const NativeDeathCameraView &ordinary);
private:
  struct View {
    SourceObserverState observer{};
    std::array<std::uint32_t,7> producer{};
    std::uint32_t unit=0xffffffffu;
    bool seated{};
  };
  MultiplayerObjectRuntime &runtime_;
  const OriginalData &data_;
  std::array<View,4> views_{};
  DeathCameraSavedClusters saved_clusters_;
};
} // namespace halo2
