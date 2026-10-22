#include "halo2/source_object_admission_memory.hpp"
#include "halo2/source_object_spatial_pool.hpp"

namespace halo2 {
SourceObjectClusterMemberships source_object_cluster_memberships(
    const SourceObjectAdmissionMemory& memory,std::size_t clusters) {
    if(!memory.read || clusters>512)throw FormatError("Source cluster census lacks its actual BSP/banks");
    if(!clusters)return {};
    const auto read=[&](std::uint32_t address,std::size_t size){return Bytes(memory.read(address,size));};
    const auto header=[&](std::uint32_t address,unsigned stride) {
        const auto h=read(address,76);
        if(h.u32(0x20)!=2048 || h.u32(0x24)!=stride || !h.u32(0x44) || !h.u32(0x48))
            throw FormatError("Source cluster census data-array shape differs");
        return h;
    };
    const auto objects=header(memory.objects,12);
    const auto records=read(objects.u32(0x44),2048*12),occupied=read(objects.u32(0x48),256);
    const auto pool=[&](std::uint32_t heads,std::uint32_t entries) {
        const auto h=header(entries,32);
        return std::pair{read(heads,clusters*4),read(h.u32(0x44),2048*32)};
    };
    const std::array pools{pool(memory.collidable_heads,memory.collidable_entries),
        pool(memory.noncollidable_heads,memory.noncollidable_entries)};
    SourceObjectClusterMemberships result;
    for(unsigned cluster=0;cluster<clusters;++cluster)for(const auto& [heads,entries]:pools) {
        auto token=heads.u32(cluster*4);unsigned links{};
        while(token!=0xffffffffu) {
            const auto index=token&65535u;
            if(index>=2048 || ++links>2048)throw FormatError("Source cluster census link exceeds pool");
            const auto owner=entries.u32(index*32+4),slot=owner&65535u;
            if(owner==0xffffffffu || slot>=2048 || !(occupied.u32((slot/32)*4)&(1u<<(slot%32))) ||
               ((std::uint32_t(records.u16(slot*12))<<16)|slot)!=owner)
                throw FormatError("Source cluster census contains a stale object generation");
            auto& membership=result[owner];
            if(membership.empty() || membership.back()!=cluster)membership.push_back(std::int16_t(cluster));
            token=entries.u32(index*32+8);
        }
    }
    return result;
}
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
        objects.push_back({(std::uint32_t(raw.u16(n*12))<<16)|n,b.u32(4),std::int8_t(std::to_integer<unsigned>(b.slice(0xaa,1)[0])),
            sphere(0x30),sphere(0x50)});
    }
    return source_object_admission(visibility,collidable,noncollidable,objects,in,prior);
}
}
