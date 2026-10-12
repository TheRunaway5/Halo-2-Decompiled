#pragma once
#include "halo2/source_game_over.hpp"
#include <array>
#include <cstdint>

namespace halo2 {
// Original session+72DD records: only the occupied/changed bytes are named.
// Their remaining18 bytes and the enclosing session are owned elsewhere.
struct SourceSessionPhaseRecord { std::uint8_t occupied{},changed{}; };
struct SourceSessionPhaseState {
  std::int32_t phase{},previous_phase{},record_count{};
  std::uint32_t generation{},sequence{},phase_time{},previous_phase_time{};
  std::uint8_t transition_pending{};
  std::array<SourceSessionPhaseRecord,16> records;
};
// Complete retail5A220 / TU5A1F0 / U5 5AED0. Milliseconds is the original
// frozen-clock/GetTickCount API input, separate from the world tick count.
void source_session_phase_set(SourceSessionPhaseState&,std::int32_t,
    std::uint32_t milliseconds);
struct SourceSessionRequest {
  std::uint8_t pending{};
  std::int32_t kind{},length{};
  std::uint32_t first_payload_word{};
};
struct SourceSessionGameOverInput {
  // Query results consumed by original72650 after its719D0 preflight. The
  // independent oracle executes the actual quiet preflight as well.
  bool preflight_changed{};
  std::int32_t main_session_state{};
  bool world_active{},world_closing{},force_end{},game_ready{};
  std::int32_t game_kind=2;
  bool same_game_identity{};
};
// Recovered game-over policy in retail6EA20 / TU6EF30 / U5 72650, including
// the next phase5 update's original request4. This does not consume requests,
// tear down a world, load mainmenu or construct a lobby. Identity-matched map
// transitions, campaign end requests and closing state3/4 services remain
// unavailable and throw before mutating state. No native mode is admitted here.
bool source_session_game_over_update(const SourceSessionGameOverInput&,
    SourceSessionPhaseState&,SourceSessionRequest&,SourceGameOverState&,
    std::int16_t tick_rate,std::uint32_t milliseconds,
    const SourceGameOverConstants&);
// Signed-byte predicates13B320/13B340 at694E0's final return-to-menu gate.
// The earlier world/readiness/predicate calls are not replaced by this leaf.
bool source_game_over_requests_menu(std::int8_t mode,
    const SourceGameOverState&);
} // namespace halo2
