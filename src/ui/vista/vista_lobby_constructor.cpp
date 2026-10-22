#include "halo2/vista_lobby_constructor.hpp"
#include "halo2/cache.hpp"
#include <algorithm>

namespace halo2 {
namespace {
// Byte stores intentionally avoid alignment, host-endian and native-pointer
// assumptions. Their order follows the original constructors.
class Constructor {
 public:
  Constructor(std::span<std::byte> bank, std::uint32_t base,
      VistaLobbyConstructorGlobals& globals,
      const std::function<std::uint32_t()>& clock)
      : bank_(bank), base_(base), globals_(globals), clock_(clock) {}

  void byte(std::size_t offset, std::uint8_t value) {
    bank_[offset] = std::byte{value};
  }
  void word(std::size_t offset, std::uint16_t value) {
    for (unsigned i = 0; i < 2; ++i) byte(offset + i, std::uint8_t(value >> (8 * i)));
  }
  void dword(std::size_t offset, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) byte(offset + i, std::uint8_t(value >> (8 * i)));
  }

  void animation(std::size_t offset) { // 6115FE, descriptor supplied by 611D81.
    const auto now = clock_();
    dword(offset + 0x34, 0xffffffff);
    dword(offset + 0x38, 0);
    word(offset + 0x3c, 0);
    word(offset + 0x3e, 0);
    word(offset + 0x40, 1);
    word(offset + 0x42, 0);
    dword(offset + 0x44, 0);
    dword(offset + 0x4c, now);
    dword(offset + 0x48, 0);
    dword(offset + 0x50, now);
    dword(offset + 0x60, 0);
    dword(offset + 0x5c, 0);
    dword(offset + 0x58, 0);
    dword(offset + 0x54, 0);
    dword(offset + 0x64, 0x3f800000);
  }

  void widget(std::size_t offset, std::uint32_t kind, std::uint32_t controller) { // 611D81.
    dword(offset + 4, kind);
    word(offset + 8, std::uint16_t(controller));
    dword(offset, 0x7cf44c);
    word(offset + 0xa, 0xffff);
    dword(offset + 0xc, 0xffffffff);
    for (auto field : {0x10, 0x14, 0x18, 0x1c}) dword(offset + field, 0);
    word(offset + 0x68, 0xffff);
    word(offset + 0x6a, 0);
    byte(offset + 0x6c, 0);
    byte(offset + 0x6d, 1);
    byte(offset + 0x6e, 1);
    byte(offset + 0x6f, 1);
    dword(offset + 0x20, 0);
    dword(offset + 0x24, 0);
    for (auto field : {0x30, 0x2c, 0x28}) dword(offset + field, 0x3f800000);
    animation(offset);
  }

  void text(std::size_t offset, std::uint32_t controller) { // 61BA07.
    widget(offset, 6, controller);
    dword(offset + 0x70, 0);
    byte(offset + 0x74, 0);
    dword(offset, 0x7cfd7c);
  }

  void string(std::size_t offset) { // 62DEFA; buffer and padding are not cleared.
    dword(offset, 0x7d24ec);
    dword(offset + 4, 0);
    word(offset + 0x14, 0);
    word(offset + 0x16, 1);
    dword(offset + 0x18, 0xffffffff);
    dword(offset + 0x1c, 2);
    dword(offset + 0x20, 0x3f800000);
    dword(offset + 0x24, 0xffffffff);
    dword(offset + 0x38, 0);
    word(offset + 0x3c, 0xffff);
    word(offset + 0x3e, 0);
    dword(offset + 0x40, 0);
    for (auto field : {0x10, 0xc, 8}) dword(offset + field, 0x3f800000);
  }

  void header_footer(std::size_t offset) { // 60F65D, both constructed with mask zero.
    text(offset, 0);
    dword(offset, 0x7cf29c);
    string(offset + 0x78);
    dword(offset + 0x78, 0x7cf22c);
    word(offset + 0xbc, 0);
  }

  void callback_node(std::size_t offset) { // 6113C6.
    dword(offset + 4, 0);
    dword(offset, 0);
    dword(offset + 8, 0);
  }

  void screen(const VistaLobbyConstructorArguments& arguments) { // 6106A2.
    widget(0, 0, arguments.controller_mask);
    dword(0x74, arguments.argument1);
    dword(0, 0x7cf2f4);
    dword(0x70, 14);
    dword(0x78, arguments.argument2);
    dword(0x7c, 0xffffffff);
    header_footer(0x80);
    header_footer(0x53c);
    word(0x9f8, 0xffff);
    for (auto field : {0x9fa, 0x9fb, 0x9fc, 0x9fd}) byte(field, 0);
    callback_node(0xa48);
    dword(0xa44, 0x7cf39c);
    dword(0xa54, base_);
    dword(0xa58, 0x610227);
    globals_.last_screen_id = 14; // 60B351.
    dword(0x7c, 0); // Source adds one to its previously written FFFFFFFF.
    dword(4, 0);
    byte(0x6d, 1);
    dword(0xc, 0);
    std::copy(globals_.screen_bounds.begin(), globals_.screen_bounds.end(), bank_.begin() + 0x20); // 6116D2.
    std::fill_n(bank_.begin() + 0xa04, 0x40, std::byte{0});
    dword(0x9fe, 0); // Original unaligned DWORD; A02/A03 remain untouched.
  }

  void button(std::size_t offset, std::uint16_t ordinal, std::uint32_t controller) { // 63F756.
    widget(offset, 3, controller);
    dword(offset, 0x7d39a4);
    string(offset + 0x70);
    dword(offset + 0x70, 0x7cf218);
    word(offset + 0xb4, 0);
    word(offset + 0xf8, ordinal);
    dword(offset + 0xf4, 0);
    dword(offset + 0xfc, 0); // 611442 callback head.
  }

  void chat(std::uint32_t controller) { // 6320C0.
    constexpr std::size_t offset = 0xd5c;
    button(offset, 4, controller);
    dword(offset, 0x7d25b4);
    string(offset + 0x100);
    dword(offset + 0x100, 0x7cf22c);
    word(offset + 0x144, 0);
    dword(offset + 0x544, 0);
    ++globals_.chat_owner_count;
  }

  void clone(std::uint32_t controller) { // 643E88.
    constexpr std::size_t offset = 0x13ac;
    text(offset, controller);
    dword(offset, 0x7d3ec4);
    string(offset + 0x78);
    dword(offset + 0x78, 0x7d3eb0);
    word(offset + 0xbc, 0);
  }

  void lobby(const VistaLobbyConstructorArguments& arguments) { // 6463AB.
    screen(arguments);
    dword(0, 0x7d3fcc);
    button(0xa5c, 0, arguments.controller_mask);
    button(0xb5c, 1, arguments.controller_mask);
    button(0xc5c, 2, arguments.controller_mask);
    chat(arguments.controller_mask);
    dword(0xd5c, 0x7d3e4c);
    button(0x12a4, 3, arguments.controller_mask);
    byte(0x13a4, 1);
    dword(0x13a8, base_ + 0xa5c);
    clone(arguments.controller_mask);
    for (auto field : {0x1c68, 0x1c69, 0x1c6a, 0x1c6c, 0x1c70, 0x1c74, 0x1c78}) byte(field, 0);
    for (auto field : {0x1c6e, 0x1c72, 0x1c76, 0x1c7a}) word(field, 0xffff);
    for (auto field : {0x1c7c, 0x207c, 0x247c, 0x287c, 0x2c7c, 0x307c}) word(field, 0);
    dword(0x347c, 0xffffffff);
    dword(0x3480, 0);
    word(0x3484, 0);
    byte(0x3486, 0);
    for (auto offset : {0x3488, 0x34a0, 0x34b8, 0x34d0, 0x34e8}) {
      callback_node(offset + 4);
      dword(offset, 0x7d3f1c);
      dword(offset + 0x10, base_);
      dword(offset + 0x14, 0x64620f);
    }
    byte(0x3500, 0);
    dword(0x3504, 0);
    byte(0x3508, 0);
    dword(0x350c, 0xffffffff);
    dword(0x3510, 0);
  }

 private:
  std::span<std::byte> bank_;
  std::uint32_t base_;
  VistaLobbyConstructorGlobals& globals_;
  const std::function<std::uint32_t()>& clock_;
};
} // namespace

std::uint32_t vista_lobby_construct(std::span<std::byte> bank,
    std::uint32_t source_base, const VistaLobbyConstructorArguments& arguments,
    VistaLobbyConstructorGlobals& globals,
    const std::function<std::uint32_t()>& read_cached_ui_clock) {
  if (bank.size() < vista_lobby_constructor_bank_size)
    throw FormatError("Original Vista lobby constructor bank is truncated");
  if (!source_base || std::uint64_t{source_base} + vista_lobby_constructor_bank_size > (std::uint64_t{1} << 32))
    throw FormatError("Original Vista lobby source bank identity wraps or is null");
  if (!read_cached_ui_clock)
    throw FormatError("Original Vista lobby cached UI clock owner is missing");
  Constructor(bank.first(vista_lobby_constructor_bank_size), source_base, globals, read_cached_ui_clock).lobby(arguments);
  return source_base;
}
} // namespace halo2
