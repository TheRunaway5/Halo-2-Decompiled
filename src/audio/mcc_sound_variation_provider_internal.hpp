#pragma once
#include "halo2/mcc_sound_variation_provider.hpp"
#include "halo2/mcc_runtime_delivery.hpp"
#include "halo2/mcc_sound_runtime_delivery.hpp"
#include "halo2/mcc_sound_timing_reference.hpp"
namespace halo2 {class SourceRuntimeResources;}

namespace halo2::detail {
// World-only construction seam. Numeric roots alone cannot create the public
// provider: construction also requires an actual current memory owner and
// complete canonical delivery/source ownership, admitted in one transaction.
MccSoundVariationProvider make_mcc_sound_variation_provider(
    const Cache& actual_owner,const Cache& expected_owner,
    const MccRuntimeCacheDelivery&,const MccSoundRuntimePlan&,const SourceRuntimeResources&,
    const std::shared_ptr<const MccSoundTimingMemoryOwner>&);
// Closed Access/Audio pairing admission. Both capabilities validate their
// actual current lease/thread before comparing the retained real owner.
// This returns no identity token, raw owner or source-memory access.
void require_mcc_sound_variation_owner(const MccSoundVariationProvider&,
    const Cache& expected_owner,const std::shared_ptr<const MccSoundTimingMemoryOwner>&);
// Genuine World publication additionally requires that World's exact retained
// data owner. This performs only immutable comparisons, without a memory read.
void require_mcc_sound_variation_world_resources(const MccSoundVariationProvider&,
    const Cache&,const SourceRuntimeResources&);
// A fresh target may retain saved PCM only with the same complete immutable
// resource identity. An unbound saved route has no such proof and rejects.
// A saved provider may
// already have a stale epoch; only the target must be current. No source
// memory or resource digest escapes this comparison.
void require_mcc_sound_variation_resources(const MccSoundVariationProvider& target,
    const MccSoundVariationProvider* saved,const Cache& expected_owner);
} // namespace halo2::detail
