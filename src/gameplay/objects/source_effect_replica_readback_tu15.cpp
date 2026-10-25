#include "halo2/source_effect_replica_render_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/cache.hpp"
#include <cmath>
#include <set>

namespace halo2 {
void validate_source_effect_replica_render_tu15(const SourceEffectReplicaRenderTu15 &v) {
    if(v.identity==0xffffffffu || !(v.identity>>16) || (v.identity&65535u)>=256u ||
       (v.attached_owner!=0xffffffffu && (!(v.attached_owner>>16)||(v.attached_owner&65535u)>=2048u)) ||
       (v.local_view!=0xffffffffu && v.local_view>=4u))
        throw FormatError("Replica effect render input lost its complete original identities/view");
    if((v.root_owner==0xffffffffu)!=(v.root_type==0xffu) ||
       (v.root_owner!=0xffffffffu && (!(v.root_owner>>16)||(v.root_owner&65535u)>=2048u||v.root_type>=16u)) ||
       (v.attached_owner==0xffffffffu && v.root_owner!=0xffffffffu))
        throw FormatError("Replica effect root lost its complete original identity/type");
    for(const auto a:{v.color.x,v.color.y,v.color.z,v.velocity.x,v.velocity.y,v.velocity.z})
        if(!std::isfinite(a))throw FormatError("Replica effect render input has a nonfinite source component");
}
SourceEffectReplicaRenderTu15 source_effect_replica_render_tu15(
    const SourceRuntimeMemory &m,std::uint32_t effect,std::uint32_t definition) {
    const auto word=[&](unsigned at){return Bytes(m.view(at,4)).u32(0);};
    const auto pool=word(0x4f720cu);
    if(word(pool+0x20u)!=256 || word(pool+0x24u)!=400 || word(pool+0x38u)>256)
        throw FormatError("Effect render readback lacks its actual original pool layout");
    const SourceRawDatumPoolServices storage{.read=[&](auto at,auto n){return m.view(at,n);}};
    const auto body=source_raw_datum_lookup(pool,effect,storage),index=effect&65535u;
    if(!body || !(word(word(pool+0x48u)+(index/32u)*4u)&(1u<<(index%32u))) || word(body+4u)!=definition)
        throw FormatError("Effect render readback lost its occupied full effect/tag generation");
    const Bytes b(m.view(body,400));
    SourceEffectReplicaRenderTu15 result{effect,b.u32(0x48),b.u32(0x58),b.u16(2),
        {b.f32(0x24),b.f32(0x28),b.f32(0x2c)},{b.f32(0x3c),b.f32(0x40),b.f32(0x44)}};
    if(result.attached_owner!=0xffffffffu) {
        const auto objects=word(0x4ecbd0u);
        if(word(objects+0x20u)!=2048u||word(objects+0x24u)!=12u||word(objects+0x38u)>2048u)
            throw FormatError("Effect root readback lacks its original object header pool");
        std::set<std::uint32_t> visited;
        for(auto owner=result.attached_owner;;) {
            if(!visited.insert(owner).second||visited.size()>2048u)throw FormatError("Effect root chain cycles");
            const auto header=source_raw_datum_lookup(objects,owner,storage),slot=owner&65535u;
            if(!header||!(word(word(objects+0x48u)+(slot/32u)*4u)&(1u<<(slot%32u)))||
               Bytes(m.view(header+6u,2)).u16(0)<0x18u)
                throw FormatError("Effect root lost an occupied complete object generation");
            const auto body=word(header+8u),parent=word(body+0x14u);
            if(parent==0xffffffffu) {
                result.root_owner=owner;result.root_type=std::to_integer<std::uint8_t>(m.view(header+3u,1)[0]);break;
            }
            owner=parent;
        }
    }
    validate_source_effect_replica_render_tu15(result);return result;
}
bool source_effect_replica_attached_world_tu15(const SourceEffectReplicaRenderTu15 &v) {
    validate_source_effect_replica_render_tu15(v);
    return v.attached_owner!=0xffffffffu&&v.root_owner!=0xffffffffu&&v.root_type!=0u&&
        v.local_view==0xffffffffu&&!(v.flags&0x100u)&&
        v.velocity.x==0.f&&v.velocity.y==0.f&&v.velocity.z==0.f;
}
bool source_effect_replica_static_world_tu15(const SourceEffectReplicaRenderTu15 &v) {
    validate_source_effect_replica_render_tu15(v);
    return v.attached_owner==0xffffffffu && v.local_view==0xffffffffu && !(v.flags&0x102u) &&
        v.velocity.x==0.f && v.velocity.y==0.f && v.velocity.z==0.f;
}
}
