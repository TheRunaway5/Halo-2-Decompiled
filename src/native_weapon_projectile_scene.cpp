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
namespace {
std::span<const std::byte> resident_projectile_metadata(const Cache &cache,
    std::uint32_t address,std::size_t extent) {
    const auto owner=cache.query_tag({.contains=TagAddressRange{address,extent},.require_address=true});
    if(!owner)throw FormatError("Projectile metadata pointer has no retained source owner");
    (void)cache.tag_payload(owner->handle());
    return cache.read_tag_bytes(address,extent);
}
}
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
    struct LightAttachment {
        std::uint32_t definition{};bool use_marker_scale{};
        std::shared_ptr<const LightVolumeDefinition> volume;
    };
    std::map<std::uint32_t,std::vector<LightAttachment>> light_banks;
    std::array<std::vector<SourceLightVolumeDraw>,4> light_draws;
    std::map<std::uint32_t,unsigned> light_materials;
    void prepare_lights(std::uint32_t definition,Mesh &mesh) {
        const auto metadata=cache.load_source_tag_metadata(definition);
        const Bytes object(metadata.bytes_at(metadata.source_address(),0x9c));
        if(metadata.group()!="proj" || object.u16(0)!=5)throw FormatError("Projectile glow lost its actual definition type");
        if(object.u32(0x38)!=0xffffffffu)return; // Posed model markers retain their own owner.
        std::vector<LightAttachment> attachments;
        const auto descriptor=read_tag_block_descriptor(object,0x94);
        const auto extent=descriptor.extent(0x18,4095);
        const Bytes rows(descriptor.count?resident_projectile_metadata(cache,descriptor.address,extent):std::span<const std::byte>{});
        for(unsigned n=0;n<unsigned(descriptor.count);++n) {
            const auto at=n*0x18;
            if(rows.u32(at)!=0x4d475332u || rows.u32(at+4)==0xffffffffu)continue;
            // Whole41980/B8D30/BE6D0 on these actual no-model/free tags:
            // marker0 falls back to the completed root; no opacity function,
            // object fade or color selector means literal one/white.
            if(rows.u32(at+8) || rows.u32(at+0x10) || rows.u16(at+0xc) || (object.u16(2)&0x200u))
                throw FormatError("Projectile glow requires its original marker/function/fade/color producer");
            const auto tag=rows.u32(at+4);
            auto volume=std::make_shared<LightVolumeDefinition>(cache.load_light_volume(tag));
            if(std::any_of(volume->volumes.begin(),volume->volumes.end(),
                [](const auto &v){return v.flags&8u;}))
                throw FormatError("Projectile glow requires its original fuzzy GPU owner");
            for(const auto &v:volume->volumes)if(!light_materials.contains(v.bitmap)) {
                if(v.bitmap!=0xffffffffu && cache.bitmap_count(v.bitmap)!=1)
                    throw FormatError("Projectile glow bitmap requires its original frame selection owner");
                Material glow;glow.name="rasterizer/projectile_glow/"+std::to_string(v.bitmap);
                //4F010 passes frame0 to4F7E0. The non-fuzzy program is
                //4FE20 for both the default texture and actual bitm branch.
                glow.diffuse=v.bitmap==0xffffffffu?cache.load_light_volume_default_bitmap():cache.load_bitmap(v.bitmap,0);
                glow.forward_pass=true;glow.light_volume_pass=true;
                glow.forward_program=light_volume_program();glow.forward_blend=2;
                glow.forward_raster.color_write=0x10101;glow.forward_raster.cull=0;
                light_materials.emplace(v.bitmap,unsigned(mesh.materials.size()));mesh.materials.push_back(std::move(glow));
            }
            const bool scale=!volume->volumes.empty() && (volume->volumes.front().flags&0x20u);
            attachments.push_back({tag,scale,std::move(volume)});
        }
        if(attachments.empty())return;
        light_banks.emplace(definition,std::move(attachments));
    }
    Impl(const Cache &c,std::size_t bsp,Mesh &mesh,const CollisionWorld &world,
        std::span<const std::uint32_t> definitions,std::size_t slots)
        :cache(c),collision(world),capacity(slots),lighting(c.load_bsp_lighting(bsp)),
         surface(c.load_bsp_surface_lighting(bsp)),conversion(c.load_object_lighting_definition()) {
        const auto sh=c.load_source_prt_lighting(bsp);
        for(auto definition:definitions) {
            if(banks.contains(definition)||invisible.contains(definition)||unresolved.contains(definition))continue;
            try {prepare_lights(definition,mesh);}
            catch(const FormatError &e){std::cerr<<"Native projectile glow unresolved for "<<std::hex<<definition<<std::dec<<": "<<e.what()<<'\n';}
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
                    const auto channels=read_tag_block_descriptor(object,0xac);
                    const Bytes rows(resident_projectile_metadata(c,channels.address,channels.extent(16,4)));
                    const auto functions=read_tag_block_descriptor(rows,i*16+8);
                    if(functions.count!=1)throw FormatError("Projectile color function descriptor differs from its source channel");
                    const Bytes function(resident_projectile_metadata(c,functions.address,functions.extent(40,1)));
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
            if(invisible.contains(p.definition) && !light_banks.contains(p.definition))continue;
            if(const auto at=unresolved.find(p.definition);at!=unresolved.end()) {
                if(reported.insert(p.definition).second)
                    std::cerr<<"Native projectile geometry unresolved for "<<std::hex<<p.definition<<std::dec<<": "<<at->second<<'\n';
                continue;
            }
            if(!banks.contains(p.definition) && !light_banks.contains(p.definition))throw FormatError("projectile has no authored prepared model bank");
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
            auto &owner=owners.at(handle);const auto &p=owner.projectile;
            if(!banks.contains(p.definition))continue;
            auto &bank=banks.at(p.definition);
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
    const std::vector<SourceLightVolumeDraw>& lights(unsigned view,Vec3 forward,const SourceViewProjectionBank &camera,
        RenderViewport viewport,float distance) {
        auto &out=light_draws.at(view);out.clear();
        for(auto handle:order) {
            const auto &p=owners.at(handle).projectile;
            const auto found=light_banks.find(p.definition);if(found==light_banks.end())continue;
            const auto root=source_object_transform(p.position,p.forward,p.up,1.f,false);
            for(const auto &attachment:found->second) {
                SourceLightVolumeDraw draw;draw.object=handle;draw.definition=attachment.definition;
                for(const auto &v:attachment.volume->volumes)
                    if(std::none_of(draw.bitmap_materials.begin(),draw.bitmap_materials.end(),[&](const auto &m){return m.bitmap==v.bitmap;}))
                        draw.bitmap_materials.push_back({v.bitmap,light_materials.at(v.bitmap)});
                if(draw.bitmap_materials.empty())continue;
                draw.material=draw.bitmap_materials.front().material;
                const auto state=source_light_volume_attachment_state(root,attachment.use_marker_scale,1.f,{1,1,1});
                source_light_volume_command(draw.command,handle,attachment.definition,state,camera.camera_to_world.position,forward);
                draw.volume=attachment.volume;draw.tables=cache.shader_function_tables();draw.state=state;
                draw.camera=camera;draw.viewport=viewport;draw.actual_w_distance=distance;
                out.push_back(std::move(draw));
            }
        }
        return out;
    }
};
NativeWeaponProjectileScene::NativeWeaponProjectileScene(const Cache &c,std::size_t bsp,Mesh &m,
    const CollisionWorld &world,std::span<const std::uint32_t> definitions,std::size_t capacity)
    :impl_(std::make_unique<Impl>(c,bsp,m,world,definitions,capacity)){}
NativeWeaponProjectileScene::~NativeWeaponProjectileScene()=default;
void NativeWeaponProjectileScene::reset_lifetimes() noexcept {
    auto &s=*impl_;s.owners.clear();s.order.clear();
    for(auto &view:s.draws)view.clear();
    for(auto &view:s.light_draws)view.clear();
    for(auto &[definition,bank]:s.banks)bank.geometry.reset_history();
}
void NativeWeaponProjectileScene::synchronize(std::span<const NativeWeaponProjectile> p){impl_->synchronize(p);}
const std::vector<NativeWeaponProjectileDraw>& NativeWeaponProjectileScene::draw_view(std::int32_t frame,const SourceObjectViewLod &v){return impl_->draw(frame,v);}
const std::vector<SourceLightVolumeDraw>& NativeWeaponProjectileScene::light_volume_view(unsigned view,Vec3 forward,
    const SourceViewProjectionBank &camera,RenderViewport viewport,float distance){return impl_->lights(view,forward,camera,viewport,distance);}
}
