#include "halo2/effect_audio_auxiliary.hpp"
#include "halo2/cache.hpp"
#include <limits>

namespace halo2 {
bool EffectAudioAuxiliary::witness(const std::uint32_t *binding,
    std::uint32_t value,bool committed_only)const noexcept {
    const auto &target=target_;
    if(!target || target->executor!=std::this_thread::get_id() ||
       !binding || target->binding!=binding || target->saved_value!=value)return false;
    const auto owner=target->owner.lock();
    if(!owner || !owner->alive.load(std::memory_order_acquire) ||
       owner->executor!=std::this_thread::get_id())return false;
    const auto phase=target->phase.load(std::memory_order_acquire);
    if(phase==Phase::Prepared)return !committed_only &&
        target->generation!=std::numeric_limits<std::uint64_t>::max() &&
        owner->generation==target->generation;
    if(phase!=Phase::Committed ||
       target->generation==std::numeric_limits<std::uint64_t>::max() ||
       owner->generation!=target->generation+1 || owner->current!=target ||
       owner->current.owner_before(target) || target.owner_before(owner->current))return false;
    // The actual Effects object owns the liveness control and must be
    // destroyed on this executor. Its borrowed RNG word must outlive it.
    return *target->binding==target->saved_value;
}
bool EffectAudioAuxiliary::ready(const std::uint32_t *binding,
    std::uint32_t value)const noexcept {
    return witness(binding,value,false);
}
bool EffectAudioAuxiliary::committed(const std::uint32_t *binding,
    std::uint32_t value)const noexcept {
    return witness(binding,value,true);
}
const ValidatedAuthorityCut &EffectAudioAuxiliary::cut()const {
    if(!target_)throw FormatError("Missing Effects auxiliary target agreement");
    return target_->cut;
}
}
