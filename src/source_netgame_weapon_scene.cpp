#include "halo2/source_netgame_weapon_scene.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_object_colors.hpp"
#include "halo2/object_light_query.hpp"
#include "halo2/renderer.hpp"
#include "source_object_geometry.hpp"
#include <algorithm>
#include <map>
#include <set>

namespace halo2 {
struct SourceNetgameWeaponScene::Impl {
    struct Template {SourceObjectGeometry geometry;};
    struct Owner {unsigned registration{};ObjectLightingCache light;};
    const CollisionWorld &collision;
    BspLighting lighting;
    BspSurfaceLighting surface;
    ObjectLightingDefinition conversion;
    std::shared_ptr<const ShaderFunctionTables> function_tables;
    SourceNetgameWeaponSceneStats counts;
    std::vector<SourceNetgameWeaponRegistration> registrations;
    std::vector<Template> templates;
    std::map<std::uint32_t,Owner> owners;
    Impl(const Cache&cache,std::span<const NetgameEquipment> rows,std::size_t bsp,Mesh&mesh,
         const CollisionWorld&collision,int source_engine):collision(collision),lighting(cache.load_bsp_lighting(bsp)),
        surface(cache.load_bsp_surface_lighting(bsp)),conversion(cache.load_object_lighting_definition()),
        function_tables(cache.shader_function_tables()){
        if(rows.size()>100)throw FormatError("source netgame equipment capacity exceeded");
        counts.rows=unsigned(rows.size());
        for(unsigned ordinal=0;ordinal<rows.size();++ordinal){
            const auto&row=rows[ordinal];
            if(row.classification || row.collection_group!=0x69746d63u ||
               row.collection==0xffffffffu || row.items.size()!=1 ||
               row.items.front().group!=0x77656170u){++counts.selector_unported;continue;}
            //15A090 is a source engine selector, independent of the row's
            //authored pose or any map-name heuristic.
            if(source_netgame_equipment_selector(row,true,source_engine)<0)continue;
            const auto definition=cache.load_source_world_weapon_definition(row.items.front().definition);
            if(!(row.flags&1u)){++counts.ground_unported;continue;}
            if(definition.animation_graph!=0xffffffffu || (definition.item_flags&1u)){
                ++counts.controller_unported;continue;}
            if(std::any_of(definition.widget_groups.begin(),definition.widget_groups.end(),[](auto group){
                return group!=0x65666665u && group!=0x6c736e64u && group!=0x6c696768u;
            })){++counts.widget_unported;continue;}
            if(definition.render_model==0xffffffffu)
                throw FormatError("source levitated weapon has no authored render model");
            SourceNetgameWeaponRegistration registration;
            registration.row=ordinal;registration.definition=definition;
            registration.birth=source_netgame_weapon_birth(row,definition,2);
            const auto cold=source_netgame_weapon_initial_state(definition,registration.birth);
            const auto initial=cache.load_initial_model_definition(definition.definition);
            const auto selection=cache.load_initial_model_selection(definition.definition,registration.birth.variant);
            if(!selection.resolved())throw FormatError("source levitated weapon region selection requires actual RNG");
            ScenarioObject object;object.type=2;object.definition=definition.definition;
            object.variant=registration.birth.variant;object.position=row.position;
            object.scale=registration.birth.object_scale;
            Template t;t.geometry.load(cache,object,initial,selection,definition.render_model);
            const auto frame=source_object_transform(registration.birth.position,registration.birth.forward,
                registration.birth.up,registration.birth.object_scale,false);
            const auto nodes=pose_model_nodes_world(t.geometry.model(),frame);
            if(nodes.empty())throw FormatError("source levitated weapon has no initial query node0");
            const auto sphere=source_shadow_sphere(cache.load_object_shadow_definition(definition.definition),
                nodes.front(),registration.birth.object_scale);
            registration.sphere_center=sphere.center;registration.sphere_radius=sphere.radius;
            t.geometry.pose_world([&](const RenderModel&model){return pose_model_world(model,frame);});
            const auto colors_definition=cache.load_source_object_color_definition(definition.definition);
            for(const auto&channel:colors_definition.channels)if(channel.function_count)
                throw FormatError("source levitated weapon color function requires actual owner input");
            const auto variant=selection.variant_index<0?0u:initial.variants.at(unsigned(selection.variant_index)).name;
            const auto colors=source_object_initial_colors(colors_definition,row.position,variant);
            const auto resident=t.geometry.prepare(cache,colors,nodes,{},mesh,
                source_unattached_weapon_camouflage(cold.item));
            if(resident.prt_materials || resident.union_prt_materials)
                throw FormatError("source world weapon PRT producer is not admitted");
            counts.materials+=t.geometry.material_count;
            registrations.push_back(std::move(registration));templates.push_back(std::move(t));
            ++counts.resident_rows;
        }
    }
};
SourceNetgameWeaponScene::SourceNetgameWeaponScene(const Cache&cache,std::span<const NetgameEquipment>rows,
    std::size_t bsp,Mesh&mesh,const CollisionWorld&collision,int source_engine):impl_(std::make_unique<Impl>(cache,rows,bsp,mesh,collision,source_engine)){}
SourceNetgameWeaponScene::~SourceNetgameWeaponScene()=default;
std::span<const SourceNetgameWeaponRegistration> SourceNetgameWeaponScene::registrations()const{return impl_->registrations;}
const SourceNetgameWeaponSceneStats&SourceNetgameWeaponScene::stats()const{return impl_->counts;}
std::optional<SourceWorldWeaponShaderInput> SourceNetgameWeaponScene::shader_state(unsigned material,
    std::span<const SourceNativeWorldWeapon>objects)const{
    for(unsigned index=0;index<impl_->templates.size();++index){const auto&g=impl_->templates[index].geometry;
        if(material<g.first_material || material>=g.first_material+g.material_count)continue;
        std::optional<SourceWorldWeaponShaderInput> result;
        for(const auto&owner:objects)if(owner.registration==index){
            if(owner.row!=impl_->registrations[index].row || owner.handle==0xffffffffu)
                throw FormatError("source world weapon material has invalid owner identity");
            if(result && (result->weapon.item_flags!=owner.state.shader.item_flags ||
                result->weapon.weapon_flags!=owner.state.shader.weapon_flags ||
                result->weapon.power_fraction!=owner.state.shader.power_fraction ||
                result->magazines.size()!=owner.state.magazines.size() ||
                !std::equal(result->magazines.begin(),result->magazines.end(),owner.state.magazines.begin(),
                    [](const auto&a,const auto&b){return a.loaded==b.loaded&&a.reserve==b.reserve;})))
                throw FormatError("source row copies require distinct animated material storage");
            const auto&definition=impl_->registrations[index].definition;
            result=SourceWorldWeaponShaderInput{owner.state.shader,definition.magazines,owner.state.magazines,
                                               definition.functions,impl_->function_tables.get()};
        }return result;
    }return {};
}
void SourceNetgameWeaponScene::append_view(std::vector<Vertex>&vertices,std::vector<ObjectLightingBinding>&bindings,
    std::span<const SourceNativeWorldWeapon>objects,std::int32_t frame,const SourceObjectViewLod&view){
    std::set<std::uint32_t> handles;
    for(const auto&object:objects){
        if(object.handle==0xffffffffu || !handles.insert(object.handle).second ||
           object.registration>=impl_->registrations.size())
            throw FormatError("source world weapon view has an invalid full owner identity");
        const auto&r=impl_->registrations[object.registration];
        if(object.row!=r.row)throw FormatError("source world weapon view changed its authored row");
        auto&geometry=impl_->templates[object.registration].geometry;
        auto [entry,created]=impl_->owners.try_emplace(object.handle,Impl::Owner{object.registration,{}});
        if(!created && entry->second.registration!=object.registration)
            throw FormatError("source world weapon full handle changed its definition");
        if(created)geometry.reset_history();
        const auto selected=geometry.select(r.row,object.handle,r.sphere_center,r.sphere_radius,view);
        auto&light=entry->second.light;
        const bool frozen=bool(object.state.item.control_c1&1u);
        const bool due=object_lighting_query_due(light,frame,r.sphere_center,selected.source_distance,frozen);
        ObjectLightQueryContext context;context.bank_valid=source_object_light_query_bank_valid(impl_->lighting);
        context.object_type=2;context.definition_flags=r.definition.object_flags;
        const auto query=due?source_object_light_query(context,[&](unsigned mode){
            return query_object_lightmap(impl_->surface,impl_->collision,r.sphere_center,
                ObjectLightMapQueryContext{mode,!(light.sample_valid&&light.lighting_valid),1.f,light.target});
        },[]{return ObjectLightQueryResult{};}):ObjectLightQueryAdmission{};
        if(query.query.status==ObjectLightQueryStatus::attribution_unresolved)
            throw FormatError("source world weapon lighting contact attribution unresolved: "+r.definition.name);
        update_object_lighting_cache(light,impl_->conversion,2,frame,r.sphere_center,{},due,
            query.accepted?query.query.sample:std::optional<ObjectLightmapSample>{},true,frozen);
        const auto&draw=geometry.view(view.view_index);
        vertices.insert(vertices.end(),draw.vertices.begin(),draw.vertices.end());
        if(draw.ordinary_lighting && !draw.vertices.empty())
            bindings.push_back({geometry.first_material,geometry.material_count,light.lighting});
    }
    for(auto at=impl_->owners.begin();at!=impl_->owners.end();)
        if(!handles.contains(at->first))at=impl_->owners.erase(at);else ++at;
}
}
