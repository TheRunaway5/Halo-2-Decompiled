#pragma once
#include <array>
#include <bit>
#include <cstdint>
#include <span>

namespace halo2 {
using SourcePlayerIdentity = std::array<std::uint8_t,12>;
using SourceMachineIdentity = std::array<std::uint8_t,6>;
using SourcePlayerConfiguration = std::array<std::uint8_t,0x90>;
using SourceFullPlayerProfile = std::array<std::uint8_t,0x1e0>;

// Whole14B410/14CB60/14DF40. Machine bytes must come from a real platform
// address provider. A player datum, display name or session nonce is not an ID.
SourcePlayerIdentity source_offline_player_identity(const SourceMachineIdentity&,
    std::int32_t local_user);
// Whole1A0750/1A40B0/1AA360 and1537F0/154F40/156320 constructors.
SourceFullPlayerProfile source_default_full_player_profile();
SourcePlayerConfiguration source_default_player_configuration();

// Complete retained local-profile converter inputs. Input membership is the
// original local four-record table, not a successful Xbox online-account call.
// All bytes remain supplied by their owning local/profile/input producers.
struct SourceOfflineLocalProfile {
  SourcePlayerIdentity identity{};
  std::array<std::uint16_t,32> display_name{};
  std::array<std::uint8_t,16> appearance{};
  std::array<std::uint8_t,12> account{};
  std::array<std::uint8_t,32> account_data{};
  std::int32_t committed_team = -1;
  std::uint8_t field_200{}, field_46c{};
  std::uint32_t field_204{};
  std::int32_t input_member = -1;
  std::uint8_t input_member_present{};
  SourcePlayerConfiguration input_configuration{};
};
// TU1.5's192B91 substitutes4 when its real global51D2C8 is nonzero;
// retail/TU1.1 use the stored field. Caller supplies that version/global gate.
SourcePlayerConfiguration source_offline_local_configuration(
    const SourceOfflineLocalProfile&,bool supplement_override_four = false);
void source_convert_offline_local_profile(const SourceOfflineLocalProfile&,
    SourcePlayerIdentity* identity,SourcePlayerConfiguration* configuration,
    bool supplement_override_four = false);

//18FD94/1917C6/192BAA: signed32 local property -> signed8 wire+7C.
inline std::int8_t source_local_profile_team(std::int32_t committed) {
  if(committed == -1)return -1;
  if(committed < 0)return 0;
  return std::int8_t(committed > 16 ? 16 : committed);
}

enum class SourceProfileRepresentationPolicy { Retain, MultiplayerMapping };
struct SourcePlayerConfigurationContext {
  std::int32_t game_type{}, representation_count{};
  // Actual scenario player-start rows+2C, in original row order.
  std::span<const std::int16_t> scenario_representations;
  bool selected_engine{}, team_mode{};
  std::uint16_t team_mask{};
  // Retail14BC80 lacks the later multiplayer profile remap. This is selected
  // by the recovered source owner, not an editable gameplay setting.
  SourceProfileRepresentationPolicy representation_policy = SourceProfileRepresentationPolicy::Retain;
};
// Literal representation selection is shared by native player admission
// and loading the exact authored biped/hands definition. No cache fallback.
inline void source_sanitize_player_representation(SourcePlayerConfiguration &profile,
    const SourcePlayerConfigurationContext &context) {
  if(context.game_type==1) {
    std::int16_t selected=0;
    for(const auto value:context.scenario_representations)
      if(value!=-1){selected=value;break;}
    profile[0x7c]=1;profile[0x44]=std::uint8_t(selected);
  } else if(context.game_type==2 &&
      context.representation_policy==SourceProfileRepresentationPolicy::MultiplayerMapping) {
    const auto representation=std::bit_cast<std::int8_t>(profile[0x44]);
    profile[0x44]=(representation==1||representation==3) ? 3 : 2;
  }
  if(auto representation=std::bit_cast<std::int8_t>(profile[0x44]);representation!=-1) {
    if(representation<0)representation=0;
    else {
      const auto maximum=std::bit_cast<std::int32_t>(
          std::uint32_t(context.representation_count)-1);
      if(maximum<representation)representation=std::bit_cast<std::int8_t>(std::uint8_t(maximum));
    }
    profile[0x44]=std::uint8_t(representation);
  }
}
// Whole14BC80/14D3D0/14E7B0, including signed-byte/word clamping, NONE,
// original32-bit wrapping representation-count arithmetic and team admission.
void source_sanitize_player_configuration(SourcePlayerConfiguration&,
    const SourcePlayerConfigurationContext&);
} // namespace halo2
