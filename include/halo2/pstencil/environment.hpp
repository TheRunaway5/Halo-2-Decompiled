#pragma once
#include "halo2/cache.hpp"

namespace halo2::pstencil {
// Bounded current-asset interface: independent tangent-space environment pass.
// Other independent/combined response families remain with their current code.
bool supports_environment_response(const Material&);
// Actual default-layered resident six-combiner response. Preserve completed
// current tint/brightness attributes, environment/bump textures and alpha blend;
// copy the current view direction and the consumer-proved midrange parameters.
PixelProgram adapt_environment_response(const PixelProgram& completed_current);
// Literal older callback fallback construction, retained for reference tests.
// Its model-frame admission remains unproved; the live adapter does not use it.
// Caller supplies the actual window vector; textures/attributes/alpha ownership
// remain the explicitly declared current-asset adaptation.
PixelProgram fallback_environment_response(Vec3 window_direction);
}
