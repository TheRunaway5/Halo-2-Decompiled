#pragma once
#include "ui/mcc_frontend_string_ids.hpp"

namespace halo2::detail {
// Exact unique symbol-name join of PC build 11081 and MCC mainmenu namespaces.
// Names and IDs only: the imported MCC bank owns every localized string.
// Offline provenance: pc-unique-v1/validation.json (7,669 nonempty symbols).
inline constexpr MccFrontendStringIdentifier mcc_pc_frontend_string_identifiers[]{
#include "ui/mcc_pc_frontend_string_ids.inc"
};
inline const FrontendContent &frontend_pc_identifier_owner(const FrontendContent &content) {
  return content.vista ? *content.vista : content;
}

// Compiled PC constants only. Authored MCC widget IDs are never aliases.
inline std::optional<std::uint32_t> frontend_pc_compiled_string_id(
    const FrontendContent &content, std::uint32_t original) {
  const auto &owner=frontend_pc_identifier_owner(content);
  if (!mcc_frontend_content(owner)) return original;
  const auto index = StringId{original}.index();
  if(owner.pc_string_ids) {
    const auto found=owner.pc_string_ids->find(index);
    if(found==owner.pc_string_ids->end())return {};
    return found->second;
  }
  const auto found = std::lower_bound(std::begin(mcc_pc_frontend_string_identifiers),
      std::end(mcc_pc_frontend_string_identifiers), index,
      [](const auto &row, auto value) { return row.original_index < value; });
  if (found == std::end(mcc_pc_frontend_string_identifiers) || found->original_index != index)
    return {};
  return found->mcc_id;
}

inline std::string frontend_pc_compiled_localized(const FrontendContent &content,
    const UiScreen &screen, std::uint32_t original) {
  const auto id = frontend_pc_compiled_string_id(content, original);
  return id ? frontend_pc_identifier_owner(content).localized(screen, *id) : std::string{};
}

inline std::string frontend_pc_compiled_low_localized(const FrontendContent &content,
    const UiScreen &screen, std::uint32_t original_index) {
  const auto &owner=frontend_pc_identifier_owner(content);
  if (mcc_frontend_content(owner)) {
    const auto id = frontend_pc_compiled_string_id(content, original_index);
    if (id) {
      const auto value = owner.localized(screen, *id);
      if (!value.empty()) return value;
    }
  } else {
    for (const auto &[id, text] : screen.strings)
      if ((id & 0xffffu) == original_index) return text;
  }
  throw FormatError("missing source PC settings label: " + std::to_string(original_index));
}

inline bool frontend_pc_compiled_matches(const FrontendContent &content,
    std::uint32_t authored, std::uint32_t original) {
  const auto id = frontend_pc_compiled_string_id(content, original);
  return id && authored == *id;
}

inline bool frontend_pc_compiled_low_matches(const FrontendContent &content,
    std::uint32_t authored, std::uint32_t original_index) {
  if (!mcc_frontend_content(frontend_pc_identifier_owner(content))) return (authored & 0xffffu) == original_index;
  return frontend_pc_compiled_matches(content, authored, original_index);
}

inline void validate_mcc_pc_frontend_identifiers(const Cache &cache) {
  if (!cache.is_mcc())
    throw FormatError("MCC PC frontend identifiers require their MCC cache owner");
  for (const auto &row : mcc_pc_frontend_string_identifiers)
    if (cache.string_id(row.mcc_id) != row.name)
      throw FormatError("MCC PC frontend identifier namespace differs from its named profile");
}
} // namespace halo2::detail
