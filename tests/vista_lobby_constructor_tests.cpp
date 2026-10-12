#include "halo2/vista_lobby_constructor.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

using namespace halo2;
using Json = nlohmann::json;
namespace {
std::vector<std::byte> unhex(std::string_view text) {
  if (text.size() % 2) throw FormatError("Odd original bank hex length");
  const auto digit = [](char c) -> unsigned {
    if (c >= '0' && c <= '9') return unsigned(c - '0');
    if (c >= 'a' && c <= 'f') return unsigned(c - 'a') + 10;
    throw FormatError("Invalid original bank hex digit");
  };
  std::vector<std::byte> out(text.size() / 2);
  for (std::size_t i = 0; i < out.size(); ++i)
    out[i] = std::byte{std::uint8_t(16 * digit(text[2 * i]) + digit(text[2 * i + 1]))};
  return out;
}

void compare(std::span<const std::byte> actual, const Json& hex, std::size_t row, std::string_view phase) {
  const auto expected = unhex(hex.get<std::string>());
  if (expected.size() != actual.size()) throw FormatError("Original constructor bank length mismatch");
  const auto mismatch = std::mismatch(actual.begin(), actual.end(), expected.begin());
  if (mismatch.first != actual.end()) {
    std::cerr << "constructor case " << row << ' ' << phase << " offset 0x" << std::hex
              << (mismatch.first - actual.begin()) << " actual=" << std::to_integer<unsigned>(*mismatch.first)
              << " original=" << std::to_integer<unsigned>(*mismatch.second) << std::dec << '\n';
    throw FormatError("Original Vista constructor byte mismatch");
  }
}

template<class F> void rejects(F&& operation) {
  try { operation(); } catch (const FormatError&) { return; }
  throw FormatError("Invalid original constructor boundary was admitted");
}
} // namespace

int main(int argc, char** argv) try {
  if (argc != 2) return 77;
  std::ifstream input(argv[1]);
  if (!input) return 77;
  Json rows;
  input >> rows;
  std::size_t count{}, checkpoints{};
  for (const auto& row : rows) {
    const auto& q = row.at("input");
    const auto pattern = std::byte{q.at("pattern").get<std::uint8_t>()};
    std::vector<std::byte> storage(vista_lobby_constructor_bank_size + 17, pattern);
    const std::span bank{storage.data(), vista_lobby_constructor_bank_size};
    const auto original_bounds = unhex(q.at("screen_bounds_hex").get<std::string>());
    if (original_bounds.size() != 8) throw FormatError("Original bounds length mismatch");
    VistaLobbyConstructorGlobals globals{q.at("chat_owner_count"), q.at("last_screen_id"), {}};
    std::copy(original_bounds.begin(), original_bounds.end(), globals.screen_bounds.begin());
    VistaLobbyConstructorArguments arguments{q.at("arguments").at(0), q.at("arguments").at(1), q.at("arguments").at(2)};
    std::size_t clock{};
    const auto read_clock = [&]() -> std::uint32_t {
      const auto& expected = row.at("checkpoints").at(clock);
      compare(bank, expected.at("bank_hex"), count, "before cached clock");
      if (globals.chat_owner_count != expected.at("chat_owner_count") ||
          globals.last_screen_id != expected.at("last_screen_id"))
        throw FormatError("Original constructor global/clock order mismatch");
      ++checkpoints;
      const auto& clocks = q.at("clocks");
      return clocks.at(clock++ % clocks.size()).get<std::uint32_t>();
    };
    const auto base = q.at("source_base").get<std::uint32_t>();
    if (vista_lobby_construct(storage, base, arguments, globals, read_clock) != base)
      throw FormatError("Original constructor source identity return mismatch");
    if (clock != 9 || globals.chat_owner_count != row.at("chat_owner_count") ||
        globals.last_screen_id != row.at("last_screen_id") ||
        !std::equal(globals.screen_bounds.begin(), globals.screen_bounds.end(), original_bounds.begin()))
      throw FormatError("Original constructor final globals or independent clock count mismatch");
    compare(bank, row.at("bank_hex"), count, "final");
    std::vector<bool> owned(bank.size());
    std::size_t owned_count{};
    for (const auto& range : row.at("owned_ranges_half_open")) {
      for (auto i = range.at(0).get<std::size_t>(); i < range.at(1).get<std::size_t>(); ++i) {
        if (i >= owned.size()) throw FormatError("Original owned range exceeds bank");
        if (!owned[i]) ++owned_count;
        owned[i] = true;
      }
    }
    if (owned_count != 1854 || owned_count != row.at("owned_bytes"))
      throw FormatError("Original constructor ownership mask mismatch");
    for (std::size_t i = 0; i < bank.size(); ++i)
      if (!owned[i] && bank[i] != pattern) throw FormatError("Constructor modified an unowned padding byte");
    if (!std::all_of(storage.begin() + bank.size(), storage.end(), [&](auto b) { return b == pattern; }))
      throw FormatError("Constructor modified caller storage beyond source allocation");
    ++count;
  }
  std::vector<std::byte> bank(vista_lobby_constructor_bank_size, std::byte{0xa5});
  const auto before = bank;
  VistaLobbyConstructorGlobals globals{17, 31, {}};
  const auto clock = [] { return std::uint32_t{7}; };
  rejects([&] { vista_lobby_construct(std::span(bank).first(bank.size() - 1), 0x1a00000, {}, globals, clock); });
  rejects([&] { vista_lobby_construct(bank, 0, {}, globals, clock); });
  rejects([&] { vista_lobby_construct(bank, 0xffffca6d, {}, globals, clock); });
  rejects([&] { vista_lobby_construct(bank, 0x1a00000, {}, globals, {}); });
  if (bank != before || globals.chat_owner_count != 17 || globals.last_screen_id != 31)
    throw FormatError("Rejected constructor input changed owned state");
  std::cout << "PASS " << count << " original Vista constructor cases, " << checkpoints
            << " exact pre-clock banks, final globals, source identities and untouched padding\n";
  return 0;
} catch (const std::exception& error) {
  std::cerr << error.what() << '\n';
  return 1;
}
