#pragma once
#include <string>
namespace halo2 {
// Presentation-only substitution. Keep original tag strings/assets intact.
inline std::string frontend_online_text(std::string text) {
  for (const auto *old : {"Xbox Live", "Xbox LIVE", "XBOX LIVE", "xbox live"}) {
    std::size_t at = 0;
    while ((at = text.find(old, at)) != std::string::npos) {
      const std::string replacement = "ONLINE";
      text.replace(at, std::char_traits<char>::length(old), replacement);
      at += replacement.size();
    }
  }
  return text;
}
} // namespace halo2
