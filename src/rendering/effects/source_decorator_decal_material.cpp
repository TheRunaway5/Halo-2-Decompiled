#include "halo2/source_decorator_decal_material.hpp"
#include "halo2/shader_animation.hpp"
#include <algorithm>
#include <utility>

namespace halo2 {
void validate_source_decorator_decal_material_tu15(const Material& material) {
    if(!material.forward_pass || material.source_shader_datum==0xffffffffu ||
       material.selected_shader_lod!=0 || material.lods.empty() ||
       material.forward_pass_index>=material.passes.size())
        throw FormatError("Decorator decal has no actual selected source material pass");
    const auto& pass=material.passes[material.forward_pass_index];
    const auto& lod=material.lods.front();
    if(!(lod.available_layers&(1u<<2)) || lod.layers.count()<=2 ||
       lod.layers.first()+2>=material.layers.size())
        throw FormatError("Decorator decal has no authored layer2");
    const auto layer=material.layers[lod.layers.first()+2];
    if(pass.template_pass<layer.first() || pass.template_pass-layer.first()>=layer.count() ||
       pass.implementation!=0 ||
       (pass.name!="shaders\\shader_passes\\overlay\\decorator_decal" &&
        pass.name!="shaders\\shader_passes\\overlay\\decorator_decal_multiply"))
        throw FormatError("Decorator decal differs from its original layer2 implementation");
    // Literal original Beaver Creek program SHA256fb001da8b50fcb09257ce5e079fb4381
    //3285772658cecfba827c9194e289bd4d; all qualified authored variants share it.
    std::uint64_t hash=14695981039346656037ull;
    for(const auto byte:material.vertex_program)
        hash=(hash^std::to_integer<unsigned>(byte))*1099511628211ull;
    if(material.vertex_program.size()!=212 || hash!=0xfdbc61d36bf6be78ull ||
       !material.vertex_declaration || material.vertex_declaration->class_index!=0 ||
       material.vertex_declaration->semantics!=std::vector<std::uint16_t>{0,3,9,14})
        throw FormatError("Decorator decal program or compiled declaration differs from TU1.5");
    // v2 supplies only TEX1 and v3.w is unread in this literal program. The
    // actual draw owner must still supply c53, which produces oD0 alpha.
    if(material.forward_program.words[54]!=1)
        throw FormatError("Decorator decal requires an unqualified additional texture coordinate owner");
    if(material.diffuse.layers!=1 || material.diffuse.depth ||
       material.diffuse.pixels.empty() || material.diffuse.mips.empty())
        throw FormatError("Decorator decal has no actual decoded two-dimensional image and mip chain");
}
Material source_decorator_decal_material_tu15(const Cache& cache,std::uint32_t shader) {
    auto material=cache.load_material(shader);
    if(material.lods.empty())throw FormatError("Decorator decal has no authored shader LOD");
    const auto& lod=std::as_const(material).lods.front();
    if(!(lod.available_layers&(1u<<2)) || lod.layers.count()<=2 ||
       lod.layers.first()+2>=material.layers.size())
        throw FormatError("Decorator decal has no authored layer2");
    const auto layer=std::as_const(material).layers[lod.layers.first()+2];
    std::optional<unsigned> selected;
    for(unsigned index=0;index<material.passes.size();++index) {
        const auto& pass=std::as_const(material).passes[index];
        if(pass.implementation==0 && pass.template_pass>=layer.first() &&
           pass.template_pass-layer.first()<layer.count()) {
            if(selected)throw FormatError("Decorator layer2 has more than one source implementation");
            selected=index;
        }
    }
    if(!selected)throw FormatError("Decorator decal has no selected layer2 implementation");
    const auto& pass=std::as_const(material).passes[*selected];
    if(!pass.vertex_externs.empty() || !pass.pixel_externs.empty() || !pass.state_externs.empty())
        throw FormatError("Decorator decal requires an unresolved source material extern");
    material.selected_shader_lod=0;material.forward_pass=true;material.world_pass=true;
    material.forward_pass_index=*selected;material.forward_vertex=3;
    material.forward_program=pass.program;material.vertex_program=pass.vertex_code;
    material.vertex_declaration=pass.vertex_declaration;material.program_constants=pass.constants;
    std::copy(pass.texture_states.begin(),pass.texture_states.end(),material.texture_states.begin());
    bool texture_bound=false;
    for(const auto& binding:pass.textures) {
        if(binding.stage>=4)throw FormatError("Decorator texture stage exceeds the source slots");
        if(!((pass.program.words[54]>>(binding.stage*5))&31u))continue;
        if(binding.stage!=0 || binding.external || binding.parameter==255 || texture_bound)
            throw FormatError("Decorator decal requires a different actual texture owner");
        const auto remap=std::ranges::find_if(pass.bitmap_remaps,[&](const auto& row) {
            return (row.destination&255u)==binding.parameter;
        });
        if(remap==pass.bitmap_remaps.end() || remap->source>=material.bitmaps.size())
            throw FormatError("Decorator bitmap remap escapes the authored material bank");
        const auto& bitmap=std::as_const(material).bitmaps[remap->source];
        material.diffuse=bitmap.datum==0xffffffffu?cache.load_scope_default_bitmap():
            cache.load_bitmap(bitmap.datum,bitmap.index);
        texture_bound=true;
    }
    if(!texture_bound)throw FormatError("Decorator decal has no actual stage0 image binding");
    unsigned source=1,destination=0,operation=0x8006;bool blend=false;
    material.forward_raster={};
    for(const auto& state:pass.states)switch(state.index) {
        case 57:material.forward_raster.depth_function=state.value;break;
        case 58:material.forward_raster.alpha_function=state.value;break;
        case 59:blend=state.value!=0;break;
        case 60:material.forward_raster.alpha_test=state.value;break;
        case 61:material.forward_raster.alpha_reference=state.value;break;
        case 62:source=state.value;break;
        case 63:destination=state.value;break;
        case 64:material.forward_raster.depth_write=state.value;break;
        case 67:material.forward_raster.color_write=state.value;break;
        case 74:operation=state.value;break;
        case 75:material.forward_blend_color=state.value;break;
        case 143:material.forward_raster.depth_enable=state.value;break;
        case 147:material.forward_raster.cull=state.value;break;
    }
    if(!blend || operation!=0x8006)
        throw FormatError("Decorator decal requires a different original blend operation");
    if(source==0x306 && destination==0)material.forward_blend=9;
    else if(source==0x302 && destination==0x303)material.forward_blend=1;
    else throw FormatError("Decorator decal has an unqualified original blend equation");
    validate_source_decorator_decal_material_tu15(material);
    prepare_bitmap_animation(cache,material);
    std::erase(material.rendering_gaps,"material uses an unsupported rendering pass");
    return material;
}
}
