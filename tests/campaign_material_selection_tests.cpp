#include "campaign/campaign_material_selection.hpp"
#include <iostream>
using namespace halo2;
int main()try {
    std::vector<Material> materials(40);
    CampaignMaterialSelection selection(materials);
    pstencil::CampaignMaterialConsumers consumers;
    ModelView view;consumers.views={&view,1};
    const auto expect=[&](std::initializer_list<unsigned> wanted) {
        if(selection.collect(materials,consumers)!=std::vector<unsigned>(wanted))
            throw FormatError("completed campaign consumer material closure differs");
    };
    expect({});
    std::array<Vertex,1> vertex{};vertex[0].material=1;
    for(auto* span:{&consumers.world,&consumers.debug,&consumers.foreground,&consumers.background,&consumers.hud,
                    &view.vertices,&view.foreground,&view.background,&view.water_reflection_background}) {
        *span=vertex;expect({1});*span={};
    }
    ModelObjectDraw object;object.vertices=vertex;view.objects={&object,1};expect({1});view.objects={};
    SourceBspPartBinding bsp;bsp.material=2;view.source_bsp_bindings={&bsp,1};expect({2});view.source_bsp_bindings={};
    EffectLightDraw effect;effect.material_slot=3;view.effect_lights={&effect,1};expect({3});view.effect_lights={};
    SourceLensDraw lens;lens.material=5;view.lens_draws={&lens,1};expect({5});view.lens_draws={};
    SourceLightVolumeDraw volume;volume.material=6;volume.bitmap_materials={{0,7},{1,8}};
    view.light_volumes={&volume,1};expect({6,7,8});view.light_volumes={};
    std::array<SourceTransparentCommand,5> commands{};
    for(auto& command:commands)command.callback=0x41020;
    std::array<SourceTransparentDrawBinding,5> bindings;
    // A discarded prefix and a null callback must not select invalid inputs.
    std::array<Vertex,1> invalid{};invalid[0].material=9999;
    bindings[0]=SourceTransparentGeometryDraw{.vertices=invalid};
    commands[1].callback=0;bindings[1]=SourceTransparentGeometryDraw{.vertices=invalid};
    bindings[2]=SourceTransparentGeometryDraw{.vertices=vertex};
    std::array<Vertex,1> shader_vertex{};shader_vertex[0].material=9;
    bindings[3]=SourceTransparentShaderGeometryDraw{.callback=0x41490,.geometry={.vertices=shader_vertex}};
    commands[3].callback=0x41490;commands[4].callback=source_light_volume_callback;
    bindings[4]=SourceTransparentLightVolumeDraw{&volume};
    for(auto* bank:{&view.world_transparent,&view.foreground_transparent}) {
        *bank=SourceTransparentDrawBank{commands,1,bindings};expect({1,6,7,8,9});bank->reset();
    }
    std::array<SourceShadowPlanDraw,2> shadow_draws{};shadow_draws[0].material_slot=10;shadow_draws[1].material_slot=11;
    SourceShadowPlanRoot shadow_root;shadow_root.casters={&shadow_draws[0],1};shadow_root.receivers={&shadow_draws[1],1};
    view.source_shadows.emplace().roots={&shadow_root,1};expect({10,11});view.source_shadows.reset();
    // Inactive owners sharing a child must retain full-bank last-writer order.
    auto child=std::make_shared<Material>();materials[1].environment_pass=child;
    materials[30].world_overlays_after.push_back(child);view.vertices=vertex;expect({1,30});
    // Copy-on-write and replacement graphs must refresh the retained cache.
    materials[30].world_overlays_after[0]=std::make_shared<Material>(*child);expect({1});
    materials[35].planar_fog_pass=child;expect({1,35});
    materials[1].environment_pass.reset();expect({1});
    std::cout<<"Campaign world/UI/object/BSP/transparent/shadow/light/lens consumers and changing shared-pass closure passed\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
