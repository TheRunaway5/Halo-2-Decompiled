#pragma once
#include <compare>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace halo2 {
struct FrontendContent;
struct UiScreen;

// Identity is source-qualified. PC Settings19 and Xbox Settings19 are distinct
// definitions; a source-local tag datum must never be resolved in the other map.
enum class FrontendMenuSource { original_xbox, vista };
struct FrontendMenuKey {
  FrontendMenuSource source{};
  std::uint16_t id{};
  auto operator<=>(const FrontendMenuKey &) const = default;
};
enum class FrontendMenuRelation { original, vista_variant, vista_addition };
struct FrontendMenuCatalogEntry {
  FrontendMenuKey key;
  std::string path;
  unsigned pane_count{};
  FrontendMenuRelation relation{};
  std::optional<std::uint32_t> source_datum;
  std::optional<FrontendMenuKey> original;
};

// Reservation/definition metadata only. A catalog entry does not imply loaded
// assets, a renderable screen, a navigation route, or implemented PC settings.
// This class neither mutates FrontendContent nor calls MenuState/FrontendPainter.
class FrontendExtensionCatalog {
public:
  explicit FrontendExtensionCatalog(const FrontendContent &original);
  std::span<const FrontendMenuCatalogEntry> entries() const noexcept;
  const FrontendMenuCatalogEntry *find(FrontendMenuKey) const noexcept;
  const FrontendMenuCatalogEntry *find(FrontendMenuSource,
                                      std::string_view path) const noexcept;
  bool reserved(FrontendMenuKey) const noexcept;
  // A future separate PC importer can check its output identity here. This does
  // not register the definition as loaded or validate its runtime behavior.
  void validate_definition(FrontendMenuKey, const UiScreen &) const;

private:
  void add(FrontendMenuCatalogEntry);
  std::vector<FrontendMenuCatalogEntry> entries_;
  std::map<FrontendMenuKey, std::size_t> keys_;
  std::map<std::pair<FrontendMenuSource, std::string>, std::size_t> paths_;
};

// Supplied PC version8/build11081.07.04.30.0934.main mainmenu.map. All131
// identities and pane counts are retained by the reproducible inventory under
// analysis/frontend/hd-ui-followup-20261009; no Vista layout is made live here.
std::string_view frontend_vista_menu_catalog_source_sha256() noexcept;
} // namespace halo2
