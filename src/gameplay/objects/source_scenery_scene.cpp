#include "halo2/source_scenery_scene.hpp"
#include "halo2/native_parallel.hpp"
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
#include "halo2/source_live_model_geometry_tu15.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <new>
#include <set>
#include <thread>
#include <type_traits>
#include <utility>

namespace halo2 {
struct SourceSceneryScene::Impl {
    struct SourceSection {unsigned lod{};std::vector<Vertex> vertices;std::vector<std::uint32_t> ordinals;ObjectColors colors;};
    struct SourceView {std::vector<SourceSection> sections;bool submitted{};};
    struct Lod {
        struct ShaderPart {unsigned section{},part{},material{};float scale=1;std::uint32_t shader{};};
        RenderModel model;
        std::vector<unsigned> sections,active_sections,materials;
        std::vector<Vertex> vertices;
        std::vector<ShaderPart> shader_parts;
    };
    struct Scenery {
        std::size_t source_object{};
        std::uint32_t handle=0xffffffffu;
        std::uint64_t created_tick{};
        bool active=false,visible=false,dynamic{},selection_requires_history{};
        ScenarioObject authored;
        SceneryDefinition definition;
        SourceInitialModelSelection selection;
        SourceInitialModelDefinition initial;
        bool region_union{};
        std::optional<std::string> resource_error;
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
        ObjectColors initial_colors;
        std::array<std::optional<SourceView>,4> source_views;
    };
    // Only the values written after resident resource preparation. A cut must
    // own all six posed streams and active collector lists, not just node0.
    struct LodState {
        std::vector<unsigned> active_sections;
        std::vector<Vertex> vertices;
    };
    struct State {
        std::uint32_t handle{};
        std::uint64_t created_tick{};
        bool active{},visible{};
        SourceInitialModelSelection selection;
        std::array<LodState,6> lods;
        std::array<std::int8_t,4> previous_lod;
        Transform root,node0;
        std::vector<Transform> world_nodes;
        std::optional<std::vector<Transform>> solved_nodes;
        float scale{};
        ObjectLightingCache light;
        SourceSceneryQueryState query_state;
        bool query_state_valid{};
        std::array<std::optional<SourceView>,4> source_views;
        explicit State(const Scenery &s):handle(s.handle),created_tick(s.created_tick),
            active(s.active),visible(s.visible),selection(s.selection),previous_lod(s.previous_lod),
            root(s.root),node0(s.node0),world_nodes(s.world_nodes),solved_nodes(s.solved_nodes),
            scale(s.scale),light(s.light),query_state(s.query_state),query_state_valid(s.query_state_valid),source_views(s.source_views) {
            for(unsigned i=0;i<lods.size();++i)lods[i]={s.lods[i].active_sections,s.lods[i].vertices};
        }
        void exchange(Scenery &s) noexcept {
            static_assert(std::is_nothrow_swappable_v<SourceInitialModelSelection>);
            static_assert(std::is_nothrow_swappable_v<ObjectLightingCache>);
            using std::swap;
            source_views.swap(s.source_views);
            swap(handle,s.handle);swap(created_tick,s.created_tick);swap(active,s.active);swap(visible,s.visible);
            swap(selection,s.selection);swap(previous_lod,s.previous_lod);swap(root,s.root);swap(node0,s.node0);
            swap(world_nodes,s.world_nodes);swap(solved_nodes,s.solved_nodes);swap(scale,s.scale);
            swap(light,s.light);swap(query_state,s.query_state);swap(query_state_valid,s.query_state_valid);
            for(unsigned i=0;i<lods.size();++i){
                lods[i].active_sections.swap(s.lods[i].active_sections);lods[i].vertices.swap(s.lods[i].vertices);
            }
        }
    };
    const std::thread::id executor=std::this_thread::get_id();
    const std::shared_ptr<const void> owner=std::make_shared<const unsigned char>(0);
    std::uint64_t revision{},bindings{};
    bool busy{};
    void own()const {
        if(executor!=std::this_thread::get_id())throw FormatError("SCEN presentation crossed its executor");
    }
    bool try_mutating()noexcept {
        if(executor!=std::this_thread::get_id()||busy||revision==std::numeric_limits<std::uint64_t>::max())return false;
        ++revision;return true;
    }
    void mutating(){
        own();if(revision==std::numeric_limits<std::uint64_t>::max())throw FormatError("SCEN presentation revision exhausted");
        ++revision;if(busy)throw FormatError("SCEN presentation mutation is already active");
    }
    void completed()const {
        own();if(busy)throw FormatError("SCEN checkpoint requires a completed presentation mutation");
    }
    struct Mutation {
        Impl *state{};
        explicit Mutation(Impl &s):state(&s){s.mutating();s.busy=true;}
        Mutation(Impl &s,std::nothrow_t)noexcept {
            if(s.try_mutating()){state=&s;s.busy=true;}
        }
        ~Mutation(){if(state)state->busy=false;}
        Mutation(const Mutation &)=delete;
        Mutation &operator=(const Mutation &)=delete;
        explicit operator bool()const noexcept{return state;}
    };
    void rebind(){
        if(bindings==std::numeric_limits<std::uint64_t>::max())throw FormatError("SCEN callback binding revision exhausted");
        ++bindings;
    }
    const Cache &cache;
    const CollisionWorld &collision;
    BspLighting lighting;
    BspSurfaceLighting surface;
    ObjectLightingDefinition conversion;
    std::vector<SceneryLightprobe> probes;
    std::vector<Scenery> scenery;
    std::map<std::uint32_t,SourceShaderLodDefinition> shader_definitions;
    std::map<unsigned,std::string> unresolved_materials;
    std::vector<std::size_t> unresolved_owners;
    std::vector<SourceSceneryResourceBoundary> deferred_resources;
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
         const CollisionWorld &cw,const InitialSelection &selected,const OmitUnresolvedSelection &omit,bool region_union)
        :cache(c),collision(cw),lighting(c.load_bsp_lighting(bsp)),
         surface(c.load_bsp_surface_lighting(bsp)),conversion(c.load_object_lighting_definition()),
         probes(c.load_scenery_lightprobes(bsp)) {
        for(std::size_t index=0;index<objects.size();++index) {
            const auto &object=objects[index];
            if(object.type==6 && object.definition!=0xffffffffu)source_placements.insert(index);
            // Resource preparation never chooses game-type birth or BSP
            // visibility. Actual source lifetimes own both during synchronize.
            if(object.type!=6 || object.definition==0xffffffffu)continue;
            ++counts.placements;
            Scenery s;s.source_object=index;s.authored=object;
            const auto material_base=mesh.materials.size();const auto before=counts;
            try {
            const auto definition=cache.load_scenery_definition(object.definition);
            if(!definition)throw FormatError("SCEN source definition missing");
            s.definition=*definition;
            const auto initial=cache.load_initial_model_definition(object.definition);
            s.initial=initial;s.region_union=region_union;
            if(initial.render_model==0xffffffffu){nonrendered_placements.insert(index);continue;}
            const auto deterministic=cache.load_initial_model_selection(object.definition,object.variant);
            s.selection_requires_history=!deterministic.resolved();
            s.selection=selected?selected(index,object,initial):deterministic;
            if(!s.selection.resolved() && !region_union) {
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
                if(s.selection.resolved())g.active_sections=source_initial_model_sections(initial,s.selection,lod);
                g.sections=g.active_sections;
                if(region_union) {
                    g.sections.clear();
                    for(const auto &region:initial.render_sections)for(const auto &permutation:region) {
                        const auto section=permutation[lod];
                        if(section>=255)throw FormatError("SCEN source resource section exceeds runtime byte representation");
                        if(section>=0 && std::find(g.sections.begin(),g.sections.end(),unsigned(section))==g.sections.end())
                            g.sections.push_back(unsigned(section));
                    }
                    g.model=cache.load_model_sections(object.definition,lod==5,object.variant,lod,g.sections);
                } else g.model=cache.load_model(object.definition,lod==5,object.variant,s.selection,lod);
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
            s.initial_colors=colors;
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
                                unresolved_materials.emplace(unsigned(mesh.materials.size()),material.name);
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
                            if(!prepared)unresolved_materials.emplace(unsigned(mesh.materials.size()),raw.name);
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
            } catch(const FormatError &error) {
                if(!region_union)throw;
                // An inactive authored placement is not a current resource
                // request. Retain its failure and identity, roll back every
                // partial material slot, and reject if it is actually shown.
                mesh.materials.resize(material_base);counts=before;
                unresolved_materials.erase(unresolved_materials.lower_bound(unsigned(material_base)),unresolved_materials.end());
                s.lods={};s.shader_slots.clear();s.first_material=0;s.material_count=0;
                s.resource_error=error.what();
                deferred_resources.push_back({index,*s.resource_error});++counts.deferred_resources;
                scenery.push_back(std::move(s));
            }
        }
    }
    // The six LOD poses are independent: compute them together, then the
    // caller publishes them in LOD order exactly as the serial loop did.
    struct PosedLods {
        mutable std::array<std::vector<Vertex>,6> vertices;
        std::array<std::exception_ptr,6> errors;
    };
    template<class Pose> static PosedLods pose_lods(const Scenery &s,const Pose &pose) {
        PosedLods out;
        native_parallel_for(s.lods.size(),[&](std::size_t n) {
            try{out.vertices[n]=pose(s.lods[n].model);}catch(...){out.errors[n]=std::current_exception();}
        });
        return out;
    }
    static void pose(Scenery &s,float phase,bool controller_frame=false,
                     std::optional<AnimationFrameWindow> frames={}) {
        const auto &model=s.lods[5].model;
        if(model.bones.empty())throw FormatError("SCEN model has no source query node0");
        if(s.solved_nodes) {
            if(s.solved_nodes->empty())throw FormatError("SCEN source owner has no current WORLD node0");
            s.world_nodes=*s.solved_nodes;
            s.node0=s.solved_nodes->front();
            const auto posed=pose_lods(s,[&](const RenderModel &m){return skin_model_world_nodes(m,*s.solved_nodes);});
            for(std::size_t n=0;n<s.lods.size();++n) {
                auto &g=s.lods[n];
                if(posed.errors[n])std::rethrow_exception(posed.errors[n]);
                g.vertices=std::move(posed.vertices[n]);
                if(g.vertices.size()!=g.materials.size())throw FormatError("SCEN WORLD pose changed source vertex order");
                for(unsigned i=0;i<g.vertices.size();++i)g.vertices[i].material=g.materials[i];
            }
            return;
        }
        const auto clip=s.idle?&*s.idle:nullptr;
        // Cold10A1B0 stores+138 NONE, so10A390 adds no deferred root override.
        s.world_nodes=frames?pose_model_nodes_world_between_frames(model,s.root,*clip,*frames):
            controller_frame?pose_model_nodes_world_frame(model,s.root,clip,phase):
            pose_model_nodes_world(model,s.root,clip,phase);
        s.node0=s.world_nodes.front();
        const auto posed=pose_lods(s,[&](const RenderModel &m) {
            return frames?pose_model_world_between_frames(m,s.root,*clip,*frames):
                controller_frame?pose_model_world_frame(m,s.root,clip,phase):
                pose_model_world(m,s.root,clip,phase);
        });
        for(std::size_t n=0;n<s.lods.size();++n) {
            auto &g=s.lods[n];
            if(posed.errors[n])std::rethrow_exception(posed.errors[n]);
            g.vertices=std::move(posed.vertices[n]);
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
            if(born)s.source_views={};
            if(s.resource_error) {
                if(object.visible)throw FormatError("Visible SCEN placement "+std::to_string(s.source_object)+
                    " requires source resources: "+*s.resource_error);
                if(!selected || !object.solved_nodes)
                    throw FormatError("Deferred SCEN lifetime requires its actual source regions and WORLD bank");
                if(!born && s.created_tick!=object.created_tick)
                    throw FormatError("Deferred SCEN full handle changed creation tick");
                s.handle=object.handle;s.created_tick=object.created_tick;
                s.selection=selected(s.source_object,object.handle,object.created_tick);
                s.root=scene_object_transform(object);s.scale=s.root.scale;
                s.solved_nodes=object.solved_nodes;s.world_nodes=*object.solved_nodes;
                s.active=true;s.visible=false;
                if(!s.world_nodes.empty()) {
                    s.node0=s.world_nodes.front();
                    if(born && birth_capture)birth_capture(s.source_object,s.handle,s.created_tick,
                        s.authored.definition,s.node0,s.scale);
                }
                continue;
            }
            if(s.region_union) {
                if(!selected)throw FormatError("SCEN source union requires actual current region bytes");
                auto actual=selected(s.source_object,object.handle,object.created_tick);
                if(!actual.resolved() || actual.variant_index!=source_initial_model_variant(s.initial,s.authored.variant?s.authored.variant:s.initial.default_variant))
                    throw FormatError("SCEN source variant requires its actual named color context");
                std::array<std::vector<unsigned>,6> sections;
                for(unsigned lod=0;lod<6;++lod) {
                    sections[lod]=source_initial_model_sections(s.initial,actual,lod);
                    for(auto section:sections[lod])if(std::find(s.lods[lod].sections.begin(),s.lods[lod].sections.end(),section)==s.lods[lod].sections.end())
                        throw FormatError("SCEN actual region selected an unprepared collector");
                }
                s.selection=std::move(actual);
                for(unsigned lod=0;lod<6;++lod)s.lods[lod].active_sections=std::move(sections[lod]);
            }
            if(born) {
                if(!s.region_union && s.selection_requires_history && !selected)
                    throw FormatError("SCEN birth requires its actual historical region selection");
                if(!s.region_union && selected && !same_selection(s.selection,selected(s.source_object,object.handle,object.created_tick)))
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
        const auto &geometry=s.lods[lod];std::size_t size=0;
        if(s.region_union)for(auto section:geometry.active_sections)
            size+=std::count_if(geometry.vertices.begin(),geometry.vertices.end(),[&](const auto &v){return v.source_section==section;});
        else size=geometry.vertices.size();
        if(size)s.previous_lod[view.view_index]=std::int8_t(lod);
        return {s.source_object,lod,choice.transition,size,input.draw_metric};
    }
    std::vector<Vertex> shader_vertices(const Scenery &s,const Lod &g,const SourceObjectViewLod &view)const {
        std::vector<Vertex> vertices;
        if(s.region_union) {
            // Retain source region order and repeated collectors; the resource
            // union itself never chooses a live draw or consumes RNG.
            for(auto section:g.active_sections)for(const auto &vertex:g.vertices)
                if(vertex.source_section==section)vertices.push_back(vertex);
        } else vertices=g.vertices;
        const auto require_bound=[&](unsigned slot) {
            if(const auto at=unresolved_materials.find(slot);at!=unresolved_materials.end())
                throw FormatError("Admitted SCEN draw requires its live source shader inputs: "+at->second);
        };
        if(!view.shader){for(const auto &vertex:vertices)require_bound(vertex.material);return vertices;}
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
            // Resource union preparation may retain an unbound shader for a
            // different BSP or unselected LOD.41040 requires its actual inputs
            // only after this part is admitted. Never submit unresolved values.
            require_bound(binding->second);vertex.material=binding->second;return false;
        }),vertices.end());
        return vertices;
    }
};
struct SourceSceneryScene::Checkpoint::Data {
    std::weak_ptr<const void> owner;
    std::thread::id executor;
    std::uint64_t bindings{},tick{};
    std::int32_t render_frame{};
    std::vector<Impl::State> scenery;
};
struct SourceSceneryScene::PreparedRestore::Data {
    Checkpoint::Data replacement;
    std::uint64_t revision{};
    bool consumed{};
};
bool SourceSceneryScene::Checkpoint::valid()const noexcept {
    return data_&&!data_->owner.expired();
}
SourceSceneryScene::Checkpoint SourceSceneryScene::checkpoint()const {
    const auto &s=*impl_;s.completed();auto data=std::make_shared<Checkpoint::Data>();
    data->owner=s.owner;data->executor=s.executor;data->bindings=s.bindings;
    data->tick=s.tick;data->render_frame=s.render_frame;data->scenery.reserve(s.scenery.size());
    for(const auto &object:s.scenery)data->scenery.emplace_back(object);
    Checkpoint out;out.data_=std::move(data);return out;
}
bool SourceSceneryScene::owns_checkpoint(const Checkpoint &checkpoint)const noexcept {
    return impl_&&impl_->executor==std::this_thread::get_id()&&checkpoint.data_&&
        checkpoint.data_->owner.lock()==impl_->owner&&checkpoint.data_->executor==impl_->executor&&
        checkpoint.data_->bindings==impl_->bindings&&checkpoint.data_->scenery.size()==impl_->scenery.size();
}
SourceSceneryScene::PreparedRestore::PreparedRestore()=default;
SourceSceneryScene::PreparedRestore::~PreparedRestore()=default;
SourceSceneryScene::PreparedRestore::PreparedRestore(PreparedRestore &&)noexcept=default;
SourceSceneryScene::PreparedRestore &SourceSceneryScene::PreparedRestore::operator=(PreparedRestore &&)noexcept=default;
SourceSceneryScene::PreparedRestore SourceSceneryScene::prepare_restore(const Checkpoint &checkpoint)const {
    const auto &s=*impl_;s.completed();
    if(!owns_checkpoint(checkpoint)||s.revision==std::numeric_limits<std::uint64_t>::max())
        throw FormatError("SCEN checkpoint has a different resident owner or callback binding");
    PreparedRestore out;out.data_=std::make_unique<PreparedRestore::Data>(PreparedRestore::Data{*checkpoint.data_,s.revision,false});
    return out;
}
bool SourceSceneryScene::restore_ready(const PreparedRestore &prepared)const noexcept {
    const auto *p=prepared.data_.get();return impl_&&impl_->executor==std::this_thread::get_id()&&!impl_->busy&&p&&!p->consumed&&
        p->replacement.owner.lock()==impl_->owner&&p->replacement.executor==impl_->executor&&
        p->replacement.bindings==impl_->bindings&&p->revision==impl_->revision&&
        impl_->revision!=std::numeric_limits<std::uint64_t>::max()&&p->replacement.scenery.size()==impl_->scenery.size();
}
bool SourceSceneryScene::commit_restore(PreparedRestore &prepared)noexcept {
    if(!restore_ready(prepared))return false;
    auto &s=*impl_;auto &p=*prepared.data_;using std::swap;
    swap(s.tick,p.replacement.tick);swap(s.render_frame,p.replacement.render_frame);
    for(std::size_t i=0;i<s.scenery.size();++i)p.replacement.scenery[i].exchange(s.scenery[i]);
    p.consumed=true;++s.revision;return true;
}
SourceSceneryScene::SourceSceneryScene(const Cache &c,std::span<const ScenarioObject>objects,std::size_t bsp,
    Mesh &mesh,const CollisionWorld &cw,InitialSelection selected,OmitUnresolvedSelection omit,bool region_union)
    :impl_(std::make_unique<Impl>(c,objects,bsp,mesh,cw,selected,omit,region_union)){}
SourceSceneryScene::~SourceSceneryScene()=default;
void SourceSceneryScene::reset_lifetimes() noexcept {
    if(!impl_)return;Impl::Mutation mutation(*impl_,std::nothrow);if(!mutation)return;
    auto &s=*impl_;s.tick=0;s.render_frame=0;
    for(auto &o:s.scenery){
        o.handle=0xffffffffu;o.created_tick=0;o.active=o.visible=false;
        o.previous_lod.fill(-1);o.solved_nodes.reset();o.world_nodes.clear();
        o.light={};o.query_state_valid=false;o.source_views={};
    }
}
SourceSceneryScene::SourceSceneryScene(SourceSceneryScene&&)noexcept=default;
SourceSceneryScene&SourceSceneryScene::operator=(SourceSceneryScene&&)noexcept=default;
void SourceSceneryScene::synchronize(const SceneObjects &scene,LifetimeSelection selected){Impl::Mutation mutation(*impl_);impl_->synchronize(scene,selected);}
void SourceSceneryScene::interpolate(const SceneObjects &scene,const SceneInterpolation &history,float fraction){
    Impl::Mutation mutation(*impl_);
    for(auto &s:impl_->scenery)if(s.active){
        const auto &object=scene.get(s.handle);
        s.root=history.sample(object,fraction);s.scale=s.root.scale;
        if(object.solved_nodes)s.solved_nodes=history.sample_nodes(object,fraction);
    }
}
void SourceSceneryScene::lighting_query_state(LightingQueryState state){Impl::Mutation mutation(*impl_);impl_->rebind();impl_->query_state=std::move(state);}
void SourceSceneryScene::animation_time(AnimationTime time){Impl::Mutation mutation(*impl_);impl_->rebind();impl_->animation_time=std::move(time);}
void SourceSceneryScene::animation_frame(AnimationFrame frame){Impl::Mutation mutation(*impl_);impl_->rebind();impl_->animation_frame=std::move(frame);}
void SourceSceneryScene::animation_frames(AnimationFrames frames){Impl::Mutation mutation(*impl_);impl_->rebind();impl_->animation_frames=std::move(frames);}
void SourceSceneryScene::birth_query_capture(BirthQueryCapture capture){
    Impl::Mutation mutation(*impl_);
    if(capture && std::any_of(impl_->scenery.begin(),impl_->scenery.end(),[](const auto &s){return s.handle!=0xffffffffu;}))
        throw FormatError("SCEN birth capture must register before lifetime synchronization");
    impl_->rebind();impl_->birth_capture=std::move(capture);
}
void SourceSceneryScene::update_pose(std::uint64_t tick,std::int32_t frame,Mesh&){Impl::Mutation mutation(*impl_);impl_->update_pose(tick,frame);}
std::vector<SourceSceneEffectOwner> SourceSceneryScene::effect_owners() const {
    impl_->own();
    std::vector<SourceSceneEffectOwner> out;
    for(const auto &s:impl_->scenery)if(s.active && s.handle!=0xffffffffu)
        out.push_back({s.source_object,s.handle,s.created_tick,s.world_nodes,s.selection.permutations,s.root.position});
    return out;
}
void SourceSceneryScene::append_view(std::vector<Vertex>&vertices,std::vector<ObjectLightingBinding>&bindings,
    const SourceObjectViewLod &view,std::vector<SourceObjectViewSelection>*selections) {
    Impl::Mutation mutation(*impl_);
    for(auto &s:impl_->scenery)if(s.active && s.visible) {
        if(const auto& source=s.source_views.at(view.view_index);source) {
            if(!source->submitted)throw FormatError("Retained native SCEN draw was not submitted");
            continue;
        }
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
std::vector<std::uint32_t> SourceSceneryScene::source_render_models()const {
    impl_->own();std::set<std::uint32_t> roots;
    for(const auto& s:impl_->scenery)if(!s.resource_error && s.initial.render_model!=~0u)roots.insert(s.initial.render_model);
    return {roots.begin(),roots.end()};
}
void SourceSceneryScene::begin_source_frame() {
    Impl::Mutation mutation(*impl_);for(auto& s:impl_->scenery)for(auto& view:s.source_views)view.reset();
}
bool SourceSceneryScene::retain_source_view(unsigned view,std::span<const SourceLiveModelBaseDrawTu15> draws,
    std::span<const Transform> nodes,const SourceModelShaderView& shader) {
    Impl::Mutation mutation(*impl_);if(view>=4)throw FormatError("Native SCEN view exceeds four players");
    if(draws.empty())return false;
    const auto at=std::find_if(impl_->scenery.begin(),impl_->scenery.end(),[&](const auto& s){return s.active&&s.visible&&s.handle==draws.front().object;});
    if(at==impl_->scenery.end())return false;auto& scenery=*at;
    if(scenery.resource_error)throw FormatError("Native SCEN draw requires its actual resident resources: "+*scenery.resource_error);
    if(scenery.source_views[view])throw FormatError("Native SCEN owner was retained twice");
    Impl::SourceView retained;
    for(const auto& draw:draws) {
        if(draw.object!=scenery.handle || draw.definition!=scenery.authored.definition || draw.unit.words[1]!=scenery.initial.render_model || draw.lod>=6)
            throw FormatError("Native SCEN draw lost its actual object/definition/model/LOD");
        if(draw.override_slot!=255 || draw.lighting_class!=(scenery.dynamic?2u:1u))return false;
        const auto& geometry=scenery.lods[draw.lod];const auto section=(draw.unit.words[3]>>9)&511u;
        if(std::find(geometry.sections.begin(),geometry.sections.end(),section)==geometry.sections.end())
            throw FormatError("Native SCEN selected an unprepared original section");
        const auto posed=skin_model_world_nodes(geometry.model,nodes);
        std::map<std::pair<unsigned,unsigned>,unsigned> parts;
        for(const auto& part:geometry.shader_parts)if(part.section==section) {
            const auto choice=source_model_shader_part({0,draw.coverage},part.scale,impl_->shader_definitions.at(part.shader),shader).choice;
            parts.emplace(std::pair{part.section,part.part},choice.accepted&&choice.index>=0?
                scenery.shader_slots.at(part.material).at(unsigned(choice.index)):~0u);
        }
        Impl::SourceSection selected;selected.lod=draw.lod;selected.colors=source_live_model_base_colors_tu15(draw,scenery.initial_colors);
        for(unsigned n=0;n<posed.size();++n)if(posed[n].source_section==section) {
            const auto binding=parts.find({posed[n].source_section,posed[n].source_part});
            if(binding==parts.end())throw FormatError("Native SCEN vertex lost its actual shader part");
            if(binding->second==~0u)continue;
            if(impl_->unresolved_materials.contains(binding->second))throw FormatError("Native SCEN selected unresolved shader inputs");
            selected.vertices.push_back(posed[n]);selected.vertices.back().material=binding->second;
            selected.vertices.back().source_stipple_level=source_shader_instance_stipple(std::uint8_t(draw.unit.words[6]>>16)).level;
            selected.ordinals.push_back(n);
        }
        retained.sections.push_back(std::move(selected));
    }
    scenery.source_views[view]=std::move(retained);return true;
}
void SourceSceneryScene::append_source_models(std::vector<ModelObjectDraw>& output,const SourceObjectViewLod& view) {
    Impl::Mutation mutation(*impl_);
    for(auto& scenery:impl_->scenery)if(scenery.active&&scenery.visible) {
        auto& source=scenery.source_views.at(view.view_index);if(!source)continue;
        if(source->submitted || !view.input)throw FormatError("Native SCEN submission lost its unique lighting owner");
        impl_->retain_query_state(scenery);
        const auto sphere=view.source_bound?view.source_bound(scenery.handle):ShadowCasterSphere{scenery.query_state.sphere_center,scenery.query_state.sphere_radius};
        const auto input=view.input(scenery.source_object,scenery.handle,sphere.center,sphere.radius);
        impl_->query_lighting(scenery,input.draw_metric);
        for(const auto& section:source->sections)if(!section.vertices.empty()) {
            output.push_back({section.vertices,scenery.dynamic?std::optional{scenery.probe?scenery.probe->lighting:scenery.light.lighting}:std::nullopt,section.colors});
            output.back().source_model_selection=SourceModelDepthSelection{scenery.handle,scenery.authored.definition,
                scenery.initial.render_model,section.lod,&scenery.lods[section.lod].model,0,section.ordinals};
        }
        source->submitted=true;
    }
}
const SourceScenerySceneStats&SourceSceneryScene::stats()const{return impl_->counts;}
std::span<const std::size_t>SourceSceneryScene::unresolved_selection_owners()const{return impl_->unresolved_owners;}
std::span<const SourceSceneryResourceBoundary>SourceSceneryScene::deferred_resource_owners()const{return impl_->deferred_resources;}
}
