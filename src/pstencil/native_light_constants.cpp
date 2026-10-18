#include "halo2/pstencil/native_light_constants_tu15.hpp"
#include "halo2/pstencil/native_light_shape_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <bit>
#include <cmath>
#include <cstring>

namespace halo2::pstencil {
EffectLightUniforms native_light_constants_tu15(PhysicsLiveWorldTu15& world,
    std::uint32_t light,const EffectLightViewFades& fades,float selected,
    const std::array<ShaderVector,9>& previous) {
    for(float value:{fades.diffuse,fades.specular,fades.secondary,fades.overall,fades.volume,selected})
        if(!std::isfinite(value))throw FormatError("Native light constants require completed finite fades");
    const auto& memory=world.memory();
    const auto word=[&](std::uint32_t address){return Bytes(memory.view(address,4)).u32(0);};
    const auto pool=word(0x4ecbdcu);
    if(word(pool+0x20u)!=350 || word(pool+0x24u)!=272)
        throw FormatError("Native light constants lost the actual light pool");
    const SourceRawDatumPoolServices storage{.read=[&](auto p,auto size){return memory.view(p,size);}};
    const auto record=source_raw_datum_lookup(pool,light,storage);
    if(!record || Bytes(memory.view(record,2)).u16(0)!=std::uint16_t(light>>16))
        throw FormatError("Native light constants require their full occupied light generation");
    const auto definition=word(record+4u),tag=word(0x4f0414u)+(definition&65535u)*16u;
    if(word(tag+4u)!=definition)throw FormatError("Native light constants lost their full definition generation");
    const auto shape=native_light_shape_tu15(world,light);
    const Bytes source(memory.view(record,272));
    // Literal4A28F..4A369: native color components multiplied after the
    // overall/selected product; specular uses min(secondary,overall).
    const float diffuse=fades.overall*selected;
    const float specular=fades.secondary>fades.overall?fades.overall:fades.secondary;
    const Vec3 diffuse_color{source.f32(0xd4)*diffuse,source.f32(0xd8)*diffuse,source.f32(0xdc)*diffuse};
    const Vec3 specular_color{source.f32(0xe0)*specular,source.f32(0xe4)*specular,source.f32(0xe8)*specular};
    auto call=world.prepare_source_call();const auto caller=call.gp[4];
    const auto shape_input=caller-0x100u,output=caller-0x500u,diffuse_input=caller-0x600u,
        specular_input=caller-0x610u,shift=caller-0x620u,gel_u=caller-0x650u,gel_v=caller-0x660u;
    const auto put=[&](std::uint32_t address,const auto& value) {
        world.write_source_call_input(address,std::as_bytes(std::span(&value,1)));
    };
    put(shape_input,shape.words);put(output,previous);put(diffuse_input,diffuse_color);put(specular_input,specular_color);
    world.write_source_call_input(gel_u,memory.view(0x49222cu,16));
    world.write_source_call_input(gel_v,memory.view(0x49223cu,16));
    // Literal4A3A1..4A3C2: all eight stack arguments plus EAX destination
    // and ECX completed shape. Reserve a separate actual caller frame.
    call.gp[4]=caller-0x3000u;
    const std::array args{word(caller),record+0x84u,word(tag+8u),diffuse_input,specular_input,
        source.u32(0xec),shift,gel_u,gel_v};
    put(call.gp[4],args);call.gp[0]=output;call.gp[1]=shape_input;
    std::array<bool,4> shift_written{};
    world.run_actual(0x49790u,call,[&](const SourcePhysicsContactRecordStore& store) {
        for(unsigned n=0;n<store.written.size();++n)
            if(store.address+n>=shift && store.address+n<shift+4u)shift_written[store.address+n-shift]=true;
    });
    for(bool written:shift_written)if(!written)throw FormatError("Original light constants did not produce a color shift");
    EffectLightUniforms out;
    std::memcpy(out.constants.data(),memory.view(output,sizeof(out.constants)).data(),sizeof(out.constants));
    std::memcpy(out.gel_basis_u.data(),memory.view(gel_u,sizeof(out.gel_basis_u)).data(),sizeof(out.gel_basis_u));
    std::memcpy(out.gel_basis_v.data(),memory.view(gel_v,sizeof(out.gel_basis_v)).data(),sizeof(out.gel_basis_v));
    out.color_shift=word(shift);
    if(out.color_shift>2)throw FormatError("Original light constants produced an invalid color shift");
    return out;
}
}
