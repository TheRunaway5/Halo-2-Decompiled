#pragma once
#include "halo2/vista_session_birth.hpp"
#include <functional>

namespace halo2 {
struct VistaSessionPhaseMessage {
  std::uint32_t transport_owner,session_slot,channel;
  //5BED40(slot,channel,0,22h,4B4h,payload). AL is not consumed.
  std::array<std::byte,0x4b4> payload;
};
struct VistaSessionPhaseServices {
  std::function<std::uint32_t()> network_clock;
  std::function<void(const VistaSessionPhaseMessage&)> send;
};
// Complete raw-bank5C596E. The two real clock callbacks occur between the
// original writes, preserving observable order and synchronous service writes.
void vista_session_phase_write(std::span<std::byte>,std::uint32_t requested,
    const VistaSessionPhaseServices&);
//5C884A including local phase writes and remote3/4 message construction.
// The source's host/local gate, absent association and unchanged phase AL are
// retained; acceptance is not relabeled network delivery or UI readiness.
bool vista_session_phase_request(std::span<std::byte>,std::uint32_t requested,
    const VistaSessionPhaseServices&);
//6152B0/5AD940 and the actual6156D2→5AD9E3 Done session action. Kind0/1
// retains its own false/true branch. PCR input itself owns its earlier query.
bool vista_session_pcr_host(std::span<const std::byte>,std::uint8_t lifecycle_enabled);
bool vista_session_pcr_continue(std::span<std::byte>,std::uint32_t lifecycle_kind,
    const VistaSessionPhaseServices&);
} // namespace halo2
