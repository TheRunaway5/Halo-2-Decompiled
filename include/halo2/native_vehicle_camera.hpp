#pragma once
#include "halo2/native_death_camera.hpp"
#include "halo2/source_vehicle_director_tu15.hpp"

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
private:
  struct View {
    SourceObserverState observer{};
    SourceCameraControllerState controller{};
    std::uint32_t player=0xffffffffu;
    bool seated{},handing_off{};
  };
  MultiplayerObjectRuntime &runtime_;
  const OriginalData &data_;
  std::array<View,4> views_{};
  DeathCameraSavedClusters saved_clusters_;
  std::uint32_t random_{};
};
} // namespace halo2
