#include "halo2/native_tu15_render_script_flags.hpp"

namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory& memory,std::uint32_t address) {
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
    return value;
}
void publish(SourcePhysicsContactMemory& memory,const NativeTu15RenderFlagPublisher& writer,
    std::uint32_t address,std::uint32_t value,unsigned size) {
    std::array<std::byte,4> bytes{};
    for(unsigned n=0;n<size;++n)bytes[n]=std::byte(value>>(8u*n));
    const auto output=std::span(bytes).first(size);
    if(writer)writer(address,output);else memory.write(address,output);
}
}
void native_tu15_warthog_chaingun_light_enable(SourcePhysicsContactMemory& memory,
    std::uint8_t enable,const NativeTu15RenderFlagPublisher& writer) {
    const auto bank=word(memory,0x4eabc4u);
    publish(memory,writer,bank+0x82u,enable==0,1);
}
void native_tu15_object_cinematic_visibility(SourcePhysicsContactMemory& memory,
    std::uint32_t object,std::uint8_t visible,const NativeTu15RenderFlagPublisher& writer) {
    if(object==0xffffffffu)return;
    const auto pool=word(memory,0x4ecbd0u);
    const auto records=word(memory,pool+0x44u);
    const auto body=word(memory,records+(object&65535u)*12u+8u);
    const auto flags=word(memory,body+4u);
    publish(memory,writer,body+4u,visible?flags|0x00200000u:flags&0xffdfffffu,4);
}
}
