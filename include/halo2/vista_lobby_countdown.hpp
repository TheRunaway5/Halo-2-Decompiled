#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

namespace halo2 {
// Supplied PC image; the bounded consumer is644878..6448CD. The remaining
// sign-in tail of644878 and the stage producer616B7A are not implemented here.
inline constexpr std::string_view vista_lobby_countdown_source_sha256 =
    "33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109";
inline constexpr std::uint32_t vista_lobby_countdown_source_begin = 0x644878;
inline constexpr std::uint32_t vista_lobby_countdown_source_end = 0x6448cd;
inline constexpr std::string_view vista_lobby_countdown_source_code_sha256 =
    "8054f54b3b1ce7b059e6fc45d5e2ae874fcdd46c40e38967fdac8c2831985715";

struct VistaLobbyCountdownState {
  std::int32_t stage = -1; // Screen+347C; not a duration in seconds.
  std::uint32_t stage3_clock{}; // Screen+3480, written only on a changed stage3.
};
// Explicit original service results. Inactive updates do not consume stage;
// clock is consumed only after a changed stage3 sound intent. This API supplies
// no session/readiness query, stage timer, native audio activation or clock.
struct VistaLobbyCountdownQueryResults {
  bool active{}; //616BA2 AL truth value.
  std::int32_t stage{}; //616B7A EAX, compared as signed32.
  std::uint32_t clock{}; //6096AE EAX: raw caller-owned globalD71900 word.
};
enum class VistaLobbyCountdownService {
  active_query, stage_query, ui_sound, clock_query
};
struct VistaLobbyCountdownCallIntent {
  VistaLobbyCountdownService service{};
  // Only61DD04's ui_sound intent has an argument (slot10).
  std::optional<std::uint32_t> argument;
  bool operator==(const VistaLobbyCountdownCallIntent&) const = default;
};
struct VistaLobbyCountdownUpdate {
  std::array<VistaLobbyCountdownCallIntent,4> calls{};
  unsigned call_count{};
  std::span<const VistaLobbyCountdownCallIntent> call_intents() const noexcept {
    return {calls.data(),call_count};
  }
};
// Records the original service-call order and resulting two fields. It does
// not execute external services or model their unowned side effects. In
// particular, soundslot10 is an intent, not a resolved PCM/audio event.
// Original6448AD rereads+347C after61DD04. Possible reentrant service mutation
// of that owner requires a separate callback contract, not this pure intent API.
// This consumer establishes no complete UI owner or session timing behavior.
VistaLobbyCountdownUpdate vista_lobby_countdown_consume(
    VistaLobbyCountdownState&,const VistaLobbyCountdownQueryResults&);

// Dynamic original616BA2/616B7A/643A40/644878 owners. Every callback is a
// consumed service, not a requested readiness value. Unconsumed callbacks may
// be absent; a missing consumed callback rejects before that service occurs.
struct VistaLobbyCountdownServices {
  std::function<std::int32_t()> ui_state; // Complete616490(NULL) producer.
  std::function<std::uint8_t()> lifecycle_active; //5B5038 AL.
  std::function<std::uint32_t()> lifecycle_slot; //5AD660.
  std::function<std::int32_t()> session_stage; //5B13C7.
  std::function<void(std::uint32_t)> ui_sound; //61DD04; actual Wigl slot.
  std::function<std::uint32_t()> ui_clock; //6096AE, separate from network clock.
  //615A4B queries5B1BDE, ignores its return/output, then returns literal10.
  std::function<void()> duration_context;
  std::function<std::int32_t(std::int32_t)> controller_user; //60687F.
  std::function<std::uint8_t()> host_permission; //5AD940 AL.
  std::function<std::uint8_t(std::uint8_t,std::int32_t,std::uint8_t,
                            std::int32_t)> write; //5B11E2; last pointer is NULL.
};
bool vista_lobby_countdown_active(const VistaLobbyCountdownServices&);
std::int32_t vista_lobby_countdown_stage(const VistaLobbyCountdownServices&);
// Complete countdown prefix with the sign-in tail argument zero. A sound
// callback may mutate state: the original rereads stage before the stage3
// clock write. Changed stages0..3 beep once; stages10..4/unchanged are silent.
void vista_lobby_countdown_update(VistaLobbyCountdownState&,
    const VistaLobbyCountdownServices&);
// Complete643A40 caller and617328 request: initial10, quick-advance floor3.
// Only an inactive rejected request plays source error soundslot2.
bool vista_lobby_countdown_start_action(std::int32_t controller,
    const VistaLobbyCountdownServices&);
} // namespace halo2
