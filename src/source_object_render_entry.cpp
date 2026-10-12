#include "halo2/source_object_render_entry.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_object_render_queue.hpp"
#include "halo2/source_model_build.hpp"
#include <bit>
#include <cstring>

namespace halo2 {
namespace {
template<class T>T read(const SourceObjectDrawCollector& c,unsigned at) {
    T value;std::memcpy(&value,c.bytes.data()+at,sizeof(T));return value;
}
template<class T>void put(SourceObjectDrawCollector& c,unsigned at,T value) {
    std::memcpy(c.bytes.data()+at,&value,sizeof(T));
}
}
void source_object_build_collector(SourceObjectDrawCollector& c,
    const SourceObjectBuildRequest& q,const SourceModelBuildWorld& world) {
    const auto& memory=world.admission.memory;
    if(!c.address || !memory.write || !memory.banks.read)
        throw FormatError("source object build collector current memory owner absent");
    const auto extent=c.bytes.size()-4;
    auto output=memory.write(c.address+4,extent);
    if(output.size()!=extent)throw FormatError("source object build collector output truncated");
    std::memcpy(output.data(),c.bytes.data()+4,extent);
    source_model_build(q.owner,std::bit_cast<float>(q.scalar_1b8),
        std::bit_cast<float>(q.scalar_1b4),c.address+4,q.scalar_1a8,q.flags,q.mode,world);
    const auto produced=memory.banks.read(c.address+4,extent);
    if(produced.size()!=extent)throw FormatError("source object build collector result truncated");
    std::memcpy(c.bytes.data()+4,produced.data(),extent);
}
std::uint32_t source_render_collector_flags(std::uint16_t queue,
    const SourceRenderCollectorFlagsContext& c) {
    std::uint32_t flags=(((queue>>2)&0x2c00u)|(queue&0x4000u))>>9;
    flags|=(queue&0x200u)<<5;
    if(c.query_flags&0x80u)flags|=1u;
    if(c.fog_classification==3) {
        if(c.planar_datum!=0xffffffffu && c.eye_fraction==0.f && !(queue&0x400u)) {
            if(!c.planar_flags)throw FormatError("source collector planar fog definition absent");
            if(!(*c.planar_flags&2u))flags|=0x100u;
        }
        if(!c.planar_only || (queue&0x400u))flags|=0x80u;
    }
    return flags;
}
std::uint8_t source_object_render_entry(SourceBspAdmissionRecord& row,
    SourceObjectDrawCollector& c,const SourceObjectRenderEntryRequest& q,
    const SourceObjectRenderEntryContext& context,const SourceObjectRenderEntryServices& s) {
    if(!s.sphere || !s.fade || !s.build)
        throw FormatError("source object render entry sphere/fade/build owner absent");
    const bool alternate=(q.alternate&255u)!=0;
    const auto build_flags=std::uint8_t(context.mode==2 || alternate);
    const auto sphere=s.sphere(row.id);
    put(c,0,row.id);put(c,0x194,sphere.center);put(c,0x1a0,sphere.radius);
    const auto coverage=source_shader_object_coverage(sphere.center,sphere.radius,
        context.coverage_multiplier,context.coverage);
    put(c,0x1b4,coverage);
    const auto metric=source_geometry_lod_distance(sphere.center,context.eye,context.distance_scale);
    put(c,0x1b8,metric);
    const auto fade=s.fade(row.id,metric);
    put(c,0x1bc,fade.first);
    put(c,0x1ac,source_render_collector_flags(row.flags,context.flags));
    // Only AL of the shifted queue word is stored. Preserve the other three
    // bytes passed by value to4B5D0, whose sky-rebase argument tests lowbyte.
    put(c,0x1a8,std::uint8_t((row.flags>>9)&1u));
    put(c,0x1c0,q.section_group);put(c,0x1c8,q.section_counter);
    put(c,0x1cc,std::uint8_t(0));put(c,0x1d0,0xffffffffu);
    put(c,0x1d4,std::uint8_t(0));put(c,0x1d8,0.f);put(c,0x1dc,0.f);put(c,0x1e0,0.f);
    put(c,0x1a4,0u);put(c,0x1be,std::uint16_t(65535));
    if(!(row.flags&0x800u) && row.association!=65535u) {
        put(c,0x1b0,0x40u);put(c,0x1be,row.association);
    } else put(c,0x1b0,1u);
    s.build(c,{row.id,std::bit_cast<std::uint32_t>(metric),
        std::bit_cast<std::uint32_t>(coverage),read<std::uint32_t>(c,0x1a8),context.mode,
        sphere.center,build_flags});
    if(read<std::uint32_t>(c,0)!=row.id)
        throw FormatError("source object render entry build changed caller owner");
    put(c,0x1b0,read<std::uint32_t>(c,0x1b0)|0x208u);
    if(alternate) {
        put(c,0x1ac,(read<std::uint32_t>(c,0x1ac)&~2u)|4u);
        put(c,0x16c,std::uint8_t(0));
    } else if(read<std::uint8_t>(c,0x16c) && context.mode==1)
        put(c,0x1ac,read<std::uint32_t>(c,0x1ac)&~4u);
    if(read<std::uint8_t>(c,0x1bc)) {
        if(!(read<std::uint8_t>(c,0x170)&2u) || context.mode==0) {
            if(!s.present)throw FormatError("source object render entry presentation owner absent");
            s.present(c,q.children,0,build_flags);
        }
    } else row.flags=std::uint16_t(row.flags|1u);
    return read<std::uint8_t>(c,0x16c);
}
}
