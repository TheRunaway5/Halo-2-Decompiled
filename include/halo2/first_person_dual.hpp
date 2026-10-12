#pragma once
#include "halo2/model.hpp"
namespace halo2 {
//169965 chooses the first weapon model node, its graph parent and the
//opposite graph hand flag.1E0C80 changes only that root's local transform
//and parent; authored left-arm channels remain left-arm channels.
struct FirstPersonDualRoot {int node=-1, parent=-1;};
FirstPersonDualRoot first_person_dual_root(const AnimationClip &,std::uint32_t weapon_root);
Transform source_first_person_dual_local(Transform);
std::vector<Transform> source_first_person_dual_nodes(std::span<const BonePose>,
    std::span<const int> parents,FirstPersonDualRoot,const Transform &root);
} // namespace halo2
