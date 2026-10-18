#include "halo2/pstencil/projected_lights.hpp"
#include "halo2/pstencil/projected_light_material.hpp"
#include "halo2/pstencil/native_light_constants_tu15.hpp"
#include "halo2/pstencil/native_light_shape_tu15.hpp"
#include "halo2/pstencil/material.hpp"
#include "halo2/pstencil/projected_constants_2003.hpp"
#include "halo2/pstencil/caster.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_effect_light_opaque.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/menu_world_depth.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <tuple>

namespace halo2::pstencil {
struct ProjectedLights::Impl {
    const Cache& cache;unsigned bsp;
    SourceBspAdmissionBsp geometry;
    std::shared_ptr<SharedLightMaterials> shared;
    PointCasterBank casters;
    std::map<std::uint32_t,EffectLightDefinition> lights;
    // A resident slot owns immutable textures/vertex family/raster state.
    // Current light program/constants are supplied independently per packet.
    // Combined implementations do not bind a gel; other implementations share
    //residency only when their literal authored gel bitmap identity is equal.
    using Key=std::tuple<std::uint32_t,unsigned,unsigned,std::uint32_t>;
    struct Entry {unsigned slot;MenuWorldSourcePass pass;};
    std::map<Key,std::vector<Entry>> materials;
    struct Section {
        std::shared_ptr<const MenuWorldSourceSection> raw;
        SourceBspRangeGeometry ranges;
    };
    using SectionKey=std::tuple<MenuWorldSourceSectionKind,std::uint32_t,unsigned>;
    std::map<SectionKey,Section> sections;
    std::vector<std::string> gaps;
    std::optional<std::array<ShaderVector,9>> previous;
    Impl(const Cache& c,Mesh& mesh,unsigned index,std::shared_ptr<SharedLightMaterials> bank)
      :cache(c),bsp(index),geometry(c.load_source_bsp_admission(index)),
       shared(bank?std::move(bank):std::make_shared<SharedLightMaterials>(c,mesh)),casters(c,index) {
        for(const auto& tag:cache.tags())if(tag.group=="ligh") {
            const auto raw=cache.load_source_tag_metadata(tag.datum);
            const auto bytes=raw.bytes_at(raw.source_address(),228);const Bytes d(bytes);
            if(d.u16(4)!=2 || (d.u32(0)&(0x20u|0x80000u)) || d.u32(0xe0)!=0xffffffffu ||
               d.u32(0xc4) || d.u32(0xcc) || d.u32(0xd4))continue;
            EffectLightDefinition value;std::copy(bytes.begin(),bytes.end(),value.bytes.begin());
            value.flags=d.u32(0);value.radius=d.f32(0x18);value.specular_radius=d.f32(0x1c);
            value.duration=d.f32(0xb0);value.lens=d.u32(0x94);lights.emplace(tag.datum,std::move(value));
        }
        std::set<std::uint32_t> shaders;
        for(const auto& m:mesh.materials)if(m.source_shader_datum!=0xffffffffu)shaders.insert(m.source_shader_datum);
        EffectLightUniforms residency{}; // Residency seed only, never a live draw result.
        for(const auto shader:shaders) {
            try {
                const auto count=shared->authored(shader).lods.size();
                for(unsigned lod=0;lod<count;++lod) {
                    const auto authored=shared->selected(shader,lod);
                    for(const auto& [identity,definition]:lights) {
                    if(supports_projected_light_response_2003(definition))try {
                        shared->preload_specular(shader,lod,identity,definition);
                    }catch(const FormatError& e) {
                        gaps.push_back(std::to_string(identity)+"/"+authored->name+"/"+std::to_string(lod)+"/2003 response: "+e.what());
                    }
                    for(unsigned features:{0u,2u,4u,6u}) {
                        const auto implementation=projected_light_implementation(definition.flags,features,authored->shader_switches[4]!=0);
                        const auto k=key(shader,lod,implementation,definition);
                        if(materials.contains(k))continue;
                        try {
                            auto prepared=prepare_projected_light_material(cache,*authored,definition,residency,implementation,features);
                            std::vector<Entry> entries;
                            for(unsigned pass=0;pass<prepared.size();++pass) {
                                auto selected=load_menu_world_source_pass(cache,shader,{lod,12,pass,implementation});
                                (void)source_effect_light_draw(unsigned(mesh.materials.size()),prepared[pass],{});
                                entries.push_back({unsigned(mesh.materials.size()),std::move(selected)});
                                mesh.materials.push_back(std::move(prepared[pass]));
                            }
                            materials.emplace(k,std::move(entries));
                        }catch(const FormatError& e) {
                            gaps.push_back(std::to_string(identity)+"/"+authored->name+"/"+std::to_string(lod)+"/"+std::to_string(implementation)+": "+e.what());
                        }
                    }
                    }
                }
            }catch(const FormatError& e){gaps.push_back(std::to_string(shader)+": "+e.what());}
        }
    }
    static Key key(std::uint32_t shader,unsigned lod,unsigned implementation,const EffectLightDefinition& definition) {
        const auto gel=(implementation==2 || implementation==3)?Bytes(definition.bytes).u32(0x7c):0u;
        return {shader,lod,implementation,gel};
    }
    const Section& section(const SourceCullDrawUnit& unit) {
        MenuWorldSourceSectionKind kind;std::uint32_t model;unsigned ordinal;
        if(unit.words[1]!=0xffffffffu) {
            kind=MenuWorldSourceSectionKind::render_model;model=unit.words[1];ordinal=unit.words[3]>>9&511u;
        }else {
            model=0xffffffffu;const bool instance=unit.words[0]&0x1000u;
            kind=instance?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster;
            ordinal=instance?unsigned(geometry.instances.at(unit.words[3]>>18&2047u).definition):(unit.words[3]>>9&511u);
        }
        const SectionKey k{kind,model,ordinal};auto found=sections.find(k);
        if(found==sections.end()) {
            auto raw=std::make_shared<const MenuWorldSourceSection>(model!=0xffffffffu?
                load_menu_world_source_model_section(cache,model,ordinal):load_menu_world_source_bsp_section(cache,bsp,kind,ordinal));
            auto ranges=model!=0xffffffffu?source_model_range_geometry(*raw):source_bsp_range_geometry(*raw);
            found=sections.emplace(k,Section{std::move(raw),std::move(ranges)}).first;
        }
        return found->second;
    }
};
ProjectedLights::ProjectedLights(const Cache& cache,Mesh& mesh,unsigned bsp,std::shared_ptr<SharedLightMaterials> bank)
    :impl_(std::make_unique<Impl>(cache,mesh,bsp,std::move(bank))){}
ProjectedLights::~ProjectedLights()=default;
const std::vector<std::string>& ProjectedLights::unavailable()const{return impl_->gaps;}
ProjectedLightFrame ProjectedLights::draw(PhysicsLiveWorldTu15& world,const SourceEffectLightViewRecord& light,
    float selected_fade,const NativeLightReceiver& receiver,const NativeLightGeometry& geometry,
    const SourceViewProjectionBank& camera,Vec3 eye,std::array<std::int16_t,4> viewport,
    std::array<std::uint32_t,2> surface,std::vector<PointCaster>& point_casters,std::uint32_t domain) {
    auto& i=*impl_;const auto found=i.lights.find(light.definition_identity);
    if(found==i.lights.end())throw FormatError("Selected projected light has no admitted native material/gel owner");
    const auto& definition=found->second;
    if(!i.previous) {
        i.previous.emplace();
        // Literal4A3BD destination. Preserve the real caller's unwritten words
        //in this optional renderer's own consecutive light-uniform bank.
        std::memcpy(i.previous->data(),world.memory().view(0x49219cu,sizeof(*i.previous)).data(),sizeof(*i.previous));
    }
    const auto uniforms=native_light_constants_tu15(world,light.runtime.identity,light.fades,selected_fade,*i.previous);
    i.previous=uniforms.constants;
    const auto shape=native_light_shape_tu15(world,light.runtime.identity);
    std::optional<EffectLightUniforms> response;
    if(supports_projected_light_response_2003(definition)) {
        const auto word=[&](unsigned p){return Bytes(world.memory().view(p,4)).u32(0);};
        const auto pool=word(0x4ecbdcu),records=word(pool+0x44u);
        const auto record=records+(light.runtime.identity&65535u)*272u;
        if(Bytes(world.memory().view(record,2)).u16(0)!=std::uint16_t(light.runtime.identity>>16))
            throw FormatError("2003 projected response lost the actual native light generation");
        std::array<float,16> frame;
        std::memcpy(frame.data(),world.memory().view(record+0x84u,sizeof(frame)).data(),sizeof(frame));
        std::array<ShaderVector,6> previous;
        std::copy_n(uniforms.constants.begin(),previous.size(),previous.begin());
        const auto adapted=projected_constants_2003(shape.words,frame,Bytes(definition.bytes).u16(0x8e),
            uniforms.constants[5][3],previous);
        response=uniforms;
        std::copy(adapted.begin(),adapted.end(),response->constants.begin());
    }
    const Vec3 origin{std::bit_cast<float>(shape.words[11]),std::bit_cast<float>(shape.words[12]),std::bit_cast<float>(shape.words[13])};
    MenuWorldSourceConstants constants{};
    const auto bank=source_view_shader_bank(camera,1024.f,eye,
        {float(viewport[1]),float(viewport[0]),float(viewport[3]-viewport[1]),float(viewport[2]-viewport[0])},false);
    std::copy(bank.begin(),bank.end(),constants.begin());
    auto stencil_light=light.runtime;stencil_light.position=origin;
    i.casters.collect(geometry.retained.state,&world,stencil_light,constants,domain,{},point_casters);
    const auto selected=native_projected_light_selection_tu15(world,light.runtime.identity,receiver);
    ProjectedLightFrame out;out.features=selected.features;out.selected_commands=selected.records.size();out.color_shift=uniforms.color_shift;
    const auto& state=geometry.retained.state;const auto& commands=geometry.commands.banks[1];
    std::map<unsigned,EffectLightRawDraw> bindings;
    // Commands for different parts can share the same shader/LOD/pass. Keep
    // one completed metadata bank per light, with image residency independent.
    std::map<Impl::Key,std::vector<Material>> completed_materials;
    const auto ranges=source_effect_light_ordered_ranges(commands,state.units,selected.records,state.arena,0x559b54u,
        [&](unsigned command,unsigned unit_index,unsigned vertex_class) {
            const auto& c=commands.storage.at(command).words;const auto& unit=state.units.at(unit_index);
            const auto& section=i.section(unit);const auto part=c[3]&255u;
            const auto descriptor=std::find_if(selected.records.begin(),selected.records.end(),
                [&](const auto& r){return std::uint16_t(r.words[0])==command;});
            if(descriptor==selected.records.end())throw FormatError("Projected range lost its native selected command");
            const auto implementation=descriptor->words[1]&255u;
            const auto lod=c[5]>>8&255u,pass_index=c[5]&255u;
            const auto material=i.materials.find(Impl::key(c[2],lod,implementation,definition));
            if(material==i.materials.end() || pass_index>=material->second.size())
                throw FormatError("Native projected shader/LOD/pass was not admitted during residency: "+std::to_string(c[2])+"/"+std::to_string(lod)+"/"+std::to_string(implementation));
            const auto& slot=material->second[pass_index];
            if(descriptor->words[4]!=slot.pass.implementation_address())
                throw FormatError("Native projected descriptor differs from its actual compiled implementation pointer");
            std::vector<std::array<float,12>> palette;
            MenuWorldSourceOwner owner;
            if(unit.words[1]!=0xffffffffu) {
                const auto live=source_live_model_command_geometry_tu15(world,unit);
                if(!live.geometry.resident)return SourceOpaqueRangeGeometry{&section.ranges,false};
                palette=source_live_model_palette_rows_tu15(world,unit,live);
                const auto object=Bytes(world.memory().view(unit.words[4],4)).u32(0);(void)world.source_object_body(object);
                owner={MenuWorldSourceOwnerKind::dynamic_object,section.raw->owner_datum(),object&65535u,object};
            }else {
                const bool instance=unit.words[0]&0x1000u;const auto ordinal=instance?(unit.words[3]>>18&2047u):section.raw->section();
                owner={instance?MenuWorldSourceOwnerKind::bsp_instance:MenuWorldSourceOwnerKind::bsp_cluster,section.raw->owner_datum(),ordinal,{}};
                if(instance)palette.push_back(load_menu_world_source_bsp_owner(i.cache,i.bsp,ordinal).rows);
            }
            auto completed_at=completed_materials.find(material->first);
            if(completed_at==completed_materials.end()) {
                const auto authored=i.shared->selected(c[2],lod);
                completed_at=completed_materials.emplace(material->first,complete_projected_light_constants(
                    i.cache,*authored,definition,uniforms,implementation,selected.features)).first;
            }
            const auto& completed=completed_at->second;
            if(pass_index>=completed.size())throw FormatError("Native projected material lost its selected pass");
            const auto source_constants=source_effect_light_raw_constants(constants,completed[pass_index]);
            const auto topology=source_effect_light_part_ranges(unit,section.ranges,part,state.arena,0x559b54u).topology;
            auto draw=section.raw->kind()==MenuWorldSourceSectionKind::bsp_cluster?
                menu_world_source_cluster_draw(section.raw,part,topology):menu_world_source_draw(section.raw,part,topology,palette);
            auto source=menu_world_source_binding_class(i.cache,owner,std::move(draw),slot.pass.vertex_shader(),vertex_class,source_constants);
            MenuWorldRasterContext raster;raster.viewport_reset(surface);raster.game_initialize_offset();
            raster.layer_begin(MenuWorldRasterLayer::sphere_diffuse_12);
            raster.entry_begin(menu_world_raster_entry_from_source_flags(unit.words[0],false,false));
            auto packet=raster.snapshot({MenuWorldRasterPrimitive::ordinary_triangles,MenuWorldRasterPolygon::fill,std::array<std::int32_t,2>{1,1},true});raster.entry_end();
            packet.window_exclusive=false;for(auto& rect:packet.regions)rect={0,0,0,0};
            packet.regions[0]={viewport[1],viewport[0],viewport[3],viewport[2]};
            const auto depth=menu_world_depth_clip_planes({.015625f,1024.f,MenuWorldDepthBoundary::none,0,false});
            auto bound=source_effect_light_raw_draw(slot.slot,completed[pass_index],slot.pass,std::move(source),packet,
                {0,16777215.f,depth.minimum_w,depth.maximum_w},{1,1});
            bound.light_identity=light.runtime.identity;bound.light_definition=light.definition_identity;
            bound.point_light_position=origin;bound.pstencil_light_domain=domain;
            bound.pstencil_casts_shadow=shadow_casting_part(section.raw->parts().at(part).type);
            if(response)if(auto response_passes=i.shared->specular(c[2],lod,light.definition_identity,*response)) {
                // A native combined implementation has a separate gel pass.
                // The recovered dependent7 already includes that gel, so
                // replace the entire native sequence exactly once.
                bound.pstencil_specular=pass_index==0?std::move(response_passes):
                    std::make_shared<const std::vector<EffectLightDraw>>();
            }
            bindings.emplace(command,std::move(bound));return SourceOpaqueRangeGeometry{&section.ranges,true};
        });
    for(const auto& range:ranges) {
        const auto& original=bindings.at(range.command);
        for(const auto& indices:range.selected.ranges) {
            auto packet=original;auto draw=packet.source.draw();draw.range=indices;
            packet.source=menu_world_source_binding_class(i.cache,packet.source.owner(),std::move(draw),
                packet.source.vertex_shader(),range.vertex_class,packet.source.constants());
            out.draws.push_back(std::move(packet));
        }
    }
    return out;
}
}
