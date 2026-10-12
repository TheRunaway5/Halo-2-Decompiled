#pragma once
#include "halo2/vista_pcr_appearance.hpp"

namespace halo2 {
// 6157CA returns the final 0x84-byte configuration, not a profile or peer.
using VistaLobbyRosterConfiguration=std::array<std::byte,0x84>;
struct VistaLobbyRosterLocalEdit {bool active{};std::int16_t team=-1;};
struct VistaLobbyRosterInput {
  std::int32_t session_enum=-1;
  std::int16_t pane{};
  bool variant_present{};
  std::uint8_t variant_flags{}; // original variant+48, bit0 team game
  std::span<const std::uint32_t> player_ids;
  std::array<std::uint32_t,4> local_player_ids{0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu};
  std::array<VistaLobbyRosterLocalEdit,4> local_edits;
};
struct VistaLobbyRosterDescriptor {
  std::uint32_t flags{};
  std::u16string name;
  std::array<std::byte,16> appearance{};
  std::uint16_t team=0xffff,rank_range=0xffff;
  std::uint32_t team_string=0xffffffffu;
  std::uint8_t no_team{};
  std::int32_t selector{};
};
class VistaLobbyRosterServices {
 public:
  virtual ~VistaLobbyRosterServices()=default;
  virtual std::int32_t current_session_enum()=0; // 615FF6 ->615EA9
  virtual VistaLobbyRosterConfiguration player_configuration(std::uint32_t)=0; // 6157CA
  // 611CD0 applies each descriptor to one PC skin group, then hides unused groups.
  virtual void publish(std::span<const VistaLobbyRosterDescriptor>)=0;
  // 6446E2 computes these colors but does not consume its result.
  virtual void unused_colors(std::uint32_t team,std::uint8_t no_team,
      std::array<std::byte,16>)=0;
  virtual std::uint32_t lookup_text(std::int16_t)=0; // 611973, zero is missing
  virtual std::uint32_t lookup_bitmap(std::int16_t)=0; // 6119F7
  virtual void visible(std::uint32_t,bool)=0;
  virtual bool voice_available(std::uint32_t)=0; // 5B4AFC, low byte boolean
  virtual bool voice_active(std::uint32_t)=0; // 5B4AA1, queried only if available
  virtual void frame(std::uint32_t,std::uint32_t)=0; // 61C300
  virtual void hide_slot(std::uint32_t kind,std::int32_t ordinal)=0; // 611ABD
};
// Actual 6443F0 owner. Admitted occupied configurations must exist. This does
// not invent a row for a peer count or substitute a selected profile.
void vista_lobby_roster_update(const VistaLobbyRosterInput&,
    const VistaPcrAppearanceAssets&,VistaLobbyRosterServices&);

struct VistaLobbyRosterSkinView {
  std::u16string name;
  std::uint32_t team_string=0xffffffffu;
  VistaPcrRgb primary_color{},name_color{};
  bool name_visible{},primary_visible{},secondary_visible{},rank_visible{},emblem_visible{};
  std::uint16_t primary_frame{},rank_frame{};
};
// 620652/640F96 PC skin consumer for descriptors produced by 6443F0.
// Actual imported bitmap page counts and previous frames retain 61C300 behavior.
VistaLobbyRosterSkinView vista_lobby_roster_skin(const VistaLobbyRosterDescriptor&,
    const VistaPcrAppearanceAssets&,const VistaLoadingFloatingPoint&);
} // namespace halo2
