#pragma once
#include "halo2/mcc_runtime_delivery.hpp"
#include "halo2/mcc_sound_runtime_delivery.hpp"
#include "halo2/mcc_sound_timing_reference.hpp"
#include <memory>
namespace halo2 {
// Unpublished immutable plans only. Source arena allocation, memory binding,
// timing-memory leases and all original initialization remain executor-owned.
struct SourceRuntimeMccPreparation {
    std::uint32_t base{},budget{};
    MccRuntimeCacheDelivery delivery;
    MccSoundRuntimePlan sound;
    MccSoundTimingPlan timing;
};
std::unique_ptr<SourceRuntimeMccPreparation> take_prepared_mcc_world_metadata(
    const Cache&,std::uint32_t base,std::uint32_t budget);
}
