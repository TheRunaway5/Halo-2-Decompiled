#pragma once
#include "halo2/source_shadow.hpp"
#include <vector>

namespace halo2 {
// Read-only completed source body presentation. WORLD nodes and the retained
// object+30/+3C sphere are copied from the same full salted source owner.
struct SourceWorldPose {
    std::uint32_t object{0xffffffffu};
    std::vector<Transform> nodes;
    ShadowCasterSphere sphere;
};
}
