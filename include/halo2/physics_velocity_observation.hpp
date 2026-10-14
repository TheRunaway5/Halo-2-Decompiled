#pragma once
#include <cstdint>
#include <array>
namespace halo2 {
// Optional native observation context for the original linear-velocity store.
// This is host tracing state; it never changes source registers or memory.
inline thread_local std::uint32_t source_velocity_observation_stack{};
inline thread_local std::array<std::uint32_t,8> source_velocity_observation_registers{};
struct SourceVelocityObservationScope {
  std::uint32_t previous;
  std::array<std::uint32_t,8> previous_registers;
  explicit SourceVelocityObservationScope(const std::array<std::uint32_t,8> &registers)
      :previous(source_velocity_observation_stack),previous_registers(source_velocity_observation_registers) {
    source_velocity_observation_stack=registers[4];source_velocity_observation_registers=registers;
  }
  ~SourceVelocityObservationScope(){
    source_velocity_observation_stack=previous;source_velocity_observation_registers=previous_registers;
  }
  SourceVelocityObservationScope(const SourceVelocityObservationScope &)=delete;
};
}
