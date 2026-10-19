#include "halo2/source_campaign_model.hpp"
#include "halo2/content.hpp"
#include "halo2/original_data.hpp"
#include "halo2/world_material.hpp"
#include <algorithm>
#include <iostream>
#include <string_view>

using namespace halo2;
int main(int argc,char **argv)try {
    if(argc<3 || argc>4)throw FormatError("Supply original opening map and TU1.5 XBE");
    if(std::string_view(argv[2]).empty() || !std::filesystem::exists(argv[1]))return 77;
    const bool old_surface=argc==4 && std::string_view(argv[3])=="--old-surface";
    const auto original=OriginalData::load_xbe(argv[2]);
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path(),original);
    const auto content=cache.load_content(0,false);
    const auto lighting=cache.load_bsp_lighting(0);
    const auto placement=std::ranges::find_if(content.objects,[](const auto &object) {
        return object.type==6 && object.definition==0xe391021cu;
    });
    if(placement==content.objects.end())throw FormatError("Original council Elite prop missing");
    auto preview=cache.load_model(placement->definition,true,placement->variant);
    unsigned placements=0;
    for(const auto &object:content.objects)if(object.type==6 && object.definition==placement->definition) {
        if(!source_campaign_dynamic_scenery(cache,preview,lighting,object))
            throw FormatError("Council props did not reach their source dynamic-lighting owner");
        ++placements;
    }
    if(placements!=40)throw FormatError("Original council prop placement census differs: "+std::to_string(placements));
    Mesh resources;
    if(old_surface) {
        // Same shared fallback preparation used by the prior main caller for
        // non-script-lit SCEN props. No artificial failure/white pixel input.
        for(auto &material:preview.mesh.materials)prepare_independent_object_material(cache,material);
        resources.materials=std::move(preview.mesh.materials);
    } else {
        SourceCampaignModel model(cache,*placement);
        model.prepare_materials(cache,resources);
    }
    const auto helmet=std::ranges::find_if(resources.materials,[](const auto &material) {
        return material.source_shader_datum==0xe3a3022eu && material.selected_shader_lod==0;
    });
    if(helmet==resources.materials.end())throw FormatError("Original council helmet shader lost its material slot");
    if(!helmet->world_pass || !helmet->forward_pass)
        throw FormatError("Council helmet still uses the surface preview without authored lighting/environment passes");
    // Literal E1FD0088 LOD0 has layers1/3/10/11/12/13/16, no layer8.
    // Layer3 reads texture extern3, so the native material retains the layer1
    // base followed by a child with implementation2 and texture stage1's cube.
    if(helmet->lods.at(0).available_layers!=0x13c0au ||
       helmet->forward_pass_index>=helmet->passes.size())
        throw FormatError("Council helmet lost its literal selected shader bank");
    const auto &base=helmet->passes[helmet->forward_pass_index];
    if(base.template_pass!=0 || base.implementation!=0 || !helmet->environment_pass)
        throw FormatError("Council helmet lost its authored base/lighting dependency");
    const auto &lighting_pass=*helmet->environment_pass;
    if(lighting_pass.forward_pass_index>=lighting_pass.passes.size())
        throw FormatError("Council helmet lighting child lost its selected pass");
    const auto &pass=lighting_pass.passes[lighting_pass.forward_pass_index];
    if(pass.template_pass!=1 || pass.implementation!=2 || lighting_pass.detail.layers!=6 ||
       !lighting_pass.accumulation_texture || !helmet->world_overlays_after.empty())
        throw FormatError("Council helmet reflection/lighting differs: template="+std::to_string(pass.template_pass)+
            " implementation="+std::to_string(pass.implementation)+" stage1_layers="+std::to_string(lighting_pass.detail.layers)+
            " stage2_layers="+std::to_string(lighting_pass.bump.layers)+
            " accumulation="+std::to_string(lighting_pass.accumulation_texture));
    std::cout<<"PASS 40 literal council prop placements admit implementation2; helmet lighting and environment passes resident\n";
    std::cout<<"Boundary: resource/caller admission only; live source colors/light context and final pixels require visible validation\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
