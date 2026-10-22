#pragma once
#include "halo2/pstencil/campaign_materials.hpp"
#include "halo2/source_bsp_resident_plan.hpp"
#include "rendering/renderer/material_frame_selection.hpp"

namespace halo2 {
// Select the completed frame's resident roots, including every owner of any
// shared child pass. Selection does not advance animation or copy Materials.
class CampaignMaterialSelection {
    MaterialFrameSelection selection_;
public:
    explicit CampaignMaterialSelection(std::span<const Material> materials):selection_(materials){}
    std::vector<unsigned> collect(std::span<const Material> materials,
                                 const pstencil::CampaignMaterialConsumers& c) {
        // Campaign producers can replace entire child graphs, not only detach.
        selection_.begin_frame(materials);
        for(const auto vertices:{c.world,c.debug,c.foreground,c.background,c.hud})selection_.add(vertices);
        const auto light_volume=[&](const SourceLightVolumeDraw& draw) {
            selection_.add(draw.material);
            for(const auto& binding:draw.bitmap_materials)selection_.add(binding.material);
        };
        for(const auto& view:c.views) {
            for(const auto vertices:{view.vertices,view.foreground,view.background,
                                     view.water_reflection_background})selection_.add(vertices);
            for(const auto& binding:view.source_bsp_bindings)if(binding.material)selection_.add(*binding.material);
            for(const auto& object:view.objects)selection_.add(object.vertices);
            for(const auto& draw:view.light_volumes)light_volume(draw);
            for(const auto& draw:view.effect_lights)selection_.add(draw.material_slot);
            for(const auto& draw:view.raw_effect_lights) {
                selection_.add(draw.material_slot);
                if(draw.pstencil_specular)for(const auto& pass:*draw.pstencil_specular)
                    selection_.add(pass.material_slot);
            }
            for(const auto& draw:view.lens_draws)selection_.add(draw.material);
            for(const auto* bank:{&view.world_transparent,&view.foreground_transparent})if(*bank) {
                // Prefix and null callbacks are not submitted by Renderer.
                const auto& b=**bank;
                if(b.first>b.commands.size() || b.commands.size()!=b.bindings.size())
                    throw FormatError("campaign transparent material selection lost its command range");
                for(std::size_t j=b.first;j<b.commands.size();++j)if(b.commands[j].callback) {
                    const auto& binding=b.bindings[j];
                    if(const auto* geometry=std::get_if<SourceTransparentGeometryDraw>(&binding))
                        selection_.add(geometry->vertices);
                    else if(const auto* shader=std::get_if<SourceTransparentShaderGeometryDraw>(&binding))
                        selection_.add(shader->geometry.vertices);
                    else if(const auto* light=std::get_if<SourceTransparentLightVolumeDraw>(&binding)) {
                        if(!light->draw)throw FormatError("campaign transparent light-volume lost its owner");
                        light_volume(*light->draw);
                    }
                }
            }
            if(view.source_shadows)for(const auto& root:view.source_shadows->roots) {
                for(const auto& draw:root.casters)selection_.add(draw.material_slot);
                for(const auto& draw:root.receivers)selection_.add(draw.material_slot);
            }
            if(view.raw_source_shadows)for(const auto& root:view.raw_source_shadows->roots) {
                for(const auto& draw:root.casters)selection_.add(draw.material_slot);
                for(const auto& draw:root.receivers)selection_.add(draw.material_slot);
            }
        }
        return selection_.roots();
    }
};
}
