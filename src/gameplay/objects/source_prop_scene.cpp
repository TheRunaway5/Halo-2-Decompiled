#include "halo2/source_prop_scene.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_prt.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/scenery.hpp"
#include "halo2/appearance.hpp"
#include "halo2/object_light_query.hpp"
#include "halo2/source_object_colors.hpp"
#include "halo2/renderer.hpp"
#include "halo2/world_material.hpp"
#include "halo2/math.hpp"
#include "gameplay/objects/source_object_geometry.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <cstring>
#include <cstdlib>
#include <cfenv>
#include <limits>
#include <thread>
#include <type_traits>
#include <utility>
#if defined(__SSE2__) || defined(_M_X64)
#include <xmmintrin.h>
#endif

namespace halo2 {
struct SourcePropScene::Impl {
    std::function<void(unsigned,std::uint32_t,std::uint32_t,const ObjectLightingState&,bool,unsigned,std::span<const Vertex>)> source_draw_observer;
    std::function<void(unsigned,std::uint32_t,std::uint32_t,std::span<const Vertex>)> draw_observer;
    struct SourceMainSection {
        unsigned lod{};
        std::vector<Vertex> vertices;
        ObjectColors colors;
    };
    struct SourceMainView {std::vector<SourceMainSection> sections;bool submitted{};};
    struct Prop {
        std::size_t source_object{};
        std::uint32_t handle=0xffffffffu;
        std::uint64_t created_tick{};
        bool active=true,visible=true;
        bool source_regions{};
        std::int32_t prepared_variant=-1;
        SourcePropDefinition definition;
        std::uint32_t object_definition=0xffffffffu;
        SourceObjectGeometry geometry;
        SourcePropQueryState query_state;
        bool query_state_valid{};
        Transform object;
        std::optional<Transform> posed_object;
        std::optional<std::vector<Transform>> solved_nodes,posed_nodes;
        int posed_rounding{};
        unsigned posed_mxcsr{};
        float scale=1;
        std::vector<Transform> world_nodes;
        std::vector<std::uint8_t> active_permutations;
        ObjectLightingCache light;
        ObjectColors initial_colors;
        std::array<std::optional<SourceMainView>,4> source_views;
    };
    // A historical frame owns only changing presentation values. Authored
    // definitions, section/material catalogs and callback owners stay resident.
    struct PropPresentation {
        std::uint32_t handle=0xffffffffu;
        std::uint64_t created_tick{};
        bool active{},visible{};
        SourceObjectGeometry::PresentationState geometry;
        SourcePropQueryState query_state;
        bool query_state_valid{};
        Transform object;
        std::optional<Transform> posed_object;
        std::optional<std::vector<Transform>> solved_nodes,posed_nodes;
        int posed_rounding{};
        unsigned posed_mxcsr{};
        float scale=1;
        std::vector<Transform> world_nodes;
        std::vector<std::uint8_t> active_permutations;
        ObjectLightingCache light;
        std::array<std::optional<SourceMainView>,4> source_views;
    };
    std::vector<PropPresentation> capture_props()const {
        std::vector<PropPresentation> out;out.reserve(props.size());
        for(const auto &p:props)out.push_back({p.handle,p.created_tick,p.active,p.visible,
            p.geometry.capture_presentation(),p.query_state,p.query_state_valid,p.object,p.posed_object,
            p.solved_nodes,p.posed_nodes,p.posed_rounding,p.posed_mxcsr,p.scale,p.world_nodes,
            p.active_permutations,p.light,p.source_views});
        return out;
    }
    void exchange_props(std::vector<PropPresentation> &saved)noexcept {
        static_assert(std::is_nothrow_swappable_v<PropPresentation>);
        for(std::size_t n=0;n<props.size();++n) {
            auto &p=props[n];auto &v=saved[n];
            using std::swap;
            swap(p.handle,v.handle);swap(p.created_tick,v.created_tick);
            swap(p.active,v.active);swap(p.visible,v.visible);p.geometry.exchange_presentation(v.geometry);
            swap(p.query_state,v.query_state);swap(p.query_state_valid,v.query_state_valid);
            swap(p.object,v.object);swap(p.posed_object,v.posed_object);
            swap(p.solved_nodes,v.solved_nodes);swap(p.posed_nodes,v.posed_nodes);
            swap(p.posed_rounding,v.posed_rounding);swap(p.posed_mxcsr,v.posed_mxcsr);swap(p.scale,v.scale);
            p.world_nodes.swap(v.world_nodes);p.active_permutations.swap(v.active_permutations);
            swap(p.light,v.light);p.source_views.swap(v.source_views);
        }
    }
    const Cache &cache;
    const CollisionWorld &collision;
    Mesh &mesh;
    BspLighting lighting;
    BspSurfaceLighting surface;
    ObjectLightingDefinition conversion;
    SourcePrtLighting sh;
    SourcePropSceneStats counts;
    LightingQueryState lighting_state;
    std::function<const SourceInitialModelSelection *(std::uint32_t)> region_state;
    std::int32_t render_frame{};
    std::vector<Prop> props;
    std::set<std::size_t> prop_placements,nonrendered_placements;
    const std::thread::id executor=std::this_thread::get_id();
    const std::shared_ptr<const int> identity=std::make_shared<const int>(0);
    std::uint64_t revision{},bindings{};
    bool mutating{};

    void own()const {
        if(std::this_thread::get_id()!=executor)throw FormatError("Source prop scene crossed its executor");
    }
    struct Mutation {
        Impl &state;
        explicit Mutation(Impl &s,bool binding=false):state(s) {
            s.own();
            if(s.mutating||s.revision==std::numeric_limits<std::uint64_t>::max()||
                (binding&&s.bindings==std::numeric_limits<std::uint64_t>::max()))
                throw FormatError("Source prop scene mutation is reentrant or exhausted");
            ++s.revision;if(binding)++s.bindings;s.mutating=true;
        }
        ~Mutation(){state.mutating=false;}
    };
    // update_banks changes only these immutable PRT bank links in the resident
    // material passes. Copying Material would alias the mutable pass object.
    struct MaterialBank {
        unsigned material{};
        std::shared_ptr<Material> pass;
        std::shared_ptr<const VertexProgramBank> bank;
    };
    // Scalar schema only: retaining a pass shared_ptr here would change normal
    // render-time copy-on-write decisions even when no checkpoint exists.
    struct MaterialOwner {
        unsigned material{};
        std::uint32_t shader{};
        bool environment{};
        unsigned vertex{};
    };
    std::vector<MaterialOwner> resident_materials;
    bool material_schema()const noexcept {
        for(const auto &slot:resident_materials) {
            if(slot.material>=mesh.materials.size())return false;
            const auto &m=mesh.materials[slot.material];
            if(m.source_shader_datum!=slot.shader||bool(m.environment_pass)!=slot.environment||
                (m.environment_pass&&m.environment_pass->forward_vertex!=slot.vertex)||
                (slot.environment&&slot.vertex==9&&!m.environment_pass->vertex_bank))return false;
        }
        return true;
    }
    std::vector<MaterialBank> material_banks()const {
        if(!material_schema())throw FormatError("Source prop resident material schema changed");
        std::vector<MaterialBank> out;
        for(const auto &slot:resident_materials)if(slot.environment&&slot.vertex==9) {
            const auto &material=mesh.materials[slot.material];
            out.push_back({slot.material,material.environment_pass,material.environment_pass->vertex_bank});
        }
        return out;
    }
    bool material_owners(const std::vector<MaterialBank> &banks)const noexcept {
        if(!material_schema())return false;
        std::size_t n{};
        for(const auto &slot:resident_materials)if(slot.environment&&slot.vertex==9) {
            if(n>=banks.size()||banks[n].material!=slot.material||!banks[n].pass||
                mesh.materials[slot.material].environment_pass!=banks[n].pass)return false;
            ++n;
        }
        return n==banks.size();
    }
    bool material_target(const std::vector<MaterialBank> &banks,
        const MaterialPresentationContinuity::TargetGraph &target)const noexcept {
        std::size_t n{};
        for(const auto &slot:resident_materials)if(slot.environment&&slot.vertex==9) {
            if(n>=banks.size()||banks[n].material!=slot.material||!banks[n].pass||!banks[n].bank||
                !target.matches_environment(mesh,slot.material,banks[n].pass.get(),banks[n].bank))return false;
            ++n;
        }
        return n==banks.size();
    }

    Impl(const Cache &c,std::span<const ScenarioObject> objects,std::size_t bsp,Mesh &mesh,const CollisionWorld &cw,bool region_union)
        :cache(c),collision(cw),mesh(mesh),lighting(c.load_bsp_lighting(bsp)),surface(c.load_bsp_surface_lighting(bsp)),
         conversion(c.load_object_lighting_definition()),sh(c.load_source_prt_lighting(bsp)) {
        for(std::size_t source_object=0;source_object<objects.size();++source_object){
            const auto&object=objects[source_object];
            if(object.type==11)prop_placements.insert(source_object);
            // Prepare authored resources independently of source birth/BSP
            // admission. Persistent owners can originate in another BSP;
            // synchronize admits geometry only for their actual live lifetime.
            if(object.type!=11 || object.definition==null_datum)continue;
            ++counts.placements;
            Prop p;p.source_object=source_object;p.object_definition=object.definition;p.definition=cache.load_source_prop_definition(object.definition);
            if(!p.definition.constructor_admitted)
                throw FormatError("source prop constructor has no physics body bank: "+p.definition.name);
            if(p.definition.render_model==0xffffffffu){nonrendered_placements.insert(source_object);continue;}
            if(!source_object_light_query_bank_valid(lighting))
                throw FormatError("prop model lighting implementation3 requires original active bank: "+p.definition.name);
            //11BBF0 has no named-idle override. Its generic1CB0D0 initial
            //controller is NONE for all supplied placements. A later active
            //controller requires its actual frame/channel producer.
            if(cache.load_initialized_object_idle(object.definition))
                throw FormatError("prop initial controller requires source frame state: "+p.definition.name);
            const auto initial_definition=cache.load_initial_model_definition(object.definition);
            const auto selection=cache.load_initial_model_selection(object.definition,object.variant);
            p.source_regions=region_union;p.prepared_variant=selection.variant_index;
            if(region_union) {
                // Resource preparation does not select an RNG outcome. Current
                // original region bytes bind only with the actual full lifetime.
                p.active=false;p.visible=false;
                p.geometry.load_union(cache,object,initial_definition,p.definition.render_model);
            } else {
                p.active_permutations=selection.permutations;
                if(!selection.resolved())
                    throw FormatError("prop initial model RNG selection requires source state: "+p.definition.name);
                p.geometry.load(cache,object,initial_definition,selection,p.definition.render_model,true);
            }
            const auto basis=authored_basis(object.rotation);p.scale=object.scale;
            p.object=source_object_transform(object.position,basis.forward,basis.up,p.scale,false);
            pose(p);
            const auto color_definition=cache.load_source_object_color_definition(object.definition);
            for(const auto&channel:color_definition.channels)if(channel.function_count)
                throw FormatError("prop current color function requires source inputs: "+p.definition.name);
            const auto actual_variant=selection.variant_index<0?0u:
                initial_definition.variants.at(unsigned(selection.variant_index)).name;
            const auto colors=source_object_initial_colors(color_definition,object.position,actual_variant);
            p.initial_colors=colors;
            const auto resident=p.geometry.prepare(cache,colors,p.world_nodes,sh,mesh);
            counts.dynamic_materials+=resident.dynamic_materials;counts.prt_materials+=resident.prt_materials;
            counts.section_banks+=resident.section_banks;
            counts.resident_dynamic_materials+=resident.resident_dynamic_materials;
            counts.resident_prt_materials+=resident.resident_prt_materials;
            counts.union_dynamic_materials+=resident.union_dynamic_materials;
            counts.union_prt_materials+=resident.union_prt_materials;
            counts.union_section_banks+=resident.union_section_banks;
            props.push_back(std::move(p));++counts.render_models;
        }
        for(const auto &p:props)for(unsigned n=0;n<p.geometry.material_count;++n) {
            const auto slot=p.geometry.first_material+n;const auto &m=mesh.materials.at(slot);
            resident_materials.push_back({slot,m.source_shader_datum,bool(m.environment_pass),
                m.environment_pass?m.environment_pass->forward_vertex:0});
        }
    }
    static void pose(Prop&p){
        // Retain immutable geometry only while both the root and every source
        // WORLD node are unchanged. A solved child body can move independently
        // of its root; it must invalidate the posed geometry too.
        const int rounding=std::fegetround();unsigned mxcsr=0;
#if defined(__SSE2__) || defined(_M_X64)
        // Ignore sticky exception flags; retain arithmetic mode, including
        // denormal-input and flush-to-zero behavior.
        mxcsr=_mm_getcsr()&0xffc0u;
#endif
        const auto *reference=std::getenv("HALO2_REFERENCE_REPOSE_STATIC_PROPS");
        const bool same_nodes=bool(p.solved_nodes)==bool(p.posed_nodes) &&
            (!p.solved_nodes || (p.solved_nodes->size()==p.posed_nodes->size() &&
            (p.solved_nodes->empty() || !std::memcmp(p.solved_nodes->data(),p.posed_nodes->data(),p.solved_nodes->size()*sizeof(Transform)))));
        const bool reuse=!(reference && *reference) && p.posed_object && same_nodes &&
            p.posed_rounding==rounding && p.posed_mxcsr==mxcsr &&
            !std::memcmp(&*p.posed_object,&p.object,sizeof(Transform));
        const auto *verify=std::getenv("HALO2_VERIFY_STATIC_PROP_POSES");
        if(reuse && !(verify && *verify))return;
        std::vector<Transform> old_nodes;
        std::array<std::vector<Vertex>,6> old_vertices;
        if(reuse) {
            old_nodes=p.world_nodes;
            for(unsigned lod=0;lod<6;++lod)old_vertices[lod]=p.geometry.at(lod).vertices;
        }
        const auto&model=p.geometry.model();
        p.world_nodes=p.solved_nodes?*p.solved_nodes:pose_model_nodes_world(model,p.object);
        if(p.world_nodes.empty())throw FormatError("prop model has no query node0: "+p.definition.name);
        p.geometry.pose_world([&](const RenderModel&model){return p.solved_nodes?
            skin_model_world_nodes(model,p.world_nodes):pose_model_world(model,p.object);});
        if(reuse) {
            if(old_nodes.size()!=p.world_nodes.size() || (!old_nodes.empty() &&
                std::memcmp(old_nodes.data(),p.world_nodes.data(),old_nodes.size()*sizeof(Transform))))
                throw FormatError("retained static prop changed complete WORLD nodes");
            for(unsigned lod=0;lod<6;++lod) {
                const auto &actual=p.geometry.at(lod).vertices,&expected=old_vertices[lod];
                if(actual.size()!=expected.size() || (!actual.empty() &&
                    std::memcmp(actual.data(),expected.data(),actual.size()*sizeof(Vertex))))
                    throw FormatError("retained static prop changed complete posed LOD vertices");
            }
        }
        static_assert(sizeof(Transform)==13*sizeof(float));
        p.posed_object=p.object;p.posed_nodes=p.solved_nodes;p.posed_rounding=rounding;p.posed_mxcsr=mxcsr;
    }
    void synchronize(const SceneObjects&scene){
        std::map<std::size_t,const SceneObjects::Object*> live;
        for(const auto&object:scene.objects())if(object.handle!=0xffffffffu){
            // This bank owns type11 placements. Other scene types may retain
            // multiple generations of one netgame row; they are not props.
            if(!prop_placements.contains(object.placement))continue;
            const auto [at,inserted]=live.emplace(object.placement,&object);
            if(!inserted)
                throw FormatError("prop lifetime has duplicate live source placement="+
                    std::to_string(object.placement)+" handles="+std::to_string(at->second->handle)+
                    ","+std::to_string(object.handle)+" prop="+
                    std::to_string(prop_placements.contains(object.placement)));
            if(prop_placements.contains(object.placement) && !nonrendered_placements.contains(object.placement) &&
                std::none_of(props.begin(),props.end(),[&](const auto&p){return p.source_object==object.placement;}))
                throw FormatError("live prop placement requires unprepared source geometry");
        }
        for(auto&p:props){
            const auto at=live.find(p.source_object);
            if(at==live.end()){p.active=false;p.visible=false;continue;}
            const auto&object=*at->second;
            if(p.handle!=object.handle){p.handle=object.handle;p.created_tick=object.created_tick;p.light={};p.geometry.reset_history();p.query_state_valid=false;}
            else if(p.created_tick!=object.created_tick)
                throw FormatError("prop full handle changed its source creation tick");
            p.object=scene_object_transform(object);p.scale=p.object.scale;
            p.solved_nodes=object.solved_nodes;
            const auto *selection=region_state?region_state(p.handle):nullptr;
            if(p.source_regions && !selection)
                throw FormatError("prop source union requires actual current region bytes: "+p.definition.name);
            if(selection) {
                if(p.source_regions && selection->variant_index!=p.prepared_variant)
                    throw FormatError("prop source variant requires its actual named color context: "+p.definition.name);
                p.geometry.regions(*selection);p.active_permutations=selection->permutations;
            }
            p.active=true;p.visible=object.visible;
        }
    }
    void update_pose(std::int32_t frame,Mesh&mesh){
        render_frame=frame;
        for(auto&p:props){
            if(!p.active || !p.visible)continue;
            pose(p);p.geometry.update_banks(cache,p.world_nodes,sh,mesh);
            p.query_state_valid=false;
        }
    }
    void retain_query_state(Prop&p){
        if(p.query_state_valid)return;
        if(!lighting_state)
            throw FormatError("prop lighting requires source cached/physics query state: "+p.definition.name);
        p.query_state=lighting_state(p.source_object,p.handle,p.world_nodes.front(),p.scale);
        p.query_state_valid=true;
    }
    void query_lighting(Prop&p,std::int32_t frame,float distance){
        //3DFE0 deduplicates actual queries, not cadence evaluations: a later
        //view can cross the distance interval after an earlier view skipped it.
        retain_query_state(p);const auto&state=p.query_state;const auto origin=state.sphere.center;
        const bool due=object_lighting_query_due(p.light,frame,origin,distance,state.frozen);
        ObjectLightQueryContext context;context.bank_valid=source_object_light_query_bank_valid(lighting);
        context.object_type=11;context.definition_flags=p.definition.object_flags;context.runtime_flags=state.runtime_flags;
        const auto query=due?source_object_light_query(context,[&](unsigned mode){
            return query_object_lightmap(surface,collision,origin,ObjectLightMapQueryContext{mode,
                !(p.light.sample_valid && p.light.lighting_valid),1.f,p.light.target});},
            []{return ObjectLightQueryResult{};}):ObjectLightQueryAdmission{};
        if(query.query.status==ObjectLightQueryStatus::attribution_unresolved)
            throw FormatError("prop source lighting contact attribution unresolved: "+p.definition.name);
        update_object_lighting_cache(p.light,conversion,11,frame,origin,state.velocity,due,
            query.accepted?query.query.sample:std::optional<ObjectLightmapSample>{},true,state.frozen);
    }
    void update(std::int32_t frame,Vec3 eye,float draw_scale,Mesh&mesh){
        update_pose(frame,mesh);
        for(auto&p:props)if(p.active && p.visible){retain_query_state(p);
            query_lighting(p,frame,source_geometry_lod_distance(p.query_state.sphere.center,eye,draw_scale));}
    }

};
struct SourcePropScene::Checkpoint::Data {
    std::weak_ptr<const int> owner;
    std::thread::id executor;
    std::uint64_t bindings{};
    const Mesh *mesh{};
    std::int32_t render_frame{};
    std::vector<Impl::PropPresentation> props;
    std::vector<Impl::MaterialBank> banks;
};
struct SourcePropScene::PreparedData {
    std::weak_ptr<const int> owner;
    std::thread::id executor;
    std::uint64_t revision{},bindings{};
    const Mesh *mesh{};
    std::int32_t render_frame{};
    std::vector<Impl::PropPresentation> props;
    std::vector<Impl::MaterialBank> banks;
    std::vector<std::shared_ptr<const VertexProgramBank>> expected_banks;
    std::optional<MaterialPresentationContinuity::TargetGraph> target;
    bool consumed{};
};
SourcePropScene::SourcePropScene(const Cache&c,std::span<const ScenarioObject>objects,std::size_t bsp,Mesh&mesh,
    const CollisionWorld&collision,bool source_regions):impl_(std::make_unique<Impl>(c,objects,bsp,mesh,collision,source_regions)){}
SourcePropScene::~SourcePropScene()=default;
bool SourcePropScene::Checkpoint::valid()const noexcept {
    return data_&&!data_->owner.expired()&&data_->executor==std::this_thread::get_id();
}
SourcePropScene::Checkpoint SourcePropScene::checkpoint()const {
    if(!impl_)throw FormatError("Moved source prop scene has no presentation owner");
    const auto &s=*impl_;s.own();
    if(s.mutating)throw FormatError("Source prop checkpoint requires a completed presentation call");
    Checkpoint out;out.data_=std::make_shared<const Checkpoint::Data>(Checkpoint::Data{
        s.identity,s.executor,s.bindings,&s.mesh,s.render_frame,s.capture_props(),s.material_banks()});return out;
}
bool SourcePropScene::owns_checkpoint(const Checkpoint &checkpoint)const noexcept {
    if(!impl_||!checkpoint.data_)return false;
    const auto &s=*impl_;const auto &p=*checkpoint.data_;
    return std::this_thread::get_id()==s.executor&&p.executor==s.executor&&p.owner.lock()==s.identity&&
        p.bindings==s.bindings&&p.mesh==&s.mesh&&p.props.size()==s.props.size()&&s.material_owners(p.banks);
}
SourcePropScene::PreparedRestore::PreparedRestore()=default;
SourcePropScene::PreparedRestore::~PreparedRestore()=default;
SourcePropScene::PreparedRestore::PreparedRestore(PreparedRestore &&)noexcept=default;
SourcePropScene::PreparedRestore &SourcePropScene::PreparedRestore::operator=(PreparedRestore &&)noexcept=default;
SourcePropScene::PreparedRestore SourcePropScene::prepare_restore(const Checkpoint &checkpoint)const {
    if(!owns_checkpoint(checkpoint)||impl_->mutating||impl_->revision==std::numeric_limits<std::uint64_t>::max())
        throw FormatError("Source prop restore lacks its completed owner, bindings or resident material passes");
    const auto &s=*impl_;const auto &saved=*checkpoint.data_;
    auto data=std::make_unique<PreparedData>(PreparedData{s.identity,s.executor,s.revision,s.bindings,
        &s.mesh,saved.render_frame,saved.props,saved.banks,{},{},false});
    data->expected_banks.reserve(data->banks.size());
    for(const auto &bank:data->banks)data->expected_banks.push_back(bank.pass->vertex_bank);
    PreparedRestore out;out.data_=std::move(data);return out;
}
SourcePropScene::PreparedRestore SourcePropScene::prepare_restore(const Checkpoint &checkpoint,
    const MaterialPresentationContinuity::TargetGraph &target)const {
    if(!impl_||!checkpoint.data_)throw FormatError("Source prop restore has no historical owner");
    const auto &s=*impl_;const auto &saved=*checkpoint.data_;
    if(std::this_thread::get_id()!=s.executor||saved.executor!=s.executor||saved.owner.lock()!=s.identity||
        saved.bindings!=s.bindings||saved.mesh!=&s.mesh||saved.props.size()!=s.props.size()||
        s.mutating||s.revision==std::numeric_limits<std::uint64_t>::max()||
        !(target.ready()||target.applied())||!s.material_target(saved.banks,target))
        throw FormatError("Source prop restore lacks its exact prepared historical material graph");
    PreparedRestore out;out.data_=std::make_unique<PreparedData>(PreparedData{s.identity,s.executor,s.revision,
        s.bindings,&s.mesh,saved.render_frame,saved.props,saved.banks,{},target,false});return out;
}
bool SourcePropScene::restore_ready(const PreparedRestore &prepared)const noexcept {
    if(!impl_||!prepared.data_)return false;
    const auto &s=*impl_;const auto &p=*prepared.data_;
    if(std::this_thread::get_id()!=s.executor||p.executor!=s.executor||p.owner.lock()!=s.identity||p.consumed||
        s.mutating||p.revision!=s.revision||s.revision==std::numeric_limits<std::uint64_t>::max()||
        p.bindings!=s.bindings||p.mesh!=&s.mesh||p.props.size()!=s.props.size())return false;
    if(p.target)return (p.target->ready()||p.target->applied())&&s.material_target(p.banks,*p.target);
    if(!s.material_owners(p.banks)||p.expected_banks.size()!=p.banks.size())return false;
    for(std::size_t n=0;n<p.banks.size();++n)if(p.banks[n].pass->vertex_bank!=p.expected_banks[n])return false;
    return true;
}
bool SourcePropScene::commit_restore(PreparedRestore &prepared)noexcept {
    if(!restore_ready(prepared)||(prepared.data_->target&&!prepared.data_->target->applied()))return false;
    static_assert(std::is_nothrow_swappable_v<std::vector<Impl::PropPresentation>>);
    static_assert(std::is_nothrow_swappable_v<std::shared_ptr<const VertexProgramBank>>);
    static_assert(std::is_nothrow_swappable_v<std::int32_t>);
    auto &s=*impl_;auto &p=*prepared.data_;
    // A bound material owner has already installed the exact target pass/bank
    // topology. The direct overload instead publishes these same-pass links.
    for(std::size_t n=0;!p.target&&n<p.banks.size();++n) {
        // Material alternatives may share one pass. Publish its bank once;
        // repeating the swap would restore the discarded future on the alias.
        bool first=true;for(std::size_t before=0;before<n;++before)if(p.banks[before].pass==p.banks[n].pass){first=false;break;}
        if(first)p.banks[n].pass->vertex_bank.swap(p.banks[n].bank);
    }
    s.exchange_props(p.props);std::swap(s.render_frame,p.render_frame);++s.revision;p.consumed=true;return true;
}
void SourcePropScene::reset_lifetimes() noexcept {
    if(!impl_||std::this_thread::get_id()!=impl_->executor||impl_->mutating||
        impl_->revision==std::numeric_limits<std::uint64_t>::max())return;
    auto &s=*impl_;Impl::Mutation mutation(s);s.render_frame=0;
    for(auto &p:s.props){
        p.handle=0xffffffffu;p.created_tick=0;p.active=p.visible=false;
        p.posed_object.reset();p.posed_nodes.reset();p.solved_nodes.reset();
        p.world_nodes.clear();if(p.source_regions)p.active_permutations.clear();p.source_views={};
        p.query_state_valid=false;p.light={};p.geometry.reset_history();
    }
}
SourcePropScene::SourcePropScene(SourcePropScene&&)noexcept=default;
SourcePropScene&SourcePropScene::operator=(SourcePropScene&&)noexcept=default;
void SourcePropScene::synchronize(const SceneObjects&scene){Impl::Mutation mutation(*impl_);impl_->synchronize(scene);}
void SourcePropScene::observe_draws(std::function<void(unsigned,std::uint32_t,std::uint32_t,std::span<const Vertex>)> observer){Impl::Mutation mutation(*impl_,true);impl_->draw_observer=std::move(observer);}
void SourcePropScene::region_state(std::function<const SourceInitialModelSelection *(std::uint32_t)> state){Impl::Mutation mutation(*impl_,true);impl_->region_state=std::move(state);}
void SourcePropScene::interpolate(const SceneObjects &scene,const SceneInterpolation &history,float fraction){
    Impl::Mutation mutation(*impl_);
    for(auto &p:impl_->props)if(p.active){
        const auto &object=scene.get(p.handle);
        p.object=history.sample(object,fraction);p.scale=p.object.scale;
        if(object.solved_nodes)p.solved_nodes=history.sample_nodes(object,fraction);
    }
}
void SourcePropScene::lighting_query_state(LightingQueryState state){Impl::Mutation mutation(*impl_,true);impl_->lighting_state=std::move(state);}
void SourcePropScene::update(std::int32_t frame,Vec3 eye,float scale,Mesh&mesh){Impl::Mutation mutation(*impl_);if(&mesh!=&impl_->mesh)throw FormatError("Source prop update has a foreign resident Mesh");impl_->update(frame,eye,scale,mesh);}
void SourcePropScene::update_pose(std::int32_t frame,Mesh&mesh){Impl::Mutation mutation(*impl_);if(&mesh!=&impl_->mesh)throw FormatError("Source prop pose has a foreign resident Mesh");impl_->update_pose(frame,mesh);}
void SourcePropScene::append(std::vector<Vertex>&out,std::vector<ObjectLightingBinding>&bindings)const{
    impl_->own();
    for(const auto&p:impl_->props)if(p.active && p.visible){const auto g=p.geometry.active_draw(5);
        out.insert(out.end(),g.vertices.begin(),g.vertices.end());
        if(g.ordinary_lighting)bindings.push_back({p.geometry.first_material,p.geometry.material_count,p.light.lighting});}
}
void SourcePropScene::append_view(std::vector<Vertex>&out,std::vector<ObjectLightingBinding>&bindings,
    const SourceObjectViewLod&view,std::vector<SourceObjectViewSelection>*selections){
    Impl::Mutation mutation(*impl_);
    if(view.view_index>=4)throw FormatError("source prop view exceeds the original four views");
    for(auto&p:impl_->props)if(p.active && p.visible){
        if(const auto& source=p.source_views.at(view.view_index);source) {
            if(!source->submitted)throw FormatError("retained source prop base was not submitted before legacy view");
            continue;
        }
        impl_->retain_query_state(p);
        const auto selected=p.geometry.select(p.source_object,p.handle,p.query_state.sphere.center,p.query_state.sphere.radius,view);
        const auto&g=p.geometry.view(view.view_index);
        //4B5D0 updates the owner cache before41040 selects part shaders.
        //The selected ordinary flag controls uniform binding only.
        impl_->query_lighting(p,impl_->render_frame,selected.source_distance);
        out.insert(out.end(),g.vertices.begin(),g.vertices.end());
        if(impl_->draw_observer && !g.vertices.empty())
            impl_->draw_observer(view.view_index,p.handle,std::uint32_t(p.source_object),g.vertices);
        if(g.ordinary_lighting && !g.vertices.empty())
            bindings.push_back({p.geometry.first_material,p.geometry.material_count,p.light.lighting});
        if(selections)selections->push_back(selected);
    }
}
void SourcePropScene::observe_source_draws(std::function<void(unsigned,std::uint32_t,std::uint32_t,
    const ObjectLightingState&,bool,unsigned,std::span<const Vertex>)> observer) {
    Impl::Mutation mutation(*impl_,true);
    impl_->source_draw_observer=std::move(observer);
}
std::vector<std::uint32_t> SourcePropScene::source_render_models()const {
    impl_->own();
    std::set<std::uint32_t> models;
    for(const auto& prop:impl_->props)models.insert(prop.definition.render_model);
    return {models.begin(),models.end()};
}
void SourcePropScene::begin_source_frame() {
    Impl::Mutation mutation(*impl_);
    for(auto& prop:impl_->props)for(auto& view:prop.source_views)view.reset();
}
bool SourcePropScene::retain_source_view(unsigned view,std::span<const SourceLiveModelBaseDrawTu15> draws,
    std::span<const Transform> nodes,const SourceModelShaderView& shader_view) {
    Impl::Mutation mutation(*impl_);
    if(view>=4)throw FormatError("source prop view exceeds the original four views");
    if(draws.empty())return false;
    const auto owner=draws.front().object;
    const auto at=std::find_if(impl_->props.begin(),impl_->props.end(),[&](const auto& p){return p.active && p.visible && p.handle==owner;});
    if(at==impl_->props.end())return false;
    auto& prop=*at;Impl::SourceMainView retained;std::size_t count{};
    for(const auto& draw:draws) {
        // Full-owner admission is atomic. Unconsumed PRT/override state keeps
        // its existing path and never receives a mismatched sphere-light pass.
        if(draw.object!=owner || draw.definition!=prop.object_definition)
            throw FormatError("source prop base lost its complete definition identity");
        if(draw.lighting_class!=2 || draw.override_slot!=255 || !(draw.unit.words[0]&1u) ||
           draw.unit.words[1]!=prop.definition.render_model || draw.lod>=6 || nodes.empty())return false;
        const auto section=draw.unit.words[3]>>9&511u;
        const auto& sections=prop.geometry.at(draw.lod).sections;
        if(std::find(sections.begin(),sections.end(),section)==sections.end())return false;
        const SourceModelShaderCollector collector{0,draw.coverage};
        auto selected=prop.geometry.selected_section_world(draw.lod,section,collector,shader_view,nodes);
        if(selected.prt_lighting)return false;
        count+=selected.vertices.size();
        retained.sections.push_back({draw.lod,std::move(selected.vertices),source_live_model_base_colors_tu15(draw,prop.initial_colors)});
    }
    if(!count)return false;
    if(prop.source_views[view])throw FormatError("source prop owner was retained twice in one view");
    prop.source_views[view]=std::move(retained);return true;
}
void SourcePropScene::append_source_models(std::vector<ModelObjectDraw>& out,const SourceObjectViewLod& view) {
    Impl::Mutation mutation(*impl_);
    for(auto& prop:impl_->props)if(prop.active && prop.visible) {
        auto& source=prop.source_views.at(view.view_index);if(!source)continue;
        if(source->submitted)throw FormatError("source prop base was submitted twice in one view");
        if(!view.input)throw FormatError("source prop base lacks its current lighting-distance producer");
        impl_->retain_query_state(prop);
        const auto input=view.input(prop.source_object,prop.handle,prop.query_state.sphere.center,prop.query_state.sphere.radius);
        impl_->query_lighting(prop,impl_->render_frame,input.draw_metric);
        for(const auto& section:source->sections)if(!section.vertices.empty()) {
            out.push_back({section.vertices,prop.light.lighting,section.colors});
            if(impl_->source_draw_observer)impl_->source_draw_observer(view.view_index,prop.handle,prop.object_definition,
                prop.light.lighting,prop.light.lighting_valid,section.lod,section.vertices);
            if(impl_->draw_observer)impl_->draw_observer(view.view_index,prop.handle,std::uint32_t(prop.source_object),section.vertices);
        }
        source->submitted=true;
    }
}
const SourcePropSceneStats&SourcePropScene::stats()const{impl_->own();return impl_->counts;}
std::vector<SourceSceneEffectOwner> SourcePropScene::effect_owners() const {
    impl_->own();
    std::vector<SourceSceneEffectOwner> out;
    for(const auto &p:impl_->props)if(p.active && p.handle!=0xffffffffu)
        out.push_back({p.source_object,p.handle,p.created_tick,p.world_nodes,p.active_permutations,p.object.position});
    return out;
}
}
