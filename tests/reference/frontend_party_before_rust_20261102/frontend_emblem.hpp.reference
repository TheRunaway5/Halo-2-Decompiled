#pragma once
#include "halo2/frontend_appearance.hpp"
#include "halo2/cache.hpp"
namespace halo2 {
// 22A664 selects the authored emblem pixel pass but forces screen_hud,
// which passes the same untransformed v3.xy to texture stages0 and1.
Material frontend_emblem_material(const Cache &, std::uint32_t shader);
// Original14CCF0/22ACE7 appearance function inputs and widget opacity.
float frontend_emblem_input(const FrontendAppearance &, std::uint32_t name,
                            float opacity);
// 2B10A3/2B102F writes appearance+5 to the biped object+24. In the main-menu
// scenario (type2), BCC20 -> BC970 supplies these model object functions.
// Unlike UI14CCF0, BCC20 clamps the signed flip outputs into [0,1]. Other
// names remain the caller's ordinary object-function responsibility.
std::optional<float> frontend_model_emblem_input(const FrontendAppearance &,
                                               std::uint32_t name);
void update_frontend_emblem(Material &, const FrontendAppearance &,
                            std::span<const Vec3> palette, float opacity);
} // namespace halo2
