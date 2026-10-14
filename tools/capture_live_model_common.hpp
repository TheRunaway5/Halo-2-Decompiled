#pragma once
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/content.hpp"
#include <bit>

// Shared capture-tool selection: source metadata supplies the original owner,
// placement and palette. No diagnostic geometry or pose is synthesized here.
inline halo2::ScenarioObject original_model_placement(halo2::PhysicsLiveWorldTu15& world,
                                                     std::uint32_t render_model) {
    using namespace halo2;
    const auto word=[&](std::uint32_t p){return Bytes(world.memory().view(p,4)).u32(0);};
    const auto half=[&](std::uint32_t p){return std::bit_cast<std::int16_t>(Bytes(world.memory().view(p,2)).u16(0));};
    const auto scenario=word(0x4ecc20u);std::optional<ScenarioObject> selected;
    // Use the original type table, palette and placement. A filename selects
    // the requested owner; it does not provide geometry, position or bounds.
    for(unsigned type=0;type<13 && !selected;++type) {
        const auto description=word(0x474b90u+type*4u);if(!description)continue;
        const auto placements=half(description+10),palette=half(description+12),stride=half(description+14);
        if(placements<0 || palette<0 || stride<=0)continue;
        const auto count=word(scenario+placements),rows=word(scenario+placements+4u);
        for(unsigned n=0;n<count;++n) {
            const auto row=rows+n*stride;const auto choice=half(row);if(choice<0)continue;
            if(unsigned(choice)>=word(scenario+palette))throw FormatError("original placement palette index invalid");
            const auto definition=word(word(scenario+palette+4u)+unsigned(choice)*40u+4u);
            if(definition==null_datum)continue;
            const auto tag_table=word(0x4f0414u);
            const auto definition_address=word(tag_table+(definition&65535u)*16u+8u);
            const auto initial_model=word(definition_address+0x38u);
            if(initial_model==null_datum)continue;
            const auto initial_address=word(tag_table+(initial_model&65535u)*16u+8u);
            if(word(initial_address+4u)!=render_model)continue;
            ScenarioObject object;object.type=int(type);object.placement=int(n);object.definition=definition;
            selected=object;break;
        }
    }
    if(!selected)throw FormatError("requested model has no original scenario placement");
    return *selected;
}
