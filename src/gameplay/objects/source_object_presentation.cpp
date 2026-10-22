#include "halo2/source_object_presentation.hpp"
#include "halo2/source_draw_resources.hpp"
#include <bit>
#include <cstring>
#include <set>

namespace halo2 {
namespace {
std::uint32_t word(const SourceObjectDrawCollector& c,unsigned at) {
    std::uint32_t v;std::memcpy(&v,c.bytes.data()+at,4);return v;
}
std::uint8_t byte(const SourceObjectDrawCollector& c,unsigned at) {
    return std::to_integer<std::uint8_t>(c.bytes[at]);
}
void put(SourceObjectDrawCollector& c,unsigned at,std::uint32_t v) {
    std::memcpy(c.bytes.data()+at,&v,4);
}
void put_half(SourceObjectDrawCollector& c,unsigned at,std::uint16_t v) {
    std::memcpy(c.bytes.data()+at,&v,2);
}
Vec3 center(const SourceObjectDrawCollector& c) {
    return {std::bit_cast<float>(word(c,0x194)),std::bit_cast<float>(word(c,0x198)),
        std::bit_cast<float>(word(c,0x19c))};
}
float depth(const SourceObjectDrawCollector& c,const SourceObjectPresentationContext& ctx) {
    // Literal SUBSS/MULSS/ADDSS order, Z+Y then X. Do not contract to FMA.
    const auto p=center(c);
    const float z=(p.z-ctx.eye.z)*ctx.forward.z;
    const float y=(p.y-ctx.eye.y)*ctx.forward.y;
    const float x=(p.x-ctx.eye.x)*ctx.forward.x;
    return (z+y)+x;
}
SourceObjectDrawCollector copy(const SourceObjectDrawCollector& c,
    const SourceObjectPresentationServices& s) {
    if(!s.copy_identity)throw FormatError("source object presentation copy identity absent");
    auto result=c;result.address=s.copy_identity();return result;
}
void section(SourceObjectDrawCollector& c,unsigned kind,const SourceObjectPresentationServices& s) {
    if(byte(c,0x1cc))return;
    if(!s.section_counter)throw FormatError("source object presentation section counter absent");
    auto& next=s.section_counter(word(c,0x1c8));
    c.bytes[0x1cc]=std::byte{1};put(c,0x1c0,next++);put(c,0x1c4,kind);
}
void weapon_reset(SourceObjectDrawCollector& c) {
    c.bytes[0x1cc]=std::byte{0};put(c,0x1c0,31);put(c,0x1c4,0xffffffff);
    put(c,0x1d0,0xffffffff);c.bytes[0x1d4]=std::byte{0};put(c,0x1d8,0);
}
void present(SourceObjectDrawCollector& c,std::uint32_t children,std::uint32_t skip,
    std::uint32_t flags,const SourceObjectPresentationContext& ctx,
    const SourceObjectPresentationServices& s,std::set<std::uint32_t>& ancestry) {
    std::int16_t models;std::memcpy(&models,c.bytes.data()+0x166,2);
    if(models>0) {
        bool suppress=false;
        if(!(skip&255u)) {
            if(byte(c,0x170)&128u) {
                put(c,0x1e0,std::bit_cast<std::uint32_t>(depth(c,ctx)));
                section(c,3,s);
                auto effect=copy(c,s);
                put(effect,0x1ac,word(effect,0x1b0));put(effect,0x1b0,0);
                put(effect,0x1d0,word(effect,0x180));put(effect,0x1dc,word(effect,0x190));
                present(effect,1,1,flags,ctx,s,ancestry);
            }
            if(byte(c,0x170)&2u) {
                const float projected=depth(c,ctx);section(c,1,s);suppress=true;
                auto effect=copy(c,s);
                put(effect,0x1ac,word(c,0x1b0));put(effect,0x1b0,0);
                put(effect,0x1d0,word(effect,0x178));effect.bytes[0x1d4]=std::byte{1};
                put(effect,0x1d8,word(effect,0x188));put(effect,0x1e0,std::bit_cast<std::uint32_t>(projected));
                present(effect,1,1,flags,ctx,s,ancestry);
            }
            for(unsigned bit:{1u,32u})if(byte(c,0x170)&bit) {
                auto effect=copy(c,s);
                put(c,0x170,word(c,0x170)&~bit);
                suppress=(word(c,0x170)&2u)!=0;
                put(effect,0x1ac,word(effect,0x1ac)|0x8000u);
                put(effect,0x1d0,word(effect,bit==1?0x174:0x17c));
                put(effect,0x1dc,word(effect,bit==1?0x184:0x18c));
                present(effect,0,1,flags,ctx,s,ancestry);
            }
        }
        if(!suppress) {
            if(!s.collect)throw FormatError("source object presentation draw collector absent");
            s.collect(c,(children&255u)?0u:1u);
        }
    }
    if(!(children&255u)||byte(c,0x16c))return;
    if(!s.link)throw FormatError("source object presentation child links absent");
    const auto owner=word(c,0);
    if(!ancestry.insert(owner).second)throw FormatError("source object presentation cyclic child ancestry");
    std::set<std::uint32_t> siblings;
    for(auto child=s.link(owner,0x10);child!=0xffffffffu;child=s.link(child,0xc)) {
        if(!siblings.insert(child).second)throw FormatError("source object presentation cyclic sibling links");
        auto object=copy(c,s);put(object,0,child);
        if(word(object,0x1c4)==1) {
            if(!s.owner)throw FormatError("source object presentation child body absent");
            const auto body=s.owner(child);
            if(body.kind==2) {
                if(!s.weapon_section_policy)throw FormatError("source object presentation weapon policy absent");
                if(s.weapon_section_policy(body.definition))weapon_reset(object);
            }
        }
        put_half(object,0x1be,0xffff);
        put(object,0x1b0,(word(object,0x1b0)&~0x40u)|1u);
        if(!s.build)throw FormatError("source object presentation4B5D0 producer absent");
        s.build(object,{child,word(object,0x1b8),word(object,0x1b4),word(object,0x1a8),
            ctx.mode,center(object),std::uint8_t(flags)});
        if(word(object,0)!=child)throw FormatError("source object presentation producer changed caller owner");
        present(object,1,skip,flags,ctx,s,ancestry);
    }
    ancestry.erase(owner);
}
}
SourceObjectPresentationServices source_object_presentation_resource_services(const SourceDrawResourceWorld& world) {
    SourceObjectPresentationServices s;const auto fields=source_object_draw_resource_services(world);
    s.owner=fields.owner;s.weapon_section_policy=fields.weapon_section_policy;
    s.link=[&world](std::uint32_t owner,unsigned at) {
        if(at!=0xc && at!=0x10)throw FormatError("source object presentation link offset differs");
        if(!world.read)throw FormatError("source object presentation resource reader absent");
        const auto read=[&](std::uint32_t address) {
            const auto raw=world.read(address,4);
            if(raw.size()!=4)throw FormatError("source object presentation field truncated");
            std::uint32_t v;std::memcpy(&v,raw.data(),4);return v;
        };
        return read(read(read(world.objects+0x44)+(owner&65535u)*12+8)+at);
    };
    return s;
}
void source_object_present(SourceObjectDrawCollector& c,std::uint32_t children,
    std::uint32_t skip,std::uint32_t flags,const SourceObjectPresentationContext& ctx,
    const SourceObjectPresentationServices& s) {
    std::set<std::uint32_t> ancestry;present(c,children,skip,flags,ctx,s,ancestry);
}
}
