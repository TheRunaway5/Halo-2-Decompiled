#pragma once
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace halo2 {
class Cache;
struct RetailFont;
enum class GameplayFontDomain { Original, Mcc };
struct GameplayFontSource {
    GameplayFontDomain domain{GameplayFontDomain::Original};
    std::filesystem::path directory;
    std::vector<std::string> names;
};
// Selection is source-qualified by Cache. Existing callers passing the absent
// MCC maps/fonts path are accepted only after authenticating its h2_fonts owner.
std::filesystem::path gameplay_font_directory(const Cache&);
GameplayFontSource load_gameplay_font_source(const std::filesystem::path&);
// Known MCC files retain their actual decoded atlas. The finite original
// layout profile supplies logical metrics/lookup/kerning, never original pixels.
RetailFont load_gameplay_font(const GameplayFontSource&, unsigned selector);
int gameplay_font_horizontal_offset(const GameplayFontSource&, unsigned selector);
// An already projected MCC font may skip only a byte-identical ordinary source
// sample. Custom redraws require a separate, explicit projection owner.
void require_mcc_gameplay_font_sample(const GameplayFontSource&,unsigned selector,
                                     const std::filesystem::path& sample);
// Private dedicated action-button fonts only, after authenticated MCC source
// selection and authored prompt sampling. Preflight every owner/record/metric
// and sample before changing only the20 control metrics. Returns the number of
// changed controls (zero for selector5 or a complete prior projection); an
// unqualified or partial state throws without changing the font.
std::size_t apply_mcc_action_button_metrics(RetailFont&,unsigned selector);
// Default imported MCC menu prompt bank only. Selectors1/7 own the same exact
// Conduit12 source and authored samples. Applies the same atomic/idempotent
// finite control projection; ordinary text and every other selector stay intact.
std::size_t apply_mcc_menu_button_metrics(RetailFont&,unsigned selector);
}
