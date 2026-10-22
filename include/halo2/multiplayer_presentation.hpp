#pragma once
#include "halo2/multiplayer.hpp"
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
}
