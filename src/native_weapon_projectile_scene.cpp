#include "halo2/native_weapon_projectile_scene.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_object_colors.hpp"
#include "halo2/object_light_query.hpp"
#include "halo2/collision.hpp"
#include "source_object_geometry.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <iostream>

namespace halo2 {
struct NativeWeaponProjectileScene::Impl {
    struct Color {unsigned channel{};Vec3 lower,upper;};
    struct Bank {
        SourceObjectGeometry geometry;ShadowCasterDefinition bounds;std::uint16_t flags{};
        std::vector<Color> colors;float initial_velocity{},final_velocity{},acceleration_start{},acceleration_scale{};
    };
    struct Owner {
        NativeWeaponProjectile projectile;
        std::array<std::int8_t,4> previous_lod{-1,-1,-1,-1};ObjectLightingCache light;
    };
    const Cache &cache;const CollisionWorld &collision;std::size_t capacity;
    BspLighting lighting;BspSurfaceLighting surface;ObjectLightingDefinition conversion;
    std::map<std::uint32_t,Bank> banks;std::set<std::uint32_t> invisible;
    std::map<std::uint32_t,std::string> unresolved;
    std::set<std::uint32_t> reported;
    std::map<std::uint32_t,Owner> owners;std::vector<std::uint32_t> order;
    std::array<std::vector<NativeWeaponProjectileDraw>,4> draws;
    Impl(const Cache &c,std::size_t bsp,Mesh &mesh,const CollisionWorld &world,
        std::span<const std::uint32_t> definitions,std::size_t slots)
        :cache(c),collision(world),capacity(slots),lighting(c.load_bsp_lighting(bsp)),
         surface(c.load_bsp_surface_lighting(bsp)),conversion(c.load_object_lighting_definition()) {
        const auto sh=c.load_source_prt_lighting(bsp);
        for(auto definition:definitions) {
            if(banks.contains(definition)||invisible.contains(definition)||unresolved.contains(definition))continue;
            const auto before=mesh.materials.size();
            try {
                const auto tag=c.load_source_tag_metadata(definition);
                const Bytes object(tag.bytes_at(tag.source_address(),tag.size()));
                if(tag.group()!="proj" || object.u16(0)!=5)throw FormatError("projectile tag/type differs");
                if(object.u32(0x38)==0xffffffffu){invisible.insert(definition);continue;}
                const auto initial=c.load_initial_model_definition(definition);
                const auto selection=c.load_initial_model_selection(definition);
                const auto model=c.load_source_tag_metadata(initial.model);
                const Bytes model_data(model.bytes_at(model.source_address(),model.size()));
                const auto colors=c.load_source_object_color_definition(definition);
                if(model_data.u32(0x14)!=0xffffffffu)throw FormatError("projectile requires its live animation producer");
                if(!initial.variants.empty() || !selection.resolved() || selection.variant_index!=-1 || initial.regions.size()!=1 ||
                   selection.permutations!=std::vector<std::uint8_t>{0} ||
                   initial.regions[0].permutation_flags!=std::vector<std::uint8_t>{0})
                    throw FormatError("projectile requires its live region producer: variants="+std::to_string(initial.variants.size())+
                        " regions="+std::to_string(initial.regions.size())+" variant="+std::to_string(selection.variant_index));
                if(c.load_source_prt_definition(initial.render_model))throw FormatError("projectile requires its live PRT producer");
                Bank bank;bank.flags=object.u16(2);bank.bounds=c.load_object_shadow_definition(definition);
                bank.initial_velocity=object.f32(0x17c);bank.final_velocity=object.f32(0x180);
                bank.acceleration_start=object.f32(0x18c);bank.acceleration_scale=object.f32(0x194);
                for(unsigned i=0;i<colors.channels.size();++i) {
                    const auto &channel=colors.channels[i];
                    if(!channel.permutations.empty() || channel.function_count>1)
                        throw FormatError("projectile color selection requires its constructor owner");
                    if(!channel.function_count)continue;
                    const Bytes row(tag.bytes_at(object.u32(0xb0)+i*16,16));
                    const Bytes function(tag.bytes_at(row.u32(12),40));
                    // Literal BE8E0/131C20 RGB interpolation on the actual
                    // FBE80 acceleration_range input, with no scale function.
                    if(function.u32(0) || function.u32(4) || function.u32(32) || function.u32(36)!=0x12000565u)
                        throw FormatError("projectile color requires an unresolved authored function");
                    bank.colors.push_back({i,{function.f32(8),function.f32(12),function.f32(16)},
                        {function.f32(20),function.f32(24),function.f32(28)}});
                }
                ScenarioObject request;request.type=5;request.definition=definition;request.variant=initial.default_variant;request.scale=1;
                bank.geometry.load(c,request,initial,selection,initial.render_model);
                const auto root=source_object_transform({},{1,0,0},{0,0,1},1,false);
                const auto nodes=pose_model_nodes_world(bank.geometry.model(),root);
                if(nodes.size()!=1)throw FormatError("projectile requires its live node producer");
                bank.geometry.pose_world([&](const RenderModel &m){return pose_model_world(m,root);});
                // Retail33670/TU33660 returns zero for non-biped type5
                // with parent NONE. This native flight bank contains only
                // free projectiles: contacts retire them, never attach them.
                // Supply that proved scalar37 branch to active-camo passes;
                // do not borrow the shooter's Unit camouflage.
                bank.geometry.prepare(c,source_object_initial_colors(colors,{},0),nodes,sh,mesh,0.f);
                banks.emplace(definition,std::move(bank));
            }catch(const FormatError &e){mesh.materials.resize(before);unresolved.emplace(definition,e.what());}
        }
    }
    void synchronize(std::span<const NativeWeaponProjectile> live) {
        std::set<std::uint32_t> present;std::set<std::uint16_t> slots;order.clear();
        for(const auto &p:live) {
            if(p.datum==0xffffffffu || (p.datum&65535u)>=capacity ||
               !present.insert(p.datum).second || !slots.insert(std::uint16_t(p.datum)).second)
                throw FormatError("projectile draw lost actual full native owner");
            if(invisible.contains(p.definition))continue;
            if(const auto at=unresolved.find(p.definition);at!=unresolved.end()) {
                if(reported.insert(p.definition).second)
                    std::cerr<<"Native projectile geometry unresolved for "<<std::hex<<p.definition<<std::dec<<": "<<at->second<<'\n';
                continue;
            }
            if(!banks.contains(p.definition))throw FormatError("projectile has no authored prepared model bank");
            auto [at,created]=owners.try_emplace(p.datum);
            if(!created && at->second.projectile.definition!=p.definition)
                throw FormatError("projectile full owner changed definition");
            at->second.projectile=p;order.push_back(p.datum);
        }
        for(auto at=owners.begin();at!=owners.end();)
            if(!present.contains(at->first))at=owners.erase(at);else ++at;
    }
    const std::vector<NativeWeaponProjectileDraw>& draw(std::int32_t frame,const SourceObjectViewLod &view) {
        auto &out=draws.at(view.view_index);out.clear();
        for(auto handle:order) {
            auto &owner=owners.at(handle);const auto &p=owner.projectile;auto &bank=banks.at(p.definition);
            const auto root=source_object_transform(p.position,p.forward,p.up,1,false);
            const auto nodes=pose_model_nodes_world(bank.geometry.model(),root);
            const auto sphere=source_shadow_sphere(bank.bounds,nodes.at(0),1);
            bank.geometry.pose_world([&](const RenderModel &m){return pose_model_world(m,root);});
            const auto selected=bank.geometry.select(handle&65535u,handle,sphere.center,sphere.radius,view,owner.previous_lod);
            const bool due=object_lighting_query_due(owner.light,frame,sphere.center,selected.source_distance,false);
            ObjectLightQueryContext context;context.bank_valid=source_object_light_query_bank_valid(lighting);
            context.object_type=5;context.definition_flags=bank.flags;
            // Native flight does not mutate generic object+4 lighting flags.
            // This is its retained cold construction branch, not contact flags.
            context.runtime_flags=0;
            const auto query=due?source_object_light_query(context,[&](unsigned mode){
                return query_object_lightmap(surface,collision,sphere.center,ObjectLightMapQueryContext{mode,
                    !(owner.light.sample_valid&&owner.light.lighting_valid),1,owner.light.target});
            },[]{return ObjectLightQueryResult{};}):ObjectLightQueryAdmission{};
            if(query.query.status==ObjectLightQueryStatus::attribution_unresolved)
                throw FormatError("projectile light contact attribution requires its source owner");
            update_object_lighting_cache(owner.light,conversion,5,frame,sphere.center,p.velocity,due,
                query.accepted?query.query.sample:std::optional<ObjectLightmapSample>{},true,false);
            const auto &vertices=bank.geometry.view(view.view_index);
            ObjectColors colors{{{1,1,1},{1,1,1},{1,1,1},{1,1,1}}};
            // Native shots use no positive source speed override and remove
            // contacts instead of retaining an attached projectile. Neither
            // state is inferred from velocity or a definition name.
            const auto amount=source_projectile_acceleration_range(p.traveled,bank.initial_velocity,
                bank.final_velocity,bank.acceleration_start,bank.acceleration_scale,0);
            for(const auto &color:bank.colors)
                colors[color.channel]=source_projectile_rgb_color(color.lower,color.upper,amount);
            out.push_back({handle,p.definition,vertices.vertices,owner.light.lighting,vertices.ordinary_lighting,colors});
        }
        return out;
    }
};
NativeWeaponProjectileScene::NativeWeaponProjectileScene(const Cache &c,std::size_t bsp,Mesh &m,
    const CollisionWorld &world,std::span<const std::uint32_t> definitions,std::size_t capacity)
    :impl_(std::make_unique<Impl>(c,bsp,m,world,definitions,capacity)){}
NativeWeaponProjectileScene::~NativeWeaponProjectileScene()=default;
void NativeWeaponProjectileScene::synchronize(std::span<const NativeWeaponProjectile> p){impl_->synchronize(p);}
const std::vector<NativeWeaponProjectileDraw>& NativeWeaponProjectileScene::draw_view(std::int32_t frame,const SourceObjectViewLod &v){return impl_->draw(frame,v);}
}
