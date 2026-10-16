#pragma once
#include "halo2/cache.hpp"
#include <optional>

namespace halo2::pstencil {
// Exact May2003 Elite paths whose resident texture pass binds default_detail.
// Admit only the current independent texture pass's isolated T0*T1 stage.
std::optional<unsigned> reference_neutral_detail_stage(const Material&);
// The primary default_detail bitmap contains uniform RGBA(128,128,128,255).
// Substitute that sample in this one multiply, including destination alpha.
// Other texture samples, change colors, normal maps and material data survive.
PixelProgram neutral_detail_response(const PixelProgram&,unsigned stage);
}
