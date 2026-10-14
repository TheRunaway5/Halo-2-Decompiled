#include "halo2/source_object_admission_memory.hpp"
#include "halo2/source_object_spatial_pool.hpp"

namespace halo2 {
SourceObjectAdmission source_object_admission_memory(const SourcePortalVisibilitySet& visibility,
    const SourceObjectAdmissionMemory& memory,const SourceObjectAdmissionInput& in,
    std::span<const std::array<std::uint32_t,4>> prior) {
    if(!memory.read || in.containing_fog.size()>512)
        throw FormatError("source object admission requires current memory and fog banks");
    const auto read=[&](std::uint32_t address,std::size_t size){return Bytes(memory.read(address,size));};
    const auto header=[&](std::uint32_t address,unsigned stride) {
        const auto h=read(address,76);
        if(h.u32(0x20)!=2048 || h.u32(0x24)!=stride || !h.u32(0x44) || !h.u32(0x48))
            throw FormatError("source object admission data-array shape differs");
        return h;
    };
    SourceObjectSpatialPool collidable(in.containing_fog.size()),noncollidable(in.containing_fog.size(),false);
    const auto load=[&](SourceObjectSpatialPool& out,std::uint32_t heads,std::uint32_t entries) {
        const auto h=header(entries,32);
        const auto raw=read(h.u32(0x44),2048*32),links=read(heads,out.heads.size()*4);
        for(unsigned n=0;n<out.heads.size();++n)out.heads[n]=links.u32(n*4);
        for(unsigned n=0;n<2048;++n)for(unsigned k=0;k<8;++k)out.entries.records[n][k]=raw.u32(n*32+k*4);
    };
    load(collidable,memory.collidable_heads,memory.collidable_entries);
    load(noncollidable,memory.noncollidable_heads,memory.noncollidable_entries);
    const auto h=header(memory.objects,12),raw=read(h.u32(0x44),2048*12),occupied=read(h.u32(0x48),256);
    std::vector<SourceObjectAdmissionObject> objects;
    for(unsigned n=0;n<2048;++n)if(occupied.u32((n/32)*4)&(1u<<(n%32))) {
        const auto address=raw.u32(n*12+8);
        if(raw.u16(n*12+6)<0xacu || !address)
            throw FormatError("source object admission has incomplete live object body");
        const auto b=read(address,0xacu);
        const auto sphere=[&](unsigned p){return ObjectLightSpatialSphere{
            {b.f32(p),b.f32(p+4),b.f32(p+8)},b.f32(p+12)};};
        objects.push_back({(std::uint32_t(raw.u16(n*12))<<16)|n,b.u32(4),std::int8_t(b.u8(0xaa)),
            sphere(0x30),sphere(0x50)});
    }
    return source_object_admission(visibility,collidable,noncollidable,objects,in,prior);
}
}
