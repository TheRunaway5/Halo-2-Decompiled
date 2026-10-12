#pragma once
#include "halo2/death_camera.hpp"
#include "halo2/unit_scope_camera.hpp"
#include <array>

namespace halo2 {
struct SourceObserverConstants {
  UnitScopeCameraConstants scalar;
  std::array<std::int16_t,6> packet_width{},motion_width{};
  std::array<float,6> acceleration_caps{};
  float epsilon{},orthogonal_tolerance{},quarter{},pi{},full_turn{},velocity_cubic{};
};
// Literal retained358-byte observer. Source pointers are tokens in this native
// representation; packet submission passes the actual packet separately.
using SourceObserverState=std::array<std::uint32_t,0x358/4>;
//170C10 and170FF0. Parent-relative packets retain their actual matrix and
// transform all current derivatives when parent ownership changes.
void source_observer_register_adopt(SourceObserverState&,DeathCameraPacket&,
    const SourceObserverConstants&);
// Complete171910 six-channel owner, including shared-duration cancellation,
// angular delta/application and basis repair. Input is original170D00 dt.
void source_observer_motion(SourceObserverState&,float source_dt,
    const SourceObserverConstants&);
} // namespace halo2
