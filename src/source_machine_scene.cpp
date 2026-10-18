#include "halo2/source_machine_scene.hpp"
#include "halo2/source_device.hpp"
#include "halo2/source_device_world.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_prt.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/scenery.hpp"
#include "halo2/appearance.hpp"
#include "halo2/object_light_query.hpp"
#include "halo2/renderer.hpp"
#include "halo2/world_material.hpp"
#include "halo2/math.hpp"
#include "halo2/scene.hpp"
#include "halo2/reference_player.hpp"
#include "source_object_geometry.hpp"
#include "halo2/render_interpolation.hpp"
#include <map>
#include <algorithm>
#include <set>

namespace halo2 {
struct SourceMachineScene::Impl {
    struct Machine {
        std::size_t source_object{};
        unsigned authored_placement{};
        std::uint32_t definition_datum{};
        std::optional<ReferenceDeviceState> reference;
        std::uint32_t handle=0xffffffffu;
        std::uint64_t advanced_tick{},created_tick{};
        bool active=false,visible=false,simulation_owned{};
        SourceMachineDefinition definition;
        SourceMachinePlacement placement;
        SourcePortalOwnerIdentity portal_identity;
        SourceMachinePortalDefinition portal_definition;
        std::uint32_t initial_device_flags{};
        ShadowCasterDefinition bounds;
        SourceObjectGeometry geometry;
        std::optional<std::string> resource_error;
        std::optional<std::vector<Transform>> solved_nodes;
        Transform object;
        float position{},power{},scale{},group_position{};
        struct Presentation {Transform root;float position{},power{};};
        std::optional<Presentation> presentation;
        std::uint32_t device_flags{};
        std::uint32_t runtime_flags{};
        std::vector<std::uint8_t> active_permutations;
        std::vector<Transform> world_nodes;
        ObjectLightingCache light;
        ShadowCasterSphere sphere;
    };
    const Cache &cache;
    const CollisionWorld &collision;
    BspLighting lighting;
    BspSurfaceLighting surface;
    ObjectLightingDefinition conversion;
    SourcePrtLighting sh;
    SourceMachineSceneStats counts;
    std::uint64_t last_tick{};
    std::int32_t render_frame{};
    bool reference_bound{},lifetimes_bound{};
    std::vector<Machine> machines;
    std::vector<SourceMachineResourceBoundary> deferred_resources;
    std::set<std::size_t> machine_placements,nonrendered_placements;

    Impl(const Cache &c,std::span<const ScenarioObject> objects,std::size_t bsp,Mesh &mesh,const CollisionWorld &cw,bool source_resource_defer)
        :cache(c),collision(cw),lighting(c.load_bsp_lighting(bsp)),surface(c.load_bsp_surface_lighting(bsp)),
         conversion(c.load_object_lighting_definition()),sh(c.load_source_prt_lighting(bsp)) {
        for(std::size_t source_object=0;source_object<objects.size();++source_object){
            const auto&object=objects[source_object];
            if(object.type==7)machine_placements.insert(source_object);
            // Resource preparation does not perform source birth admission.
            // Named/persistent owners can be born globally outside their origin
            // BSP, and an initially absent machine can be created later. Only
            // synchronize's actual lifetimes/visibility select submitted models.
            if(object.type!=7 || object.definition==null_datum)continue;
            ++counts.placements;
            Machine m;m.source_object=source_object;m.definition_datum=object.definition;
            m.authored_placement=unsigned(object.placement);m.definition=cache.load_source_machine_definition(object.definition);
            m.placement=cache.load_source_machine_placement(unsigned(object.placement));
            if(object.type<0 || object.type>127 || object.source< -128 || object.source>127 ||
                object.origin_bsp< -32768 || object.origin_bsp>32767)
                throw FormatError("machine portal identity exceeds source field widths");
            m.portal_identity={object.unique_id,std::int16_t(object.origin_bsp),std::int8_t(object.type),std::int8_t(object.source)};
            m.portal_definition=cache.load_source_machine_portal_definition(object.definition);
            m.initial_device_flags=source_machine_initial_device_flags(m.placement.device_flags);
            m.position=m.placement.initial_position;m.power=m.placement.initial_power;m.scale=object.scale;
            //All nineteen supplied gears have private power/position groups.
            //Named gear groups require1152E0's shared group write producer.
            if(m.definition.type==2 && (m.placement.power_group!=-1 || m.placement.position_group!=-1))
                throw FormatError("named gear group progression requires source shared state: "+m.definition.name);
            m.runtime_flags=source_machine_object_flags(0,m.definition);
            if(m.definition.position)++counts.position_overlays;
            if(m.definition.type!=2)++counts.initial_state_only;
            if(m.definition.render_model==0xffffffffu){nonrendered_placements.insert(source_object);continue;}
            if(!source_object_light_query_bank_valid(lighting))
                throw FormatError("machine model lighting implementation3 requires original active bank: "+m.definition.name);
            m.bounds=cache.load_object_shadow_definition(object.definition);
            const auto basis=authored_basis(object.rotation);
            m.object=source_object_transform(object.position,basis.forward,basis.up,m.scale,false);
            const auto initial_definition=cache.load_initial_model_definition(object.definition);
            const auto initial_selection=cache.load_initial_model_selection(object.definition,object.variant);
            if(!initial_selection.resolved())
                throw FormatError("machine initial model RNG selection requires source state: "+m.definition.name);
            m.active_permutations=initial_selection.permutations;
            m.geometry.load(cache,object,initial_definition,initial_selection,m.definition.render_model);
            pose(m);
            const auto colors=cache.load_object_colors(object.definition,object.variant);
            const auto material_base=mesh.materials.size();
            try {
                const auto resident=m.geometry.prepare(cache,colors,m.world_nodes,sh,mesh);
                counts.dynamic_materials+=resident.dynamic_materials;counts.prt_materials+=resident.prt_materials;
                counts.union_dynamic_materials+=resident.union_dynamic_materials;
                counts.union_prt_materials+=resident.union_prt_materials;
                counts.union_section_banks+=resident.union_section_banks;
                ++counts.render_models;
            } catch(const FormatError &error) {
                if(!source_resource_defer)throw;
                mesh.materials.resize(material_base);
                m.resource_error=error.what();
                deferred_resources.push_back({source_object,*m.resource_error});++counts.deferred_resources;
            }
            machines.push_back(std::move(m));
        }
    }
    static void require_resources(const Machine &m) {
        if(m.resource_error)throw FormatError("Visible machine placement "+std::to_string(m.source_object)+
            " requires original source resources: "+*m.resource_error);
    }
    static void pose(Machine &m){
        const auto&model=m.geometry.model();
        const auto position=m.presentation?m.presentation->position:m.position;
        const auto power=m.presentation?m.presentation->power:m.power;
        const auto &root=m.presentation?m.presentation->root:m.object;
        const auto locals=m.solved_nodes?std::vector<BonePose>{}:source_machine_local_pose(model,m.definition,position,power);
        m.world_nodes=m.solved_nodes?*m.solved_nodes:pose_model_locals_nodes_world(model,root,locals);
        if(m.world_nodes.empty())throw FormatError("machine model has no query node0: "+m.definition.name);
        m.sphere=source_shadow_sphere(m.bounds,m.world_nodes.front(),root.scale);
        if(!m.resource_error)m.geometry.pose_world([&](const RenderModel&resident){return m.solved_nodes?
            skin_model_world_nodes(resident,m.world_nodes):pose_model_locals_world(resident,root,locals);});
    }
    void bind_banks(Machine &m,Mesh &mesh){m.geometry.update_banks(cache,m.world_nodes,sh,mesh);}
    static void reference_pose(Machine &m) {
        m.solved_nodes.reset();
        const auto&r=*m.reference;m.position=r.position;m.power=r.power;m.scale=r.root_scale;
        m.object=source_object_transform(r.root_position,r.root_forward,r.root_up,r.root_scale,
            bool(r.source_object_flags&0x400));
        m.runtime_flags=r.source_object_flags;
    }
    void set_reference_devices(std::span<const ReferenceDeviceState> devices,Mesh&mesh) {
        if(reference_bound || last_tick!=0)throw FormatError("reference devices must bind once before scene advancement");
        std::set<unsigned> seen;std::set<std::uint32_t> handles;
        std::vector<std::pair<Machine*,const ReferenceDeviceState*>> matched;
        for(const auto&r:devices) {
            auto m=std::find_if(machines.begin(),machines.end(),[&](const auto&m){return m.authored_placement==r.scenario_placement;});
            const auto finite=[](Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);};
            if(r.object_type!=7 || m==machines.end() || m->definition_datum!=r.definition ||
                m->definition.render_model!=r.render_model || !seen.insert(r.scenario_placement).second ||
                !handles.insert(r.source_object_datum).second || (r.source_object_datum&0xffffu)==0xffffu ||
                !std::isfinite(r.position) || r.position<0 || r.position>=1 || !std::isfinite(r.power) || r.power<0 || r.power>1 ||
                !std::isfinite(r.root_scale) || r.root_scale<=0 || !finite(r.root_position) ||
                !finite(r.root_forward) || !finite(r.root_up) || length(cross(r.root_forward,r.root_up))<.01f)
                throw FormatError("reference device has no unique admitted source machine");
            require_resources(*m);matched.push_back({&*m,&r});
        }
        for(const auto [m,r]:matched){m->reference=*r;reference_pose(*m);pose(*m);bind_banks(*m,mesh);}
        reference_bound=true;
    }
    void synchronize(const SceneObjects &scene,const SourceDeviceWorld *devices){
        lifetimes_bound=false;
        struct Live {std::uint32_t handle;std::uint64_t created_tick;Transform root;bool visible;const SourceDeviceWorldOwner*device;const std::optional<std::vector<Transform>>*nodes{};};
        std::map<std::size_t,Live> live;
        const auto insert=[&](std::size_t placement,Live object){
            if(!machine_placements.contains(placement))return;
            if(!live.emplace(placement,object).second)
                throw FormatError("machine lifetime has duplicate live source placement");
            if(!nonrendered_placements.contains(placement) &&
                std::none_of(machines.begin(),machines.end(),[&](const auto&m){return m.source_object==placement;}))
                throw FormatError("live machine placement requires unprepared source geometry");
        };
        if(devices){
            for(const auto&[handle,o]:devices->owners()) {
                const auto at=std::find_if(scene.objects().begin(),scene.objects().end(),[&](const auto &p){return p.handle==handle && p.placement==o.placement;});
                insert(o.placement,{handle,o.created_tick,o.root,o.visible,&o,
                    at==scene.objects().end()?nullptr:&at->solved_nodes});
            }
        }else for(const auto&object:scene.objects())if(object.handle!=0xffffffffu){
            insert(object.placement,{object.handle,object.created_tick,
                scene_object_transform(object),object.visible,nullptr,&object.solved_nodes});
        }
        for(auto&m:machines){
            m.presentation.reset();
            const auto at=live.find(m.source_object);
            if(at==live.end()){m.active=false;m.visible=false;continue;}
            const auto&object=at->second;
            if(object.visible)require_resources(m);
            m.solved_nodes=object.nodes?*object.nodes:std::optional<std::vector<Transform>>{};
            if(m.handle!=object.handle){
                if(!devices && (m.handle!=0xffffffffu || object.created_tick!=0) &&
                    (m.placement.power_group!=-1 || m.placement.position_group!=-1))
                    throw FormatError("machine recreation requires current source named groups: "+m.definition.name);
                m.handle=object.handle;m.created_tick=object.created_tick;m.advanced_tick=object.created_tick;
                m.position=m.placement.initial_position;m.power=m.placement.initial_power;
                m.runtime_flags=source_machine_object_flags(0,m.definition);m.light={};m.geometry.reset_history();
            }else if(m.created_tick!=object.created_tick)
                throw FormatError("machine full handle changed its source creation tick");
            m.object=object.root;m.scale=object.root.scale;
            m.simulation_owned=devices!=nullptr;
            if(devices){
                const auto*owner=object.device;
                if(!owner || owner->placement!=m.source_object || owner->created_tick!=object.created_tick)
                    throw FormatError("rendered machine lacks its current simulation device owner");
                m.position=owner->motion.position;m.power=owner->motion.power;
                m.group_position=devices->position_group(*owner).value;m.device_flags=owner->motion.flags;
            }
            if(m.reference)reference_pose(m);
            m.active=true;m.visible=object.visible;
        }
        lifetimes_bound=true;
    }
    void update_pose(std::uint64_t tick,std::int32_t frame,Mesh&mesh){
        if(tick<last_tick)throw FormatError("machine scene tick moved backwards");
        render_frame=frame;
        for(auto&m:machines){
            if(!m.active)continue;
            if(m.visible)require_resources(m);
            if(tick<m.advanced_tick)throw FormatError("machine creation tick exceeds source simulation clock");
            for(auto step=m.advanced_tick;step<tick;++step)if(!m.reference && !m.simulation_owned && !m.solved_nodes && m.definition.type==2)
                m.position=source_machine_gear_step(m.definition,m.position,m.power,1.f/30.f).position;
            m.advanced_tick=tick;pose(m);
            //1152E0 progression continues while hidden; B9CE0 independently
            //prevents its render-light/geometry submission.
            if(m.visible)bind_banks(m,mesh);
        }last_tick=tick;
    }
    void query_lighting(Machine&m,std::int32_t frame,float distance){
        const auto origin=m.sphere.center;
        const bool due=object_lighting_query_due(m.light,frame,origin,distance,false);
        ObjectLightQueryContext context;context.bank_valid=source_object_light_query_bank_valid(lighting);
        context.object_type=7;context.definition_flags=m.definition.object_flags;context.runtime_flags=m.runtime_flags;
        const auto query=due?source_object_light_query(context,[&](unsigned mode){
            return query_object_lightmap(surface,collision,origin,ObjectLightMapQueryContext{mode,
                !(m.light.sample_valid && m.light.lighting_valid),1.f,m.light.target});},
            []{return ObjectLightQueryResult{};}):ObjectLightQueryAdmission{};
        if(query.query.status==ObjectLightQueryStatus::attribution_unresolved)
            throw FormatError("machine source lighting contact attribution unresolved: "+m.definition.name);
        update_object_lighting_cache(m.light,conversion,7,frame,origin,{},due,
            query.accepted?query.query.sample:std::optional<ObjectLightmapSample>{});
    }
    void update(std::uint64_t tick,std::int32_t frame,Vec3 eye,float draw_scale,Mesh&mesh){
        update_pose(tick,frame,mesh);
        for(auto&m:machines)if(m.active && m.visible)
            query_lighting(m,frame,source_geometry_lod_distance(m.sphere.center,eye,draw_scale));
    }

};
SourceMachineScene::SourceMachineScene(const Cache&c,std::span<const ScenarioObject>objects,std::size_t bsp,
    Mesh&mesh,const CollisionWorld&collision,bool source_resource_defer):impl_(std::make_unique<Impl>(c,objects,bsp,mesh,collision,source_resource_defer)){}
SourceMachineScene::~SourceMachineScene()=default;
SourceMachineScene::SourceMachineScene(SourceMachineScene&&) noexcept=default;
SourceMachineScene&SourceMachineScene::operator=(SourceMachineScene&&) noexcept=default;
void SourceMachineScene::update(std::uint64_t tick,std::int32_t frame,Vec3 eye,float scale,Mesh&mesh){
    impl_->update(tick,frame,eye,scale,mesh);
}
void SourceMachineScene::update_pose(std::uint64_t tick,std::int32_t frame,Mesh&mesh){impl_->update_pose(tick,frame,mesh);}
void SourceMachineScene::interpolate(const SourceDeviceWorldSnapshot &previous,float fraction){
    validate_render_fraction(fraction);
    for(auto &m:impl_->machines) {
        m.presentation.reset();
        if(!m.active || !m.visible || m.reference || !m.simulation_owned || m.solved_nodes)continue;
        const auto at=std::find_if(previous.owners.begin(),previous.owners.end(),[&](const auto &p){
            const auto &o=p.state;
            return p.definition==m.definition_datum && o.handle==m.handle &&
                o.placement==m.source_object && o.created_tick==m.created_tick && o.visible && !o.bsp_hidden;
        });
        if(at==previous.owners.end())continue;
        const auto &old=at->state;
        float position=std::lerp(old.motion.position,m.position,fraction);
        if(m.definition.type==2 && m.position<old.motion.position &&
           source_machine_gear_step(m.definition,old.motion.position,m.power,0).rate>0) {
            position=old.motion.position+(m.position+1-old.motion.position)*fraction;
            if(position>=1)position-=1;
            if(fraction==0)position=old.motion.position;
            if(fraction==1)position=m.position;
        }
        m.presentation=Impl::Machine::Presentation{render_transform(old.root,m.object,fraction),
            position,std::lerp(old.motion.power,m.power,fraction)};
    }
}
void SourceMachineScene::interpolate(const SceneObjects &scene,const SceneInterpolation &history,float fraction) {
    validate_render_fraction(fraction);
    for(auto &m:impl_->machines)if(m.active && m.solved_nodes && !m.reference) {
        const auto &object=scene.get(m.handle);
        if(!object.solved_nodes)throw FormatError("Machine source WORLD endpoint became unavailable");
        m.object=history.sample(object,fraction);m.scale=m.object.scale;
        m.solved_nodes=history.sample_nodes(object,fraction);
    }
}
void SourceMachineScene::synchronize(const SceneObjects&scene,const SourceDeviceWorld*devices){impl_->synchronize(scene,devices);}
void SourceMachineScene::set_reference_devices(std::span<const ReferenceDeviceState> devices,Mesh&mesh){
    impl_->set_reference_devices(devices,mesh);
}
void SourceMachineScene::append(std::vector<Vertex>&out,std::vector<ObjectLightingBinding>&bindings)const{
    for(const auto&m:impl_->machines)if(m.active && m.visible){Impl::require_resources(m);const auto&g=m.geometry.at(5);
        out.insert(out.end(),g.vertices.begin(),g.vertices.end());
        bindings.push_back({m.geometry.first_material,m.geometry.material_count,m.light.lighting});}
}
void SourceMachineScene::append_view(std::vector<Vertex>&out,std::vector<ObjectLightingBinding>&bindings,
    const SourceObjectViewLod&view,std::vector<SourceObjectViewSelection>*selections){
    for(auto&m:impl_->machines)if(m.active && m.visible){
        Impl::require_resources(m);
        const auto selected=m.geometry.select(m.source_object,m.handle,m.sphere.center,m.sphere.radius,view);
        const auto&g=m.geometry.view(view.view_index);
        //4B5D0's owner lighting-cache call precedes part shader selection.
        impl_->query_lighting(m,impl_->render_frame,selected.source_distance);
        out.insert(out.end(),g.vertices.begin(),g.vertices.end());
        if(g.ordinary_lighting && !g.vertices.empty())
            bindings.push_back({m.geometry.first_material,m.geometry.material_count,m.light.lighting});
        if(selections)selections->push_back(selected);
    }
}
const SourceMachineSceneStats&SourceMachineScene::stats()const{return impl_->counts;}
std::span<const SourceMachineResourceBoundary>SourceMachineScene::deferred_resource_owners()const{return impl_->deferred_resources;}
std::vector<SourceMachinePortalState> SourceMachineScene::portal_owners()const{
    if(!impl_->lifetimes_bound)throw FormatError("Machine portal snapshot requires synchronized source lifetimes");
    std::vector<SourceMachinePortalState> out;
    for(const auto&m:impl_->machines)if(m.active){
        if(m.handle==0xffffffffu)throw FormatError("portal owner snapshot requires synchronized machine lifetimes");
        if(m.reference && m.definition.type!=2)
            throw FormatError("reference door portal state requires its actual shared group/device bank");
        SourceMachinePortalState packet;packet.source_object=m.source_object;packet.created_tick=m.created_tick;
        packet.owner.handle=m.handle;packet.owner.identity=m.portal_identity;packet.owner.definition=m.portal_definition;
        //1076E0 creates both private and named groups. Bound simulation state
        //contains106790's current channels/flags and the actual group target.
        packet.door={true,m.position,m.simulation_owned?m.group_position:
            (m.definition.type==2?m.position:m.placement.initial_position),
            m.simulation_owned?m.device_flags:m.initial_device_flags};
        packet.initialized_group_state_only=!m.simulation_owned && m.definition.type!=2;
        out.push_back(packet);
    }
    return out;
}
std::vector<SourceSceneEffectOwner> SourceMachineScene::effect_owners() const {
    std::vector<SourceSceneEffectOwner> out;
    for(const auto &m:impl_->machines)if(m.active && m.handle!=0xffffffffu)
        out.push_back({m.source_object,m.handle,m.created_tick,m.world_nodes,m.active_permutations,
            m.presentation?m.presentation->root.position:m.object.position});
    return out;
}
} // namespace halo2
