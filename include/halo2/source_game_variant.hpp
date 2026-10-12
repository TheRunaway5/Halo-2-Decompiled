#pragma once
#include <array>
#include <cstdint>
#include <initializer_list>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace halo2 {
struct UiVariant;
// Tag rows apply setters in authored order, including repeated IDs/aliases.
// Editing is a later write. Replacing an earlier write to that same ID is
// safe because its fixed descriptor writes exactly the same field/bits.
class GameVariantSettings {
public:
  using Write = std::pair<unsigned, int>;
  GameVariantSettings() = default;
  GameVariantSettings(std::initializer_list<Write> writes) : writes_(writes) {}
  // Explicit legacy map input has only its key order available.
  GameVariantSettings(const std::map<unsigned, int> &values)
      : writes_(values.begin(), values.end()) {}
  void append(unsigned parameter, int value) { writes_.emplace_back(parameter, value); }
  void set(unsigned parameter, int value);
  auto begin() const { return writes_.begin(); }
  auto end() const { return writes_.end(); }
  std::size_t size() const { return writes_.size(); }
  bool empty() const { return writes_.empty(); }
  bool operator==(const GameVariantSettings &) const = default;
private:
  std::vector<Write> writes_;
};
// Literal304-byte source settings buffer. Localized UTF16 title production is
// separate; the initializer's no-locale branch leaves that title empty.
struct SourceGameVariant {
  std::array<std::uint8_t, 0x130> bytes{};
};
struct SourceGameVariantDescriptor {
  int kind = -1;
  unsigned offset{};
  std::uint32_t operand{};
};
std::span<const SourceGameVariantDescriptor, 112> source_game_variant_descriptors();
// Original19D220 / TU1A0A90 frontend enum0..6 -> source types2,3,7,4,1,9,8.
SourceGameVariant source_game_variant_default(unsigned frontend_engine,
                                               std::u16string_view title = {});
// Source UTF16 name at+4, bounded to31 code units with word+42 always NUL.
// The caller supplies the localized tag result; no English title is invented.
void source_game_variant_set_title(SourceGameVariant &, std::u16string_view);
std::u16string source_game_variant_title(const SourceGameVariant &);
std::int32_t source_game_variant_type(const SourceGameVariant &);
// Original2373BE / TU2394EA. Missing descriptors (including out-of-range
// parameter IDs) are no-ops. Byte/short fields retain original narrowing.
void source_game_variant_set(SourceGameVariant &, std::int32_t parameter,
                             std::int32_t value);
// Original2374F0 / TU23961C. Aliases read the same source field.
std::int32_t source_game_variant_get(const SourceGameVariant &, std::int32_t parameter);
// Original19D650 / TU1A0EC0. Returns true only when the complete input was
// already valid. Any correction replaces all304 bytes with default Slayer,
// using the caller's localized reset title (empty models absent globals).
bool source_game_variant_normalize(SourceGameVariant &,
                                   std::u16string_view slayer_title = {});
// Original19D620 / TU1A0E90 checks a copy and never alters the caller.
bool source_game_variant_valid(const SourceGameVariant &);
// Applies the caller's ordered overrides without normalization, matching the
// raw descriptor writer. Admission/load consumers explicitly validate it.
SourceGameVariant source_game_variant(const UiVariant &);
} // namespace halo2
