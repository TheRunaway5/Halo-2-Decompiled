#include "halo2/vista_map_selector.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
template <class F> const F &owner(const F &f) {
  if (!f)
    throw FormatError("Original Vista map selector service owner is missing");
  return f;
}
void validate(VistaMapSelectorStorage s) {
  if (s.bytes.size() < vista_map_selector_bank_size || !s.source_identity ||
      std::uint64_t{s.source_identity} + vista_map_selector_bank_size >
          (std::uint64_t{1} << 32))
    throw FormatError(
        "Original Vista map selector storage is truncated, null or wraps");
}
void byte(VistaMapSelectorStorage s, std::size_t at, std::uint8_t v) {
  s.bytes[at] = std::byte{v};
}
void dword(VistaMapSelectorStorage s, std::size_t at, std::uint32_t v) {
  for (unsigned i = 0; i < 4; ++i)
    byte(s, at + i, std::uint8_t(v >> (i * 8)));
}
VistaMapSelectorStorage child(VistaMapSelectorStorage s, std::size_t at,
                              std::size_t size) {
  return {s.bytes.subspan(at, size), s.source_identity + std::uint32_t(at)};
}
std::uint32_t cvttss2si(float value) {
  // CVTTSS2SI returns the integer-indefinite bit pattern for invalid inputs.
  if (!std::isfinite(value) || double(value) >= 2147483648. ||
      double(value) < -2147483648.)
    return 0x80000000u;
  return std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(value));
}
std::int16_t add_word(std::int16_t v, std::uint32_t offset) {
  return std::bit_cast<std::int16_t>(std::uint16_t(std::uint16_t(v) + offset));
}
} // namespace

std::uint32_t
vista_map_selector_construct(VistaMapSelectorStorage storage,
                             const VistaMapSelectorArguments &a,
                             const VistaMapSelectorConstructionServices &s) {
  validate(storage);
  // 611159 calls the base first, writes the dual-list vtable, then the active
  // list.
  owner(s.screen)(storage, a.screen_id, a.argument1, a.argument2, a.controller);
  dword(storage, 0, 0x7cf3a4);
  dword(storage, 0xa5c, storage.source_identity + 0x1268);
  dword(storage, 0, 0x7d9194);
  owner(s.custom_list)(child(storage, 0xa60, 0x808), a.controller);
  owner(s.standard_list)(child(storage, 0x1268, 0x808), a.controller);
  byte(storage, 0x1a70, std::uint8_t(a.alternate));
  dword(storage, 0x1a74, 0);
  byte(storage, 0x1a78, 0);
  owner(s.text)(child(storage, 0x1a84, 0x4bc), a.controller);
  byte(storage, 0x1a6c, std::uint8_t(a.edit));
  byte(storage, 0x1264, std::uint8_t(a.edit));
  dword(storage, 0x1f40, 0);
  return storage.source_identity;
}

VistaMapSelectorStorage
vista_map_selector_factory(const VistaMapSelectorRequest &request,
                           bool alternate,
                           const VistaMapSelectorConstructionServices &s) {
  auto storage = owner(s.allocate)(0x1f44, 0);
  validate(storage);
  vista_map_selector_construct(storage,
                               {request.manager_kind, request.mask,
                                request.controller, alternate ? 17u : 206u,
                                alternate ? 1u : 0u, 0},
                               s);
  byte(storage, 0x6c, 1);
  if (!alternate && request.edit) {
    byte(storage, 0x1a6c, 1);
    byte(storage, 0x1264, 1);
  }
  owner(s.register_screen)(storage, request);
  return storage;
}

std::uint32_t
vista_map_selector_factory_identity(std::span<const std::byte> bank) {
  if (bank.size() <= 0x1a70)
    throw FormatError("Original Vista map selector identity bank is truncated");
  return bank[0x1a70] == std::byte{0} ? 0x64f819u : 0x64f8bfu;
}

bool vista_map_selector_focused(const VistaMapSelectorWidget &widget,
                                std::uint32_t argument,
                                const VistaMapSelectorFocusServices &services) {
  // 612152 only admits a parentless widget when its kind is the screen kind0.
  if (!widget.parent && widget.kind != 0)
    return false;
  const auto context38 = owner(services.v38)(widget);
  const auto context34 = owner(services.v34)(widget);
  for (auto *focused = owner(services.focused)(argument, context34, context38);
       focused; focused = focused->parent)
    if (focused == &widget)
      return true;
  return false;
}

std::uint32_t vista_map_selector_selected_handle(
    const VistaMapSelectorWidget &list,
    const VistaMapSelectorFocusServices &services) {
  for (auto *row = list.first_child; row; row = row->next)
    if (vista_map_selector_focused(*row, 0, services))
      return row->kind == 2 ? row->handle : 0xffffffffu;
  return 0xffffffffu;
}

const VistaMapSelectorWidget *
vista_map_selector_text(const VistaMapSelectorWidget &screen,
                        std::uint32_t ordinal) {
  const auto low = std::uint16_t(ordinal);
  if (low & 0x8000u)
    return nullptr;
  auto remaining = low;
  for (auto *widget = screen.first_child; widget; widget = widget->next)
    if (widget->kind == 6 && remaining-- == 0)
      return widget;
  return nullptr;
}

std::array<std::int16_t, 4>
vista_map_selector_bounds(const VistaMapSelectorWidget &widget) {
  auto bounds = widget.bounds;
  const auto x = cvttss2si(widget.animation_x),
             y = cvttss2si(widget.animation_y);
  bounds[1] = add_word(bounds[1], x);
  bounds[3] = add_word(bounds[3], x);
  bounds[0] = add_word(bounds[0], y);
  bounds[2] = add_word(bounds[2], y);
  return bounds;
}

bool vista_map_selector_hit(const VistaMapSelectorWidget &widget,
                            std::array<std::int16_t, 2> point) {
  auto b = vista_map_selector_bounds(widget);
  if (b[1] > b[3])
    std::swap(b[1], b[3]);
  if (b[0] > b[2])
    std::swap(b[0], b[2]);
  return point[0] >= b[1] && point[0] < b[3] && point[1] >= b[0] &&
         point[1] < b[2];
}

std::uint8_t
vista_map_selector_mouse_tabs(VistaMapSelectorInputState &state,
                              std::uint32_t &event,
                              const VistaMapSelectorInputServices &services) {
  const auto point = owner(services.mouse)();
  if (state.next_tab && vista_map_selector_hit(*state.next_tab, point))
    event = 2;
  else if (state.previous_tab &&
           vista_map_selector_hit(*state.previous_tab, point))
    event = 4;
  else
    return 0;
  (void)vista_map_selector_input(state, event, services);
  return 1;
}

std::uint8_t
vista_map_selector_input(VistaMapSelectorInputState &state,
                         std::uint32_t &event,
                         const VistaMapSelectorInputServices &services) {
  if (event == 6 && vista_map_selector_mouse_tabs(state, event, services))
    return 1;
  bool handled = false;
  if (event == 2) {
    ++state.tab;
    handled = true;
  } else if (event == 4) {
    --state.tab;
    handled = true;
  }
  if (handled)
    state.changed = 1;
  // Source only maps negative DWORDs to1 and exact2 to0. Values3..7FFFFFFF
  // survive; a generic modulo/clamp would alter the original owner.
  if (state.tab & 0x80000000u)
    state.tab = 1;
  else if (state.tab == 2)
    state.tab = 0;
  return handled ? std::uint8_t{1} : owner(services.base_input)(event);
}
} // namespace halo2
