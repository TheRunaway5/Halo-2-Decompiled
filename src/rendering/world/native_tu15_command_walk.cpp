#include "halo2/native_tu15_command_walk.hpp"
#include <bit>

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory& m,std::uint32_t at,unsigned size=4) {
    std::array<std::byte,4> b{};m.read(at,std::span(b).first(size));std::uint32_t v{};
    for(unsigned i=0;i<size;++i)v|=std::to_integer<unsigned>(b[i])<<(8*i);
    return v;
}
void write(SourcePhysicsContactMemory& m,std::uint32_t at,std::uint32_t v,unsigned size=4) {
    std::array<std::byte,4> b{};for(unsigned i=0;i<size;++i)b[i]=std::byte(v>>(8*i));
    m.write(at,std::span(b).first(size));
}
std::int32_t signed_half(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
std::uint32_t unit_address(std::uint32_t unit){return 0x4c6958u+32u*std::uint32_t(signed_half(unit));}
std::int32_t signed_word(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
}
bool native_tu15_command_part_visible_460c0(const SourcePhysicsContactMemory& m,
    std::uint32_t unit,std::uint32_t part,std::uint32_t mask) {
    const auto at=unit_address(unit),flags=read(m,at);
    if(!(flags&0x40u))return true;
    const auto model=read(m,at+4u),packed=read(m,at+0xcu);
    std::uint32_t geometry;
    if(model!=0xffffffffu) {
        const auto table=read(m,0x4f0414u),definition=read(m,table+16u*(model&65535u)+8u);
        const auto sections=read(m,definition+0x28u);
        geometry=read(m,sections+92u*((packed>>9)&511u)+0x34u);
    } else {
        const auto bsp=read(m,0x4ecc18u);
        if(flags&0x1000u) {
            const auto instances=read(m,bsp+0x144u);
            const auto definition=std::uint32_t(signed_half(read(m,instances+88u*((packed>>18)&2047u)+0x34u,2)));
            const auto definitions=read(m,bsp+0x13cu);
            geometry=read(m,definitions+200u*definition+0x50u);
        } else {
            const auto clusters=read(m,bsp+0xa0u);
            geometry=read(m,clusters+176u*((packed>>9)&511u)+0x50u);
        }
    }
    const auto parts=read(m,geometry+4u),p=parts+72u*part;
    const auto first=signed_half(read(m,p+0xau,2)),count=signed_half(read(m,p+0xcu,2));
    for(auto bit=first;bit<first+count;++bit)
        if(read(m,mask+4u*std::uint32_t(bit>>5))&(1u<<(std::uint32_t(bit)&31u)))return true;
    return false;
}
void native_tu15_command_parts_44ab0(SourcePhysicsContactMemory& m,std::uint32_t mode,
    std::uint32_t flag_mask,std::uint32_t group,NativeTu15PhysicsFrameFp& fp,const NativeTu15CommandPartServices& s) {
    const auto prefix=read(m,0x4cd39cu),draw_count=read(m,0x4cd398u); // CMOV reads both
    const auto end=std::uint16_t(mode?draw_count:prefix);
    for(std::uint32_t unit=mode?std::uint16_t(prefix):0u;unit<end;++unit) {
        const auto at=unit_address(unit),flags=read(m,at);
        if(flags&0x400u) {
            if(!mode) {
                const auto count=read(m,0x515f1cu);
                if(signed_word(count)<32) {
                    write(m,0x4d4f60u+2u*count,unit,2);write(m,0x515f1cu,count+1u);
                }
            } else {
                const auto offset=0x408u*read(m,0x515f20u),count_at=0x4d2f1cu+offset;
                const auto count=signed_half(read(m,count_at,2));
                if(count<32) {
                    const auto dest=0x4d2b18u+offset+32u*std::uint32_t(count);
                    // REP MOVSD reads and writes each word in order, including
                    // overlap. Its final INC rereads the potentially aliased row.
                    for(unsigned i=0;i<8;++i)write(m,dest+4u*i,read(m,at+4u*i));
                    write(m,count_at,read(m,count_at,2)+1u,2);
                }
            }
            continue;
        }
        if(!(flags&flag_mask&0x1fffffu))continue;
        const auto packed=read(m,at+0xcu);
        if(((packed>>4)&31u)!=group || !(flags&1u) || !read(m,at+0x1au,1))continue;
        const auto coverage=read(m,0x4cd3a4u+unit,1),matrix=packed&15u;
        const auto slot=read(m,0x4cd6f6u+std::uint32_t(signed_half(unit)),1);
        const auto override_shader=slot==255u?0xffffffffu:read(m,0x4cda48u+4u*slot);
        const auto geometry=s.geometry_449d0(unit,(flags>>12)&0xffffff01u,fp);
        if(!geometry || signed_word(read(m,geometry))<=0)continue;
        for(std::uint32_t part=0;;) {
            if(s.prepare_45cd0(unit,part,matrix,fp) &&
                native_tu15_command_part_visible_460c0(m,unit,part,read(m,at+0x14u))) {
                // Source reads the packet even when the pinned override wins.
                auto shader=read(m,s.packet);
                if(override_shader!=0xffffffffu)shader=override_shader;
                if(shader!=0xffffffffu)s.emit_41030(mode,coverage,shader,group!=31u,unit,part,fp);
            }
            const auto count=read(m,geometry);++part;
            if(signed_word(part)>=signed_word(count))break;
        }
    }
}
void native_tu15_command_frame_40e80(SourcePhysicsContactMemory& m,std::uint32_t selector,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15CommandFrameServices& s) {
    const auto mask=!selector?0xffffffffu:selector==2u?0x10u:0x20u;
    const auto context=read(m,0x4cd3a0u),mode=read(m,context+0x2a78u);
    if(!mode) {
        for(const auto at:{0x4ce26cu,0x4ce274u,0x4ce270u,0x4c2aa0u,0x4c2aa4u})write(m,at,0);
    } else {
        for(const auto at:{0x4ce284u,0x4ce28cu,0x4ce288u,0x4ce29cu,0x4ce2a4u,0x4ce2a0u})write(m,at,0);
    }
    s.prepare_41c70(context,mode,fp);
    native_tu15_command_parts_44ab0(m,mode,mask,31,fp,s.parts);
    if(mode)return;
    if(read(m,read(m,0x4f3218u)+8u)==3)s.producer_179b40(fp);
    else {s.producer_179a40(fp);s.producer_1770c0(fp);}
    s.producer_42090(fp);s.producer_42750(mask,fp);s.producer_41630(fp);
}
}
