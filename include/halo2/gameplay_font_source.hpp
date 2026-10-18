#pragma once
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
}
