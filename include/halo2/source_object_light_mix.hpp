#pragma once
#include "halo2/source_prt_contact.hpp"

namespace halo2 {
struct SourceObjectLightMix { Vec3 incident{};std::uint32_t lightmap_rgb{}; };
// D1E10's complete object PRT/radiance combination at D247D..D292E. Baked
// input exists only when both D33A0 radiance and D31C0 incident succeed.
// Missing inputs preserve the original default direction supplied by caller.
// Cache/residency status and later material diffuse sampling are separate.
SourceObjectLightMix source_object_light_mix(
    const std::optional<SourcePrtContactSample>& prt,
    const std::optional<SourcePrtContactSample>& baked,Vec3 default_incident);
}
