#pragma once
#include "halo2/physics_body_factory.hpp"
#include <array>
#include <optional>
#include <vector>
#include <span>

namespace halo2 {
// TU1.5 engine+E4[4],F4,F5[4],FA. Original15EBA0 is the tick producer;
//15C2A0 consumes the actual local output-user and geometry-preload status.
// This is viewport presentation state, distinct from the audio fade and
// from the later full-surface loading compositor that covers the scoreboard.
struct SourceGameplayFadeStateTu15 {
  std::array<std::uint32_t,4> opacity_bits{};
  std::uint8_t activation_mask{};
  std::array<std::uint8_t,4> rendered_frames{};
  std::int16_t phase_counter{};
  bool operator==(const SourceGameplayFadeStateTu15 &)const=default;
};
struct SourceGameplayFadePlayerTu15 {
  bool present{};
  std::uint32_t current_unit=0xffffffffu;
  std::uint16_t flags{};
  std::int8_t membership=-1;
  std::int16_t lives{};
  std::int32_t respawn_seconds{}; // Original player+170, not+164 tick timer.
  bool operator==(const SourceGameplayFadePlayerTu15 &)const=default;
};
struct SourceGameplayFadeTickTu15 {
  bool engine_present{};
  std::uint8_t raw_mode=1;
  std::int16_t phase{},rate=30;
  std::int32_t pending_phase{},life_limit{};
  std::uint32_t tick{},round_start_tick{};
  std::array<SourceGameplayFadePlayerTu15,4> users;
  bool operator==(const SourceGameplayFadeTickTu15 &)const=default;
};
// The15AF40 E4/F4 writes only. Player+170's separate source birth store is
// excluded: a presentation binding must not reset a player's real respawn.
void source_gameplay_fade_activate_tu15(SourceGameplayFadeStateTu15 &,unsigned actual_view);
// Exact consumed15ED59..15EF94 tail, including original15B470/160570
// eligibility. Call once per actual original world tick with its real users.
void source_gameplay_fade_tick_tu15(SourceGameplayFadeStateTu15 &,
    const SourceGameplayFadeTickTu15 &,SourcePhysicsFactoryFloatingPoint);
struct SourceGameplayFadeRenderTu15 {
  bool engine_present{};
  unsigned actual_view=0xffffffffu;
  std::int32_t unresolved_world_geometry{},rate=30;
};
// Only the15C2A0 viewport-black projection. The original has other-player
// model fades before this and a separate full scoreboard after it. Its D3D
// color constant contains the exact binary32 alpha; never quantize to ARGB8.
std::optional<float> source_gameplay_fade_render_tu15(SourceGameplayFadeStateTu15 &,
    const SourceGameplayFadeRenderTu15 &,SourcePhysicsFactoryFloatingPoint);
// Pointer-free observations of original15ED59, after1617A0 and before any
// viewport fade arithmetic. Each frame retains all admitted logical players;
// each machine projects only its actual local views. The circular history
// spans more than the transport's ten-second liveness window at30Hz.
inline constexpr unsigned source_gameplay_fade_history_tu15=512;
inline constexpr unsigned source_gameplay_fade_journal_header_wire_tu15=8+1+1+2+2+4+16*(4+8)+2;
inline constexpr unsigned source_gameplay_fade_frame_wire_tu15=8+4+4+1+1+2+2+2+4+4+2+4+16*(4+8+1+4+2+1+2+4);
struct SourceGameplayFadeIdentityTu15 {
  std::uint32_t player=0xffffffffu;
  std::uint64_t activation{};
  bool operator==(const SourceGameplayFadeIdentityTu15 &)const=default;
};
struct SourceGameplayFadeFrameTu15 {
  std::uint64_t sequence{};
  std::uint32_t tick{},round_start_tick{};
  bool engine_present{};
  std::uint8_t raw_mode{};
  std::int16_t phase{},rate{},phase_counter_before{};
  std::int32_t pending_phase{},life_limit{};
  std::uint16_t x87_control_word{};
  std::uint32_t mxcsr{};
  std::array<SourceGameplayFadeIdentityTu15,16> identities;
  std::array<SourceGameplayFadePlayerTu15,16> players;
  bool operator==(const SourceGameplayFadeFrameTu15 &)const=default;
};
struct SourceGameplayFadeActivationTu15 {
  std::uint32_t player=0xffffffffu;
  bool engine_present{};
  std::int16_t rate{};
  std::uint16_t x87_control_word{};
  std::uint32_t mxcsr{};
};
struct SourceGameplayFadeJournalTu15 {
  std::uint64_t newest{};
  bool context_available{},engine_present{};
  std::int16_t rate{};
  std::uint16_t x87_control_word{};
  std::uint32_t mxcsr{};
  std::array<SourceGameplayFadeIdentityTu15,16> current;
  std::vector<SourceGameplayFadeFrameTu15> slots;
  bool operator==(const SourceGameplayFadeJournalTu15 &)const=default;
};
void validate_source_gameplay_fade_journal_tu15(const SourceGameplayFadeJournalTu15 &);
std::vector<std::byte> encode_source_gameplay_fade_journal_tu15(const SourceGameplayFadeJournalTu15 &);
SourceGameplayFadeJournalTu15 decode_source_gameplay_fade_journal_tu15(std::span<const std::byte>);
// View-local presentation state. Publication cannot seed opacity/mask/counters;
// it can supply only actual original tick inputs and lifecycle activation.
class SourceGameplayFadePresentationTu15 {
  SourceGameplayFadeStateTu15 state_;
  std::array<SourceGameplayFadeIdentityTu15,4> applied_{};
  std::uint64_t sequence_{};
  bool context_available_{},engine_present_{};
  std::int32_t rate_{};
  SourcePhysicsFactoryFloatingPoint fp_{};
  void activate(const std::array<SourceGameplayFadeIdentityTu15,16> &,std::span<const std::uint32_t>);
public:
  void consume(const SourceGameplayFadeJournalTu15 &,std::span<const std::uint32_t> actual_local_players);
  std::optional<float> render(unsigned actual_view,std::int32_t unresolved_world_geometry);
  const SourceGameplayFadeStateTu15 &state()const noexcept{return state_;}
  std::uint64_t sequence()const noexcept{return sequence_;}
};
}
