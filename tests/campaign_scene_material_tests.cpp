#include "halo2/source_campaign_model.hpp"
#include "halo2/content.hpp"
#include "halo2/original_data.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/world_material.hpp"
#include <algorithm>
#include <iostream>
#include <set>
#include <string_view>

using namespace halo2;
namespace {
void check_part_submission(const Cache &cache,std::uint32_t definition,bool anchor) {
    SourceCampaignModel model(cache,definition);Mesh materials;
    model.prepare_materials(cache,materials);
    const auto initial=cache.load_initial_model_definition(definition);
    std::set<unsigned> sections;
    for(const auto &origin:model.hierarchy().vertex_origins)sections.insert(origin.section);
    if(sections.empty() || model.hierarchy().mesh.vertices.empty())
        throw FormatError("Part submission fixture has no authored geometry");
    // The real cinematic helper has drawable triangles in the resource bank,
    // but its authored lightmap-only part must produce no ordinary draw.
    // A real council Elite is the positive control for the same handoff.
    if(anchor && !std::ranges::all_of(model.hierarchy().mesh.vertices,[](const auto &vertex) {
        return vertex.source_part_type==5;
    }))throw FormatError("Cinematic anchor fixture no longer has its authored type5 part");
    if(!anchor && !std::ranges::any_of(model.hierarchy().mesh.vertices,[](const auto &vertex) {
        return vertex.source_part_type==2 || vertex.source_part_type==3;
    }))throw FormatError("Council Elite fixture lacks a visible authored part");
    const auto nodes=pose_model_nodes_world(model.hierarchy(),{});
    SourceModelShaderView view;
    view.shader=source_shader_lod_view(0,480,false,false);
    view.object_coverage_multiplier=1.f;
    view.part_shader_multiplier=source_shader_lod_player_multiplier(1);
    std::vector<SourceLiveModelBaseDrawTu15> draws;
    for(const auto section:sections) {
        SourceLiveModelBaseDrawTu15 draw;
        draw.object=0x12340001u;draw.definition=definition;draw.lod=5;
        draw.lighting_class=2;draw.override_slot=255;draw.coverage=255;
        draw.unit.words[0]=(2u<<29)|1u;draw.unit.words[1]=initial.render_model;
        draw.unit.words[3]=section<<9;draw.geometry.geometry.resident=true;
        draws.push_back(draw);
    }
    const auto cpu=model.draw(draws.front().object,draws,nodes,view);
    for(const bool allow_gpu:{false,true}) {
        const auto deferred=model.draw_deferred(draws.front().object,draws,nodes,view,nullptr,allow_gpu);
        if(deferred.vertices.size()!=cpu.size())
            throw FormatError("Campaign CPU/deferred part submission counts differ");
        if(anchor && (!deferred.vertices.empty() || !deferred.deferred_skins.empty()))
            throw FormatError("Cinematic anchor submitted ordinary geometry");
        if(!anchor && deferred.vertices.empty())
            throw FormatError("Campaign part filter removed the visible council Elite control");
        for(std::size_t n=0;n<cpu.size();++n) {
            const auto &a=cpu[n],&b=deferred.vertices[n];
            if(a.source_section!=b.source_section || a.source_part!=b.source_part || a.material!=b.material)
                throw FormatError("Campaign CPU/deferred selection changed authored part order/materials");
        }
    }
    std::cout<<"PASS "<<(anchor?"cinematic anchor":"visible council Elite")<<": "
        <<model.hierarchy().mesh.vertices.size()<<" authored vertices, "<<cpu.size()
        <<" submitted CPU vertices; both deferred policies agree\n";
}
}
int main(int argc,char **argv)try {
    if(argc<3 || argc>4)throw FormatError("Supply original opening map and TU1.5 XBE");
    if(std::string_view(argv[2]).empty() || !std::filesystem::exists(argv[1]))return 77;
    const bool old_surface=argc==4 && std::string_view(argv[3])=="--old-surface";
    const auto original=OriginalData::load_xbe(argv[2]);
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path(),original);
    const auto content=cache.load_content(0,false);
    const auto placement=std::ranges::find_if(content.objects,[](const auto &object) {
        return object.type==6 && object.definition==0xe391021cu;
    });
    if(placement==content.objects.end())throw FormatError("Original council Elite prop missing");
    auto preview=cache.load_model(placement->definition,true,placement->variant);
    unsigned placements=0;
    for(const auto &object:content.objects)if(object.type==6 && object.definition==placement->definition) {
        if(!source_campaign_lit_scenery(preview,object))
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
    std::optional<std::uint32_t> anchor;
    for(const auto &tag:cache.select_tags({.require_address=true}))
        if(tag.group=="scen" && (tag.name=="cinematic_anchor" || tag.name.ends_with("\\cinematic_anchor"))) {
            if(anchor)throw FormatError("Opening map has ambiguous cinematic anchor definitions");
            anchor=tag.datum;
        }
    if(!anchor)throw FormatError("Opening map has no authored cinematic anchor definition");
    check_part_submission(cache,*anchor,true);
    check_part_submission(cache,placement->definition,false);
    std::cout<<"Boundary: resource/caller admission only; live source colors/light context and final pixels require visible validation\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
