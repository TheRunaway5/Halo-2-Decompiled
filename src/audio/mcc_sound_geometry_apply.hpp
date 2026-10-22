#pragma once
#include "halo2/mcc_sound_geometry.hpp"
#include "halo2/mcc_sound_runtime_delivery.hpp"
namespace halo2::mcc_sound_geometry_detail {
// Apply only independently authenticated DAT-resource tokens to the complete
// native extra-info rows. Raw MCC metadata and duration words stay unchanged.
void apply(MccSoundRuntimePlan&,std::span<const MccSoundGeometryResource>);
}
