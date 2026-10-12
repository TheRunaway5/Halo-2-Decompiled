#include "halo2/source_effect_light_geometry_frame.hpp"

namespace halo2 {
SourceEffectLightGeometryFrameResult source_effect_light_geometry_frame(SourceDrawUnitState& state,
    SourceBspAdmission& admission,SourceBspPartCommandState& commands,
    std::span<SourceOpaqueSortRecord> opaque,const SourceEffectLightGeometryFrameWorld& world,
    const SourceEffectLightGeometryFrameContext& context,
    const SourceEffectLightGeometryFrameObserver& observer) {
    if(!context.draw.mode || !world.dynamic || !world.object_commands)
        throw FormatError("source light geometry frame lacks its current secondary/object owners");
    source_bsp_collect_draw_units(state,admission,world.draw,context.draw);
    if(observer)observer(SourceEffectLightGeometryFramePhase::collected,state,admission);
    world.dynamic(state,admission,context.draw);
    auto cull=context.cull;
    cull.mode=context.draw.mode;cull.main_prefix=state.main_prefix;cull.draw_count=state.draw_count;
    if(cull.draw_count>state.units.size() || cull.main_prefix>cull.draw_count)
        throw FormatError("source light frame dynamic collector escaped current draw bank");
    std::array<SourceCullDrawBanks,850> banks;
    for(unsigned i=cull.main_prefix;i<cull.draw_count;++i) {
        const auto flags=state.units[i].words[0];
        if((flags&1u) || !(flags&0x40u))continue;
        if(!world.cull_bank)throw FormatError("source light frame lacks current cull geometry owner");
        banks[i]=world.cull_bank(i,state.units[i]);
    }
    SourceEffectLightGeometryFrameResult result;
    result.cull_calls=source_draw_unit_sphere_cull(state.units,banks,state.arena,
        context.draw.mask_arena_address,cull);
    if(observer)observer(SourceEffectLightGeometryFramePhase::culled,state,admission);
    auto command_context=context.commands;
    command_context.mode=context.draw.mode;
    command_context.main_prefix=state.main_prefix;command_context.draw_count=state.draw_count;
    command_context.mask_arena_address=context.draw.mask_arena_address;
    command_context.coverage=state.coverage;command_context.override_slots=state.override_slots;
    command_context.overrides=state.overrides;
    source_bsp_build_part_commands(commands,state.units,state.arena,world.commands,
        command_context,context.command_selector,world.object_commands);
    result.opaque_count=source_effect_light_opaque_select(commands.banks[1],state.units,world.metadata,
        world.opaque_geometry,opaque);
    return result;
}
}
