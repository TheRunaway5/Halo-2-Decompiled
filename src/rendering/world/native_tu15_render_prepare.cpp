#include "halo2/native_tu15_render_prepare.hpp"
#include <bit>
#include <stdexcept>
#include <vector>

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory& m,std::uint32_t at,unsigned size=4) {
    std::array<std::byte,4> b{};m.read(at,std::span(b).first(size));std::uint32_t v{};
    for(unsigned i=0;i<size;++i)v|=std::to_integer<unsigned>(b[i])<<(8*i);return v;
}
std::int32_t signed_half(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
}
void native_tu15_render_object_prepare_41c10(const SourcePhysicsContactMemory& m,
    std::uint32_t mode,std::uint32_t object,std::uint32_t flags,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15RenderPrepareServices& s) {
    std::vector<std::uint32_t> parents;
    for(;;) {
        const auto pool=read(m,0x4ecbd0u),rows=read(m,pool+0x44u);
        auto body=read(m,rows+12u*(object&65535u)+8u);
        s.update_d7230(object,mode,flags,fp);s.attached_41970(mode,object,fp);
        const auto child=read(m,body+0x10u);
        if(child!=0xffffffffu){parents.push_back(body);object=child;continue;}
        for(;;) {
            const auto sibling=read(m,body+0xcu);
            if(sibling!=0xffffffffu){object=sibling;break;}
            if(parents.empty())return;
            body=parents.back();parents.pop_back();
        }
    }
}
void native_tu15_render_prepare_41c70(const SourcePhysicsContactMemory& m,
    std::uint32_t context,std::uint32_t mode,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15RenderPrepareServices& s) {
    auto queue=read(m,context+0x14u);
    if(signed_half(read(m,queue+4u,2))<=0)return;
    for(std::uint32_t index=0;;) {
        const auto flags_array=read(m,queue+8u),objects=read(m,queue+0xcu);
        const auto row=std::uint32_t(signed_half(index));
        const auto flags=std::uint32_t(signed_half(read(m,flags_array+2u*row,2)));
        const auto object=read(m,objects+4u*row);
        native_tu15_render_object_prepare_41c10(m,mode,object,flags,fp,s);
        queue=read(m,context+0x14u);
        const auto count=signed_half(read(m,queue+4u,2));++index;
        if(std::int32_t(index)>=count)return;
    }
}
std::uint32_t native_tu15_render_geometry_449d0(const SourcePhysicsContactMemory& m,
    const NativeTu15RenderUnitReader& unit,std::uint8_t request,std::uint8_t owner,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15RenderGeometryRequest& resource) {
    const auto model=unit(4u);
    std::uint32_t section,geometry_offset,header_offset;
    if(model!=0xffffffffu) {
        const auto table=read(m,0x4f0414u),definition=read(m,table+16u*(model&65535u)+8u);
        const auto packed=unit(0xcu),sections=read(m,definition+0x28u);
        section=sections+92u*((packed>>9)&511u);geometry_offset=0x34u;header_offset=0x38u;
    } else {
        const auto bsp=read(m,0x4ecc18u);
        if(owner) {
            const auto packed=unit(0xcu),instances=read(m,bsp+0x144u),definitions=read(m,bsp+0x13cu);
            const auto definition=std::uint32_t(signed_half(read(m,instances+88u*((packed>>18)&2047u)+0x34u,2)));
            section=definitions+200u*definition;
        } else {
            const auto packed=unit(0xcu),clusters=read(m,bsp+0xa0u);
            section=clusters+176u*((packed>>9)&511u);
        }
        geometry_offset=0x50u;header_offset=0x28u;
    }
    if(request) {
        if(!resource)throw std::runtime_error("Native449D0 requires its genuine130980 resource owner");
        if(!resource(section+header_offset,fp))return 0;
    }
    return read(m,section+geometry_offset);
}
std::uint32_t native_tu15_render_geometry_449d0(const SourcePhysicsContactMemory& m,
    std::uint32_t unit,std::uint8_t request,std::uint8_t owner,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15RenderGeometryRequest& resource) {
    const auto at=0x4c6958u+32u*std::uint32_t(signed_half(unit));
    return native_tu15_render_geometry_449d0(m,[&](auto offset){return read(m,at+offset);},request,owner,fp,resource);
}
std::uint32_t native_tu15_render_section_44930(const SourcePhysicsContactMemory& m,
    const NativeTu15RenderUnitReader& unit,std::uint8_t selector) {
    const auto model=unit(4u);
    if(model!=0xffffffffu) {
        const auto tags=read(m,0x4f0414u),definition=read(m,tags+16u*(model&65535u)+8u);
        const auto packed=unit(0xcu),sections=read(m,definition+0x28u);
        return sections+92u*((packed>>9)&511u)+4u;
    }
    const auto bsp=read(m,0x4ecc18u),packed=unit(0xcu);
    if(!selector)return read(m,bsp+0xa0u)+176u*((packed>>9)&511u);
    const auto instances=read(m,bsp+0x144u);
    const auto definition=std::uint32_t(signed_half(read(m,instances+88u*((packed>>18)&2047u)+0x34u,2)));
    const auto definitions=read(m,bsp+0x13cu);
    return definitions+200u*definition;
}
std::uint32_t native_tu15_render_section_44930(const SourcePhysicsContactMemory& m,
    std::uint32_t unit,std::uint8_t selector) {
    const auto at=0x4c6958u+32u*std::uint32_t(signed_half(unit));
    return native_tu15_render_section_44930(m,[&](auto offset){return read(m,at+offset);},selector);
}
}
