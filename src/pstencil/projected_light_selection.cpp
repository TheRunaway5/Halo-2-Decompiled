#include "halo2/pstencil/projected_light_selection_tu15.hpp"
#include "halo2/pstencil/projected_light_material.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <bit>
#include <cstring>

namespace halo2::pstencil {
ProjectedLightSelection native_projected_light_selection_tu15(PhysicsLiveWorldTu15& world,
    std::uint32_t light,const NativeLightReceiver& receiver) {
    const auto& memory=world.memory();
    const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    const auto real=[&](std::uint32_t at){return std::bit_cast<float>(word(at));};
    const auto put=[&](std::uint32_t at,const auto& value){world.write_source_call_input(at,std::as_bytes(std::span(&value,1)));};
    if(receiver.context!=0x5598f8u || word(0x4cd3a0u)!=receiver.context)
        throw FormatError("Projected selection lost its actual secondary query");
    const SourceRawDatumPoolServices storage{.read=[&](auto at,auto n){return memory.view(at,n);}};
    const auto record=source_raw_datum_lookup(word(0x4ecbdcu),light,storage);
    if(!record || Bytes(memory.view(record,2)).u16(0)!=std::uint16_t(light>>16))
        throw FormatError("Projected selection lost its full native light generation");
    const auto definition=word(record+4u),tag=word(0x4f0414u)+(definition&65535u)*16u;
    if(word(tag+4u)!=definition)throw FormatError("Projected selection lost its full native definition");
    const auto root=word(tag+8u);const Bytes metadata(memory.view(root,228));
    if(!metadata.u16(4))throw FormatError("Projected selector consumed a sphere");
    //4A28F..4A369 supplies the pre-49790 specular RGB. The selected-light
    //fade affects diffuse only; specular uses min(secondary,overall).
    const auto secondary=real(record+0x104u),overall=real(record+0x100u);
    const auto factor=secondary>overall?overall:secondary;
    const Vec3 specular{real(record+0xe0u)*factor,real(record+0xe4u)*factor,real(record+0xe8u)*factor};
    ProjectedLightSelection out;
    out.features=projected_light_feature_mask(specular,real(0x46a18cu),
        receiver.shadow_enabled && (word(0x4ce2a0u)&0x400u),real(record+0x108u),metadata.u16(0x14));
    constexpr std::uint32_t bank=0x4ce280u,capacity=2560u;
    std::array<std::uint32_t,6> header;
    std::memcpy(header.data(),memory.view(bank,sizeof(header)).data(),sizeof(header));
    if(header[0]!=1024 || header[1]>header[0] || header[3]>header[1])
        throw FormatError("Projected selector lost its original command bank");
    const auto mask=word(0x4c6834u),previous_definition=word(0x4920e4u);
    const auto active=memory.view(0x4c6838u,1)[0];
    const auto restore=[&] {put(0x4c6834u,mask);put(0x4c6838u,active);put(0x4920e4u,previous_definition);};
    // Publish completed caller inputs only. Keep every selected descriptor,
    //implementation pointer, vertex class and source sort output original.
    put(0x4c6834u,out.features);put(0x4c6838u,std::uint8_t(1));put(0x4920e4u,root);
    try {
        auto call=world.prepare_source_call(std::array{1u});const auto caller=call.gp[4];
        const auto output=caller-0xf000u,callee=caller-0xf01cu;
        put(caller-0xf008u,call.gp[5]);
        const std::array arguments{word(caller),bank,1u,12u,output};put(callee,arguments);
        call.gp[4]=callee;call.gp[0]=output;call.gp[3]=12;call.gp[5]=1;call.gp[7]=bank;
        world.run_actual(0x3baa0u,call);
        const auto count=call.gp[0];
        if(call.gp[4]!=caller-0xf008u || count>capacity || count>header[1]-header[3])
            throw FormatError("Projected selector escaped its original caller output");
        out.records.resize(count);
        if(count)std::memcpy(out.records.data(),memory.view(output,count*sizeof(out.records[0])).data(),count*sizeof(out.records[0]));
    }catch(...) {restore();throw;}
    restore();return out;
}
}
