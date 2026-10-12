#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include "halo2/vista_lobby_layout.hpp"

namespace halo2 {
// Supplied halo2.exe SHA33119398...109, session+4AE0. Field names retain
// offsets where the original owner does not establish a semantic name.
struct VistaLobbyStatusRecord { std::array<std::uint8_t,0xa0> bytes{}; };
struct VistaLobbyStatusPeers {
  std::uint32_t mask{}; // session membership+11E8.
  std::array<std::array<std::uint8_t,8>,16> identities{}; //+11EC, stride128h.
};
struct VistaLobbyStatusRecordSource {
  std::uint8_t available{}; //5AD736 AL; a non-null session is also required.
  const VistaLobbyStatusRecord* record{}; //5C2843 ->session+4AE0.
};
struct VistaLobbyStatusTuple {
  std::int32_t field04{},peer08=-1;
  // Original output3/4 alias the source record. They remain non-owning and
  // valid only for the lifetime of the supplied record; no string is inferred.
  const std::uint8_t* field08{};
  const std::uint8_t* field48{};
  std::int32_t field88{},peer8c=-1;
  std::uint16_t field94{},field96{},field98{};
  std::uint32_t field9c{};
};
struct VistaLobbyStatusQueryServices {
  std::function<VistaLobbyStatusRecordSource()> record; //5AD736/5C2843.
  std::function<VistaLobbyStatusPeers()> peers; //5C20C8(0,0), fresh each call.
};
// Full5B0B79 with all ten output pointers supplied, as consumed by644E0F.
// Failure leaves every output unchanged. The first matching occupied peer is
// selected; the second identity query runs regardless of field04.
bool vista_lobby_status_tuple(VistaLobbyStatusTuple&,const VistaLobbyStatusQueryServices&);
// Bind the literal5AD736/5C2843/5C20C8 leaves to a retained native source bank.
// lifecycle_enabled is the actual820FC0 predicate. The caller owns the record
// copy for the lifetime of output3/4; failure preserves both it and the tuple.
// Membership is session+70, so its mask/identities are bank+1258/+125C.
bool vista_lobby_status_session_tuple(std::span<const std::byte> session_bank,
    std::uint8_t lifecycle_enabled,VistaLobbyStatusRecord& retained,VistaLobbyStatusTuple&);

struct VistaLobbyStatusText {
  std::uint8_t visible{}; //widget+6E.
  VistaLobbyLayoutRect bounds{}; //widget+20.
  std::uint16_t buffer_flags{}; //virtual+4C result+14, status style bit0.
};
struct VistaLobbyStatusState {
  std::int16_t pane{}; //screen+3484.
  std::uint8_t dialog_shown{}; //screen+3579, not a guessed dialog lifetime.
  std::int32_t previous_kind{}; //source globalD78E00, retained across updates.
};
struct VistaLobbyStatusServices {
  std::function<VistaLobbyStatusText*(std::int16_t)> text; //611973 runtime index.
  std::function<void(VistaLobbyStatusText&,std::uint32_t)> set_string; //61BF85.
  std::function<void(VistaLobbyStatusText&,std::uint32_t)> append_string; //61BFF6.
  std::function<void(VistaLobbyStatusText&)> clear_string; //virtual+4C then+4(empty).
  std::function<void(VistaLobbyStatusText&,const std::u16string&)> set_raw; //61BAA9.
  std::function<void()> refresh_button_key; //screen virtual+60.
  std::function<std::uint8_t()> countdown_active; //616BA2.
  std::function<std::uint8_t()> favorite; //6156B9.
  std::function<std::uint8_t(std::array<std::uint32_t,2>&)> identity; //6152B5.
  std::function<std::uint8_t(const std::array<std::uint32_t,2>&)> favorite_identity; //60967F ->5B3E98.
  std::function<std::uint8_t()> multiplayer; //615FD9, only tested for team variants.
  std::function<bool(VistaLobbyStatusTuple&)> query; //5B0B79, all ten outputs.
  std::function<std::int32_t()> readiness; //616490(NULL), NOT tuple.field04.
  std::function<std::uint8_t()> reset_dialog; //61553F.
  std::function<void(VistaLobbyStatusState&)> status7_dialog; //64499F, independent owner.
  std::function<std::uint32_t(std::int32_t)> status_string; //61678C.
  std::function<std::uint8_t(std::uint32_t)> custom_map; //4D108F.
  std::function<std::optional<std::u16string>(std::uint8_t,std::uint8_t)> custom_text; //4D10BB.
  std::function<std::int32_t()> count; //61525A.
  std::function<std::int32_t()> maximum; //615214.
  std::function<std::uint32_t()> overflow_string; //6101E9(screen,0,3).
  std::function<std::int32_t()> user; //61569C.
  std::function<std::uint8_t(std::int32_t)> user_ready; //615885.
  std::function<std::uint8_t()> state2; //6156A1.
  std::function<std::uint8_t()> campaign; //615FF6.
  std::function<std::uint8_t()> state9; //615246.
  std::function<std::uint8_t()> team_change; //6168FC, separately queried at end.
  std::function<std::int32_t()> map_team_limit; //643B35.
  std::function<void(std::int32_t)> screen_a0; //screen virtual+A0(1).
  std::function<std::uint8_t()> factory_flag; //60B365.
};
// Full644D0D. Other four original arguments are not read. The optional byte
// is the actual selected variant+48, not a native team-game inference.
void vista_lobby_button_key(std::optional<std::uint8_t> variant48,std::uint8_t host,
    const VistaLobbyStatusServices&);
// Full644E0F.61553C in this supplied executable always returns false without
// writing its output. When status_string returns0F0001FD, source still passes
// an unspecified stack dword to4D108F. Expose its seed solely for a bounded
// instruction oracle;4D10BB(false,...) does not consume that result to choose
// its string. No caller may interpret the seed as native map readiness.
void vista_lobby_status(VistaLobbyStatusState&,std::uint8_t host,
    std::uint32_t original_stack_seed,const VistaLobbyStatusServices&);
} //namespace halo2
