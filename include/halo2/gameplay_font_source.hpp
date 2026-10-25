#pragma once
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <cstdint>

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
// One match owns one authenticated, immutable bank. Sharing decoded fonts
// avoids rebuilding their large glyph maps for every weapon HUD. A new match
// selects and authenticates its source again; this is not a pathname cache.
class GameplayFontBank {
public:
    explicit GameplayFontBank(const std::filesystem::path&);
    ~GameplayFontBank();
    const GameplayFontSource& source() const noexcept;
    bool matches_directory(const std::filesystem::path&) const;
    std::shared_ptr<const RetailFont> font(unsigned selector);
    std::shared_ptr<const RetailFont> raw(unsigned selector);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
int gameplay_font_horizontal_offset(const GameplayFontSource&, unsigned selector);
// Immutable original layout metadata, independent of atlas/file readiness.
// Missing profiles remain explicit; no host-font or default metrics are used.
std::optional<std::array<std::int16_t,3>> gameplay_font_reference_line_metrics(unsigned selector);
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
