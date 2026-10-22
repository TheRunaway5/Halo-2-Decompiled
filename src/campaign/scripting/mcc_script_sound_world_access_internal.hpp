#pragma once
#include "halo2/mcc_script_sound_world_access.hpp"
#include "halo2/mcc_sound_runtime_delivery.hpp"
#include "halo2/mcc_sound_timing_reference.hpp"
#include "halo2/mcc_sound_variation_provider.hpp"
namespace halo2::detail {
MccScriptSoundWorldAccess make_mcc_script_sound_world_access(
    PhysicsLiveWorldTu15&,const Cache& actual_owner,const Cache& expected_owner,
    MccSoundVariationProvider,const MccSoundRuntimePlan&,MccSoundTimingPlan,
    const std::shared_ptr<const MccSoundTimingMemoryOwner>&,
    SoundDriverFloatingPointContext);
}
