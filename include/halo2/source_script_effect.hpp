#pragma once
#include "halo2/transform.hpp"
#include <functional>
#include <optional>

namespace halo2 {
// BAD60's completed first-marker result. The original caller requests one
// marker with interpolation disabled. A zero count still has a separately
// owned output record; the constructor's second lookup does not test count.
struct SourceScriptEffectMarker {
    std::uint16_t count{};
    Transform world;
};
using SourceScriptEffectMarkerLookup=std::function<SourceScriptEffectMarker(
    std::uint32_t object,std::uint32_t marker)>;
struct SourceEffectSeedMarker {
    Vec3 position,forward;
    std::uint32_t name{};
};
static_assert(sizeof(SourceEffectSeedMarker)==28);
struct SourceScriptEffectRequest {
    std::uint32_t definition{},object{},marker{};
    std::array<SourceEffectSeedMarker,2> seeds;
    // Complete original179700 parameter packet. The lifetime owner supplies
    // the address of the two retained seed records; no host pointer is cast.
    std::array<std::uint32_t,24> parameters(std::uint32_t seed_address)const;
};
// Whole2AA360 ->179700 request construction, ending before178E30's effect
// allocation. NONE inputs and a failed first marker lookup produce no request.
// This does not allocate effects, admit PVS, consume RNG or render particles,
// lights/lenses. Those services must consume the returned actual request.
std::optional<SourceScriptEffectRequest> source_script_effect_new_on_object_marker(
    std::uint32_t definition,std::uint32_t object,std::uint32_t marker,
    const SourceScriptEffectMarkerLookup &);
}
