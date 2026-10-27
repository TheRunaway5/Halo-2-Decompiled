#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
// Original4DF90/4EBC0 floating/projected decorator material contract. This
// binds authored LOD0/layer2/class0 resources only. It supplies no visibility,
// geometry, camera or c53 fade bank; those belong to the completed draw owner.
void validate_source_decorator_decal_material_tu15(const Material&);
Material source_decorator_decal_material_tu15(const Cache&,std::uint32_t shader);
}
