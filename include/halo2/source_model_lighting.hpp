#pragma once
#include "halo2/source_model_resources.hpp"

namespace halo2 {
struct SourceModelLightingWorld {
    SourceModelResourceWorld memory;
    std::uint32_t default_lighting{},bsp{},fallback_lighting{},scratch{};
    bool interpolation{};
    // Actual D1850 query and D4080 conversion. Both operate on current banks;
    // query writes its five-word sample, conversion its21-word lighting bank.
    std::function<bool(std::uint32_t,bool,std::uint32_t)> query;
    std::function<void(std::uint32_t,std::int32_t,bool,std::uint32_t)> convert;
    // Actual BA1D0 velocity and3E5A0 incident interpolation owners. Neither
    // supplies cache stamps, validity, routing or parent inheritance writes.
    std::function<void(std::uint32_t,std::uint32_t)> velocity;
    std::function<void(std::uint32_t,std::uint32_t,float)> incident;
};
// Complete3DB00/BAF80 routing and3DFE0 cache chronology, with native3E4E0 RGB
// interpolation and BADC0 typed-owner test. Returns the current84-byte bank's
// address; the caller retains its final metadata word and ownership.
std::uint32_t source_model_lighting(std::uint32_t owner,float metric,
    const SourceModelLightingWorld&);
//3DFE0 does not advance the last-rendered stamp.4B5D0 owns that later write.
void source_model_lighting_update(std::uint32_t token,std::uint32_t owner,
    float metric,const SourceModelLightingWorld&);
}
