#pragma once
#include "halo2/source_first_person_pose_tu15.hpp"
#include <array>
#include <span>

namespace halo2 {
struct SourceFirstPersonDrawBindingTu15 {
    std::uint32_t unit=0xffffffffu,arms_model=0xffffffffu;
    std::array<std::uint32_t,2> weapons{0xffffffffu,0xffffffffu};
    std::array<std::uint32_t,2> weapon_models{0xffffffffu,0xffffffffu};
};
struct SourceFirstPersonDrawsTu15 {
    const SourceFirstPersonModelTu15 *arms{};
    std::array<const SourceFirstPersonModelTu15*,2> weapons{};
};
// Consume precisely the actual168DED foreground list. Native visibility can
// omit either/both registered weapons while retaining one shared arms bank.
// No pose is created, substituted, reflected or recovered from a registration.
inline SourceFirstPersonDrawsTu15 source_first_person_draws_tu15(
    std::span<const SourceFirstPersonModelTu15> models,const SourceFirstPersonDrawBindingTu15 &binding) {
    SourceFirstPersonDrawsTu15 result;
    for(const auto &model:models) {
        if(model.mode==0) {
            if(model.owner!=binding.unit)throw FormatError("Native FP WORLD record lost its actual unit owner");
            continue;
        }
        if(model.mode!=1)throw FormatError("Native FP record has an unknown rendering mode");
        const SourceFirstPersonModelTu15 **slot=nullptr;
        if(model.owner==binding.unit && model.render_model==binding.arms_model)slot=&result.arms;
        else for(unsigned hand=0;hand<2;++hand)
            if(binding.weapons[hand]!=0xffffffffu && model.owner==binding.weapons[hand] &&
                model.render_model==binding.weapon_models[hand]) {slot=&result.weapons[hand];break;}
        if(!slot || *slot)throw FormatError("Native FP foreground record has an unbound or duplicate owner/model");
        *slot=&model;
    }
    return result;
}
}
