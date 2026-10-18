#include "halo2/pstencil/native_light_receiver_tu15.hpp"
#include "halo2/pstencil/native_light_shape_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <bit>
#include <cstring>

namespace halo2::pstencil {
bool native_light_render_admission_tu15(PhysicsLiveWorldTu15& world,std::uint32_t light) {
    const auto& memory=world.memory();
    const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    const SourceRawDatumPoolServices storage{.read=[&](auto at,auto n){return memory.view(at,n);}};
    const auto record=source_raw_datum_lookup(word(0x4ecbdcu),light,storage);
    if(!record || Bytes(memory.view(record,2)).u16(0)!=std::uint16_t(light>>16))
        throw FormatError("Native projected admission lost its full light generation");
    const auto definition=word(record+4u),tag=word(0x4f0414u)+(definition&65535u)*16u;
    if(word(tag+4u)!=definition)
        throw FormatError("Native projected admission lost its full definition generation");
    auto call=world.prepare_source_call();const auto caller=call.gp[4],shape=caller-0x100u;
    // The real caller owns the shape above the callee's stack. Keep native
    //31570/C3820/C3AB0 scratch below this independent caller output too.
    call.gp[4]=caller-0x3000u;call.gp[7]=light;
    const std::array args{word(caller),shape};
    world.write_source_call_input(call.gp[4],std::as_bytes(std::span(args)));
    world.run_actual(0x31570u,call);
    return (call.gp[0]&255u)!=0;
}
NativeLightReceiver native_light_receiver_tu15(PhysicsLiveWorldTu15& world,
    std::uint32_t light) {
    const auto& memory=world.memory();
    const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    const auto half=[&](std::uint32_t at){return Bytes(memory.view(at,2)).u16(0);};
    const auto put=[&](std::uint32_t at,const auto& value) {
        world.write_source_call_input(at,std::as_bytes(std::span(&value,1)));
    };
    const SourceRawDatumPoolServices storage{.read=[&](auto at,auto n){return memory.view(at,n);}};
    const auto pool=word(0x4ecbdcu),record=source_raw_datum_lookup(pool,light,storage);
    if(!record || half(record)!=std::uint16_t(light>>16) || word(pool+0x24u)!=272)
        throw FormatError("Native light receiver lost its full live light generation");
    const auto definition=word(record+4u),tag=word(0x4f0414u)+(definition&65535u)*16u;
    if(word(tag+4u)!=definition)throw FormatError("Native light receiver lost its full definition");
    const auto flags=word(word(tag+8u));
    auto cluster=std::int32_t(std::bit_cast<std::int16_t>(half(record+0x48u)));
    if(cluster==-1 && (flags&0x20u) && (flags&0x40000u))
        cluster=std::int32_t(word(0x4c6704u));
    const auto cameras=native_light_cameras_tu15(world,light);
    if(cameras.empty() || cameras.size()>6)throw FormatError("Native light receiver has no original camera bank");
    auto call=world.prepare_source_call();const auto caller=call.gp[4];
    const auto input=caller-0x2000u;
    world.write_source_call_input(input,std::as_bytes(std::span(cameras)));
    //31DDB..31DF2: use the real prior main context only when the native
    // caller's4C6830 flag is clear. Do not fabricate an auxiliary visible set.
    const auto auxiliary=memory.view(0x4c6830u,1)[0]==std::byte{}?0x556e18u:0u;
    call.gp[4]=caller-0x3000u;
    const std::array arguments{word(caller),(flags>>12)&0x400u,input,record+0x18u,
        word(record+0x24u),auxiliary,0u,std::uint32_t(cluster)};
    put(call.gp[4],arguments);call.gp[0]=1;call.gp[1]=std::uint32_t(cameras.size());
    call.gp[2]&=0xffffff00u; // Actual31E93 DL0, not a supplied query mode.
    world.run_actual(0x134c00u,call);
    NativeLightReceiver out;out.context=call.gp[0];
    if(out.context!=0x5598f8u || word(0x4cd3a0u)!=out.context)
        throw FormatError("Original light query did not retain its secondary context");
    // Complete31650 classifies source receiver/caster flags after134C00.
    // Its exact31E9F..31EBD ABI keeps the light identity in EAX, the query
    // context in ESI, and the definition datum in ECX.
    const auto shape=native_light_shape_tu15(world,light);
    auto shadow=std::uint8_t(!(flags&8u) && memory.view(0x4c6840u,1)[0]!=std::byte{} &&
        std::bit_cast<float>(word(record+0xf8u))>std::bit_cast<float>(word(0x46a0e8u)) && shape.type()!=0);
    const auto shadow_input=caller-0x100u;put(shadow_input,shadow);
    const auto owner=half(record+0x54u)==0xffffu?0xffffffffu:word(record+0x4cu);
    auto root=0xffffffffu;
    if(owner!=0xffffffffu) {
        auto hierarchy=world.prepare_source_call();hierarchy.gp[1]=owner;
        world.run_actual(0xbcfb0u,hierarchy);root=hierarchy.gp[0];
    }
    call=world.prepare_source_call(std::array{owner,root,std::uint32_t(shape.type()==0),shadow_input});
    call.gp[0]=light;call.gp[1]=definition;call.gp[6]=out.context;
    world.run_actual(0x31650u,call);
    out.shadow_enabled=memory.view(shadow_input,1)[0]!=std::byte{};
    auto& view=out.view;out.visibility_bank=word(out.context);
    view.flags=word(out.context+0x2a60u);view.pass_mode=word(out.context+0x2a78u);
    view.query_mode=word(out.context+0x2a7cu);
    std::memcpy(&view.location,memory.view(out.context+4u,sizeof(view.location)).data(),sizeof(view.location));
    const auto copy=[&](std::uint32_t at,auto& output,std::size_t count) {
        output.resize(count);
        if(count)std::memcpy(output.data(),memory.view(at,count*sizeof(output[0])).data(),count*sizeof(output[0]));
    };
    const auto bank=out.visibility_bank;
    const auto groups=half(bank),clusters=half(bank+0xa6cu),frustums=half(bank+0x1970u);
    if(groups>6 || clusters>128 || frustums>512)
        throw FormatError("Original light query exceeded its native visibility banks");
    copy(bank+4u,view.visibility.groups,groups);
    copy(bank+0xa6eu,view.visibility.clusters,clusters);
    copy(bank+0x1770u,view.visibility.sorted_cluster_indices,clusters);
    copy(bank+0x1974u,view.visibility.frustums,frustums);
    // Literal context association/map banks shared by BSP/object admission.
    auto& bsp=view.bsp;
    std::memcpy(bsp.cluster_entries.data(),memory.view(out.context+0x1cu,sizeof(bsp.cluster_entries)).data(),sizeof(bsp.cluster_entries));
    std::memcpy(bsp.marked_clusters.data(),memory.view(out.context+0x21cu,sizeof(bsp.marked_clusters)).data(),sizeof(bsp.marked_clusters));
    const auto associations=half(out.context+0xa5eu);
    if(associations>512)throw FormatError("Original light query exceeded its association bank");
    copy(out.context+0xa60u,bsp.associations,associations);
    out.objects.associations=bsp.associations;
    const auto queue=[&](unsigned offset,auto& output) {
        const auto header=word(out.context+offset);const auto count=half(header+4u);
        if(count>half(header))throw FormatError("Original light query exceeded a native queue");
        for(unsigned n=0;n<count;++n)output.push_back({word(word(header+12u)+n*4u),
            half(word(header+8u)+n*2u),half(word(header+20u)+n*2u),word(word(header+16u)+n*4u)});
    };
    queue(12,bsp.clusters);queue(20,out.objects.objects);queue(24,bsp.instances);
    return out;
}
NativeLightGeometry native_light_geometry_tu15(PhysicsLiveWorldTu15& world,
    const NativeLightReceiver& receiver) {
    const auto word=[&](std::uint32_t at){return Bytes(world.memory().view(at,4)).u32(0);};
    if(receiver.context!=0x5598f8u || word(0x4cd3a0u)!=receiver.context ||
       word(receiver.context)!=receiver.visibility_bank)
        throw FormatError("Native light geometry lost its completed receiver query");
    world.call_actual(0x2cb10u);
    auto call=world.prepare_source_call();call.gp[3]=receiver.context;
    world.run_actual(0x135ea0u,call); //31EDA's retained EBX context.
    world.call_actual(0x136030u,std::array{receiver.context});
    call=world.prepare_source_call(std::array{1u});call.gp[0]=receiver.context;
    world.run_actual(0x136230u,call);
    world.call_actual(0x44710u);
    call=world.prepare_source_call();call.gp[0]=0;
    world.run_actual(0x40e80u,call);
    return {source_live_effect_light_state_tu15(world.memory()),
        source_live_effect_light_private_commands_tu15(world.memory()).legacy};
}
}
