#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_model_effect_lights.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_effect_light_geometry_frame.hpp"
#include "halo2/source_tag_metadata_store.hpp"
#include "halo2/world_material.hpp"
#include "halo2/menu_world_depth.hpp"
#include <algorithm>
#include <cstring>
#include <map>
#include <tuple>

namespace halo2 {
struct SourceBspEffectLights::Impl {
    const Cache& cache;
    std::size_t bsp_index;
    SourceBspAdmissionBsp bsp;
    SourceTagMetadataStore store;
    SourceTagMetadataProviders metadata;
    std::vector<SourceBspDrawGeometry> clusters,definitions;
    std::vector<SourceBspDrawInstance> instances;
    std::vector<Transform> transforms;
    std::vector<std::array<float,12>> palettes;
    std::vector<SourceBspCommandGeometry> cluster_commands,definition_commands;
    std::vector<std::int16_t> instance_definitions;
    std::vector<SourceBspCommandMaterial> materials;
    std::vector<std::byte> visibility=std::vector<std::byte>(0x4204);
    std::map<std::uint32_t,SourceBspCommandShader> shaders;
    using SectionKey=std::pair<bool,unsigned>;
    struct Section {
        std::shared_ptr<const MenuWorldSourceSection> raw;
        SourceBspRangeGeometry ranges;
        SourceOpaqueGeometryMetadata metadata;
    };
    std::map<SectionKey,Section> sections;
    using ModelKey=std::pair<std::uint32_t,unsigned>;
    struct ModelSection {
        SourceBspRangeGeometry ranges;
        SourceOpaqueGeometryMetadata metadata;
        std::unique_ptr<SourceModelEffectLights> draw;
    };
    std::map<ModelKey,ModelSection> models;
    using MaterialKey=std::tuple<std::uint32_t,std::uint32_t,unsigned,unsigned>;
    struct Slot {unsigned material;Material authored;MenuWorldSourcePass pass;};
    std::map<MaterialKey,Slot> slots;
    std::map<std::uint32_t,EffectLightDefinition> lights;
    SourceDrawUnitState units;
    SourceBspRangeArena main_arena,light_arena;
    SourceBspPartCommandState commands;
    std::array<SourceOpaqueSortRecord,1024> opaque{};
    std::vector<std::string> unavailable;
    bool lightmap_matches{};
    // These are native bank identities. They are never borrowed reference-RAM
    // addresses, and no host pointer is narrowed to32 bits.
    static constexpr std::uint32_t main_arena_token=0x71000000,
        light_arena_token=0x71001000,instance_token=0x71002000;

    Impl(const Cache& c,Mesh& mesh,std::size_t index,std::span<const std::uint32_t> ids,std::span<const std::uint32_t> model_ids)
      :cache(c),bsp_index(index),bsp(c.load_source_bsp_admission(index)),
       store(std::shared_ptr<const Cache>(&c,[](const Cache*){}),{"shad","stem","spas","vrtx"}),metadata(store.providers()),
       clusters(bsp.clusters.size()),definitions(bsp.definitions.size()),instances(bsp.instances.size()),
       transforms(bsp.instances.size()),palettes(bsp.instances.size()),
       cluster_commands(clusters.size()),definition_commands(definitions.size()) {
        const auto owned=cache.load_source_bsp_metadata(index);
        const Bytes root(owned.bytes_at(owned.source_address(),owned.size()));
        const auto read=[&](auto p,auto n){return Bytes(owned.bytes_at(p,n));};
        const auto section=[&](bool instance,unsigned i,std::uint32_t address) {
            const auto kind=instance?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster;
            const auto m=read(address,0x6c);
            auto& target=(instance?definitions:clusters)[i];
            target.geometry_class=m.u16(0x14);target.mask_subparts=m.u16(0x24);
            if(!(instance?bsp.definitions[i].geometry_count:bsp.clusters[i].geometry_count))return;
            Section value;value.ranges=load_source_bsp_range_geometry(cache,index,kind,i);
            value.metadata=load_source_opaque_bsp_geometry_metadata(cache,index,kind,i);
            value.raw=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,index,kind,i));
            // This is actual owned native cache backing, loaded successfully
            // before the first frame. It is not an async completion fixture.
            target.resident=true;
            auto& command=(instance?definition_commands:cluster_commands)[i];
            command.resident=true;command.parts=value.ranges.parts;
            sections.emplace(SectionKey{instance,i},std::move(value));
        };
        const auto cp=root.u32(16+0xa0),dp=root.u32(16+0x13c),ip=root.u32(16+0x144);
        for(unsigned i=0;i<clusters.size();++i)section(false,i,cp+i*176);
        for(unsigned i=0;i<definitions.size();++i)section(true,i,dp+i*200);
        for(unsigned i=0;i<instances.size();++i) {
            const auto raw=read(ip+i*88,88);std::memcpy(&transforms[i],raw.slice(0,52).data(),52);
            palettes[i]=load_menu_world_source_bsp_owner(cache,index,i).rows;
            instances[i].lighting_policy=raw.u16(0x56);
            instances[i].transform_address=instance_token+i*88;
            instance_definitions.push_back(bsp.instances[i].definition);
        }
        // Resolve original2D000's presence/bitmap/checksum and per-instance
        // lightmap backing from actual map metadata, rather than geometry size.
        const auto lightmaps=cache.load_bsp_lighting(index);
        lightmap_matches=lightmaps.structure_matches && lightmaps.bitmap_datum!=0xffffffffu;
        if(lightmap_matches)for(unsigned i=0;i<instances.size();++i) {
            const auto bucket=lightmaps.instance_buckets.at(i).bucket;
            instances[i].lightmap_resident=bucket>=0 && unsigned(bucket)<lightmaps.buckets.size();
        }
        source_bsp_material_visibility_initialize(visibility);
        const auto count=root.u32(16+0xa4),start=root.u32(16+0xa8);
        for(unsigned i=0;i<count;++i) {
            const auto m=read(start+i*32,32);materials.push_back({m.u32(12),std::to_integer<std::uint8_t>(m.slice(28,1)[0])});
        }
        // Literal3B8A0 capacities; each retained24-byte record is owned here.
        for(unsigned i=0;i<3;++i) {const unsigned capacity=std::array{0xa00u,0x400u,0x200u}[i];
            commands.banks[i].header[0]=capacity;commands.banks[i].storage.resize(capacity);}
        for(auto id:ids)lights.emplace(id,load_effect_light(cache,id));
        for(const auto& material:materials)if(material.shader!=0xffffffffu && !shaders.contains(material.shader)) {
            const auto& shad=metadata.definition(material.shader);
            const auto sr=Bytes(metadata.read(shad.source_address(),0x40));
            const auto stem=Bytes(metadata.read(sr.u32(0x24),4)).u32(0);
            const auto& st=metadata.definition(stem);
            const auto post=Bytes(metadata.read(Bytes(metadata.read(st.source_address(),0x60)).u32(0x5c),8));
            auto& shader=shaders[material.shader];shader.lod=cache.load_source_shader_lod_definition(material.shader);
            auto authored=cache.load_material(material.shader);
            for(unsigned lod=0;lod<post.u32(0);++lod) {
                const auto lr=Bytes(metadata.read(post.u32(4)+lod*10,10));shader.layers.push_back(lr.u32(2));
                authored.selected_shader_lod=lod;
                for(const auto& [id,definition]:lights)try {
                    EffectLightState state;state.size=1;state.brightness=1;
                    const auto uniforms=source_effect_light_view_uniforms(definition,state,{}, {1,0,0},{0,0,1},{1,1,1,1,1},1,{});
                    auto prepared=prepare_effect_light_diffuse_material(cache,authored,definition,uniforms);
                    for(unsigned pass=0;pass<prepared.size();++pass) {
                        const auto slot=unsigned(mesh.materials.size());mesh.materials.push_back(std::move(prepared[pass]));
                        slots.emplace(MaterialKey{id,material.shader,lod,pass},Slot{slot,authored,load_menu_world_source_pass(cache,material.shader,{lod,12,pass,1})});
                    }
                }catch(const FormatError& e){unavailable.push_back(std::to_string(id)+"/"+std::to_string(material.shader)+": "+e.what());}
            }
        }
        for(auto model:model_ids) {
            const auto tag=cache.load_source_tag_metadata(model);
            const Bytes root(tag.bytes_at(tag.source_address(),0x68));
            for(unsigned n=0;n<root.u32(0x60);++n) {
                const auto id=Bytes(tag.bytes_at(root.u32(0x64)+n*32u,32)).u32(12);
                if(id==0xffffffffu || shaders.contains(id))continue;
                SourceBspCommandShader value;value.lod=cache.load_source_shader_lod_definition(id);
                const auto& shad=metadata.definition(id);
                const auto stem=Bytes(metadata.read(Bytes(metadata.read(shad.source_address(),0x28)).u32(0x24),4)).u32(0);
                const auto& st=metadata.definition(stem);
                const auto post=Bytes(metadata.read(Bytes(metadata.read(st.source_address(),0x60)).u32(0x5c),8));
                for(unsigned lod=0;lod<post.u32(0);++lod)value.layers.push_back(Bytes(metadata.read(post.u32(4)+lod*10,10)).u32(2));
                shaders.emplace(id,std::move(value));
            }
            for(unsigned section=0;section<root.u32(0x24);++section) {
                const auto row=Bytes(tag.bytes_at(root.u32(0x28)+section*92u,92));
                if(!row.u16(4) || models.contains({model,section}))continue;
                ModelSection value;value.ranges=load_source_model_range_geometry(cache,model,section);
                value.metadata=load_source_opaque_model_geometry_metadata(cache,model,section);
                value.draw=std::make_unique<SourceModelEffectLights>(cache,mesh,model,section,ids);
                for(const auto& gap:value.draw->unavailable())unavailable.push_back(gap);
                models.emplace(ModelKey{model,section},std::move(value));
            }
        }
    }
    const ModelSection& model(const SourceCullDrawUnit& unit)const {
        const auto at=models.find({unit.words[1],unit.words[3]>>9&511u});
        if(at==models.end())throw FormatError("selected live light model section was not preloaded");
        return at->second;
    }
    SectionKey key(const SourceCullDrawUnit& unit) const {
        if(unit.words[1]!=0xffffffffu)throw FormatError("BSP light bank consumes a dynamic model owner");
        if(unit.words[0]&0x1000u) {
            const auto instance=unit.words[3]>>18&2047u;
            return {true,unsigned(bsp.instances.at(instance).definition)};
        }
        return {false,unit.words[3]>>9&511u};
    }
};
SourceBspEffectLights::SourceBspEffectLights(const Cache& c,Mesh& mesh,std::size_t bsp,
    std::span<const std::uint32_t> ids,std::span<const std::uint32_t> models)
    :impl_(std::make_unique<Impl>(c,mesh,bsp,ids,models)){}
SourceBspEffectLights::~SourceBspEffectLights()=default;
const std::vector<std::string>& SourceBspEffectLights::unavailable() const{return impl_->unavailable;}
std::vector<EffectLightRawDraw> SourceBspEffectLights::draw(const SourceEffectLightView& lights,
    std::span<const WorldEffectLightReceiver> receivers,const SourceBspAdmission& main,
    const SourceViewProjectionBank& camera,const BspFogSample& fog,Vec3 eye,
    std::array<std::int16_t,4> viewport,std::array<std::uint32_t,2> surface,unsigned players,
    SourceEffectLightModelViewTu15* objects) {
    auto& i=*impl_;std::vector<EffectLightRawDraw> output;if(receivers.empty())return output;
    const auto coverage=source_shader_coverage_camera(camera);
    SourceBspDrawWorld draw_world{i.bsp,i.clusters,i.definitions,i.instances,i.lightmap_matches};
    auto admission=main;source_bsp_range_arena_begin(i.main_arena);i.units.arena=i.main_arena;
    source_bsp_collect_draw_units(i.units,admission,draw_world,{0,Impl::main_arena_token,0,fog,coverage});
    const auto object_context=[&](const SourceBspDrawContext& c) {
        auto out=objects->context;
        out.entry.mode=out.draw.draw.mode=c.mode;
        out.draw.draw.mask_arena_address=c.mask_arena_address;
        out.entry.eye=out.draw.eye=eye;out.draw.viewport=viewport;out.draw.rendering=camera;
        out.entry.coverage=coverage;out.local_players=players;
        out.entry.flags={std::uint8_t(c.override_byte),std::int32_t(fog.classification),fog.planar_datum,
            fog.eye_fraction,fog.planar_only,std::uint8_t(fog.planar_flags)};
        return out;
    };
    if(objects)source_live_model_draws_tu15(objects->world,objects->main,i.units,
        object_context({0,Impl::main_arena_token,0,fog,coverage}));
    i.main_arena=i.units.arena;
    SourceBspPartCommandWorld command_world{i.cluster_commands,i.definition_commands,i.instance_definitions,
        i.materials,i.visibility,std::int16_t(i.bsp_index),[&](auto datum)->const SourceBspCommandShader&{return i.shaders.at(datum);}};
    MenuWorldSourceConstants constants{};
    const auto bank=source_view_shader_bank(camera,1024.f,eye,
        {float(viewport[1]),float(viewport[0]),float(viewport[3]-viewport[1]),float(viewport[2]-viewport[0])},false);
    std::copy(bank.begin(),bank.end(),constants.begin());
    for(const auto& receiver:receivers) {
        i.commands.calls.clear();
        const auto light=std::find_if(lights.records.begin(),lights.records.end(),[&](const auto& r){return r.runtime.identity==receiver.identity;});
        if(light==lights.records.end() || !light->definition)throw FormatError("BSP light receiver lost its live light");
        const auto& runtime=light->runtime;
        const auto uniforms=source_effect_light_view_uniforms(*light->definition,runtime.state,runtime.position,
            runtime.forward,runtime.up,light->fades,receiver.selected_fade,{});
        auto query=receiver.view.bsp;source_bsp_range_arena_begin(i.light_arena);i.units.arena=i.light_arena;
        SourceEffectLightGeometryFrameWorld world{draw_world,command_world,i.metadata};
        // Both geometry families append to the same units, command banks and
        // final sort. Optional model ownership is explicit for BSP-only callers.
        SourceObjectAdmission object_receivers;
        std::map<std::array<std::uint32_t,8>,SourceModelCommandGeometry> live_models;
        const auto live_model=[&](const SourceCullDrawUnit& unit)->const SourceModelCommandGeometry& {
            if(!objects)throw FormatError("model light unit has no live world owner");
            auto at=live_models.find(unit.words);
            if(at==live_models.end())at=live_models.emplace(unit.words,
                source_live_model_command_geometry_tu15(objects->world,unit)).first;
            return at->second;
        };
        world.commands.model=live_model;
        world.dynamic=[&](auto& units,auto& query,const auto& draw_context) {
            if(!objects)return;
            SourceObjectAdmissionInput input;
            input.flags=receiver.view.flags;input.context_2a78=receiver.view.pass_mode;input.mode=receiver.view.query_mode;
            input.sphere_center=runtime.spatial_center;input.sphere_radius=runtime.spatial_radius;
            input.containing_fog=objects->containing_fog;input.auxiliary=&objects->main;
            object_receivers=objects->world.admit_render_objects(receiver.view.visibility,input,query.associations);
            query.associations=object_receivers.associations;
            source_live_model_draws_tu15(objects->world,object_receivers,units,object_context(draw_context));
        };
        world.object_commands=[&](auto& state,auto mode) {
            if(objects)objects->world.append_render_attachments(state,object_receivers,mode);
        };
        world.cull_bank=[&](auto,const auto& unit) {
            if(unit.words[1]!=0xffffffffu) {
                if(!objects)throw FormatError("light model cull lacks its live world");
                return source_live_model_cull_geometry_tu15(objects->world,unit,i.model(unit).ranges);
            }
            const auto& section=i.sections.at(i.key(unit));SourceCullDrawBanks result;
            result.geometry_resolved=true;result.geometry=source_bsp_sphere_cull_geometry(section.ranges);
            if(unit.words[0]&0x1000u)result.transform=i.transforms.at(unit.words[3]>>18&2047u);return result;
        };
        world.opaque_geometry=[&](unsigned ordinal) {
            const auto& unit=i.units.units.at(ordinal);
            if(unit.words[1]!=0xffffffffu){const auto& section=i.model(unit);return SourceOpaqueGeometrySelection{section.metadata,section.ranges.parts};}
            const auto& section=i.sections.at(i.key(unit));return SourceOpaqueGeometrySelection{section.metadata,section.ranges.parts};
        };
        SourceEffectLightGeometryFrameContext context;
        context.draw={receiver.view.pass_mode,Impl::light_arena_token,std::int8_t(receiver.view.flags),fog,coverage};
        context.cull.query_mode=receiver.view.query_mode;context.cull.center=runtime.spatial_center;context.cull.radius=runtime.spatial_radius;
        context.commands.player_multiplier=source_shader_lod_player_multiplier(players);
        context.commands.lod_view=source_shader_lod_view(viewport[0],viewport[2],false,false);
        const auto result=source_effect_light_geometry_frame(i.units,query,i.commands,i.opaque,world,context);
        i.light_arena=i.units.arena;
        const auto& commands=i.commands.banks[1];
        const auto selected=std::span(i.opaque).first(result.opaque_count);
        // Bind the actual raw source inputs before accepting1CAA0's range
        // continuation. A declaration/program/resource failure is not success.
        std::map<unsigned,MenuWorldSourceBinding> bindings;
        std::map<unsigned,Material> completed;
        std::map<unsigned,EffectLightRawDraw> model_bindings;
        const auto ranges=source_effect_light_ordered_ranges(commands,i.units.units,selected,i.units.arena,
            Impl::light_arena_token,[&](unsigned command,unsigned unit,unsigned vertex_class) {
                const auto& c=commands.storage.at(command).words;const auto& u=i.units.units.at(unit);
                if(u.words[1]!=0xffffffffu) {
                    if(!objects)throw FormatError("light model binding lacks its live world");
                    const auto& section=i.model(u);
                    const auto& geometry=live_model(u);
                    const auto palette=source_live_model_palette_rows_tu15(objects->world,u,geometry);
                    SourceModelEffectLightDrawInput input;
                    input.object=Bytes(objects->world.memory().view(u.words[4],4)).u32(0);
                    input.light=light->definition_identity;input.shader=c[2];input.shader_lod=c[5]>>8&255u;
                    input.pass=c[5]&255u;input.part=c[3]&255u;input.vertex_class=vertex_class;
                    input.topology=source_effect_light_part_ranges(u,section.ranges,input.part,i.units.arena,Impl::light_arena_token).topology;
                    input.palette=palette;input.uniforms=uniforms;input.constants=constants;
                    MenuWorldRasterContext raster;raster.viewport_reset(surface);raster.game_initialize_offset();
                    raster.layer_begin(MenuWorldRasterLayer::sphere_diffuse_12);
                    raster.entry_begin(menu_world_raster_entry_from_source_flags(u.words[0],false,false));
                    input.raster=raster.snapshot({MenuWorldRasterPrimitive::ordinary_triangles,MenuWorldRasterPolygon::fill,
                        std::array<std::int32_t,2>{1,1},true});raster.entry_end();
                    input.raster.window_exclusive=false;for(auto& rect:input.raster.regions)rect={0,0,0,0};
                    input.raster.regions[0]={viewport[1],viewport[0],viewport[3],viewport[2]};
                    const auto depth=menu_world_depth_clip_planes({.015625f,1024.f,MenuWorldDepthBoundary::none,0,false});
                    input.clip_range={0,16777215.f,depth.minimum_w,depth.maximum_w};input.fog_parameters={1,1};
                    model_bindings.emplace(command,section.draw->draw(input));
                    return SourceOpaqueRangeGeometry{&section.ranges,true};
                }
                const auto& section=i.sections.at(i.key(u));
                const auto slot=i.slots.find({light->definition_identity,c[2],(c[5]>>8)&255u,c[5]&255u});
                if(slot==i.slots.end())throw FormatError("Selected BSP light material was not admitted during preload");
                auto prepared=prepare_effect_light_diffuse_material(i.cache,slot->second.authored,*light->definition,uniforms);
                const auto pass_index=c[5]&255u;if(pass_index>=prepared.size())throw FormatError("Selected BSP light pass missing");
                auto& material=completed.emplace(command,std::move(prepared[pass_index])).first->second;
                const auto source_constants=source_effect_light_raw_constants(constants,material);
                const auto part=c[3]&255u;const auto topology=source_effect_light_part_ranges(u,section.ranges,part,i.units.arena,Impl::light_arena_token).topology;
                const bool instance=u.words[0]&0x1000u;
                MenuWorldSourceOwner owner{instance?MenuWorldSourceOwnerKind::bsp_instance:MenuWorldSourceOwnerKind::bsp_cluster,section.raw->owner_datum(),instance?(u.words[3]>>18&2047u):section.raw->section(),{}};
                const auto draw=instance?menu_world_source_draw(section.raw,part,topology,std::span(&i.palettes.at(owner.ordinal),1)):
                    menu_world_source_cluster_draw(section.raw,part,topology);
                bindings.emplace(command,menu_world_source_binding_class(i.cache,owner,draw,slot->second.pass.vertex_shader(),vertex_class,source_constants));
                return SourceOpaqueRangeGeometry{&section.ranges,true};
            });
        MenuWorldRasterContext raster;raster.viewport_reset(surface);raster.game_initialize_offset();raster.layer_begin(MenuWorldRasterLayer::sphere_diffuse_12);
        for(const auto& range:ranges) {
            const auto& c=commands.storage.at(range.command).words;const auto& u=i.units.units.at(range.unit);
            if(u.words[1]!=0xffffffffu) {
                const auto& bound=model_bindings.at(range.command);
                for(const auto& indices:range.selected.ranges) {
                    auto packet=bound;auto draw=bound.source.draw();draw.range=indices;
                    packet.source=menu_world_source_binding_class(i.cache,bound.source.owner(),std::move(draw),
                        bound.source.vertex_shader(),range.vertex_class,bound.source.constants());
                    output.push_back(std::move(packet));
                }
                continue;
            }
            const auto& slot=i.slots.at({light->definition_identity,c[2],(c[5]>>8)&255u,c[5]&255u});
            const auto& bound=bindings.at(range.command);const auto& completed_material=completed.at(range.command);
            raster.entry_begin(menu_world_raster_entry_from_source_flags(u.words[0],false,false));
            auto packet=raster.snapshot({MenuWorldRasterPrimitive::ordinary_triangles,MenuWorldRasterPolygon::fill,std::array<std::int32_t,2>{1,1},true});raster.entry_end();
            // The view owns the original player rectangle. The raw vertex
            // output already includes its offset; Vulkan uses the full surface.
            packet.window_exclusive=false;for(auto& rect:packet.regions)rect={0,0,0,0};
            packet.regions[0]={viewport[1],viewport[0],viewport[3],viewport[2]};
            const auto depth=menu_world_depth_clip_planes({.015625f,1024.f,MenuWorldDepthBoundary::none,0,false});
            for(const auto& indices:range.selected.ranges) {
                auto draw=bound.draw();draw.range=indices;
                auto binding=menu_world_source_binding_class(i.cache,bound.owner(),std::move(draw),bound.vertex_shader(),range.vertex_class,bound.constants());
                output.push_back(source_effect_light_raw_draw(slot.material,completed_material,slot.pass,std::move(binding),packet,
                    {0,16777215.f,depth.minimum_w,depth.maximum_w},{1,1}));
            }
        }
    }
    return output;
}
}
