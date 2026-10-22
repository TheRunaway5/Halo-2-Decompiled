#pragma once
#include "halo2/native_vehicle_camera.hpp"
#include "halo2/source_player_profile.hpp"
namespace halo2 {
struct NativeVehicleCamera::Checkpoint::Data {
  AuthorityBinding binding{};std::uint32_t tick{};
  std::array<View,4> views;
  DeathCameraSavedClusters clusters;
  std::uint32_t random{};
  std::array<SourcePlayerIdentity,4> identities{};
  bool matches(const NativeVehicleCamera &camera)const noexcept {
    return views==camera.views_&&clusters.bsp==camera.saved_clusters_.bsp&&
        clusters.clusters==camera.saved_clusters_.clusters&&random==camera.random_;
  }
};
}
