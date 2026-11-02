#pragma once
#include "halo2/native_tu15_damage_region_eligibility.hpp"
#include "halo2/physics_contact_record.hpp"
#include <utility>

namespace halo2 {
struct SourceDamageRegionEligibilityObservationTu15 {
    std::uint32_t object{},section{},response{},damage_info{},response_address{},active_base{};
    std::int16_t runtime_region{};
    std::int32_t region_count{};
    NativeTu15DamageRegionEligibility reason{};
    bool allowed{};
};
namespace damage_region_eligibility_observation_detail {
using Observer=std::function<void(const SourceDamageRegionEligibilityObservationTu15&)>;
struct Binding {const SourcePhysicsContactMemory* memory;Observer observer;};
inline thread_local Binding* active{};
}
class SourceDamageRegionEligibilityObservationScopeTu15 {
    damage_region_eligibility_observation_detail::Binding binding_;
    damage_region_eligibility_observation_detail::Binding* prior_;
public:
    SourceDamageRegionEligibilityObservationScopeTu15(const SourcePhysicsContactMemory* memory,
        damage_region_eligibility_observation_detail::Observer observer)
        :binding_{memory,std::move(observer)},prior_(damage_region_eligibility_observation_detail::active){
        damage_region_eligibility_observation_detail::active=&binding_;
    }
    ~SourceDamageRegionEligibilityObservationScopeTu15(){damage_region_eligibility_observation_detail::active=prior_;}
    SourceDamageRegionEligibilityObservationScopeTu15(const SourceDamageRegionEligibilityObservationScopeTu15&)=delete;
};
// Synchronous completed native observation only. It cannot select, restore or
// provide the eligibility result, and another memory bank cannot receive it.
inline void source_observe_damage_region_eligibility_tu15(const SourcePhysicsContactMemory& memory,
    const SourceDamageRegionEligibilityObservationTu15& value) {
    const auto binding=damage_region_eligibility_observation_detail::active;
    if(binding&&binding->memory==&memory&&binding->observer)binding->observer(value);
}
}
