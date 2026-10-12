#include "halo2/frontend_extension_catalog.hpp"
#include "halo2/frontend.hpp"
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
template<class F> void rejects(F call, const char *message) {
  try { call(); } catch (const FormatError &) { return; }
  throw std::runtime_error(message);
}
constexpr FrontendMenuKey xbox(unsigned id) {
  return {FrontendMenuSource::original_xbox, std::uint16_t(id)};
}
constexpr FrontendMenuKey vista(unsigned id) {
  return {FrontendMenuSource::vista, std::uint16_t(id)};
}
void identities(const FrontendContent &original) {
  const auto *settings = &original.screens.at(19);
  FrontendExtensionCatalog catalog(original);
  check(catalog.entries().size() == 215, "expected84 actual originals plus131 Vista reservations");
  unsigned original_count = 0, vista_count = 0, additions = 0, variants = 0;
  std::set<FrontendMenuKey> keys;
  std::set<std::pair<FrontendMenuSource, std::string>> paths;
  for (const auto &entry : catalog.entries()) {
    check(keys.insert(entry.key).second && paths.emplace(entry.key.source, entry.path).second,
          "duplicate source-qualified reservation");
    check(catalog.find(entry.key) == &entry && catalog.reserved(entry.key) &&
          catalog.find(entry.key.source, entry.path) == &entry,
          "catalog lookup lost authored identity");
    if (entry.key.source == FrontendMenuSource::original_xbox) {
      ++original_count;
      check(entry.relation == FrontendMenuRelation::original && !entry.source_datum &&
            !entry.original, "original reservation fabricated PC provenance");
      const auto &screen = original.screens.at(entry.key.id);
      check(entry.path == screen.name && entry.pane_count == screen.panes.size(),
            "original definition was displaced");
      catalog.validate_definition(entry.key, screen);
    } else {
      ++vista_count;
      check(entry.source_datum && *entry.source_datum != 0xffffffffu,
            "Vista reservation lost actual source datum");
      if (entry.relation == FrontendMenuRelation::vista_addition) {
        ++additions;
        check(!entry.original && entry.key.id >= 243 && entry.key.id <= 297,
              "Vista addition displaced an original screen");
      } else {
        ++variants;
        check(entry.relation == FrontendMenuRelation::vista_variant && entry.original &&
              entry.original->source == FrontendMenuSource::original_xbox &&
              entry.original->id == entry.key.id &&
              catalog.find(*entry.original)->path == entry.path,
              "common PC path lost its separate original identity");
      }
    }
  }
  check(original_count == 84 && vista_count == 131 && additions == 47 && variants == 84,
        "actual source menu coverage changed");
  const auto *x = catalog.find(xbox(19)), *v = catalog.find(vista(19));
  check(x && v && x != v && x->path == v->path && x->pane_count == 2 &&
        v->pane_count == 6 && v->source_datum == 0xe55c033au && v->original == xbox(19),
        "Settings2/6 pane variants were merged");
  check(catalog.find(vista(38))->pane_count == 3 && catalog.find(xbox(38))->pane_count == 6,
        "PC profile pane reduction was applied to Xbox");
  check(catalog.find(vista(210))->pane_count == 3 && catalog.find(xbox(210))->pane_count == 4,
        "PC matchmaking pane variant was applied to Xbox");
  check(!catalog.reserved(vista(244)) && !catalog.find(vista(244)) &&
        !catalog.reserved(xbox(243)) && catalog.reserved(vista(243)) &&
        !catalog.find(FrontendMenuSource::vista, "unknown\\screen"),
        "unknown source identity was fabricated or crossed namespaces");
  check(catalog.find(vista(243))->path ==
            "ui\\screens\\game_shell\\settings_screen\\player_profile\\keyboard_settings" &&
        catalog.find(vista(243))->pane_count == 4 &&
        catalog.find(vista(249))->path ==
            "ui\\screens\\game_shell\\live_game_browser\\live_game_browser" &&
        catalog.find(vista(297))->path ==
            "ui\\screens\\game_shell\\settings_screen\\about\\about_dialog",
        "actual PC extension paths changed");
  check(&original.screens.at(19) == settings && settings->panes.size() == 2,
        "catalog mutated original content");
  check(frontend_vista_menu_catalog_source_sha256() ==
            "eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44",
        "Vista source provenance changed");

  // This is an identity admission control, not proof of a loaded PC layout.
  auto descriptor = *settings;
  descriptor.panes.resize(6);
  catalog.validate_definition(vista(19), descriptor);
  rejects([&] { catalog.validate_definition(xbox(19), descriptor); },
          "PC six-pane descriptor admitted into Xbox Settings");
  rejects([&] { catalog.validate_definition(vista(19), *settings); },
          "two-pane Xbox descriptor admitted as PC Settings");
  descriptor.id = 20;
  rejects([&] { catalog.validate_definition(vista(19), descriptor); },
          "wrong source ID admitted");
  descriptor.id = 19;
  descriptor.name = "ui\\different";
  rejects([&] { catalog.validate_definition(vista(19), descriptor); },
          "wrong source path admitted");
  rejects([&] { catalog.validate_definition(vista(244), descriptor); },
          "unreserved source ID admitted");
}
void malformed() {
  FrontendContent content;
  UiScreen first; first.id = 19; first.name = "ui\\duplicate";
  content.screens.emplace(19, first);
  first.id = 38;
  content.screens.emplace(38, first);
  rejects([&] { FrontendExtensionCatalog c(content); },
          "duplicate original path accepted");
  content.screens.erase(38);
  content.screens.at(19).id = 20;
  rejects([&] { FrontendExtensionCatalog c(content); },
          "map ID differing from source definition accepted");
  content.screens.at(19).id = 19;
  content.screens.at(19).name.clear();
  rejects([&] { FrontendExtensionCatalog c(content); },
          "empty source path accepted");
  content.screens.clear();
  FrontendExtensionCatalog empty(content);
  check(empty.entries().size() == 131 && !empty.find(xbox(19)) &&
        empty.find(vista(19)) && !empty.find(vista(19))->original,
        "metadata-only catalog fabricated a loaded original definition");
}
} // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 2 || !std::filesystem::exists(std::filesystem::path(argv[1]) / "mainmenu.map")) {
      std::cerr << "usage: frontend_extension_catalog_tests <Xbox maps directory>\n";
      return 77;
    }
    Cache cache(std::filesystem::path(argv[1]) / "mainmenu.map");
    const auto original = cache.load_frontend();
    check(original.screens.size() == 84, "expected original84-screen mainmenu fixture");
    identities(original);
    malformed();
    std::cout << "Frontend extension catalog:84 original identities,131 separate Vista reservations,47 additions\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
