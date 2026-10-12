#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <span>

namespace halo2 {
// Source query inputs, not a native lobby's inferred online/offline identity.
// Fields are the admitted original current session's +73A0,+1C,+4C78,+84.
struct VistaLobbyInfoSession {
  bool present{};
  std::uint32_t state{};
  std::int32_t session_class{-1};
  std::uint32_t privacy{};
  std::int32_t membership_count{};
};
std::int32_t vista_lobby_info_session_class(const VistaLobbyInfoSession&);
std::uint32_t vista_lobby_info_session_privacy(const VistaLobbyInfoSession&);
std::int32_t vista_lobby_info_session_count(const VistaLobbyInfoSession&);
bool vista_lobby_info_privacy_hidden(std::int32_t session_enum);

using VistaLobbyInfoText = std::uint32_t;
using VistaLobbyInfoDescriptionList = std::span<const std::uint32_t>;
struct VistaLobbyInfoDescriptions {
  bool root_present{},tag_present{};
  std::span<const VistaLobbyInfoDescriptionList> lists;
};
// Bounded authored-data branch of 61C223. No malformed pointer interpretation.
std::optional<std::uint32_t> vista_lobby_info_description_id(
    const VistaLobbyInfoDescriptions&,std::int32_t list,std::int32_t entry);

// Explicit source services. Text token zero means a missing original child.
// Query callbacks provide the actual source wrapper outputs above; none may
// invent a session class or override missing state merely to show help text.
struct VistaLobbyInfoServices {
  std::function<VistaLobbyInfoText(std::int16_t)> lookup_text;
  std::function<std::int32_t()> session_enum,session_class,membership_count;
  std::function<std::uint32_t()> privacy;
  std::function<void(VistaLobbyInfoText,std::uint8_t)> visible;
  std::function<void(VistaLobbyInfoText,std::uint32_t)> string_id;
  std::function<std::optional<std::uint32_t>(VistaLobbyInfoText,std::int32_t,std::int32_t)> description_id;
};
// 6430A4 ignores its six original caller arguments; the command helper has no
// artificial map/player arguments. Text order includes the two leading header
// and button-key text children. Pane is the original signed 16-bit +3484.
void vista_lobby_info_privacy(std::int16_t pane,const VistaLobbyInfoServices&);
// 6432D8 preserves the signed count comparison and intermediate [1,0] visible
// writes when a class/count-admitted row fails the final privacy/enum selection.
void vista_lobby_info_help(std::int16_t pane,const VistaLobbyInfoServices&);
void vista_lobby_info_description(VistaLobbyInfoText,std::int32_t list,
    std::int32_t entry,const VistaLobbyInfoServices&);
} // namespace halo2
