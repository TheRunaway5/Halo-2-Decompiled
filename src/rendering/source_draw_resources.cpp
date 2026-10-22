#include "halo2/source_draw_resources.hpp"
#include <bit>
#include <cstring>

namespace halo2 {
namespace {
constexpr auto none=0xffffffffu;
template<class T>T read(const SourceDrawResourceWorld& w,std::uint32_t at) {
    if(!w.read)throw FormatError("source draw resource reader absent");
    const auto bytes=w.read(at,sizeof(T));
    if(bytes.size()!=sizeof(T))throw FormatError("source draw resource read truncated");
    T result;std::memcpy(&result,bytes.data(),sizeof(T));return result;
}
std::uint32_t tag(const SourceDrawResourceWorld& w,std::uint32_t datum) {
    return read<std::uint32_t>(w,w.tag_table+(datum&65535u)*16+8);
}
bool request(const SourceDrawResourceWorld& w,std::uint32_t at) {
    if(!w.request)throw FormatError("source draw resource publication service absent");
    return w.request(3,at);
}
std::uint32_t object(const SourceDrawResourceWorld& w,std::uint32_t owner) {
    if(owner==none)return 0;
    const auto index=owner&65535u;
    if(std::int32_t(index)>=read<std::int32_t>(w,w.objects+0x38))return 0;
    const auto header=read<std::uint32_t>(w,w.objects+0x44)+
        index*read<std::uint32_t>(w,w.objects+0x24);
    const auto salt=read<std::uint16_t>(w,header);
    if(!salt || salt!=(owner>>16))return 0;
    // BADC0's actual mask isFFFFFFFF. Its x86 kind shift always selects a
    // bit in that mask, including the source low5 shift-count semantics.
    (void)read<std::uint8_t>(w,header+3);
    return read<std::uint32_t>(w,header+8);
}
std::uint32_t geometry(const SourceCullDrawUnit& unit,std::uint32_t selector,
    const SourceDrawResourceWorld& w) {
    const auto& u=unit.words;
    if(u[1]!=none)return read<std::uint32_t>(w,tag(w,u[1])+0x28)+
        ((u[3]>>9)&511u)*0x5c+4;
    if(!(selector&255u))return read<std::uint32_t>(w,w.bsp+0xa0)+
        ((u[3]>>9)&511u)*0xb0;
    const auto instance=read<std::uint32_t>(w,w.bsp+0x144)+((u[3]>>18)&2047u)*0x58;
    const auto definition=read<std::int16_t>(w,instance+0x34);
    return read<std::uint32_t>(w,w.bsp+0x13c)+std::uint32_t(std::int32_t(definition))*0xc8;
}
}
std::uint32_t source_draw_geometry_address(const SourceCullDrawUnit& unit,
    std::uint32_t selector,const SourceDrawResourceWorld& w) {
    return geometry(unit,selector,w);
}
std::uint32_t source_draw_resident_geometry(const SourceCullDrawUnit& unit,
    std::uint32_t selector,bool acquire,const SourceDrawResourceWorld& w) {
    const auto metadata=geometry(unit,selector,w);
    const bool dynamic=unit.words[1]!=none;
    const auto resource=metadata+(dynamic?0x34u:0x28u);
    if(acquire && !request(w,resource))return 0;
    return read<std::uint32_t>(w,metadata+(dynamic?0x30u:0x50u));
}
std::uint32_t source_draw_lighting_classification(std::uint32_t owner,
    std::uint32_t model,std::uint32_t instance,const SourceDrawResourceWorld& w) {
    if(w.lightmaps && read<std::int32_t>(w,w.lightmaps+0x80)>0 && w.bsp) {
        const auto group=read<std::uint32_t>(w,w.lightmaps+0x84);
        if(read<std::uint32_t>(w,group+0x1c)!=none &&
            read<std::uint32_t>(w,group+4)==read<std::uint32_t>(w,w.bsp+8)) {
            if(instance==0x7ff)return model==none?0:2;
            std::uint32_t reference;
            if(model!=none) {
                const auto checksums=read<std::uint32_t>(w,group+0x5c);
                if(read<std::uint32_t>(w,tag(w,model)+8)!=
                    read<std::uint32_t>(w,checksums+instance*12+8))return 3;
                reference=read<std::uint32_t>(w,group+0x64)+instance*12;
            } else reference=read<std::uint32_t>(w,group+0x54)+instance*12;
            const auto bucket=read<std::uint16_t>(w,reference+2);
            const auto resource=read<std::uint32_t>(w,group+0x44)+bucket*0x38+0xc;
            if(!request(w,resource))return 3;
            if(model!=none)return 1;
            const auto actual=read<std::uint32_t>(w,w.bsp+0x144)+instance*0x58;
            return read<std::uint16_t>(w,actual+0x56)!=0;
        }
    }
    if(model!=none && owner!=none) {
        const auto body=object(w,owner);
        if(body && (read<std::uint8_t>(w,body+7)&1u))return 2;
    }
    return 3;
}
SourceDrawUnitServices source_draw_resource_services(const SourceDrawResourceWorld& world) {
    SourceDrawUnitServices services;
    services.geometry=[&world](std::uint16_t,std::uint32_t selector,const SourceCullDrawUnit& unit) {
        const auto metadata=source_draw_geometry_address(unit,selector,world);
        return SourceDrawUnitGeometry{read<std::uint16_t>(world,metadata+0x14),
            read<std::uint16_t>(world,metadata+0x24)};
    };
    services.resident=[&world](std::uint16_t,std::uint32_t selector,bool acquire,const SourceCullDrawUnit& unit) {
        return source_draw_resident_geometry(unit,selector,acquire,world)!=0;
    };
    services.lighting=[&world](std::uint32_t owner,std::uint32_t model,std::uint32_t instance) {
        return source_draw_lighting_classification(owner,model,instance,world);
    };
    return services;
}
}
