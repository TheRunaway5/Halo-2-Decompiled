#include "halo2/vista_lobby_map_commit.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <string>
#include <utility>

namespace halo2 {
namespace {
template<class F, class... A> decltype(auto) call(const F& f, A&&... args) {
  if (!f) throw FormatError("Missing original Vista map callback service");
  return f(std::forward<A>(args)...);
}
std::span<const std::byte> row(const VistaLobbyMapCommitState& state,
    std::uint32_t handle, std::size_t stride) {
  const auto at = std::size_t(handle & 0xffffu) * stride;
  if (at > state.rows.size() || stride > state.rows.size() - at)
    throw FormatError("Vista map callback requires its genuine allocated row");
  return state.rows.subspan(at, stride);
}
std::uint32_t word(std::span<const std::byte> data, std::size_t at) {
  return std::to_integer<std::uint32_t>(data[at]) |
      (std::to_integer<std::uint32_t>(data[at + 1]) << 8) |
      (std::to_integer<std::uint32_t>(data[at + 2]) << 16) |
      (std::to_integer<std::uint32_t>(data[at + 3]) << 24);
}
void word(std::span<std::byte> data, std::size_t at, std::uint32_t value) {
  for (unsigned i = 0; i < 4; ++i) data[at + i] = std::byte(value >> (8 * i));
}
std::span<std::byte> edit_owner(const VistaLobbyMapCommitServices& s) {
  auto bank = call(s.edit_owner);
  if (bank.size() < 0x298) throw FormatError("Truncated original Vista map edit owner");
  return bank;
}
bool locked(const VistaLobbyMapCommitState& state, const VistaLobbyMapCommitServices& s) {
  if (!state.locked) return false;
  const auto argument = call(s.widget_v38);
  const auto channel = call(s.widget_v34);
  call(s.dialog, channel, argument, 0xb4u);
  return true;
}
void close(const VistaLobbyMapCommitServices& s) {
  const auto screen = call(s.root_screen);
  const auto argument0 = call(s.screen_v80, screen);
  const auto argument1 = call(s.screen_v84, screen);
  call(s.close, argument0, argument1);
}
std::u16string name(std::span<const std::byte> record) {
  std::u16string result;
  for (std::size_t i = 0; i + 1 < record.size(); i += 2) {
    const auto value = char16_t(std::to_integer<unsigned>(record[i]) |
        (std::to_integer<unsigned>(record[i + 1]) << 8));
    if (!value) return result;
    result.push_back(value);
  }
  throw FormatError("Vista custom map row requires its source UTF16 terminator");
}
} // namespace
void vista_lobby_standard_map_commit(const VistaLobbyMapCommitState& state,
    std::uint32_t handle, const VistaLobbyMapCommitServices& s) {
  if (handle == 0xffffffffu) return;
  const auto record = row(state, handle, 8);
  if (locked(state, s)) return;
  if (state.edit) {
    auto bank = edit_owner(s);
    bank[0xee] = std::byte{3};
    word(bank, 0xf4, word(record, 4));
    std::fill_n(bank.begin() + 0xf8, 0x60, std::byte{});
    const auto definition = call(s.definition, word(record, 4));
    const auto value = call(s.definition_name, definition);
    call(s.copy_name, bank.subspan(0x258, 64), value);
    call(s.publish_edit);
  } else {
    (void)call(s.select_map, 0xffffffffu, word(record, 4));
  }
  close(s);
}
void vista_lobby_custom_map_commit(const VistaLobbyMapCommitState& state,
    std::uint32_t handle, const VistaLobbyMapCommitServices& s) {
  if (handle == 0xffffffffu) return;
  const auto record = row(state, handle, 0x60);
  if (locked(state, s)) return;
  if (call(s.class_query)) {
    call(s.selection_context);
    (void)call(s.select_custom_context, 0u, record);
  } else {
    (void)call(s.select_custom_session, record);
  }
  if (state.edit) {
    auto bank = edit_owner(s);
    bank[0xee] = std::byte{4};
    word(bank, 0xf4, 0xffffffffu);
    std::copy(record.begin(), record.end(), bank.begin() + 0xf8);
    call(s.copy_name, bank.subspan(0x258, 64), name(record));
    call(s.publish_edit);
  }
  close(s);
}
void vista_lobby_standard_map_label(const VistaLobbyMapCommitState& state,
    std::uint32_t handle, const VistaLobbyMapCommitServices& s) {
  if (handle == 0xffffffffu) return;
  const auto record = row(state, handle, 8);
  const auto definition = call(s.definition, word(record, 4));
  if (const auto text = call(s.text, 0u)) {
    const auto value = call(s.definition_name, definition);
    call(s.string, text, value);
  }
}
} // namespace halo2
