#include "halo2/pstencil/native_lights.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <bit>
#include <cstring>
#include <set>

namespace halo2::pstencil {
namespace {
constexpr auto none=0xffffffffu;
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p){return Bytes(m.view(p,4)).u32(0);}
struct Pool {
    std::uint32_t address,records,bitmap,capacity,stride,high,count;
};
Pool pool(const SourceRuntimeMemory& m,std::uint32_t address,unsigned capacity,unsigned stride) {
    if(!address)throw FormatError("pStencil native light snapshot lost its datum pool");
    Pool p{address,word(m,address+0x44u),word(m,address+0x48u),word(m,address+0x20u),
        word(m,address+0x24u),word(m,address+0x38u),word(m,address+0x3cu)};
    if(p.capacity!=capacity || p.stride!=stride || p.high>capacity || p.count>p.high)
        throw FormatError("pStencil native light snapshot has an incompatible datum pool");
    (void)m.view(p.records,std::size_t(capacity)*stride);
    (void)m.view(p.bitmap,((capacity+31u)/32u)*4u);
    unsigned occupied=0;
    for(unsigned n=0;n<capacity;++n)if(word(m,p.bitmap+(n/32u)*4u)&(1u<<(n%32u))) {
        if(n>=p.high || Bytes(m.view(p.records+n*stride,2)).u16(0)==0)
            throw FormatError("pStencil native light snapshot has an invalid occupied generation");
        ++occupied;
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
void copy_spatial(const SourceRuntimeMemory& m,const Pool& source,SourceObjectSpatialDatumPool& out) {
    out.record_words=3;out.highwater=source.high;out.active_count=source.count;
    std::memcpy(out.occupied.data(),m.view(source.bitmap,sizeof(out.occupied)).data(),sizeof(out.occupied));
    for(unsigned n=0;n<source.high;++n)
        std::memcpy(out.records[n].data(),m.view(source.records+n*12u,12).data(),12);
}
}
NativeLightSnapshot native_lights_tu15(const SourceRuntimeMemory& m,std::size_t clusters) {
    const auto bsp=word(m,0x4ecc18u);
    if(!bsp || word(m,bsp+0x9cu)!=clusters)
        throw FormatError("pStencil native light snapshot differs from its current BSP");
    // C4D30->C5120 registers through this literal three-pointer bank;
    // C3430->C3509/whole1D30F0 retires the same reference/entry lists.
    const auto light=pool(m,word(m,0x4ecbdcu),350,272);
    const auto entry=pool(m,word(m,0x4ecbe4u),2048,12);
    const auto reference=pool(m,word(m,0x4ecbe8u),2048,12);
    NativeLightSnapshot out(clusters);out.lights.highwater=light.high;out.lights.active_count=light.count;
    std::memcpy(out.lights.occupied.data(),m.view(light.bitmap,sizeof(out.lights.occupied)).data(),sizeof(out.lights.occupied));
    for(unsigned n=0;n<light.high;++n)
        std::memcpy(out.lights.records[n].data(),m.view(light.records+n*272u,272).data(),272);
    copy_spatial(m,entry,out.spatial.entries);copy_spatial(m,reference,out.spatial.references);
    const auto heads=word(m,0x4ecbe0u);
    std::memcpy(out.spatial.heads.data(),m.view(heads,clusters*4u).data(),clusters*4u);
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
