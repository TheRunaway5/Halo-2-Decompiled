#include "halo2/pstencil/campaign_materials.hpp"
#include "halo2/source_bsp_resident_plan.hpp"
#include "../material_frame_selection.hpp"
#include <chrono>
#include <cstdlib>
#include <fstream>

namespace halo2::pstencil {
struct CampaignMaterialUpdates::Impl {
    std::unique_ptr<MaterialFrameSelection> selection;
    std::ofstream trace;
    bool pending{};
    unsigned requests{};
    double immediate_ms{};
    using Clock=std::chrono::steady_clock;
    static double milliseconds(Clock::time_point start) {
        return std::chrono::duration<double,std::milli>(Clock::now()-start).count();
    }
    std::vector<unsigned> collect(std::span<const Material> materials,const CampaignMaterialConsumers& c) {
        if(!selection)selection=std::make_unique<MaterialFrameSelection>(materials);
        // Campaign producers may replace child graphs. Use the general check,
        // rather than assuming the multiplayer detach-only lifetime contract.
        selection->begin_frame(materials);
        selection->add(c.world);selection->add(c.debug);selection->add(c.foreground);
        selection->add(c.background);selection->add(c.hud);
        for(const auto& view:c.views) {
            selection->add(view.vertices);selection->add(view.foreground);
            selection->add(view.background);selection->add(view.water_reflection_background);
            for(const auto& binding:view.source_bsp_bindings)if(binding.material)selection->add(*binding.material);
            for(const auto& object:view.objects)selection->add(object.vertices);
            for(const auto& draw:view.light_volumes)for(const auto& binding:draw.bitmap_materials)selection->add(binding.material);
            for(const auto& draw:view.effect_lights)selection->add(draw.material_slot);
            for(const auto& draw:view.raw_effect_lights)selection->add(draw.material_slot);
            for(const auto* bank:{&view.world_transparent,&view.foreground_transparent})if(*bank)
                for(const auto& binding:(*bank)->bindings)
                    if(const auto* geometry=std::get_if<SourceTransparentGeometryDraw>(&binding))selection->add(geometry->vertices);
            if(view.source_shadows)for(const auto& root:view.source_shadows->roots) {
                for(const auto& draw:root.casters)selection->add(draw.material_slot);
                for(const auto& draw:root.receivers)selection->add(draw.material_slot);
            }
            if(view.raw_source_shadows)for(const auto& root:view.raw_source_shadows->roots) {
                for(const auto& draw:root.casters)selection->add(draw.material_slot);
                for(const auto& draw:root.receivers)selection->add(draw.material_slot);
            }
        }
        return selection->roots();
    }
};
CampaignMaterialUpdates::CampaignMaterialUpdates():impl_(std::make_unique<Impl>()){}
CampaignMaterialUpdates::~CampaignMaterialUpdates()=default;
void CampaignMaterialUpdates::request(Renderer& renderer,std::span<const Material> materials,bool pstencil) {
    auto& i=*impl_;++i.requests;
    const auto* full=std::getenv("HALO2_PSTENCIL_CAMPAIGN_ALL_MATERIALS");
    if(pstencil && !(full && *full))i.pending=true;
    else {
        const auto start=Impl::Clock::now();renderer.update_material_constants(materials);
        i.immediate_ms+=Impl::milliseconds(start);
    }
}
void CampaignMaterialUpdates::flush(Renderer& renderer,std::span<const Material> materials,
    const CampaignMaterialConsumers& consumers,std::uint64_t frame) {
    auto& i=*impl_;if(!i.requests)return;
    const auto start=Impl::Clock::now();
    std::vector<unsigned> roots;
    if(i.pending) {
        roots=i.collect(materials,consumers);
        renderer.update_material_constants(materials,roots);
    }
    const auto elapsed=i.immediate_ms+Impl::milliseconds(start);
    if(const auto* path=std::getenv("HALO2_PSTENCIL_CAMPAIGN_MATERIAL_TRACE");path && *path) {
        if(!i.trace.is_open()) {
            i.trace.open(path);
            if(!i.trace)throw FormatError("Cannot retain campaign material update trace");
            i.trace<<"frame\trequests\tactive\tresident\tselected\tupdate_ms\n";
        }
        i.trace<<frame<<'\t'<<i.requests<<'\t'<<i.pending<<'\t'<<materials.size()<<'\t'
            <<(i.pending?roots.size():materials.size())<<'\t'<<elapsed<<'\n';
    }
    i.pending=false;i.requests=0;i.immediate_ms=0;
}
}
