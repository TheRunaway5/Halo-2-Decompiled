#include "halo2/source_object_draw_collector.hpp"
#include "halo2/source_draw_resources.hpp"
#include <bit>
#include <cstring>

namespace halo2 {
namespace {
std::uint32_t word(const SourceObjectDrawCollector& c,unsigned at) {
    if(at+4>c.bytes.size())throw FormatError("source object collector word outside retained record");
    std::uint32_t value;std::memcpy(&value,c.bytes.data()+at,4);return value;
}
std::uint16_t half(const SourceObjectDrawCollector& c,unsigned at) {
    if(at+2>c.bytes.size())throw FormatError("source object collector half outside retained record");
    std::uint16_t value;std::memcpy(&value,c.bytes.data()+at,2);return value;
}
std::uint8_t byte(const SourceObjectDrawCollector& c,unsigned at) {
    if(at>=c.bytes.size())throw FormatError("source object collector byte outside retained record");
    return std::to_integer<std::uint8_t>(c.bytes[at]);
}
void put(SourceObjectDrawCollector& c,unsigned at,std::uint32_t value) {
    std::memcpy(c.bytes.data()+at,&value,4);
}
std::uint32_t context_owner(const SourceObjectDrawServices& services,std::uint32_t token) {
    if(!services.draw.context)throw FormatError("source object collector consumes absent context bank");
    const auto bytes=services.draw.context(token);
    if(bytes.size()<4)throw FormatError("source object collector context owner truncated");
    std::uint32_t owner;std::memcpy(&owner,bytes.data(),4);return owner;
}
template<class T>T resource_field(const SourceDrawResourceWorld& world,std::uint32_t at) {
    if(!world.read)throw FormatError("source object collector resource reader absent");
    const auto bytes=world.read(at,sizeof(T));
    if(bytes.size()!=sizeof(T))throw FormatError("source object collector resource field truncated");
    T result;std::memcpy(&result,bytes.data(),sizeof(T));return result;
}
}
SourceObjectDrawServices source_object_draw_resource_services(const SourceDrawResourceWorld& world) {
    SourceObjectDrawServices services;services.draw=source_draw_resource_services(world);
    services.owner=[&world](std::uint32_t owner) {
        const auto headers=resource_field<std::uint32_t>(world,world.objects+0x44);
        const auto body=resource_field<std::uint32_t>(world,headers+(owner&65535u)*12+8);
        return SourceObjectDrawOwner{resource_field<std::uint8_t>(world,body+0xaa),
            resource_field<std::uint32_t>(world,body)};
    };
    services.weapon_section_policy=[&world](std::uint32_t definition) {
        const auto metadata=resource_field<std::uint32_t>(world,world.tag_table+(definition&65535u)*16+8);
        return resource_field<std::uint16_t>(world,metadata+0x290)!=0;
    };
    return services;
}
void source_object_collect_draw_units(SourceDrawUnitState& state,
    const SourceObjectDrawCollector& input,std::uint32_t filter_owner,
    const SourceObjectDrawContext& context,const SourceObjectDrawServices& services) {
    const auto count=std::bit_cast<std::int16_t>(half(input,0x166));
    if(count<=0)return;
    if(count>5)throw FormatError("source object collector model count escapes its five-row bank");
    for(unsigned model=0;model<unsigned(count);++model) {
        const auto token=word(input,0x48+model*4);
        if(model && (filter_owner&255u) && context_owner(services,token)!=
            context_owner(services,word(input,0x48)))continue;
        SourceObjectDrawCollector copied;
        const SourceObjectDrawCollector* current=&input;
        if(word(input,0x1c4)==1) {
            if(!services.owner)throw FormatError("source object collector consumes absent object body");
            const auto owner=services.owner(context_owner(services,token));
            if(owner.kind==2) {
                if(!services.weapon_section_policy)throw FormatError("source object collector consumes absent weapon definition");
                if(services.weapon_section_policy(owner.definition)) {
                    if(!context.temporary_address)throw FormatError("source object collector consumes absent temporary identity");
                    copied=input;copied.address=context.temporary_address;
                    copied.bytes[0x1cc]=std::byte{0};put(copied,0x1c0,31);
                    put(copied,0x1c4,0xffffffffu);put(copied,0x1d0,0xffffffffu);
                    copied.bytes[0x1d4]=std::byte{0};put(copied,0x1d8,0);
                    current=&copied;
                }
            }
        }
        const auto& c=*current;
        const auto source_flags=word(c,0x170);
        std::uint32_t flags=((source_flags&4u)<<13)|((source_flags&0x40u)<<7);
        if(byte(c,0x70+model))flags|=0x800;
        if(byte(c,0x16c))flags|=0x400;
        flags|=word(c,0x1ac)|word(c,0x1b0);
        const auto sections=std::bit_cast<std::int32_t>(word(c,0x18+model*4));
        if(sections<=0)continue;
        if(sections>16)throw FormatError("source object collector section count escapes its sixteen-slot row");
        const Vec3 center{std::bit_cast<float>(word(c,0x194)),
            std::bit_cast<float>(word(c,0x198)),std::bit_cast<float>(word(c,0x19c))};
        auto draw_services=services.draw;
        draw_services.center=[&](std::uint32_t at) {
            if(at!=c.address+0x194)throw FormatError("source object collector section center identity differs");
            return center;
        };
        draw_services.rectangle=[&](std::uint32_t at,float radius) {
            if(at!=c.address+0x194)throw FormatError("source object collector rectangle center identity differs");
            return source_sphere_rectangle(context.rendering,context.eye,context.viewport,center,radius);
        };
        for(unsigned section=0;section<unsigned(sections);++section) {
            SourceDrawUnitInput request;
            const auto selected=byte(c,0x75+model*16+section);
            request.geometry=selected==255?0xffffffffu:selected;
            request.instance=half(c,0x16a);request.distance_byte=byte(c,0x2c+model);
            request.stack={flags,word(c,0x5c+model*4),word(c,4+model*4),section,
                word(c,0x48+model*4),half(c,0x1be),byte(c,0x1bc),0,
                word(c,0x1d0),word(c,0x1b4),word(c,0x1c0),word(c,0x1c4),
                (source_flags&2u)?((source_flags>>3)&1u):0,
                (source_flags&2u)?((source_flags>>4)&1u):0,
                byte(c,0x1d4),word(c,0x1d8),word(c,0x1dc),word(c,0x1e0),
                half(c,0xc6+model*32+section*2),c.address+0x194,word(c,0x1a0)};
            source_draw_unit_append(state,request,context.draw,draw_services);
        }
    }
}
}
