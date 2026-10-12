#include "halo2/object_light_query.hpp"
#include "halo2/math.hpp"
#include "halo2/source_object_light_rays.hpp"
#include <bit>
#include <limits>
namespace halo2 {
std::uint32_t source_object_light_collision_flags(bool initial_query) {
    return initial_query?0x4480000du:0x04800c0du;
}
ObjectLightQueryResult query_object_lightmap(const BspSurfaceLighting& bank,const CollisionWorld& world,
    Vec3 origin,unsigned mode,float detail) {
    return query_object_lightmap(bank,world,origin,ObjectLightMapQueryContext{mode,true,detail});
}
ObjectLightQueryResult query_object_lightmap(const BspSurfaceLighting& bank,const CollisionWorld& world,
    Vec3 origin,const ObjectLightMapQueryContext& context) {
    const auto collision_flags=source_object_light_collision_flags(context.initial_query);
    ObjectLightQueryResult result;
    bool unresolved_source=false;
    (void)world; // Map contacts use source BSP vector traversal, not host fan triangles.
    SourceObjectLightRaysInput input;
    input.bank_valid=source_object_light_query_bank_valid(bank.lighting);input.flags=context.mode;
    input.initial_query=context.initial_query;input.origin=origin;
    std::optional<RenderSurfaceHit> current;
    auto output=context.caller_sample.value_or(ObjectLightmapSample{});
    const auto rays=source_object_light_rays(input,[&](const SourceObjectLightRayRequest& request,
        SourceRenderContactPacket& packet) {
        auto start=request.origin;const auto ray=request.delta;
        for(unsigned attempt=0;attempt<3;++attempt) {
            bool unresolved_order=false;
            const auto collision=original_map_vector_contact(bank,start,ray,&unresolved_order,collision_flags);
            if(!collision)break;
            if(unresolved_order) {++result.attribution_failures;unresolved_source=true;break;}
            const auto surface=bank.attribute(collision->source,collision->point,collision->contact.leaf);
            if(surface) {
                current=surface;
                // The native MAP adapter retains only the source fields used
                // by its explicit geometry bank. Guest resource pointers are
                // not fabricated in the unused packet words.
                packet[0]=std::uint32_t(surface->cluster);packet[1]=std::uint32_t(surface->instance);
                packet[2]=0xffffffffu;packet[8]=std::uint32_t(surface->part);
                packet[9]=surface->strip;packet[10]=surface->lightmap_triangle;
                packet[12]=std::bit_cast<std::uint32_t>(surface->weight_b);
                packet[13]=std::bit_cast<std::uint32_t>(surface->weight_c);return true;
            }
            ++result.attribution_failures;
            // 14AF40 retries failed authored double-sided contacts at most twice, moving
            // the current ray start by its full displacement times .02.
            if(!(collision->contact.flags&1))break;
            start=start+ray*.02f;
        }
        return false;
    },[&](const SourceRenderContactPacket&,std::uint32_t,bool initial,ObjectLightmapSample& target) {
        // Original D2F90/D30D0 always sample detail1. Native decoded images
        // are synchronously resident; original asynchronous image/status
        // producers remain a separate service boundary.
        const auto sample=bank.sample(*current,1,initial,context.caller_sample);
        result.surface=current;
        if(!sample)return 1;
        target=*sample;return 0;
    },output);
    result.rays=unsigned(rays.steps.size());
    result.status=ObjectLightQueryStatus(rays.status);
    if(rays.status==0)result.sample=output;
    else if(unresolved_source)result.status=ObjectLightQueryStatus::attribution_unresolved;
    return result;
}
ObjectLightQueryResult query_object_sky_lightmap(const BspSurfaceLighting& bank,
    const ObjectSkyLightingDefinition* sky,Vec3 origin) {
    ObjectLightQueryResult result;
    if(!sky)return result;
    const auto frame=object_sky_light_frame(*sky);
    std::optional<std::int16_t> material;
    if(frame.test_collision) {
        if(bank.collision.empty()) {result.status=ObjectLightQueryStatus::unavailable;return result;}
        const auto& incident=frame.incident;
        const Vec3 ray{incident.x*100.f,incident.y*100.f,incident.z*100.f};
        // 1697C0 defaults absent facing bits to0x01800000. 168AE0 maps
        // 0x11800001 to0x23. Source query flag32 has no BSP admission effect.
        const auto collision=original_bsp_vector_query(bank.collision.front(),origin,ray,
            std::numeric_limits<float>::max(),0x23); // 169846 loads44AED4.
        ++result.rays;
        if(collision.contact)material=collision.contact->material;
    }
    result.status=ObjectLightQueryStatus::resolved;
    result.sample=object_sky_light_sample(frame,material);
    return result;
}
ObjectLightQueryAdmission source_object_light_query(const ObjectLightQueryContext& context,
    const std::function<ObjectLightQueryResult(unsigned)>& map_query,
    const std::function<ObjectLightQueryResult()>& sky_query) {
    ObjectLightQueryAdmission out;
    if(!context.bank_valid) {
        // D1850 returns true without writing its output on this guard path.
        out.accepted=true;out.query.status=ObjectLightQueryStatus::unavailable;return out;
    }
    const unsigned mode=bool(context.definition_flags&2) || bool(context.runtime_flags&(1u<<13));
    out.query=map_query(mode);
    if(out.query.status==ObjectLightQueryStatus::resolved) {out.accepted=true;return out;}
    if(out.query.status==ObjectLightQueryStatus::attribution_unresolved) {out.query.sample.reset();return out;}
    if(mode) {
        out.retried=true;out.query=map_query(0);
        if(out.query.status==ObjectLightQueryStatus::attribution_unresolved) {out.query.sample.reset();return out;}
    }
    if(out.query.status==ObjectLightQueryStatus::temporary) {out.query.sample.reset();return out;}
    bool eligible=false;
    if(context.object_type==0)eligible=context.biped_state==2;
    else if(context.object_type==1) {
        const auto shift=context.vehicle_type&31;
        eligible=bool((1u<<shift)&0x28);
    } else if(context.object_type==12)eligible=bool(context.creature_flags&16);
    if(eligible) {
        out.used_sky=true;out.query=sky_query();
        out.accepted=out.query.status==ObjectLightQueryStatus::resolved;
    }
    if(!out.accepted)out.query.sample.reset();
    return out;
}
int source_biped_light_query_initial_state(std::uint32_t definition_flags,std::uint16_t runtime_flags) {
    return runtime_flags&4?3:definition_flags&16?2:1;
}
bool source_object_light_query_bank_valid(const BspLighting& bank) {
    return bank.structure_matches && bank.bitmap_datum!=0xffffffffu;
}
} // namespace halo2
