#pragma once
#include "halo2/source_shader_lod.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/model.hpp"

namespace halo2 {
// 2232A0 constructs the enclosing frontend view before a model widget draws.
// Its 2F800 null-observer camera is separate from the widget's 2C560 camera:
// 2B0DDC prepares collectors first, then 2B0E12 installs that widget camera.
// Original standard/progressive and 4:3/widescreen video modes retain the
// canonical 640x480 outer rectangle here; host presentation does not resize it.
SourceShaderCoverageCamera frontend_model_coverage_camera();
struct FrontendModelGeometryLod {float distance{},metric{};int level{};};
// 133790 computes the whole owner's distance in2232A0's enclosing camera.
// 133E30 copies it to attached children before4BCC0 chooses their own bank.
// A visible UI biped is separately forced to5 by2B0C08→BBF40; weapons retain
// their ordinary object flags. Only the original initialized null camera and
// literal primary scenario type supply2F970's view modifier here.
FrontendModelGeometryLod frontend_model_geometry_lod(
    const SourceGeometryLodDefinition&,Vec3 cached_owner_center,
    std::uint32_t object_flags,std::uint8_t object_10a,
    std::int16_t scenario_type,std::int8_t previous_level=-1);
// Literal region/permutation selection at the source level. Resource
// residency is provided by Cache's owned disc resource decoder.
RenderModel load_frontend_model_geometry(const Cache&,std::uint32_t object,
    std::uint32_t variant,Vec3 cached_owner_center,std::uint32_t object_flags,
    std::uint8_t object_10a,std::int8_t previous_level=-1);
}
