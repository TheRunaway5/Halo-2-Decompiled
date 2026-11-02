#pragma once
#include "halo2/native_tu15_damage_region_eligibility.hpp"
#include "halo2/source_damage_region_eligibility_observation_tu15.hpp"
#include "gameplay/objects/source_object_regions_native_tu15_internal.hpp"
#include <bit>

namespace halo2::physics_world_runtime_detail {
// Complete DC4C0 with its original legacy-call ABI and live owned DATA cells.
// The native predicate makes every eligibility decision. Neither object query
// output nor the source AL result is supplied by a caller or reconstructed
// from an authored name. Older images keep their retained literal route.
template<class State,class Site>
void damage_region_eligibility_native_tu15(State& s,Site site) {
    struct Storage final:NativeTu15DamageRegionEligibilityStorage {
        State& s;Site& site;SourceDamageRegionEligibilityObservationTu15 observation;
        Storage(State& state,Site& observe):s(state),site(observe){
            observation.section=s.gp[0];observation.response=s.gp[1];observation.damage_info=s.gp[2];
        }
        std::uint32_t at(std::uint32_t actual){return site(actual-0xf50u,actual);}
        void query_regions()override {
            at(0xdc4c0u);s.gp[4]=s.flags.minus(s.gp[4],16u,4);
            at(0xdc4c3u);s.gp[2]=r(s.memory,s.gp[2]+0xc0u,4);
            at(0xdc4c9u);s.gp[0]=s.flags.product(s.gp[0],0x38u);
            s.push(at(0xdc4ccu),s.gp[3]);
            s.push(at(0xdc4cdu),s.gp[5]);
            s.push(at(0xdc4ceu),s.gp[6]);
            at(0xdc4cfu);s.gp[6]=r(s.memory,s.gp[2]+s.gp[0]+16u,4);
            at(0xdc4d3u);s.gp[5]=s.gp[1]*5u;
            s.push(at(0xdc4d6u),s.gp[7]);
            at(0xdc4d7u);s.gp[0]=s.gp[4]+20u;
            at(0xdc4dbu);s.gp[5]=s.flags.shift(s.gp[5],4u,4);
            s.push(at(0xdc4deu),s.gp[0]);
            at(0xdc4dfu);s.gp[0]=r(s.memory,s.gp[4]+40u,4);
            observation.object=s.gp[0];
            at(0xdc4e3u);s.gp[5]=s.flags.plus(s.gp[5],s.gp[6],4);
            observation.response_address=s.gp[5];
            at(0xdc4e5u);s.gp[3]=s.gp[4]+28u;
            at(0xdc4e9u);s.gp[7]=s.gp[4]+32u;
            at(0xdc4edu);s.gp[6]=s.gp[4]+40u;
            w(s.memory,s.observe,at(0xdc4f1u),s.gp[4]+23u,1,1u);
            s.push(at(0xdc4f6u),0xdc4fbu);
            const auto bank=object_regions_native_tu15(s,[&](std::uint32_t actual){return site(actual-0x1140u,actual);});
            observation.region_count=bank.count;observation.active_base=bank.active_base;
        }
        std::int16_t runtime_region()override {
            at(0xdc4fbu);const auto region=std::bit_cast<std::int16_t>(std::uint16_t(r(s.memory,s.gp[5]+34u,2)));
            s.set(0,std::uint16_t(region),2,0);
            observation.runtime_region=region;
            at(0xdc4ffu);s.flags.logic(std::uint16_t(region),2);
            at(0xdc502u);return region;
        }
        std::int32_t region_count()override {
            at(0xdc504u);s.gp[1]=r(s.memory,s.gp[4]+36u,4);
            at(0xdc508u);s.gp[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(s.gp[0]))));
            at(0xdc50bu);s.flags.minus(s.gp[0],s.gp[1],4);
            at(0xdc50du);return std::bit_cast<std::int32_t>(s.gp[1]);
        }
        std::int8_t current_state(std::int32_t region)override {
            at(0xdc50fu);s.gp[1]=r(s.memory,s.gp[4]+20u,4);
            at(0xdc513u);const auto value=std::bit_cast<std::int8_t>(std::uint8_t(r(s.memory,s.gp[1]+std::uint32_t(region)*8u+1u,1)));
            s.set(2,std::uint16_t(std::int16_t(value)),2,0);return value;
        }
        std::int16_t requested_state()override {
            at(0xdc519u);const auto requested=std::bit_cast<std::int16_t>(std::uint16_t(r(s.memory,s.gp[5]+32u,2)));
            s.flags.minus(s.reg(2,2,0),std::uint16_t(requested),2);
            at(0xdc51du);return requested;
        }
    } storage(s,site);
    const auto result=native_tu15_damage_region_eligibility(storage);
    storage.observation.reason=result;storage.observation.allowed=native_tu15_damage_region_allowed(result);
    source_observe_damage_region_eligibility_tu15(s.memory,storage.observation);
    if(native_tu15_damage_region_allowed(result)) {
        storage.at(0xdc52bu);s.set(0,r(s.memory,s.gp[4]+19u,1),1,0);
        storage.at(0xdc52fu);s.gp[7]=s.pop();
        storage.at(0xdc530u);s.gp[6]=s.pop();
        storage.at(0xdc531u);s.gp[5]=s.pop();
        storage.at(0xdc532u);s.gp[3]=s.pop();
        storage.at(0xdc533u);s.gp[4]=s.flags.plus(s.gp[4],16u,4);
        storage.at(0xdc536u);s.pop();s.gp[4]+=4u;
    } else {
        storage.at(0xdc51fu);s.gp[7]=s.pop();
        storage.at(0xdc520u);s.gp[6]=s.pop();
        storage.at(0xdc521u);s.gp[5]=s.pop();
        storage.at(0xdc522u);s.set(0,s.flags.logic(0u,1),1,0);
        storage.at(0xdc524u);s.gp[3]=s.pop();
        storage.at(0xdc525u);s.gp[4]=s.flags.plus(s.gp[4],16u,4);
        storage.at(0xdc528u);s.pop();s.gp[4]+=4u;
    }
}
}
