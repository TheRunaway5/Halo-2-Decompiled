#include "halo2/pstencil/native_light_shape_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <cstring>
namespace halo2::pstencil {
std::uint16_t NativeLightShape::type()const {
    if(!written[0] || !written[1])throw FormatError("Native light shape has no completed type");
    return std::uint16_t(words[0]);
}
NativeLightShape native_light_shape_tu15(PhysicsLiveWorldTu15& world,std::uint32_t light) {
    const auto& memory=world.memory();
    const auto word=[&](std::uint32_t address){return Bytes(memory.view(address,4)).u32(0);};
    const auto pool=word(0x4ecbdcu);
    if(word(pool+0x20u)!=350 || word(pool+0x24u)!=272)
        throw FormatError("Native light shape lost its original pool contract");
    const SourceRawDatumPoolServices storage{.read=[&](auto address,auto size){return memory.view(address,size);}};
    const auto record=source_raw_datum_lookup(pool,light,storage);
    if(!record || Bytes(memory.view(record,2)).u16(0)!=std::uint16_t(light>>16))
        throw FormatError("Native light shape requires its full occupied generation");
    const auto definition=word(record+4u),tag=word(0x4f0414u)+(definition&65535u)*16u;
    if(word(tag+4u)!=definition)throw FormatError("Native light shape lost its full tag generation");
    NativeLightShape out;auto parameters=world.prepare_source_call();
    const auto output=parameters.gp[4]-256u;
    const auto observe=[&](const SourcePhysicsContactRecordStore& store) {
        for(unsigned n=0;n<store.written.size();++n)
            if(store.address+n>=output && store.address+n<output+out.written.size())
                out.written[store.address+n-output]=1;
    };
    // Literal C3820: EAX full light, EDI caller shape; C3A20 owns tag/runtime
    // animation parameter selection. No output or source light field is set.
    parameters.gp[0]=light;parameters.gp[7]=output;
    world.run_actual(0xc3a20u,parameters,observe);
    const std::array args{word(tag+8u)};auto transform=world.prepare_source_call(args);
    transform.gp[1]=record+0x84u;transform.gp[2]=output;
    world.run_actual(0xc3ab0u,transform,observe);
    std::memcpy(out.words.data(),memory.view(output,sizeof(out.words)).data(),sizeof(out.words));
    (void)out.type();return out;
}
std::vector<SourceFrustumCamera> native_light_cameras_tu15(PhysicsLiveWorldTu15& world,
    std::uint32_t light) {
    const auto shape=native_light_shape_tu15(world,light);
    const auto require_word=[&](unsigned index) {
        for(unsigned n=0;n<4;++n)if(!shape.written[index*4u+n])
            throw FormatError("Native light camera consumed an unwritten shape field");
    };
    if(shape.type()==0) {
        for(const auto index:{0u,11u,12u,13u,17u})require_word(index);
    } else {
        for(const auto index:{0u,7u,8u,9u,10u,11u,12u,13u,15u,16u,17u,22u,23u,25u})require_word(index);
    }
    const auto& memory=world.memory();
    const auto word=[&](std::uint32_t address){return Bytes(memory.view(address,4)).u32(0);};
    const auto records=word(word(0x4ecbdcu)+0x44u);
    auto call=world.prepare_source_call();const auto caller=call.gp[4];
    const auto output=caller-0x2000u,count=caller-0x700u,input=caller-0x100u;
    // Retain the actual produced bytes as caller input. All output comes
    // from C3DC0's own reset/population, including its literal camera count.
    world.write_source_call_input(input,std::as_bytes(std::span(shape.words)));
    call.gp[4]=caller-0x3000u;
    const std::array args{word(caller),records+(light&65535u)*272u+0x84u,input,output,count};
    world.write_source_call_input(call.gp[4],std::as_bytes(std::span(args)));
    world.run_actual(0xc3dc0u,call);
    const auto size=Bytes(memory.view(count,2)).u16(0);
    if(size!=(shape.type()==0?6u:1u))throw FormatError("Native light camera changed its original shape/count contract");
    static_assert(sizeof(SourceFrustumCamera)==0x1bc);
    std::vector<SourceFrustumCamera> cameras(size);
    std::memcpy(cameras.data(),memory.view(output,cameras.size()*sizeof(SourceFrustumCamera)).data(),
        cameras.size()*sizeof(SourceFrustumCamera));
    return cameras;
}
}
