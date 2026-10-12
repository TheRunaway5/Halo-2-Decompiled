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
#include "source_object_geometry.hpp"
#include "halo2/render_interpolation.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <cstring>
#include <cstdlib>
#include <cfenv>
#if defined(__SSE2__) || defined(_M_X64)
#include <xmmintrin.h>
#endif

namespace halo2 {
struct SourcePropScene::Impl {
    struct Prop {
        std::size_t source_object{};
        std::uint32_t handle=0xffffffffu;
        std::uint64_t created_tick{};
        bool active=true,visible=true;
        SourcePropDefinition definition;
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
        ObjectLightingCache light;
    };
    const Cache &cache;
    const CollisionWorld &collision;
    BspLighting lighting;
    BspSurfaceLighting surface;
    ObjectLightingDefinition conversion;
    SourcePrtLighting sh;
    SourcePropSceneStats counts;
    LightingQueryState lighting_state;
    std::int32_t render_frame{};
    std::vector<Prop> props;
    std::set<std::size_t> prop_placements,nonrendered_placements;

    Impl(const Cache &c,std::span<const ScenarioObject> objects,std::size_t bsp,Mesh &mesh,const CollisionWorld &cw)
        :cache(c),collision(cw),lighting(c.load_bsp_lighting(bsp)),surface(c.load_bsp_surface_lighting(bsp)),
         conversion(c.load_object_lighting_definition()),sh(c.load_source_prt_lighting(bsp)) {
        for(std::size_t source_object=0;source_object<objects.size();++source_object){
            const auto&object=objects[source_object];
            if(object.type==11)prop_placements.insert(source_object);
            //D5560's NeverPlaced gate controls automatic birth, not whether
            //a later named HS object_create needs resident geometry. Prepare
            //named props too, then synchronize their actual live generation.
            if(object.type!=11 || (!scenery_automatic_creation_allowed(object,2) && object.name<0) ||
                (object.origin_bsp>=0 && object.origin_bsp!=int(bsp)))continue;
            ++counts.placements;
            Prop p;p.source_object=source_object;p.definition=cache.load_source_prop_definition(object.definition);
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
            if(!selection.resolved())
                throw FormatError("prop initial model RNG selection requires source state: "+p.definition.name);
            p.geometry.load(cache,object,initial_definition,selection,p.definition.render_model);
            const auto basis=authored_basis(object.rotation);p.scale=object.scale;
            p.object=source_object_transform(object.position,basis.forward,basis.up,p.scale,false);
            pose(p);
            const auto color_definition=cache.load_source_object_color_definition(object.definition);
            for(const auto&channel:color_definition.channels)if(channel.function_count)
                throw FormatError("prop current color function requires source inputs: "+p.definition.name);
            const auto actual_variant=selection.variant_index<0?0u:
                initial_definition.variants.at(unsigned(selection.variant_index)).name;
            const auto colors=source_object_initial_colors(color_definition,object.position,actual_variant);
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
            if(!live.emplace(object.placement,&object).second)
                throw FormatError("prop lifetime has duplicate live source placement");
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
SourcePropScene::SourcePropScene(const Cache&c,std::span<const ScenarioObject>objects,std::size_t bsp,Mesh&mesh,
    const CollisionWorld&collision):impl_(std::make_unique<Impl>(c,objects,bsp,mesh,collision)){}
SourcePropScene::~SourcePropScene()=default;
SourcePropScene::SourcePropScene(SourcePropScene&&)noexcept=default;
SourcePropScene&SourcePropScene::operator=(SourcePropScene&&)noexcept=default;
void SourcePropScene::synchronize(const SceneObjects&scene){impl_->synchronize(scene);}
void SourcePropScene::interpolate(const SceneObjects &scene,const SceneInterpolation &history,float fraction){
    for(auto &p:impl_->props)if(p.active){
        const auto &object=scene.get(p.handle);
        p.object=history.sample(object,fraction);p.scale=p.object.scale;
        if(object.solved_nodes)p.solved_nodes=history.sample_nodes(object,fraction);
    }
}
void SourcePropScene::lighting_query_state(LightingQueryState state){impl_->lighting_state=std::move(state);}
void SourcePropScene::update(std::int32_t frame,Vec3 eye,float scale,Mesh&mesh){impl_->update(frame,eye,scale,mesh);}
void SourcePropScene::update_pose(std::int32_t frame,Mesh&mesh){impl_->update_pose(frame,mesh);}
void SourcePropScene::append(std::vector<Vertex>&out,std::vector<ObjectLightingBinding>&bindings)const{
    for(const auto&p:impl_->props)if(p.active && p.visible){const auto&g=p.geometry.at(5);
        out.insert(out.end(),g.vertices.begin(),g.vertices.end());
        if(g.ordinary_lighting)bindings.push_back({p.geometry.first_material,p.geometry.material_count,p.light.lighting});}
}
void SourcePropScene::append_view(std::vector<Vertex>&out,std::vector<ObjectLightingBinding>&bindings,
    const SourceObjectViewLod&view,std::vector<SourceObjectViewSelection>*selections){
    for(auto&p:impl_->props)if(p.active && p.visible){
        impl_->retain_query_state(p);
        const auto selected=p.geometry.select(p.source_object,p.handle,p.query_state.sphere.center,p.query_state.sphere.radius,view);
        const auto&g=p.geometry.view(view.view_index);
        //4B5D0 updates the owner cache before41040 selects part shaders.
        //The selected ordinary flag controls uniform binding only.
        impl_->query_lighting(p,impl_->render_frame,selected.source_distance);
        out.insert(out.end(),g.vertices.begin(),g.vertices.end());
        if(g.ordinary_lighting && !g.vertices.empty())
            bindings.push_back({p.geometry.first_material,p.geometry.material_count,p.light.lighting});
        if(selections)selections->push_back(selected);
    }
}
const SourcePropSceneStats&SourcePropScene::stats()const{return impl_->counts;}
}
