#pragma once
#include "halo2/math.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>
namespace halo2 {
struct NativeMeleePhysicsOperands {
  std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> literals;
  std::array<std::array<std::byte,12>,3> basis;
};
struct NativeMeleeLunge {
  std::array<std::byte,0x64> state{};
  std::uint32_t target=0xffffffffu;
  bool active{};
};
void start_native_melee_lunge(NativeMeleeLunge &,std::uint32_t target,bool sword,
    const NativeMeleePhysicsOperands &);
// Complete original1EF290 arithmetic. Native scene contacts own the input
// target point/velocity and capsule motion, outside this pure retained owner.
Vec3 tick_native_melee_lunge(NativeMeleeLunge &,float distance,Vec3 target_velocity,
    Vec3 target_position,Vec3 aim,Vec3 velocity,Vec3 position,
    const NativeMeleePhysicsOperands &);
}
