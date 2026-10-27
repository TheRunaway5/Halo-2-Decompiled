#pragma once
#include "halo2/multiplayer.hpp"
#include "halo2/render_interpolation.hpp"
#include <optional>
#include <span>
#include <vector>
namespace halo2 {
class SourceWorldInterpolation;
// Discrete state is copied from current; only render transforms and continuous
// visual channels, including completed vehicle observers, are sampled. Full
// owner identities and seat/perspective changes prevent incompatible history.
MultiplayerState multiplayer_presentation(const MultiplayerState &previous,
    const MultiplayerState &current,float fraction);
// Share root/skin lifecycle boundaries for local rendering and LAN snapshots.
void capture_multiplayer_body_poses(SourceWorldInterpolation &,
    const MultiplayerState &previous,const MultiplayerState &current,bool reset=false);
void refresh_multiplayer_body_poses(SourceWorldInterpolation &,
    const MultiplayerState &previous,const MultiplayerState &current);
// Non-host movement prediction (NETCODE_ENHANCED_GOAL item 1): the own
// third-person bodies drawn where the predicted cameras are. `predicted[view]`
// is the position the movement client presented for local view `view`
// (slot `slots[view]`). A view qualifies only when `shown` carries exactly
// that position (its camera is predicted) and its player is alive, on foot
// and unseated; seated, dead and unpredicted views keep their host bodies.
// The roots name the unit with its authoritative origins at `previous` and
// `current`, the endpoints of the body pose interpolation. Presentation only.
std::vector<SourceWorldPredictedRoot> predicted_body_roots(const MultiplayerState &previous,
    const MultiplayerState &current,const MultiplayerState &shown,std::span<const unsigned> slots,
    std::span<const std::optional<Vec3>> predicted);
// Shadow/lighting sphere of a body drawn through `roots`: the completed
// current sphere carried by the same root displacement. Unlisted owners keep
// their sphere exactly.
ShadowCasterSphere predicted_body_sphere(std::span<const SourceWorldPredictedRoot> roots,
    std::uint32_t object,const ShadowCasterSphere &current);
}
