#include "halo2/pstencil/point_lights.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/posed_collision.hpp"
#include <algorithm>
#include <bit>
#include <set>

namespace halo2::pstencil {
namespace {
constexpr auto none=0xffffffffu;
void put(SourceEffectLightDatumPool::Record& r,unsigned at,float value){r.at(at/4)=std::bit_cast<std::uint32_t>(value);}
void put(SourceEffectLightDatumPool::Record& r,unsigned at,Vec3 v){put(r,at,v.x);put(r,at+4,v.y);put(r,at+8,v.z);}
}
struct PointLights::Impl {
    struct Attachment {unsigned slot;std::uint32_t light,marker;std::vector<ObjectAttachmentMarker> markers;};
    struct Live {std::uint32_t light,definition;Transform frame;};
    const Cache& cache;unsigned bsp;
    BspAtmosphere locations;ObjectLightSpatialBsp memberships;TerrainCollision terrain;
    SourceEffectLightDatumPool pool;SourceEffectLightSpatialPool spatial;
    SourceEffectLightQueryState queries;std::array<SourceShadowSelectionCache,4> selection;
    std::map<std::uint32_t,EffectLightDefinition> definitions;
    std::map<std::uint32_t,std::vector<Attachment>> attachments;
    std::map<std::tuple<std::uint32_t,unsigned,unsigned>,Live> live;
    std::set<std::string> gaps;
    SourceEffectLightLifetimeWorld inputs(std::uint64_t tick)const {
        return {locations,memberships,terrain.bsps.at(0),cache.original_data()->fade_curve,
            std::uint32_t(tick),queries.epoch,30,std::int16_t(bsp),true};
    }
    Impl(const Cache& c,unsigned index):cache(c),bsp(index),locations(c.load_bsp_atmosphere(index)),
        memberships(c.load_object_light_spatial_bsp(index)),terrain(c.load_raw_terrain_collision(index)),spatial(memberships.clusters.size()) {
        pool.activate();
        for(const auto& tag:c.tags()) {
            if(tag.group!="proj" && tag.group!="weap" && tag.group!="bipd" && tag.group!="vehi" &&
               tag.group!="scen" && tag.group!="bloc" && tag.group!="mach" && tag.group!="ctrl" && tag.group!="lifi")continue;
            const auto metadata=c.load_source_tag_metadata(tag.datum);
            const Bytes root(metadata.bytes_at(metadata.source_address(),0x9c));
            const auto block=read_tag_block_descriptor(root,0x94);const auto extent=block.extent(24,4095);
            if(!block.count)continue;
            const Bytes rows(c.read_tag_bytes(block.address,extent));
            for(unsigned n=0;n<unsigned(block.count);++n) {
                const auto at=n*24;if(rows.u32(at)!=0x6c696768u || rows.u32(at+4)==none)continue;
                try {
                    const auto id=rows.u32(at+4);auto d=load_effect_light(c,id);
                    // C0230 clears the timed flag for direct object lights.
                    // Constant unnamed functions use the authored high endpoints.
                    if(rows.u32(at+16) || rows.u32(at+20))
                        throw FormatError("direct light requires its source object function values");
                    if(d.duration!=0)throw FormatError("direct light requires its untimed attachment update");
                    if(d.flags&1)continue; // Authored disabled light.
                    Attachment a{n,id,rows.u32(at+8),{}};
                    if(a.marker) {
                        const auto model=c.load_initial_model_definition(tag.datum).render_model;
                        if(model==none)throw FormatError("named point-light marker has no authored model");
                        a.markers=source_model_markers(c.load_source_tag_metadata(model),a.marker);
                    }
                    definitions.try_emplace(id,std::move(d));attachments[tag.datum].push_back(std::move(a));
                }catch(const FormatError& e){gaps.insert(tag.name+": "+e.what());}
            }
        }
    }
};
PointLights::PointLights(const Cache& c,unsigned bsp):impl_(std::make_unique<Impl>(c,bsp)){}
PointLights::~PointLights()=default;
bool PointLights::has(std::uint32_t d)const{return impl_->attachments.contains(d);}
std::vector<std::uint32_t> PointLights::definitions()const {
    std::vector<std::uint32_t> out;for(const auto& [id,d]:impl_->definitions)out.push_back(id);return out;
}
std::vector<std::string> PointLights::unavailable()const{return {impl_->gaps.begin(),impl_->gaps.end()};}
void PointLights::synchronize(std::span<const PointLightOwner> owners,std::uint64_t tick) {
    auto& i=*impl_;std::set<std::tuple<std::uint32_t,unsigned,unsigned>> present;
    for(const auto& owner:owners) {
        const auto bank=i.attachments.find(owner.definition);if(bank==i.attachments.end())continue;
        if(owner.object==none)throw FormatError("pStencil point light requires its full object generation");
        for(const auto& a:bank->second) {
            std::vector<Transform> frames;
            if(!a.marker)frames.push_back(owner.root);
            else for(const auto& m:pose_model_markers(a.markers,owner.nodes,owner.permutations))frames.push_back(m.world);
            for(unsigned n=0;n<frames.size();++n) {
                const auto key=std::tuple{owner.object,a.slot,n};
                if(!present.insert(key).second)throw FormatError("pStencil point light has duplicate source owners");
                const auto& frame=frames[n];const auto& d=i.definitions.at(a.light);
                auto prior=i.live.find(key);
                if(prior==i.live.end()) {
                    const auto light=source_standalone_effect_light_create(i.pool,i.spatial,d,a.light,1,
                        frame.position,frame.basis[0],frame.basis[2],i.inputs(tick));
                    if(light==none){i.gaps.insert("pStencil direct point-light pool exhausted");continue;}
                    i.live.emplace(key,Impl::Live{light,a.light,frame});
                }else {
                    if(prior->second.definition!=a.light)throw FormatError("pStencil light generation changed its authored definition");
                    auto snapshot=source_standalone_effect_light(i.pool,prior->second.light);
                    auto registration=snapshot.registration;
                    if(registration.flags&8)source_effect_light_unregister(registration,i.spatial);
                    auto& r=i.pool.record(prior->second.light);
                    r[0]=(r[0]&65535u)|(std::uint32_t(registration.flags&~8u)<<16);r[0x10/4]=none;
                    put(r,0x84,frame.position);put(r,0xac,frame.basis[0]);put(r,0xb8,frame.basis[2]);
                    source_standalone_effect_light_update(i.pool,i.spatial,d,prior->second.light,i.inputs(tick));
                    registration=source_standalone_effect_light(i.pool,prior->second.light).registration;
                    registration.location.cluster=-1;registration.position=frame.position;
                    source_effect_light_register(d,registration,i.locations,std::int16_t(i.bsp),i.memberships,i.spatial,i.terrain.bsps.at(0));
                    r[0]=(r[0]&65535u)|(std::uint32_t(registration.flags)<<16);r[0x10/4]=registration.reference_head;
                    put(r,0x18,registration.spatial_center);put(r,0x24,registration.spatial_radius);
                    put(r,0x28,registration.cull_center);put(r,0x34,registration.cull_radius);put(r,0x38,registration.location_center);
                    r[0x44/4]=registration.location.leaf;
                    r[0x48/4]=std::uint16_t(registration.location.cluster)|(std::uint32_t(std::uint16_t(registration.location.bsp))<<16);
                    prior->second.frame=frame;
                }
            }
        }
    }
    for(auto at=i.live.begin();at!=i.live.end();) {
        if(!present.contains(at->first)){source_standalone_effect_light_delete(i.pool,i.spatial,at->second.light);at=i.live.erase(at);}
        else ++at;
    }
}
SourceEffectLightView PointLights::view(Vec3 camera,const SourceEffectLightCollectionContext& collection,
    const SourceEffectLightCandidateContext& candidates,std::int32_t frame) {
    auto& i=*impl_;const SourceEffectLightDefinitions definition=[&](auto id)->const EffectLightDefinition&{return i.definitions.at(id);};
    source_effect_lights_update_view(i.pool,definition,camera);
    auto result=source_effect_lights_query(i.pool,i.spatial,definition,i.queries,SourceEffectLightQueryKind::Main,collection);
    source_effect_lights_select(result,i.selection,std::bit_cast<std::int32_t>(collection.local_player),candidates,frame,false);return result;
}
std::vector<WorldEffectLightReceiver> PointLights::receivers(const SourceEffectLightView& lights,
    const SourceBspAdmissionBsp& geometry,std::span<const std::uint8_t> fog,const SourceBspAdmission* main) {
    auto& i=*impl_;std::vector<WorldEffectLightReceiver> out;
    const SourceEffectLightReceiverWorld world{i.memberships,geometry,i.locations,std::int16_t(i.bsp)};
    for(const auto& selected:lights.selected) {
        if(!(selected.requested_fade>std::bit_cast<float>(0x3d8f5c29u)))continue;
        const auto at=std::find_if(lights.records.begin(),lights.records.end(),[&](const auto& r){return r.runtime.identity==selected.identity;});
        if(at==lights.records.end())throw FormatError("pStencil selected point light lacks its source record");
        const auto& r=at->runtime;
        const auto shape=source_effect_light_sphere(*at->definition,r.position,r.forward,r.up,r.state.size);
        auto receiver=source_effect_light_receiver_view(world,r.cluster,r.spatial_center,r.spatial_radius,shape,fog,main);
        out.push_back({selected.identity,++i.queries.epoch,selected.requested_fade,std::move(receiver)});
    }
    return out;
}
std::vector<WorldEffectLight> PointLights::lights()const {
    const auto& i=*impl_;std::vector<WorldEffectLight> out;
    for(const auto& [key,live]:i.live) {
        const auto light=source_standalone_effect_light(i.pool,live.light);const auto& d=i.definitions.at(live.definition);
        WorldEffectLight value;value.definition=live.definition;value.identity=live.light;value.effect_owner=std::get<0>(key);
        value.position=light.registration.position;value.forward=light.forward;value.up=light.up;value.state=light.state;
        value.radius=d.radius;value.specular_radius=d.specular_radius;value.registration=light.registration;out.push_back(value);
    }
    return out;
}
}
