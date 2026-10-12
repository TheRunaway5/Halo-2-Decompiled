#pragma once
#include "halo2/vista_pcr_screen.hpp"

namespace halo2 {
struct VistaPcrInitialFocus {
  unsigned pane{};
  std::uint32_t selected_row=0xffffffffu;
  bool operator==(const VistaPcrInitialFocus&) const = default;
};
// Supplied Vista40B94B late selection,40DCAB/4667A0 datum construction and
// 613FE5/6136CE/61383B first-child focus. Applies to the actual six-pane PC16/
// PC185 layout (six lists, each with sixteen original61FFC9 child widgets).
// Pane0 is teams; pane1 is summary. An empty selected bank has no datum row.
VistaPcrInitialFocus vista_pcr_initial_focus(const VistaPcrResults&);
// Actual616CFC→5AD782→616C85→5AD423/5C20C8. Lifecycle flag/manager state
// are the caller's actual820FC0/+4 values; the real live bank supplies the
// membership mask and eight-byte identity. Results are source player slots,
// not displayed row indices. Duplicate identities retain the first set slot.
std::uint32_t vista_pcr_session_identity_slot(std::span<const std::byte> session,
    bool lifecycle_enabled,std::uint32_t manager_state,
    const std::array<std::byte,8>& identity);
// Actual supplied DLL3829cf82... XHVCreateEngine ordinal5008 returns88780078
// unconditionally. Source5E5ABD therefore leaves its engine-available byte0;
// 5B4AFC/5B4AA1 return0. PC16 shows disabled frame2 for a real matching live
// identity and hides only an absent identity. PC185 always hides. This owner
// describes that supplied DLL's offline service; it makes no hardware claim.
VistaPcrVoicePresentation vista_pcr_offline_voice(const VistaPcrResults&,
    unsigned row,std::uint8_t lobby_variant,std::span<const std::byte> session,
    bool lifecycle_enabled,std::uint32_t manager_state);
} // namespace halo2
