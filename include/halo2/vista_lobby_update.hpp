#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>

namespace halo2 {
// Supplied Vista64564B..645BB1. This owner does not construct the screen,
// invent session results, or replace any called helper's service boundary.
struct VistaLobbyUpdateState {
  std::uint16_t field08{};
  std::uint8_t field3486{};
  std::array<std::array<std::byte,4>,4> field1c6c{};
  std::int32_t field3514{};
  std::array<std::byte,0x60> field3518{};
  std::uint8_t field3578{};
};
struct VistaLobbyUpdateGlobals {
  std::array<std::byte,0x130> fieldd78cb8{};
  std::uint32_t fieldd78de8{},fieldd78df0{},fieldd78df4{};
  std::uint8_t fieldd78dec{},fieldd78ded{},fieldd78df8{};
};
// Source locals are not initialized wholesale. Unenumerated controller IDs
// and the upper24 host-argument bits retain actual caller stack bytes. Their
// seeds are explicit oracle/service inputs, not invented native defaults.
struct VistaLobbyUpdateScratch {
  std::array<std::uint32_t,4> local_player_ids;
  std::uint32_t host_word;
  std::uint32_t map_first_output;
};
struct VistaLobbyUpdateVariant {
  // A zero identity is the original NULL. Nonzero retains pointer identity
  // while bytes supplies the live owner storage, including +44/+48.
  std::uintptr_t identity{};
  std::span<const std::byte> bytes;
};
struct VistaLobbyUpdateMap {
  std::uint32_t first{};
  std::int32_t map_id=-1;
  std::optional<std::span<const std::byte>> identifier;
};
struct VistaLobbyUpdateTuple {
  std::int32_t session_enum{};
  VistaLobbyUpdateVariant variant;
  // Mutable source stack arrays remain shared across the six-argument calls.
  std::span<std::uint32_t,16> players;
  std::span<std::uint32_t,4> local_players;
  std::int32_t count{};
  std::uint32_t host_word{};
};
enum class VistaLobbyUpdateDisplay {caption,status,privacy,campaign_details,roster};
struct VistaLobbyUpdateServices {
  std::function<std::int32_t()> session_enum; //615EA9, each call sampled.
  std::function<VistaLobbyUpdateVariant()> variant; //615692.
  std::function<std::uint8_t()> host,campaign; //6152B0/615FF6.
  std::function<std::uint8_t()> retry_map; //6169C5.
  std::function<std::uint8_t()> pending_map_ready,pending_variant_ready; //643D85/643D3B.
  std::function<void(std::uint32_t)> pending_map; //61555C.
  std::function<void()> clear_map; //615674.
  std::function<void(std::span<const std::byte,0x130>)> pending_variant; //616901.
  std::function<void()> reset_variant; //616A87.
  std::function<void(const std::array<std::uint32_t,6>&)> notification; //60E1D8.
  std::function<std::uint32_t()> controller_mask; //60758D.
  std::function<void(std::uint32_t)> register_controllers; //611B37.
  std::function<std::uint8_t()> multiplayer; //615FD9.
  std::function<std::int32_t(std::int32_t)> next_controller; //606B13.
  std::function<std::uint32_t(std::int32_t)> local_player; //61573D.
  std::function<std::uint8_t(std::uint32_t)> occupied; //6156E1.
  //68B128 receives element size4 and comparator642FCC. The actual sorter
  //and comparator are explicit services, not a guessed native team order.
  std::function<void(std::span<std::uint32_t>,std::uint32_t,std::uint32_t)> sort_players;
  std::function<void()> dialogs; //6439B8.
  std::function<void(std::uint32_t)> countdown; //644878, original full stack word.
  std::function<void()> pane,visibility,chat; //64300D/643632/643452.
  // Bind these to the existing exact caption/status/info/roster helpers.
  std::function<void(VistaLobbyUpdateDisplay,const VistaLobbyUpdateTuple&)> display;
  //611243 has variant plus seven literal argument words6,3,4,4,47,0,0.
  std::function<void(const VistaLobbyUpdateVariant&,const std::array<std::uint32_t,7>&)> details;
  std::function<void()> map_bitmap,help; //643150/6432D8.
  std::function<std::uint8_t()> state9; //615246.
  std::function<std::uint8_t(std::uint32_t,std::uint32_t)> channel_in_use; //60A420.
  std::function<void(std::uint32_t)> settings_change; //615219.
  std::function<std::uint8_t(VistaLobbyUpdateMap&)> selected_map; //61564E.
  std::function<std::uintptr_t()> ui_context; //609674.
  std::function<std::uint8_t(std::uintptr_t)> identifier_provider; //61AA42.
  std::function<void()> unavailable_map; //5E4A7E.
  std::function<std::int32_t()> peer_count; //615285, not player count.
  std::function<void(std::uint32_t)> allow_single_peer; //6151E2.
  std::function<std::uint32_t()> unavailable_map_text; //64703E.
  std::function<void(const std::array<std::uint32_t,9>&)> question; //60E1DA.
  std::function<void()> browser; //644B1C.
  std::function<void(const VistaLobbyUpdateVariant&)> teams; //645403.
  std::function<std::uint8_t()> favorite; //6156B9.
  std::function<std::uintptr_t()> favorite_context; //60967F.
  std::function<std::uint8_t(std::array<std::uint32_t,2>&)> identity; //6152B5.
  std::function<std::uint8_t(std::uintptr_t,const std::array<std::uint32_t,2>&)> favorite_identity; //5B3E98.
  std::function<void(std::uint32_t)> screen_a0; //screen virtual+A0.
  std::function<void(std::span<const std::uint32_t,16>,std::int32_t)> spinner; //6438FE.
  std::function<void()> base_update; //612CD8, always last.
};
void vista_lobby_update(VistaLobbyUpdateState&,VistaLobbyUpdateGlobals&,
    VistaLobbyUpdateScratch,const VistaLobbyUpdateServices&);
} //namespace halo2
