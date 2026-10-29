#pragma once

namespace halo2 {
struct MultiplayerOptions;
// Owning native replay/takeover recipe. Retains the admitted session and
// gameplay selections, without copying renderer/device callbacks, desktop,
// prepared worlds or other live presentation borrowers.
MultiplayerOptions source_authority_options(const MultiplayerOptions &);
}
