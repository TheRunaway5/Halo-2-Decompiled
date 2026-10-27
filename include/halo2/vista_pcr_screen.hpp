#pragma once
#include "halo2/vista_pcr_results.hpp"
#include "halo2/vista_loading.hpp"
#include <string_view>

namespace halo2 {
// Supplied Vista33119398...: the six actual+68 row callbacks. Widget/locale
// services resolve the caller's real child objects; no UI is allocated here.
enum class VistaPcrPane { teams,summary,numeric,opponents,medals,weapons };
enum class VistaPcrRowStyle { vista,original_xbox };
struct VistaPcrTeamColor {
  std::array<std::uint32_t,3> text,bitmap;
};
struct VistaPcrRowServices {
  std::function<bool(unsigned)> grid_child,text_child,bitmap_child;
  std::function<void(unsigned,std::u16string_view)> text;
  std::function<void(unsigned,bool)> visible;
  std::function<void(unsigned,const std::array<std::uint32_t,3>&)> text_color,bitmap_color;
  // Actual5B4DB6 bitmap RGB and475D34/475E44 adjusted text RGB. The source
  // deliberately uses different colors; neither is an inferred team palette.
  std::function<VistaPcrTeamColor(std::uint16_t)> team_color;
  // Actual40D0D7 appearance owner; source identity and local-player services
  // must be resolved there before a native emblem is rendered.
  std::function<void(unsigned)> appearance;
  std::function<std::uint32_t()> selected_row;
  std::function<std::u16string(std::uint32_t)> localized;
  // Actual40C85B/44BD7B formatting service. Format and signed argument words
  // are supplied literally; this interface does not invent locale strings.
  std::function<std::u16string(std::u16string_view,std::span<const std::int32_t>)> format;
};
//40D1DF/40D32E/40D52A/40D6B2/40D7DC/40DA4E. Output callback order, absent
// children, repeated selection queries, medal visibility, and512-unit append
// semantics are preserved. The Xbox projection uses TU1.5 23B211's256-unit
// medal buffer (its actual437544 literal is also one space). Other row fields use the equivalent
// semantic carrier fields. Missing consumed owners/invalid indices reject.
void vista_pcr_bind_row(VistaPcrPane,unsigned row,const VistaPcrResults&,
    const VistaPcrRowServices&,const VistaLoadingFloatingPoint&,
    VistaPcrRowStyle=VistaPcrRowStyle::vista);
// Actual40C1A7/40C214/40C36E/40C423/40C490/40C4FD source heading
// replacements. An absent value preserves that child's authored string.
std::array<std::optional<std::uint32_t>,5> vista_pcr_headings(VistaPcrPane,
    std::uint32_t variant);
// Actual40CE6A active-pane footer selection and44BCD5 append. The platform
// predicates cannot change the reached source indices2/3. Host/details are
// genuine6152B0/6156B9 owner values, not inferred from player appearance.
std::u16string vista_pcr_footer(VistaPcrPane,std::uint8_t lobby_variant,
    std::uint8_t host,std::uint8_t details_available,
    const std::function<std::u16string(std::uint32_t)>& localized);
struct VistaPcrVoicePresentation {bool visible{};unsigned frame{};};
struct VistaPcrVoiceServices {
  std::function<std::uint32_t(const std::array<std::byte,8>&)> identity_slot;
  std::function<std::uint8_t(std::uint32_t)> enabled,talking;
};
// Actual40C5C4/40C5DA outer bitmap0 owner. PC185 hides without any
// identity query. PC16 uses retained+100 XUID and real616CFC/5B4AFC/5B4AA1.
VistaPcrVoicePresentation vista_pcr_voice(const VistaPcrResults&,unsigned row,
    std::uint8_t lobby_variant,const VistaPcrVoiceServices&);

struct VistaPcrInputServices {
  std::function<std::uint8_t()> host,details_available;
  //6152B5 writes a caller-zeroed two-word output and returns its actual AL.
  std::function<std::uint8_t(std::array<std::uint32_t,2>&)> selected_identity;
  std::function<void(const std::array<std::uint32_t,2>&)> player_details;
  std::function<void(unsigned)> close;
  //6156D2 rechecks host before the actual5AD9E3 owner. Its return is unused.
  std::function<void()> continue_session;
  std::function<void(const std::array<std::uint32_t,8>&)> dialog;
};
//40C01A. Input codes stay literal until the actual device mapper calls here.
// The automatic PCR16 Done action and lobby-review185 close are distinct.
bool vista_pcr_input(std::uint8_t lobby_variant,std::uint32_t code,
    std::uint32_t controller,const VistaPcrInputServices&);

struct VistaPcrDrawServices {
  std::function<std::uint32_t(std::uint32_t)> elapsed;
  std::function<void(std::uint32_t)> black;
  std::function<void()> base_draw;
};
//40BD5F, including its signed wrapped elapsed comparison, SSE binary32
// arithmetic and x87 FISTP alpha packing. Actual2000ms opaque+2000ms fade
// applies only to PCR16; this is not the gameplay darkening producer.
void vista_pcr_draw(std::uint8_t lobby_variant,std::uint32_t creation_clock,
    const VistaPcrDrawServices&,const VistaLoadingFloatingPoint&);
} // namespace halo2
