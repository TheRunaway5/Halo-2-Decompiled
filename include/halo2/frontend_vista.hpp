#pragma once
#include <filesystem>
#include <memory>

namespace halo2 {
struct FrontendContent;
// Source-qualified navigation IDs; the low word remains the authored PC ID.
constexpr unsigned vista_menu(unsigned id) { return 0x10000u | id; }
constexpr bool is_vista_menu(unsigned id) {
  return (id & 0xffff0000u) == 0x10000u;
}
std::shared_ptr<FrontendContent>
load_vista_settings(const std::filesystem::path &map);
// Gameplay UI is a separate source bank (the supplied solo cache contains
// the shared pause/settings definitions). Never merge its datums into shell UI.
std::shared_ptr<FrontendContent>
load_vista_pause(const std::filesystem::path &map);
constexpr unsigned vista_settings_kind(unsigned screen) {
  switch (screen & 0xffffu) {
  case 260: return 258;
  case 261: return 256;
  case 262: return 257;
  case 244: return 243;
  case 254: return 253;
  default: return screen & 0xffffu;
  }
}
constexpr bool is_vista_settings(unsigned screen, unsigned kind) {
  return is_vista_menu(screen) && vista_settings_kind(screen)==kind;
}
} // namespace halo2
