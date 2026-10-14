#include "halo2/source_scenery_scene.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/scenery.hpp"
#include "halo2/bsp_lighting.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_transparent_parts.hpp"
#include "halo2/source_object_colors.hpp"
#include "halo2/object_light_query.hpp"
#include "halo2/world_material.hpp"
#include "halo2/appearance.hpp"
#include "halo2/model.hpp"
#include "halo2/renderer.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <set>

namespace halo2 {
struct SourceSceneryScene::Impl {
    struct Lod {
        struct ShaderPart {unsigned section{},part{},material{};float scale=1;std::uint32_t shader{};};
        RenderModel model;
        std::vector<unsigned> sections,materials;
        std::vector<Vertex> vertices;
        std::vector<ShaderPart> shader_parts;
    };
    struct Scenery {
        std::size_t source_object{};
        std::uint32_t handle=0xffffffffu;
        std::uint64_t created_tick{};
        bool active=true,visible=true,dynamic{},selection_requires_history{};
        ScenarioObject authored;
        SceneryDefinition definition;
        SourceInitialModelSelection selection;
        SourceGeometryLodDefinition lod_definition;
        std::array<Lod,6> lods;
        std::array<std::int8_t,4> previous_lod{-1,-1,-1,-1};
        std::optional<AnimationClip> idle;
        Transform root,node0;
        std::vector<Transform> world_nodes;
        std::optional<std::vector<Transform>> solved_nodes;
        float scale=1;
        unsigned first_material{},material_count{};
        std::map<unsigned,std::vector<unsigned>> shader_slots;
        const SceneryLightprobe *probe{};
        ObjectLightingCache light;
        SourceSceneryQueryState query_state;
        bool query_state_valid{};
    };
    const Cache &cache;
    const CollisionWorld &collision;
    BspLighting lighting;
    BspSurfaceLighting surface;
    ObjectLightingDefinition conversion;
    std::vector<SceneryLightprobe> probes;
    std::vector<Scenery> scenery;
    std::map<std::uint32_t,SourceShaderLodDefinition> shader_definitions;
    std::vector<std::size_t> unresolved_owners;
    std::set<std::size_t> source_placements,nonrendered_placements;
    SourceScenerySceneStats counts;
    LightingQueryState query_state;
    AnimationTime animation_time;
    AnimationFrame animation_frame;
    AnimationFrames animation_frames;
    BirthQueryCapture birth_capture;
    std::int32_t render_frame{};
    std::uint64_t tick{};

    Impl(const Cache &c,std::span<const ScenarioObject> objects,std::size_t bsp,Mesh &mesh,
         const CollisionWorld &cw,const InitialSelection &selected,const OmitUnresolvedSelection &omit)
        :cache(c),collision(cw),lighting(c.load_bsp_lighting(bsp)),
         surface(c.load_bsp_surface_lighting(bsp)),conversion(c.load_object_lighting_definition()),
         probes(c.load_scenery_lightprobes(bsp)) {
        for(std::size_t index=0;index<objects.size();++index) {
            const auto &object=objects[index];
            if(object.type==6 && object.definition!=0xffffffffu &&
                scenery_multiplayer_game_allowed(object,2) &&
                (object.origin_bsp<0 || object.origin_bsp==int(bsp)))source_placements.insert(index);
            if(object.type!=6 || !scenery_automatic_creation_allowed(object,2) ||
                (object.origin_bsp>=0 && object.origin_bsp!=int(bsp)))continue;
            ++counts.placements;
            Scenery s;s.source_object=index;s.authored=object;
            const auto definition=cache.load_scenery_definition(object.definition);
            if(!definition)throw FormatError("SCEN source definition missing");
            s.definition=*definition;
            const auto initial=cache.load_initial_model_definition(object.definition);
            if(initial.render_model==0xffffffffu){nonrendered_placements.insert(index);continue;}
            const auto deterministic=cache.load_initial_model_selection(object.definition,object.variant);
            s.selection_requires_history=!deterministic.resolved();
            s.selection=selected?selected(index,object,initial):deterministic;
            if(!s.selection.resolved()) {
                if(!omit || !omit(index,object))throw FormatError("SCEN initial region bytes require actual RNG history");
                unresolved_owners.push_back(index);++counts.unresolved_selections;source_placements.erase(index);continue;
            }
            s.lod_definition=cache.load_source_geometry_lod_definition(object.definition);
            s.idle=cache.load_initialized_object_idle(object.definition);
            s.scale=object.scale;const auto basis=authored_basis(object.rotation);
            s.root=source_object_transform(object.position,basis.forward,basis.up,s.scale,false);
            // Load every literal collector before allocation. The source material
            // table is shared across levels; section-local incident/RGB is not.
            for(unsigned lod:{5u,0u,1u,2u,3u,4u}) {
                auto &g=s.lods[lod];
                g.model=cache.load_model(object.definition,lod==5,object.variant,s.selection,lod);
                g.sections=source_initial_model_sections(initial,s.selection,lod);
                if(g.model.vertex_origins.size()!=g.model.mesh.vertices.size())
                    throw FormatError("SCEN collector lost its source vertex identities");
                for(const auto &origin:g.model.vertex_origins)
                    if(std::find(g.sections.begin(),g.sections.end(),origin.section)==g.sections.end())
                        throw FormatError("SCEN geometry differs from its original collector");
                if(lod!=5 && (g.model.bones.size()!=s.lods[5].model.bones.size() ||
                    g.model.checksum!=s.lods[5].model.checksum || g.model.animation!=s.lods[5].model.animation))
                    throw FormatError("SCEN geometry LOD changed its source hierarchy");
            }
            const auto implementation=scenery_lighting_implementation(lighting,s.lods[5].model,object,s.definition);
            if(implementation==SceneryLightingImplementation::unsupported)
                throw FormatError("SCEN lighting implementation3 requires original active resources");
            s.dynamic=implementation==SceneryLightingImplementation::dynamic;
            if(s.dynamic){++counts.dynamic_models;s.probe=scenery_lightprobe(probes,object,s.definition);
                if(s.probe)++counts.direct_probes;}
            else ++counts.baked_models;
            const auto color_definition=cache.load_source_object_color_definition(object.definition);
            for(const auto &channel:color_definition.channels)if(channel.function_count)
                throw FormatError("SCEN current change-color function requires source inputs");
            const auto variant=s.selection.variant_index<0?0u:initial.variants.at(unsigned(s.selection.variant_index)).name;
            const auto colors=source_object_initial_colors(color_definition,object.position,variant);
            const auto source_materials=s.lods[5].model.mesh.materials;
            std::map<unsigned,unsigned> material_slots;s.first_material=unsigned(mesh.materials.size());
            for(unsigned lod:{5u,0u,1u,2u,3u,4u}) {
                auto &g=s.lods[lod];
                if(!s.dynamic) {
                    // Existing source bridge checks every actual section_offset
                    // and ordinal against the authored bucket before preparation.
                    // Each collector gets its own stream even when shader IDs match.
                    g.model.mesh.materials=source_materials;
                    if(!cache.apply_scenery_lighting(g.model,lighting,object))
                        throw FormatError("SCEN selected collector has no source baked reference");
                }
                for(const auto &v:g.model.mesh.vertices) {
                    auto [slot,inserted]=material_slots.try_emplace(v.material);
                    if(inserted) {
                        if(v.material>=source_materials.size())throw FormatError("SCEN source material outside bank");
                        auto material=s.dynamic?source_materials[v.material]:g.model.mesh.materials[v.material];
                        if(s.dynamic) {
                            if(!prepare_dynamic_material(cache,material,{},true))
                                throw FormatError("SCEN dynamic material requires unresolved source inputs: "+material.name);
                            ++counts.union_dynamic_materials;
                        } else {
                            if(!material.world_pass)
                                throw FormatError("SCEN baked shader did not admit source implementation1: "+material.name);
                            ++counts.union_baked_materials;
                        }
                        apply_object_colors(material,colors);slot->second=unsigned(mesh.materials.size());
                        mesh.materials.push_back(std::move(material));
                    }
                    g.materials.push_back(slot->second);
                }
                const auto parts=cache.load_source_transparent_model_parts(initial.render_model,g.sections);
                for(const auto &section:parts.sections)for(const auto &part:section.parts) {
                    const auto origin=std::find_if(g.model.vertex_origins.begin(),g.model.vertex_origins.end(),
                        [&](const auto &v){return v.section==section.section && v.part==part.ordinal;});
                    if(origin==g.model.vertex_origins.end())continue;
                    const auto index=unsigned(origin-g.model.vertex_origins.begin());
                    const auto root=g.materials.at(index);
                    const auto source=g.model.mesh.vertices.at(index).material;
                    if(source>=source_materials.size() || source_materials[source].source_shader_datum!=part.shader)
                        throw FormatError("SCEN authored part differs from its source shader");
                    auto [definition,new_definition]=shader_definitions.try_emplace(part.shader);
                    if(new_definition)definition->second=cache.load_source_shader_lod_definition(part.shader);
                    auto [slots,new_slots]=s.shader_slots.try_emplace(root);
                    if(new_slots) {
                        slots->second.push_back(root);
                        const auto &raw=source_materials[source];
                        if(raw.lods.size()!=definition->second.thresholds.size())
                            throw FormatError("SCEN shader differs from its authored LOD bank");
                        for(unsigned shader_lod=1;shader_lod<raw.lods.size();++shader_lod) {
                            auto material=raw;material.selected_shader_lod=shader_lod;
                            const bool prepared=s.dynamic?prepare_dynamic_material(cache,material,{},true):
                                prepare_world_material(cache,material,true);
                            if(!prepared)throw FormatError("SCEN shader LOD requires unresolved source inputs: "+raw.name);
                            apply_object_colors(material,colors);
                            slots->second.push_back(unsigned(mesh.materials.size()));
                            mesh.materials.push_back(std::move(material));
                            if(s.dynamic)++counts.union_dynamic_materials;else ++counts.union_baked_materials;
                        }
                    }
                    g.shader_parts.push_back({section.section,part.ordinal,root,
                        part.lod_scale>0?part.lod_scale:1.f,part.shader});
                }
                g.model.mesh.materials.clear();
            }
            s.material_count=unsigned(mesh.materials.size())-s.first_material;
            pose(s,0);scenery.push_back(std::move(s));++counts.render_models;
        }
    }
    static void pose(Scenery &s,float phase,bool controller_frame=false,
                     std::optional<AnimationFrameWindow> frames={}) {
        const auto &model=s.lods[5].model;
        if(model.bones.empty())throw FormatError("SCEN model has no source query node0");
        if(s.solved_nodes) {
            if(s.solved_nodes->empty())throw FormatError("SCEN source owner has no current WORLD node0");
            s.world_nodes=*s.solved_nodes;
            s.node0=s.solved_nodes->front();
            for(auto &g:s.lods)g.vertices=skin_model_world_nodes(g.model,*s.solved_nodes);
            return;
        }
        const auto clip=s.idle?&*s.idle:nullptr;
        // Cold10A1B0 stores+138 NONE, so10A390 adds no deferred root override.
        s.world_nodes=frames?pose_model_nodes_world_between_frames(model,s.root,*clip,*frames):
            controller_frame?pose_model_nodes_world_frame(model,s.root,clip,phase):
            pose_model_nodes_world(model,s.root,clip,phase);
        s.node0=s.world_nodes.front();
        for(auto &g:s.lods) {
            g.vertices=frames?pose_model_world_between_frames(g.model,s.root,*clip,*frames):
                controller_frame?pose_model_world_frame(g.model,s.root,clip,phase):
                pose_model_world(g.model,s.root,clip,phase);
            if(g.vertices.size()!=g.materials.size())throw FormatError("SCEN pose changed original vertex order");
            for(unsigned i=0;i<g.vertices.size();++i) {
                auto &v=g.vertices[i];
                // Authored baked incident is already world-space; do not rotate it.
                v.material=g.materials[i];
            }
        }
    }
    static bool same_selection(const SourceInitialModelSelection &a,const SourceInitialModelSelection &b) {
        return a.resolved() && b.resolved() && a.variant_index==b.variant_index &&
            a.permutations==b.permutations && a.states==b.states;
    }
    void synchronize(const SceneObjects &scene,const LifetimeSelection &selected) {
        std::map<std::size_t,const SceneObjects::Object*> live;
        for(const auto &object:scene.objects())if(object.handle!=0xffffffffu) {
            // Only prepared SCEN placement ownership belongs to this bank.
            // Vehicle netgame rows can have more than one live generation.
            if(!source_placements.contains(object.placement))continue;
            if(!live.emplace(object.placement,&object).second)throw FormatError("SCEN duplicate source lifetime");
            if(source_placements.contains(object.placement) && !nonrendered_placements.contains(object.placement) &&
                std::none_of(scenery.begin(),scenery.end(),[&](const auto &s){return s.source_object==object.placement;}))
                throw FormatError("live SCEN placement requires unprepared source selection");
        }
        for(auto &s:scenery) {
            const auto found=live.find(s.source_object);
            if(found==live.end()){s.active=false;s.visible=false;continue;}
            const auto &object=*found->second;
            const bool born=s.handle!=object.handle;
            if(born) {
                if(s.selection_requires_history && !selected)
                    throw FormatError("SCEN birth requires its actual historical region selection");
                if(selected && !same_selection(s.selection,selected(s.source_object,object.handle,object.created_tick)))
                    throw FormatError("SCEN lifetime selection differs from resident prepared geometry");
                s.handle=object.handle;s.created_tick=object.created_tick;s.light={};
                s.previous_lod.fill(-1);s.query_state_valid=false;
            } else if(s.created_tick!=object.created_tick)throw FormatError("SCEN full handle changed creation tick");
            s.root=scene_object_transform(object);s.scale=s.root.scale;
            s.solved_nodes=object.solved_nodes;
            s.active=true;s.visible=object.visible;
            if(born && birth_capture) {
                pose(s,0);
                birth_capture(s.source_object,s.handle,s.created_tick,s.authored.definition,s.node0,s.scale);
            }
        }
    }
    void update_pose(std::uint64_t simulation_tick,std::int32_t frame) {
        if(simulation_tick<tick)throw FormatError("SCEN simulation clock moved backwards");
        tick=simulation_tick;render_frame=frame;
        for(auto &s:scenery)if(s.active && s.visible) {
            if(tick<s.created_tick)throw FormatError("SCEN animation precedes actual birth");
            if(s.solved_nodes){pose(s,0);s.query_state_valid=false;continue;}
            float seconds=0;
            if(s.idle && s.idle->frame_count>1) {
                if(animation_frames) {
                    const auto frames=animation_frames(s.source_object,s.handle,s.created_tick,tick,*s.idle);
                    pose(s,0,true,frames);s.query_state_valid=false;continue;
                }
                if(animation_frame) {
                    const auto frame=animation_frame(s.source_object,s.handle,s.created_tick,tick,*s.idle);
                    pose(s,frame,true);s.query_state_valid=false;continue;
                }
                if(!animation_time)throw FormatError("SCEN initialized animation requires actual controller-time input");
                seconds=animation_time(s.source_object,s.handle,s.created_tick,tick);
                if(!std::isfinite(seconds) || seconds<0)throw FormatError("SCEN controller time is invalid");
            }
            pose(s,seconds);s.query_state_valid=false;
        }
    }
    void retain_query_state(Scenery &s) {
        if(s.query_state_valid)return;
        if(!query_state)throw FormatError("SCEN draw requires actual cached sphere/flags producer");
        s.query_state=query_state(s.source_object,s.handle,s.node0,s.scale);s.query_state_valid=true;
    }
    void query_lighting(Scenery &s,float distance) {
        if(!s.dynamic || s.probe)return;
        const auto &state=s.query_state;const auto origin=state.sphere_center;
        const bool due=object_lighting_query_due(s.light,render_frame,origin,distance,state.frozen);
        ObjectLightQueryContext context;context.bank_valid=source_object_light_query_bank_valid(lighting);
        context.object_type=6;context.definition_flags=s.definition.object_flags;context.runtime_flags=state.runtime_flags;
        const auto result=due?source_object_light_query(context,[&](unsigned mode) {
            return query_object_lightmap(surface,collision,origin,ObjectLightMapQueryContext{
                mode,!(s.light.sample_valid && s.light.lighting_valid),1.f,s.light.target});
        },[]{return ObjectLightQueryResult{};}):ObjectLightQueryAdmission{};
        if(result.query.status==ObjectLightQueryStatus::attribution_unresolved)
            throw FormatError("SCEN exact source contact attribution unresolved");
        update_object_lighting_cache(s.light,conversion,6,render_frame,origin,state.velocity,due,
            result.accepted?result.query.sample:std::optional<ObjectLightmapSample>{},true,state.frozen);
    }
    SourceObjectViewSelection select(Scenery &s,const SourceObjectViewLod &view) {
        if(view.view_index>=4 || !view.input)throw FormatError("SCEN requires source player bank/distance input");
        retain_query_state(s);const auto &state=s.query_state;
        const auto input=view.input(s.source_object,s.handle,state.sphere_center,state.sphere_radius);
        if(!std::isfinite(input.draw_metric) || input.draw_metric<0 ||
            !std::isfinite(state.sphere_radius) || state.sphere_radius<0)
            throw FormatError("SCEN cached sphere/distance is invalid");
        float metric=input.draw_metric;if(view.source_4ba019)metric*=.3f;
        if(input.control_flags&4u)metric*=1.7f;
        const auto choice=source_geometry_lod_choice(s.lod_definition,metric,s.previous_lod[view.view_index]);
        const auto lod=unsigned(source_geometry_lod_cap(choice,view.detail_policy.value_or(source_geometry_lod_policy(view.local_players)),input.object_flags,false));
        const auto size=s.lods[lod].vertices.size();
        if(size)s.previous_lod[view.view_index]=std::int8_t(lod);
        return {s.source_object,lod,choice.transition,size,input.draw_metric};
    }
    std::vector<Vertex> shader_vertices(const Scenery &s,const Lod &g,const SourceObjectViewLod &view)const {
        auto vertices=g.vertices;
        if(!view.shader)return vertices;
        const auto collector=source_model_shader_collector(s.query_state.sphere_center,
            s.query_state.sphere_radius,*view.shader);
        std::map<std::pair<unsigned,unsigned>,unsigned> selected;
        for(const auto &part:g.shader_parts) {
            const auto choice=source_model_shader_part(collector,part.scale,
                shader_definitions.at(part.shader),*view.shader).choice;
            const auto target=choice.accepted && choice.index>=0?
                s.shader_slots.at(part.material).at(unsigned(choice.index)):0xffffffffu;
            auto [binding,inserted]=selected.emplace(std::pair{part.section,part.part},target);
            if(!inserted && binding->second!=target)throw FormatError("SCEN duplicate collector changed shader binding");
        }
        vertices.erase(std::remove_if(vertices.begin(),vertices.end(),[&](auto &vertex) {
            const auto binding=selected.find({vertex.source_section,vertex.source_part});
            if(binding==selected.end())throw FormatError("SCEN emitted vertex lost its source shader part");
            if(binding->second==0xffffffffu)return true;
            vertex.material=binding->second;return false;
        }),vertices.end());
        return vertices;
    }
};
SourceSceneryScene::SourceSceneryScene(const Cache &c,std::span<const ScenarioObject>objects,std::size_t bsp,
    Mesh &mesh,const CollisionWorld &cw,InitialSelection selected,OmitUnresolvedSelection omit)
    :impl_(std::make_unique<Impl>(c,objects,bsp,mesh,cw,selected,omit)){}
SourceSceneryScene::~SourceSceneryScene()=default;
SourceSceneryScene::SourceSceneryScene(SourceSceneryScene&&)noexcept=default;
SourceSceneryScene&SourceSceneryScene::operator=(SourceSceneryScene&&)noexcept=default;
void SourceSceneryScene::synchronize(const SceneObjects &scene,LifetimeSelection selected){impl_->synchronize(scene,selected);}
void SourceSceneryScene::interpolate(const SceneObjects &scene,const SceneInterpolation &history,float fraction){
    for(auto &s:impl_->scenery)if(s.active){
        const auto &object=scene.get(s.handle);
        s.root=history.sample(object,fraction);s.scale=s.root.scale;
        if(object.solved_nodes)s.solved_nodes=history.sample_nodes(object,fraction);
    }
}
void SourceSceneryScene::lighting_query_state(LightingQueryState state){impl_->query_state=std::move(state);}
void SourceSceneryScene::animation_time(AnimationTime time){impl_->animation_time=std::move(time);}
void SourceSceneryScene::animation_frame(AnimationFrame frame){impl_->animation_frame=std::move(frame);}
void SourceSceneryScene::animation_frames(AnimationFrames frames){impl_->animation_frames=std::move(frames);}
void SourceSceneryScene::birth_query_capture(BirthQueryCapture capture){
    if(capture && std::any_of(impl_->scenery.begin(),impl_->scenery.end(),[](const auto &s){return s.handle!=0xffffffffu;}))
        throw FormatError("SCEN birth capture must register before lifetime synchronization");
    impl_->birth_capture=std::move(capture);
}
void SourceSceneryScene::update_pose(std::uint64_t tick,std::int32_t frame,Mesh&){impl_->update_pose(tick,frame);}
std::vector<SourceSceneEffectOwner> SourceSceneryScene::effect_owners() const {
    std::vector<SourceSceneEffectOwner> out;
    for(const auto &s:impl_->scenery)if(s.active && s.handle!=0xffffffffu)
        out.push_back({s.source_object,s.handle,s.created_tick,s.world_nodes,s.selection.permutations,s.root.position});
    return out;
}
void SourceSceneryScene::append_view(std::vector<Vertex>&vertices,std::vector<ObjectLightingBinding>&bindings,
    const SourceObjectViewLod &view,std::vector<SourceObjectViewSelection>*selections) {
    for(auto &s:impl_->scenery)if(s.active && s.visible) {
        auto selected=impl_->select(s,view);const auto &g=s.lods[selected.lod];
        //4B5D0 evaluates the admitted owner's lighting cache before41040
        //selects part shaders; selected vertex counts do not gate that cache.
        impl_->query_lighting(s,selected.source_distance);
        const auto draw=impl_->shader_vertices(s,g,view);selected.vertices=draw.size();
        if(!draw.empty()) {
            vertices.insert(vertices.end(),draw.begin(),draw.end());
            if(s.dynamic)bindings.push_back({s.first_material,s.material_count,s.probe?s.probe->lighting:s.light.lighting});
        }
        if(selections)selections->push_back(selected);
    }
}
const SourceScenerySceneStats&SourceSceneryScene::stats()const{return impl_->counts;}
std::span<const std::size_t>SourceSceneryScene::unresolved_selection_owners()const{return impl_->unresolved_owners;}
}
