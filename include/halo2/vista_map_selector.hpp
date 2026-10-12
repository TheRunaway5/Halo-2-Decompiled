#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
inline constexpr std::size_t vista_map_selector_bank_size = 0x1f44;
struct VistaMapSelectorStorage {
  std::span<std::byte> bytes;
  std::uint32_t source_identity{};
};
struct VistaMapSelectorRequest {
  std::uint16_t flags{}, controller{};
  std::uint32_t manager_kind{}, mask{}, edit{};
  std::uint32_t source_identity{};
};
struct VistaMapSelectorArguments {
  std::uint32_t argument1{}, argument2{}, controller{}, screen_id{},
      alternate{}, edit{};
};
struct VistaMapSelectorConstructionServices {
  std::function<VistaMapSelectorStorage(std::uint32_t, std::uint32_t)>
      allocate; // 60D2D8.
  // Complete external owners, not replaced by successful/null defaults.
  std::function<void(VistaMapSelectorStorage, std::uint32_t, std::uint32_t,
                     std::uint32_t, std::uint32_t)>
      screen; // 6106A2(screenID,arg1,arg2,controller).
  std::function<void(VistaMapSelectorStorage, std::uint32_t)>
      custom_list; // 65AE3B.
  std::function<void(VistaMapSelectorStorage, std::uint32_t)>
      standard_list;                                                // 65A95E.
  std::function<void(VistaMapSelectorStorage, std::uint32_t)> text; // 60F65D.
  std::function<void(VistaMapSelectorStorage, const VistaMapSelectorRequest &)>
      register_screen; // 60B8C3.
};
// Complete64F6A8/611159 stores and call order. Generic base/list/text owners
// remain explicit services. Source DWORD identities never become host pointers.
std::uint32_t
vista_map_selector_construct(VistaMapSelectorStorage,
                             const VistaMapSelectorArguments &,
                             const VistaMapSelectorConstructionServices &);
// Complete admitted non-null64F819/64F8BF factory paths. The retail allocation
// failure dereferences null; this boundary rejects it rather than inventing UI.
VistaMapSelectorStorage
vista_map_selector_factory(const VistaMapSelectorRequest &, bool alternate,
                           const VistaMapSelectorConstructionServices &);
std::uint32_t vista_map_selector_factory_identity(std::span<const std::byte>);

// The retained graph is supplied by the actual generic widget owner. These
// methods do not create children, infer authored ordinals or own activation.
struct VistaMapSelectorWidget {
  std::uint32_t source_identity{}, kind{}, handle{0xffffffff};
  VistaMapSelectorWidget *parent{}, *first_child{}, *next{};
  std::array<std::int16_t, 4> bounds{}; // top,left,bottom,right, widget+20.
  float animation_x{}, animation_y{};   // widget+58/+5C, consumed by611703.
};
struct VistaMapSelectorFocusServices {
  std::function<std::uint32_t(const VistaMapSelectorWidget &)> v34, v38;
  std::function<const VistaMapSelectorWidget *(std::uint32_t, std::uint32_t,
                                               std::uint32_t)>
      focused; // 609EFD(argument,v34,v38).
};
bool vista_map_selector_focused(
    const VistaMapSelectorWidget &, std::uint32_t argument,
    const VistaMapSelectorFocusServices &); // 612152.
std::uint32_t vista_map_selector_selected_handle(
    const VistaMapSelectorWidget &list,
    const VistaMapSelectorFocusServices
        &); // 61383B: first focused direct child.
const VistaMapSelectorWidget *
vista_map_selector_text(const VistaMapSelectorWidget &,
                        std::uint32_t ordinal); // 611973: signed low-WORD,
                                                // direct kind6 children only.

struct VistaMapSelectorInputState {
  std::uint32_t tab{};    // screen+1A74; arithmetic is DWORD, not a bool.
  std::uint8_t changed{}; // screen+1A78.
  const VistaMapSelectorWidget *next_tab{}, *previous_tab{}; // screen+A04/+A08.
};
struct VistaMapSelectorInputServices {
  std::function<std::array<std::int16_t, 2>()> mouse;      // 61D620: x,y.
  std::function<std::uint8_t(std::uint32_t &)> base_input; // 60EEBE.
};
std::array<std::int16_t, 4>
vista_map_selector_bounds(const VistaMapSelectorWidget &); // 611703/47708E.
bool vista_map_selector_hit(
    const VistaMapSelectorWidget &,
    std::array<std::int16_t, 2>
        point); // 611578/4770AC, exclusive right/bottom.
std::uint8_t
vista_map_selector_mouse_tabs(VistaMapSelectorInputState &,
                              std::uint32_t &event,
                              const VistaMapSelectorInputServices &); // 65A566.
std::uint8_t
vista_map_selector_input(VistaMapSelectorInputState &, std::uint32_t &event,
                         const VistaMapSelectorInputServices &); // 65A4C0.
} // namespace halo2
