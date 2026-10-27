#pragma once
#include "halo2/frontend.hpp"
#include "halo2/native_vista_session.hpp"
#include "halo2/vista_pcr_appearance.hpp"
#include "halo2/vista_pcr_publication.hpp"
#include "halo2/multiplayer_datagrams.hpp"

namespace halo2 {
class MultiplayerMatch;

// Literal supplied-PC6156CD/616D34/615820 consumers of the real lifecycle
// and current-session bank. These leaves do not create membership or XUIDs.
bool native_vista_pcr_identity_query(std::span<const std::byte> session,
    std::uint8_t lifecycle_active,std::uint32_t lifecycle_kind,
    const VistaLocalUserIdentity&);
std::uint32_t native_vista_pcr_primary_player_query(std::span<const std::byte>,
    std::uint8_t lifecycle_active,const VistaLocalUserIdentity&);
bool native_vista_pcr_player_query(std::span<const std::byte>,
    std::uint8_t lifecycle_active,std::uint32_t player);

// Original470182's Slayer branch and46C675. Decimal/zero-padded signed
// integer and literal-percent formats are the admitted44BD7B CRT subset.
// Unsupported formatting is rejected, rather than displayed approximately.
std::u16string native_vista_pcr_format(std::u16string_view,
    std::span<const std::int32_t>);
std::uint32_t native_vista_pcr_placement_key(std::int32_t) noexcept;

struct NativeVistaPcrPublicationState {
  std::uint8_t current{},finalized{};
  bool engine_present{};
};
struct NativeVistaPcrServices {
  // Actual retained completion/engine owner results. The numeric recording
  // flags alone do not establish the PC retained bank's+10/+11 publication.
  std::function<NativeVistaPcrPublicationState()> publication;
  std::function<std::uint8_t()> lifecycle_active;
  std::function<std::uint32_t()> lifecycle_kind;
  // Reached FFA local-XUID branch44A619/44B553. A live implementation must
  // own the actual achievement request; an empty success callback is not one.
  std::function<void(unsigned distinct_medals)> medal_achievement;
  VistaLoadingFloatingPoint floating_point;
};

// Application-owned bridge for actual local offline FFA Slayer participants.
// Controller0 owns report input and the selected achievement identity; all
// participant results are copied before world teardown; no match
// reference survives construction. The actual session and FrontendContent
// must outlive this object and its returned row callbacks.
//
// This is explicitly not a full retained metadata publication. Initial pane,
// initial focus, input actions and factory-time network clock remain with
// their real screen/session owners.
class NativeVistaPcr final:private VistaPcrAppearanceServices {
public:
  NativeVistaPcr(const MultiplayerMatch&,unsigned actual_native_slot,
      const NativeVistaSession&,const FrontendContent&,
      const VistaPcrPcVariant& actual_selected_variant,NativeVistaPcrServices);
  // Explicit native ONLINE identity adapter. Retained keys are authenticated
  // random room identities, not Windows/Xbox Live account metadata.
  NativeVistaPcr(VistaPcrConsumedSnapshot,std::vector<OnlineAdmission>,
      const FrontendContent&,VistaLoadingFloatingPoint,
      std::function<std::optional<std::uint16_t>()> current_presence = {});
  NativeVistaPcr(const NativeVistaPcr&)=delete;
  NativeVistaPcr& operator=(const NativeVistaPcr&)=delete;
  const VistaPcrConsumedSnapshot& source() const noexcept{return source_;}
  std::shared_ptr<const VistaPcrResults> results() const noexcept{return results_;}
  VistaPcrRowServices rows(unsigned source_screen);
  VistaPcrPlayerPresentation appearance(unsigned row,unsigned source_screen=16,
      unsigned pane=1);
  VistaPcrVoicePresentation voice(unsigned row,std::uint8_t lobby_variant,
      const VistaPcrVoiceServices&) const;
  std::uint32_t selected_row() const noexcept{return selected_row_;}
  // Native focus notification, including NONE for an empty team pane. An
  // initial value is deliberately not advertised as the original default.
  void select_row(std::uint32_t row);
  const VistaLoadingFloatingPoint& floating_point() const noexcept{return services_.floating_point;}
private:
  const NativeVistaSession* session_{};
  std::vector<OnlineAdmission> online_roster_;
  std::function<std::optional<std::uint16_t>()> online_presence_;
  std::optional<std::uint16_t> appearance_presence_;
  const FrontendContent& content_;
  NativeVistaPcrServices services_;
  VistaPcrConsumedSnapshot source_;
  std::shared_ptr<VistaPcrResults> results_;
  std::vector<VistaPcrRgb> palette_,teams_;
  VistaPcrAppearanceAssets assets_;
  std::uint32_t selected_row_=0xffffffffu;
  void initialize_assets(const FrontendContent&);
  bool identity_query(std::array<std::byte,8>) override;
  std::uint32_t primary_player_query(std::array<std::byte,8>) override;
  bool player_query(std::uint32_t) override;
};
} // namespace halo2
