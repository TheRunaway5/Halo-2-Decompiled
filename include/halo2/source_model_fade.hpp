#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
// Literal current runtime fields. An absent field is unresolved, whereas an
// explicitly supplied NONE is the source sentinel. Parent links are object+14.
struct SourceModelFadeOwner {
    std::uint32_t identity;
    std::optional<std::uint32_t> flags, parent, definition;
    std::optional<float> radius; // CURRENT object+3C, including attached owners.
};
struct SourceModelFadeDefinition {
    std::uint32_t identity;
    std::optional<std::uint32_t> model; // Current definition+38, hlmt datum.
};
struct SourceModelFadeModel {
    std::uint32_t identity;
    std::optional<float> fade_end, fade_start; // hlmt+28/+2C.
    std::array<std::optional<float>,5> render_cutoffs; // hlmt+34..+44.
    std::optional<std::uint16_t> render_lod_count; // hlmt+4C.
};
struct SourceModelFadeInput {
    std::uint32_t owner;
    // The original incoming XMM1 and actual byte4BA019, supplied by callers.
    // Neither is inferred from a native distance or source scenario flags.
    std::optional<float> draw_metric;
    std::optional<std::uint8_t> view_modifier;
};
struct SourceModelFadeBytes {
    std::uint8_t first{}, second{};
    bool operator==(const SourceModelFadeBytes &)const=default;
};
// Complete4BAF0 with literal BAF80 parent-root selection and both source
// output bytes. Flag19/view override precede modelNONE admission. A valid root
// model overrides the current model; a root modelNONE retains the current one.
// The first byte uses selected hlmt+28/+2C; the second uses selected hlmt+4C
// cutoff and CURRENT radius. Source3D7F0 consumes the second byte.
// Only fields read on the selected source branch are required. Missing or
// nonfinite read fields, negative current spheres, missing handles and cycles
// reject unresolved input. The source byte conversion uses its PC53 mode.
SourceModelFadeBytes source_model_fade(std::span<const SourceModelFadeOwner>,
    std::span<const SourceModelFadeDefinition>,std::span<const SourceModelFadeModel>,
    const SourceModelFadeInput &);
}
