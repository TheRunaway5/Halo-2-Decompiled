#include "halo2/native_tu15_light_parent_biped.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory& m,std::uint32_t at,unsigned size=4) {
    std::array<std::byte,4> bytes{};m.read(at,std::span(bytes).first(size));std::uint32_t value{};
    for(unsigned i=0;i<size;++i)value|=std::to_integer<unsigned>(bytes[i])<<(8*i);return value;
}
void put(SourcePhysicsContactMemory& m,std::uint32_t at,std::uint32_t value,unsigned size=4) {
    std::array<std::byte,4> bytes{};for(unsigned i=0;i<size;++i)bytes[i]=std::byte(value>>(8*i));
    m.write(at,std::span(bytes).first(size));
}
}
NativeTu15ObjectQueueResult native_tu15_object_queue_append_166b60(SourcePhysicsContactMemory& m,
    std::uint32_t queue,std::uint32_t object,std::uint32_t flags,std::uint32_t value,
    std::uint32_t association,std::uint32_t entry_eax) {
    auto count=read(m,queue+4u,2);const auto capacity=read(m,queue);
    if(std::int32_t(count)>=std::bit_cast<std::int32_t>(capacity-1u))return {false,entry_eax&0xffffff00u,queue,count};
    auto stream=read(m,queue+0xcu);put(m,stream+4u*count,object);
    count=read(m,queue+4u,2);stream=read(m,queue+8u);put(m,stream+2u*count,flags,2);
    count=read(m,queue+4u,2);stream=read(m,queue+0x10u);put(m,stream+4u*count,value);
    count=read(m,queue+4u,2);stream=read(m,queue+0x14u);put(m,stream+2u*count,association,2);
    put(m,queue+4u,read(m,queue+4u,2)+1u,2);
    return {true,(stream&0xffffff00u)|1u,queue,count};
}
NativeTu15LightParent native_tu15_light_parent_biped_walk(const SourcePhysicsContactMemory& m,std::uint32_t owner) {
    NativeTu15LightParent out;out.object=owner;if(owner==0xffffffffu)return out;
    const auto pool=read(m,0x4ecbd0u),headers=read(m,pool+0x44u);
    for(;;) {
        if(out.steps++==65536u)throw std::runtime_error("Native31C10 has a cyclic object-parent chain");
        const auto body=read(m,headers+12u*(out.object&65535u)+8u);out.body_residue=body;
        if(!read(m,body+0xaau,1))return out;
        const auto parent=read(m,body+0x14u);out.body_residue=parent;
        if(parent==0xffffffffu)return out;
        out.object=parent;
    }
}
NativeTu15LightParentResult native_tu15_light_parent_biped_31ec2(SourcePhysicsContactMemory& m,
    std::uint32_t definition_data,std::uint32_t shadow_address,std::uint32_t owner,
    std::uint32_t context,const NativeTu15LightParentServices& services) {
    NativeTu15LightParentResult out;if(!(read(m,definition_data)&0x400000u))return out;
    out.taken=true;out.flags=read(m,shadow_address,1)?0x3000u:0x1000u;out.object=owner;
    if(owner==0xffffffffu){out.owner_none=true;return out;}
    const auto parent=native_tu15_light_parent_biped_walk(m,owner);out.object=parent.object;out.steps=parent.steps;
    const auto queue=read(m,context+0x14u);
    out.queue=services.append?services.append(queue,out.object,out.flags,0,0xffffffffu,parent.body_residue):
        native_tu15_object_queue_append_166b60(m,queue,out.object,out.flags,0,0xffffffffu,parent.body_residue);
    return out;
}
}
