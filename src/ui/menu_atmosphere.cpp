#include "halo2/atmosphere.hpp"
#include <algorithm>
#include <cmath>
namespace halo2 {
void prepare_menu_bsp_fog(const Cache &cache,Mesh &mesh) {
    // load_menu_atmosphere rejects regional fog and nonzero fog-tag flags.
    const auto atmosphere=cache.load_menu_atmosphere();
    const auto second=cache.load_bitmap(atmosphere.gradient_datum,1);
    for(auto &material:mesh.materials) {
        if(material.lods.empty())continue;
        const auto &lod=material.lods.front();
        if(!(lod.available_layers&(1u<<16)) || lod.layers.count()<=16)continue;
        const auto index=lod.layers.first()+16;
        if(index>=material.layers.size())throw FormatError("menu planar layer outside template");
        const auto range=material.layers[index];
        const auto selected=std::find_if(material.passes.begin(),material.passes.end(),[&](const auto &pass){
            return pass.template_pass>=range.first() && pass.template_pass<range.first()+range.count() && pass.implementation==0;
        });
        if(selected==material.passes.end())throw FormatError("menu planar implementation missing");
        const auto &pass=*selected;
        if(pass.name!="shaders\\shader_passes\\fog\\fog_planar" || !pass.fragments.empty() ||
           pass.textures.size()!=2 || pass.textures[0].external!=14 || pass.textures[0].stage!=0 ||
           pass.textures[1].external!=15 || pass.textures[1].stage!=1)
            throw FormatError("unsupported menu BSP planar pass");
        auto target=std::make_shared<Material>();
        target->name=material.name+" [planar fog]";
        target->forward_pass=true;target->forward_vertex=5;target->forward_blend=5;
        target->forward_pass_index=0;target->passes.push_back(pass);
        target->forward_program=pass.program;target->vertex_program=pass.vertex_code;
        target->vertex_declaration=pass.vertex_declaration;
        target->program_constants=pass.constants;
        target->diffuse=atmosphere.gradient;target->detail=second;
        target->forward_raster.depth_function=0x202;target->forward_raster.depth_write=0;
        for(const auto &state:pass.states) {
            if(state.index==57)target->forward_raster.depth_function=state.value;
            if(state.index==64)target->forward_raster.depth_write=state.value;
            if(state.index==67)target->forward_raster.color_write=state.value;
            if((state.index==62 && state.value!=0x8001) || (state.index==63 && state.value!=0x303) ||
               (state.index==75 && state.value!=0x00ffffff))throw FormatError("unsupported planar blend");
        }
        std::copy(pass.texture_states.begin(),pass.texture_states.end(),target->texture_states.begin());
        material.planar_fog_pass=std::move(target);
    }
}
MenuAtmosphere Cache::load_menu_atmosphere() const {
    if(name_!="mainmenu" || bsps_.size()!=1)
        throw FormatError("menu atmosphere requires the retail main-menu BSP");
    const auto sky=load_visible_sky(0);
    if(!sky || !sky->atmospheric_fog)
        throw FormatError("menu atmosphere is missing its visible sky");
    const Bytes data(cache_bytes());
    
    const auto scenario=query_tag({.group=tag_groups::scenario, .name=scenario_});
    if(!scenario)throw FormatError("missing menu atmosphere scenario");
    const auto scnr=metadata_offset(scenario->address,scenario->size);
    const auto sky_tag=query_tag({.group=tag_groups::sky, .name=sky->name});
    if(!sky_tag)throw FormatError("missing menu atmosphere sky tag");
    const auto sky_root=metadata_offset(sky_tag->address,sky_tag->size);
    if(data.u32(sky_root+0x50) || data.u32(sky_root+0x58))
        throw FormatError("menu sky requires secondary or sky fog aggregation");
    const auto [lights,light]=TagAddressSpace(data, metadata_magic_).block(sky_root+0x78, 52);
    for(std::size_t i=0;i<lights;++i)
        if(data.u32(light+i*52+0x1c) || data.u32(light+i*52+0x24))
            throw FormatError("menu sky requires directional fog aggregation");
    const auto &bsp=bsps_.front();
    const Bytes metadata(data.slice(bsp.file_offset,bsp.size));
    const auto [clusters,cluster]=TagAddressSpace(metadata, bsp.memory_address).block(16+156, 176);
    const auto [planes,plane]=TagAddressSpace(metadata, bsp.memory_address).block(16+0x64, 24);
    if(!clusters || planes!=1)throw FormatError("menu atmosphere requires one visible fog plane");
    for(std::size_t i=0;i<clusters;++i) {
        const auto at=cluster+i*176;
        const auto byte=[&](std::size_t off){return std::to_integer<unsigned>(metadata.slice(at+off,1)[0]);};
        if(byte(0x6e)!=6 || byte(0x6f)!=255 || byte(0x71)!=0 ||
            (byte(0x70)!=0 && byte(0x70)!=128))
            throw FormatError("menu atmosphere requires regional fog selection");
    }
    const auto [palettes,palette]=TagAddressSpace(data, metadata_magic_).block(scnr+0x348, 16);
    const auto selected=metadata.u16(plane);
    if(selected>=palettes)throw FormatError("menu fog plane palette is out of range");
    const auto datum=data.u32(palette+selected*16+8);
    const auto fog_tag=query_tag({.handle=TagHandle{datum}, .group=tag_groups::planar_fog});
    if(!fog_tag || fog_tag->size<96)throw FormatError("missing menu planar fog");
    const auto fog=metadata_offset(fog_tag->address,fog_tag->size);
    if(data.u16(fog) || data.u32(fog+0x30) || data.f32(fog+0x18)!=0 || data.f32(fog+0x1c)!=0)
        throw FormatError("menu planar fog requires flags, patchy fog or custom transitions");
    MenuAtmosphere result{*sky->atmospheric_fog,
        {{metadata.f32(plane+4),metadata.f32(plane+8),metadata.f32(plane+12),metadata.f32(plane+16)},
         {data.f32(fog+0x24),data.f32(fog+0x28),data.f32(fog+0x2c)},
         data.f32(fog+0xc),data.f32(fog+0x10),data.f32(fog+0x14),data.f32(fog+0x20)}};
    for(float value:{result.planar.plane[0],result.planar.plane[1],result.planar.plane[2],result.planar.plane[3],
        result.planar.color.x,result.planar.color.y,result.planar.color.z,result.planar.maximum_density,
        result.planar.opaque_distance,result.planar.opaque_depth,result.planar.eye_offset})
        if(!std::isfinite(value))throw FormatError("nonfinite menu planar fog");
    if(result.atmospheric.maximum_density<=0 || result.planar.maximum_density<=0 ||
       result.planar.opaque_distance<=.0001f || result.planar.opaque_depth<=.0001f)
        throw FormatError("unsupported disabled menu fog layer");
    const auto globals=query_tag({.group=tag_groups::globals});
    if(!globals)throw FormatError("missing rasterizer fog globals");
    const auto globals_root=metadata_offset(globals->address,globals->size);
    const auto [rasterizers,rasterizer]=TagAddressSpace(data, metadata_magic_).block(globals_root+0x108, 0x1c8);
    if(rasterizers!=1)throw FormatError("unsupported rasterizer fog globals count");
    result.gradient_datum=data.u32(rasterizer+0x14);
    result.gradient=load_bitmap(result.gradient_datum,0);
    if(result.gradient.width!=16 || result.gradient.height!=16 || result.gradient.format!=11)
        throw FormatError("unsupported retail fog lookup image");
    const auto [patches,patch]=TagAddressSpace(data, metadata_magic_).block(sky_root+0x60, 80);
    if(patches!=1)throw FormatError("unsupported title patchy-fog aggregation");
    const auto patch_datum=data.u32(patch+0x4c);
    const auto patch_tag=query_tag({.handle=TagHandle{patch_datum}, .group=tag_groups::patchy_fog});
    if(!patch_tag || patch_tag->size!=80)throw FormatError("missing title patchy fog");
    const auto fpch=metadata_offset(patch_tag->address,patch_tag->size);
    if(data.u32(fpch)!=1 || data.f32(fpch+0x28)!=0 || data.f32(fpch+0x30)!=0)
        throw FormatError("title patchy fog requires separate depths and static bitmap animation/wind");
    PatchyFog patchy{{data.f32(patch),data.f32(patch+4),data.f32(patch+8)},
        {data.f32(patch+0x18),data.f32(patch+0x1c)},{data.f32(patch+0x20),data.f32(patch+0x24)},
        data.u32(fpch),data.f32(fpch+4),data.f32(fpch+8),data.f32(fpch+12),data.f32(fpch+16),
        data.f32(fpch+0x1c),data.f32(fpch+0x20),data.f32(fpch+0x24),data.f32(fpch+0x28),
        {data.f32(fpch+0x44),data.f32(fpch+0x48),data.f32(fpch+0x4c)},{}};
    for(float v:{patchy.color.x,patchy.color.y,patchy.color.z,patchy.density[0],patchy.density[1],
        patchy.distance[0],patchy.distance[1],patchy.rotation,patchy.strafe,patchy.zoom,patchy.noise_scale,
        patchy.vertical_forward,patchy.vertical_up,patchy.opacity_up,
        patchy.constant_wind.x,patchy.constant_wind.y,patchy.constant_wind.z})
        if(!std::isfinite(v))throw FormatError("nonfinite title patchy fog");
    if(patchy.distance[1]-patchy.distance[0]<=.0001f || patchy.density[0]<0 ||
        patchy.density[1]>1 || patchy.density[0]>patchy.density[1])throw FormatError("invalid title patchy fog ranges");
    const auto noise_datum=data.u32(fpch+0x18);
    const auto noise_tag=query_tag({.handle=TagHandle{noise_datum}, .group=tag_groups::bitmap});
    if(!noise_tag)throw FormatError("missing patchy noise bitmap");
    const auto noise_root=metadata_offset(noise_tag->address,noise_tag->size);
    if(data.u32(noise_root+0x44)!=1)throw FormatError("title patchy fog requires one static noise frame");
    patchy.noise.push_back(prepare_menu_noise_texture(load_bitmap(noise_datum,0)));
    result.patchy=std::move(patchy);
    return result;
}
}
