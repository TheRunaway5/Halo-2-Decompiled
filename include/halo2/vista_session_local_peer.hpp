#pragma once
#include "halo2/vista_session_birth.hpp"
#include <string_view>

namespace halo2 {
// Actual5AD7B7 transport/channel owner bank and actual926C88 pointed-to float
// bits. Neither is synthesized from UDP participant counts. A channel record
// is the source0x740 bytes, including its status, flags, and retained sample.
struct VistaSessionConnectivityValues {
  std::span<const std::byte> channel_owner;
  std::optional<std::uint32_t> statistic_scale_bits;
};
// Literal5B20EB/5C20C8/5C5B4D/5E0D55. Consumes actual session membership,
// local index, association records, and channel owner bytes. The source's
// signed clamp, scale-dependent trim, and integer division are retained.
// Unsupported consumed extents or a trim leaving no samples reject.
std::array<std::uint32_t,4> vista_session_peer_connectivity(
    std::span<const std::byte> actual_session,
    const VistaSessionConnectivityValues& actual_values);

struct VistaSessionLocalPeerValues {
  std::u16string_view name;  // Actual91A592 producer; capacity16 in packet.
  std::u16string_view party; // Actual91A5B2 producer; capacity32 in packet.
  std::uint8_t optional_metadata_byte;
  std::uint32_t metadata68,metadata70;
  std::uint32_t map_status,map_progress; // Actual91A620/91A624 owners.
  std::uint32_t bandwidth; // Actual5E0D39/926D08 result.
  VistaSessionConnectivityValues connectivity;
};
struct VistaSessionLocalPeerClocks {
  std::optional<std::uint32_t> elapsed; //5B3CB4, consumed only if last!=0.
  std::optional<std::uint32_t> publication; //5B3CA0, only after accepted call.
};
struct VistaSessionLocalPeerMessage {
  std::uint32_t transport_owner,session_slot,channel;
  // Actual5BED40 args: slot,channel,0,1Ah,D0h,payload. This is a concrete
  // transport intent; the caller's real binding owns delivery.
  std::array<std::byte,0xd0> payload;
};
struct VistaSessionLocalPeerResult {
  bool built{},publication_attempted{},accepted{};
  bool elapsed_clock_consumed{},publication_clock_consumed{};
  std::array<std::byte,200> payload{};
  std::optional<VistaSessionLocalPeerMessage> message;
};
class VistaSessionLocalPeerServices {
 public:
  virtual ~VistaSessionLocalPeerServices()=default;
  virtual std::uint32_t network_clock()=0;
  virtual void send_peer_message(const VistaSessionLocalPeerMessage&)=0;
};
// Literal5B2C51 joined with actual5C83F3. Reads the actual local record and
// compares all200 bytes. Local states5..8 publish in-place and increment both
// revisions; remote states3/4 return the exact transport intent if associated.
// Source accepted AL does not assert network delivery. Clock/state/owner input
// rejection is atomic. The separate per-session91A910 clock remains retained
// by the caller. No source name, identity, readiness, or bandwidth is invented.
VistaSessionLocalPeerResult vista_session_publish_local_peer(
    std::span<std::byte> actual_session,std::uint32_t& actual_last_publish_clock,
    std::int32_t actual_threshold,const VistaSessionLocalPeerValues&,
    const VistaSessionLocalPeerClocks&);
// Synchronous binding overload: actual clock calls occur only at the source
// call sites and remote transport dispatch precedes the publication clock.
// Services must not reenter these banks. External exceptions/effects are not
// rolled back, just as with the original owner's service calls.
VistaSessionLocalPeerResult vista_session_publish_local_peer(
    std::span<std::byte> actual_session,std::uint32_t& actual_last_publish_clock,
    std::int32_t actual_threshold,const VistaSessionLocalPeerValues&,
    VistaSessionLocalPeerServices& actual_services);
} // namespace halo2
