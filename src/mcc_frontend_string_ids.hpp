#pragma once
#include "halo2/cache.hpp"
#include "halo2/frontend.hpp"
#include <algorithm>
#include <iterator>
#include <optional>
#include <string_view>

namespace halo2::detail {
struct MccFrontendStringIdentifier {
  std::uint32_t original_index, mcc_id;
  std::string_view name;
};
// Symbol names/IDs only; no localized artwork/text or reference-bank payload.
// Exact-name provenance: common-unique-v1/validation.json 0e47271a4562e960...
inline constexpr MccFrontendStringIdentifier mcc_frontend_string_identifiers[]{
#include "mcc_frontend_string_ids.inc"
};

inline bool mcc_frontend_content(const FrontendContent &content) {
  return content.source_fonts.filename() == "h2_fonts";
}

// Call only for compiled Xbox identifier constants. Authored MCC StringIDs
// retain their own namespace and continue through FrontendContent::localized.
inline std::optional<std::uint32_t> frontend_compiled_string_id(
    const FrontendContent &content, std::uint32_t original) {
  if (!mcc_frontend_content(content))
    return original;
  const auto index = StringId{original}.index();
  const auto found = std::lower_bound(std::begin(mcc_frontend_string_identifiers),
      std::end(mcc_frontend_string_identifiers), index,
      [](const auto &row, auto value) { return row.original_index < value; });
  if (found == std::end(mcc_frontend_string_identifiers) || found->original_index != index)
    return {};
  return found->mcc_id;
}

inline std::string frontend_compiled_localized(const FrontendContent &content,
    const UiScreen &screen, std::uint32_t original) {
  const auto id = frontend_compiled_string_id(content, original);
  return id ? content.localized(screen, *id) : std::string{};
}

inline void validate_mcc_frontend_identifiers(const Cache &cache) {
  if (!cache.is_mcc())
    throw FormatError("MCC frontend identifiers require their MCC cache owner");
  for (const auto &row : mcc_frontend_string_identifiers)
    if (cache.string_id(row.mcc_id) != row.name)
      throw FormatError("MCC frontend identifier namespace differs from its named profile");
}
} // namespace halo2::detail
