#pragma once
#include "halo2/multiplayer.hpp"
namespace halo2 {
// Discrete state is copied from current; only render transforms and continuous
// visual channels are sampled. Full owner identities prevent generation reuse.
MultiplayerState multiplayer_presentation(const MultiplayerState &previous,
    const MultiplayerState &current,float fraction);
}
