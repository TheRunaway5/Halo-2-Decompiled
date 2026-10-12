#pragma once
#include "halo2/vista_pcr_results.hpp"
#include "halo2/vista_loading.hpp"

namespace halo2 {
using VistaPcrRgb=std::array<std::uint32_t,3>;
struct VistaPcrRankRange {std::int16_t first{},last{};};
struct VistaPcrAppearanceAssets {
  // Actual 409A3F global guard and matg+160 appearance palette.
  bool globals_present{};
  std::span<const VistaPcrRgb> appearance_palette;
  // Actual matg+16C/mulg+10 team palette. Absence uses literal g8127B4.
  std::span<const VistaPcrRgb> team_palette;
  // Actual 60BB1D/63967A wigl rank owner and +158 signed ranges.
  bool rank_owner_present{};
  std::span<const VistaPcrRankRange> rank_ranges;
  // Actual imported PC skin bitmap bindings, consumed by 61C300.
  bool primary_bitmap_present{},rank_bitmap_present{};
  std::uint32_t primary_page_count{},rank_page_count{};
  std::uint16_t previous_primary_frame{},previous_rank_frame{};
  std::uint32_t emblem_definition_flags{};
};
class VistaPcrAppearanceServices {
 public:
  virtual ~VistaPcrAppearanceServices()=default;
  virtual bool identity_query(std::array<std::byte,8>)=0; // 6156CD ->5B176B
  virtual std::uint32_t primary_player_query(std::array<std::byte,8>)=0; // 616D34
  virtual bool player_query(std::uint32_t)=0; // 615820; result unused
};
struct VistaPcrAppearanceView {
  std::uint32_t descriptor_flags{};
  std::array<std::byte,16> appearance{};
  std::u16string name;
  std::uint16_t team=0xffff,rank_range=0xffff;
  std::uint8_t no_team{};
  VistaPcrRgb primary_color{},name_color{};
  bool primary_visible{},secondary_visible{},rank_visible{},emblem_visible{};
  std::array<std::uint8_t,16> group_visible{};
  std::uint16_t primary_frame{},rank_frame{};
  // Literal PC 620652 child definition keys; Xbox keys differ.
  static constexpr std::uint32_t name_placeholder=0x0b000696;
  static constexpr std::uint32_t primary_placeholder=0x14000698;
  static constexpr std::uint32_t secondary_placeholder=0x0e000699;
  static constexpr std::uint32_t rank_placeholder=0x0900069a;
};
// Actual 40D0D7 descriptor producer ->611CD0 group0 ->620652 PC skin and
// 640F96 emblem owner. Source group0 is shown and remaining15 groups hidden.
// Palette/rank/bitmap bindings come from actual imported assets. This view is
// for that complete PC skin; missing arbitrary widget definitions are not
// synthesized. Identity queries must bind genuine session owner results.
VistaPcrAppearanceView vista_pcr_player_presentation(const VistaPcrResults&,
    unsigned row,const VistaPcrAppearanceAssets&,VistaPcrAppearanceServices&,
    const VistaLoadingFloatingPoint&);
// Actual 5B4E48 four-color owner, with literal 409A3F/5B4DB6 fallback rules.
std::array<VistaPcrRgb,4> vista_pcr_appearance_colors(
    std::array<std::byte,16>,std::uint16_t team,std::uint8_t no_team,
    const VistaPcrAppearanceAssets&);
// Actual 475D34 ->literal S/V replacement ->475E44 name color.
VistaPcrRgb vista_pcr_profile_name_color(VistaPcrRgb,
    const VistaLoadingFloatingPoint&);
} // namespace halo2
