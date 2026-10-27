#pragma once
#include "halo2/source_vehicle_director_tu15.hpp"
namespace halo2 {
class PhysicsLiveWorldTu15;
class NativeVehicleCamera;
// Original157EB0 output from this executor's locally constructed map. This
// value has no wire decoder/raw-word constructor and is never in an incoming
// authority checkpoint. Copying the opaque value grants no network authority.
class SourceOriginalCameraSeed {
public:
  SourceOriginalCameraSeed()=default;
  bool available()const noexcept{return available_;}
  bool operator==(const SourceOriginalCameraSeed &)const=default;
private:
  friend class PhysicsLiveWorldTu15;
  friend class NativeVehicleCamera;
  SourceCameraControllerState controller_{};
  bool available_{};
};
}
