#pragma once
#include <string_view>

namespace halo2 {
struct Material;
inline constexpr std::string_view original_matchmaking_emblem_source_name =
    "ui\\hud\\shaders\\ui_small_emblem";
inline constexpr std::string_view original_matchmaking_emblem_private_name =
    "ui\\hud\\shaders\\ui_small_emblem#og-matchmaking";

bool original_matchmaking_emblem_private(std::string_view name) noexcept;
// Clone an already admitted two-stage UI emblem material into a private
// original-goal owner. All32 background and64 foreground pages retain their
// exact DVD pixels. The private material's exact-name input IDs select the
// original page/UV/appearance consumers; generic MCC material IDs are intact.
Material make_original_matchmaking_emblem(Material source);
} // namespace halo2
