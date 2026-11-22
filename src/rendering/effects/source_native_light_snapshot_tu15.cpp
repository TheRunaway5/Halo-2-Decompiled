#include "halo2/source_native_light_snapshot_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <bit>
#include <cstdlib>
#include <cstring>
#include <set>

namespace halo2 {
namespace {
constexpr auto none=0xffffffffu;
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p){return Bytes(m.view(p,4)).u32(0);}
struct Pool {
    std::uint32_t address,records,bitmap,capacity,stride,high,count;
    std::span<const std::byte> record_bytes,bitmap_bytes;
};
Pool pool(const SourceRuntimeMemory& m,std::uint32_t address,unsigned capacity,unsigned stride,bool reference) {
    if(!address)throw FormatError("pStencil native light snapshot lost its datum pool");
    Pool p{address,word(m,address+0x44u),word(m,address+0x48u),word(m,address+0x20u),
        word(m,address+0x24u),word(m,address+0x38u),word(m,address+0x3cu),{}, {}};
    if(p.capacity!=capacity || p.stride!=stride || p.high>capacity || p.count>p.high)
        throw FormatError("pStencil native light snapshot has an incompatible datum pool");
    p.record_bytes=m.view(p.records,std::size_t(capacity)*stride);
    p.bitmap_bytes=m.view(p.bitmap,((capacity+31u)/32u)*4u);
    unsigned occupied=0;
    if(reference) {
        for(unsigned n=0;n<capacity;++n)if(word(m,p.bitmap+(n/32u)*4u)&(1u<<(n%32u))) {
            if(n>=p.high || Bytes(m.view(p.records+n*stride,2)).u16(0)==0)
                throw FormatError("pStencil native light snapshot has an invalid occupied generation");
            ++occupied;
        }
    } else {
        const Bytes bitmap(p.bitmap_bytes),records(p.record_bytes);
        for(unsigned first=0;first<capacity;first+=32) {
            auto bits=bitmap.u32(first/8u);
            const auto remaining=capacity-first;
            if(remaining<32)bits&=(1u<<remaining)-1u; // Ignore padding above the light capacity.
            while(bits) {
                const auto n=first+unsigned(std::countr_zero(bits));
                if(n>=p.high || records.u16(n*stride)==0)
                    throw FormatError("pStencil native light snapshot has an invalid occupied generation");
                ++occupied;
                bits&=bits-1u;
            }
        }
    }
    if(occupied!=p.count)throw FormatError("pStencil native light snapshot occupancy differs from its count");
    return p;
}
std::uint32_t record(const SourceRuntimeMemory& m,const Pool& p,std::uint32_t identity) {
    if(identity==none || (identity&65535u)>=p.high ||
       !(word(m,p.bitmap+((identity&65535u)/32u)*4u)&(1u<<(identity&31u))))
        throw FormatError("pStencil native light snapshot follows an unoccupied spatial link");
    const SourceRawDatumPoolServices storage{.read=[&](auto address,auto size){return m.view(address,size);}};
    const auto at=source_raw_datum_lookup(p.address,identity,storage);
    if(!at || Bytes(m.view(at,2)).u16(0)!=std::uint16_t(identity>>16))
        throw FormatError("pStencil native light snapshot follows a stale spatial generation");
    return at;
}
Bytes compact_record(const Pool& p,std::uint32_t identity) {
    const auto index=identity&65535u;
    if(identity==none || index>=p.high || !(Bytes(p.bitmap_bytes).u32((index/32u)*4u)&(1u<<(index&31u))))
        throw FormatError("pStencil native light snapshot follows an unoccupied spatial link");
    const Bytes row(p.record_bytes.subspan(index*p.stride,p.stride));
    if((!p.records && !index) || row.u16(0)!=std::uint16_t(identity>>16))
        throw FormatError("pStencil native light snapshot follows a stale spatial generation");
    return row;
}
void copy_spatial(const SourceRuntimeMemory& m,const Pool& source,SourceObjectSpatialDatumPool& out,bool reference) {
    out.record_words=3;out.highwater=source.high;out.active_count=source.count;
    if(reference) {
        std::memcpy(out.occupied.data(),m.view(source.bitmap,sizeof(out.occupied)).data(),sizeof(out.occupied));
        for(unsigned n=0;n<source.high;++n)
            std::memcpy(out.records[n].data(),m.view(source.records+n*12u,12).data(),12);
    } else {
        std::memcpy(out.occupied.data(),source.bitmap_bytes.data(),sizeof(out.occupied));
        // Guest records are 12 bytes. The host pool reserves 32 bytes per row.
        for(unsigned n=0;n<source.high;++n)
            std::memcpy(out.records[n].data(),source.record_bytes.data()+n*12u,12);
    }
}
void validate_compact_graph(const Pool& light,const Pool& entry,const Pool& reference,
    const SourceNativeLightSnapshotTu15& out) {
    const auto clusters=out.spatial.heads.size();
    std::array<unsigned,SourceObjectSpatialDatumPool::capacity> visited{};
    unsigned chain=0;
    const auto visit=[&](std::uint32_t link,const char* cycle) {
        auto& visit_chain=visited[link&65535u];
        if(visit_chain==chain)throw FormatError(cycle);
        visit_chain=chain;
    };
    for(const auto head:out.spatial.heads) {
        ++chain;
        for(auto link=head;link!=none;) {
            const auto row=compact_record(entry,link);
            // A different salt for an already visited slot is stale, not a cycle.
            visit(link,"pStencil native light cluster contains a cycle");
            (void)compact_record(light,row.u32(4));
            link=row.u32(8);
        }
    }
    for(const auto identity:out.lights.identities()) {
        ++chain;
        for(auto link=out.lights.record(identity)[0x10u/4u];link!=none;) {
            const auto row=compact_record(reference,link);
            visit(link,"pStencil native light references contain a cycle");
            const auto cluster=std::bit_cast<std::int16_t>(row.u16(4));
            if(cluster<0 || std::size_t(cluster)>=clusters)
                throw FormatError("pStencil native light reference escapes its BSP");
            link=row.u32(8);
        }
    }
}
}
SourceNativeLightSnapshotTu15 source_native_light_snapshot_tu15(const SourceRuntimeMemory& m,std::size_t clusters) {
    const auto bsp=word(m,0x4ecc18u);
    if(!bsp || word(m,bsp+0x9cu)!=clusters)
        throw FormatError("pStencil native light snapshot differs from its current BSP");
    // C4D30->C5120 registers through this literal three-pointer bank;
    // C3430->C3509/whole1D30F0 retires the same reference/entry lists.
    static const bool reference_snapshot=[]{const auto* value=std::getenv("HALO2_REFERENCE_LIGHT_SNAPSHOT");
        return value && *value && *value!='0';}();
    const auto light=pool(m,word(m,0x4ecbdcu),350,272,reference_snapshot);
    const auto entry=pool(m,word(m,0x4ecbe4u),2048,12,reference_snapshot);
    const auto reference=pool(m,word(m,0x4ecbe8u),2048,12,reference_snapshot);
    SourceNativeLightSnapshotTu15 out(clusters);out.lights.highwater=light.high;out.lights.active_count=light.count;
    if(reference_snapshot) {
        std::memcpy(out.lights.occupied.data(),m.view(light.bitmap,sizeof(out.lights.occupied)).data(),sizeof(out.lights.occupied));
        for(unsigned n=0;n<light.high;++n)
            std::memcpy(out.lights.records[n].data(),m.view(light.records+n*272u,272).data(),272);
    } else {
        static_assert(sizeof(out.lights.records[0])==272);
        std::memcpy(out.lights.occupied.data(),light.bitmap_bytes.data(),sizeof(out.lights.occupied));
        std::memcpy(out.lights.records.data(),light.record_bytes.data(),light.high*272u);
    }
    copy_spatial(m,entry,out.spatial.entries,reference_snapshot);
    copy_spatial(m,reference,out.spatial.references,reference_snapshot);
    const auto heads=word(m,0x4ecbe0u);
    std::memcpy(out.spatial.heads.data(),m.view(heads,clusters*4u).data(),clusters*4u);
    if(!reference_snapshot) {
        validate_compact_graph(light,entry,reference,out);
        return out;
    }
    for(unsigned cluster=0;cluster<clusters;++cluster) {
        std::set<std::uint32_t> visited;
        for(auto link=out.spatial.heads[cluster];link!=none;) {
            if(!visited.insert(link).second)throw FormatError("pStencil native light cluster contains a cycle");
            const auto at=record(m,entry,link);(void)record(m,light,word(m,at+4u));link=word(m,at+8u);
        }
    }
    for(const auto identity:out.lights.identities()) {
        std::set<std::uint32_t> visited;
        for(auto link=out.lights.record(identity)[0x10u/4u];link!=none;) {
            if(!visited.insert(link).second)throw FormatError("pStencil native light references contain a cycle");
            const auto at=record(m,reference,link);
            const auto cluster=std::bit_cast<std::int16_t>(Bytes(m.view(at+4u,2)).u16(0));
            if(cluster<0 || std::size_t(cluster)>=clusters)
                throw FormatError("pStencil native light reference escapes its BSP");
            link=word(m,at+8u);
        }
    }
    return out;
}
}
