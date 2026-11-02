#include "halo2/pstencil/native_incident.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <bit>
#include <cmath>
namespace halo2::pstencil {
std::optional<NativeIncident> native_incident_tu15(PhysicsLiveWorldTu15& world,
    std::uint32_t object,float metric) {
    if(!std::isfinite(metric))throw FormatError("pStencil native incident requires a finite native lighting metric");
    const auto& memory=world.memory();
    const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
    const auto half=[&](std::uint32_t p){return Bytes(memory.view(p,2)).u16(0);};
    const auto body=world.source_object_body(object);
    const auto definition=word(body),row=word(0x4f0414u)+(definition&65535u)*16u;
    if(word(row+4)!=definition)throw FormatError("pStencil distant caster lost its complete definition");
    const auto type=half(word(row+8));
    if(type!=0 && type!=1)return {}; // Actual2003 eligibility families.
    std::set<std::uint32_t> seen;
    auto owner=object;
    for(;;) {
        if(!seen.insert(owner).second)throw FormatError("pStencil native incident found a parent cycle");
        const auto parent=word(world.source_object_body(owner)+0x14u);
        if(parent==0xffffffffu)break;
        owner=parent;
    }
    const auto root=world.source_object_body(owner);
    // A static probe stores converted lighting, not a raw incident. This
    // explicit current-asset adaptation runs the original surface-cache
    // updater separately; it never clears the probe or replaces visible PRT.
    // Cinematic overrides likewise retain their original flag/default bank.
    // D3D90's unavailable-bank branch returns true WITHOUT writing a sample.
    // Cache validity alone would then admit the initialized fallback vector.
    const auto lightmaps=word(0x4ecc14u),structure=word(0x4ecc18u);
    if(!lightmaps || !structure || std::bit_cast<std::int32_t>(word(lightmaps+0x80u))<=0)return {};
    const auto group=word(lightmaps+0x84u);
    if(word(group+0x1cu)==0xffffffffu || word(group+4u)!=word(structure+8u))return {};
    // Whole3DDC0 owns allocation/generation and object+CC publication.
    const std::array acquire_args{owner};const auto token=world.call_actual(0x3ddc0u,acquire_args);
    if(token==0xffffffffu)return {};
    const auto pool=word(0x515f14u);
    if(word(pool+0x20u)!=256 || word(pool+0x24u)!=256 || (token&65535u)>=word(pool+0x38u))
        throw FormatError("pStencil native incident lost its original256-record render cache");
    const auto cache=word(pool+0x44u)+(token&65535u)*256u;
    if(word(cache+4u)!=owner || word(root+0xccu)!=token)
        throw FormatError("pStencil native incident lost its render-cache owner");
    // Literal3DB81..3DB88 ABI: cache and owner on stack, metric in XMM0.
    // No object flag, lighting value, sample status or query result is supplied.
    const std::array args{token,owner};auto call=world.prepare_source_call(args);
    call.xmm[0][0]=std::bit_cast<std::uint32_t>(metric);world.run_actual(0x3dfd0u,call);
    const Bytes result(memory.view(cache,256));
    if(!std::to_integer<unsigned>(result.slice(2,1)[0]) || !std::to_integer<unsigned>(result.slice(3,1)[0]))return {};
    NativeIncident out{owner,token,{{result.f32(0x1c),result.f32(0x20),result.f32(0x24)},result.u32(0x28),result.u32(0x2c)}};
    const auto v=out.sample.incident;
    if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z))
        throw FormatError("pStencil native incident contains nonfinite components");
    if(dot(v,v)<1.e-8f)return {};
    return out;
}
}
