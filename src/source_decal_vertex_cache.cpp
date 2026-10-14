#include "halo2/source_decal_vertex_cache.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
std::vector<SourceDecalCacheGeometry>
source_decal_vertex_cache_tu15(const SourceRuntimeMemory &memory) {
    const auto word=[&](std::uint32_t p,unsigned bytes=4) {
        const auto b=memory.view(p,bytes);std::uint32_t v=0;
        for(unsigned i=0;i<bytes;++i)v|=std::to_integer<unsigned>(b[i])<<(i*8);
        return v;
    };
    const auto scalar=[&](std::uint32_t p){return std::bit_cast<float>(word(p));};
    const auto pool=word(0x4f7220u),owner=word(0x515f28u),resource=word(0x515f24u);
    if(!pool || !owner || !resource)
        throw std::runtime_error("TU1.5 decal cache has no original constructor owners");
    if(word(pool+0x24u)!=0x40u || word(pool+0x20u)!=0x400u ||
        word(pool+0x29u,1)!=1 || word(owner+0x34u)!=6u)
        throw std::runtime_error("TU1.5 decal cache pool/page layout differs");
    const auto pages=word(owner+0x64u),capacity=word(owner+0x30u),high=word(pool+0x38u);
    if(word(pages+0x24u)!=0x18u || high>0x400u || capacity!=0x500u)
        throw std::runtime_error("TU1.5 decal cache allocation layout differs");
    const auto base=word(resource+4u)|0x80000000u;
    memory.view(base,std::size_t(capacity)*64u);
    SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return memory.view(p,n);}};
    std::vector<SourceDecalCacheGeometry> result;
    for(std::uint32_t index=0;index<high;++index) {
        const auto address=word(pool+0x44u)+index*64u,salt=word(address,2);
        if(!salt)continue;
        const auto datum=(salt<<16u)|index;
        if(source_raw_datum_lookup(pool,datum,storage)!=address)
            throw std::runtime_error("TU1.5 decal cache lost its live salted record");
        const auto token=word(address+4u),page=source_raw_datum_lookup(pages,token,storage);
        if(!page)throw std::runtime_error("TU1.5 decal references a retired vertex cache allocation");
        const auto first=word(page+8u),allocated=word(page+4u),count=word(address+0x2au,2);
        if(first>=capacity || allocated>capacity-first || count>allocated)
            throw std::runtime_error("TU1.5 decal vertex range exceeds its actual cache allocation");
        SourceDecalCacheGeometry out{datum,word(address+0x2cu),token,word(address+0x18u),
            word(address+0x24u),std::bit_cast<std::int16_t>(std::uint16_t(word(address+0xau,2))),
            std::uint8_t(word(address+3u,1)),std::uint8_t(word(address+2u,1)),
            scalar(address+0x1cu),scalar(address+0x20u),{}};
        for(unsigned q=0;q<count;++q) {
            std::array<SourceDecalCacheVertex,4> quad;
            for(unsigned v=0;v<4;++v) {
                const auto p=base+(first+q)*64u+v*16u,packed=word(p+12u);
                const auto component=[&](unsigned shift,unsigned width,float scale) {
                    auto raw=(packed>>shift)&((1u<<width)-1u);
                    const auto sign=1u<<(width-1u);
                    const auto signed_value=std::bit_cast<std::int32_t>((raw^sign)-sign);
                    return float(signed_value)/scale;
                };
                quad[v]={{scalar(p),scalar(p+4u),scalar(p+8u)},
                    {component(0,11,1023.f),component(11,11,1023.f),component(22,10,511.f)},packed};
                if(!std::isfinite(quad[v].position.x) || !std::isfinite(quad[v].position.y) ||
                    !std::isfinite(quad[v].position.z))
                    throw std::runtime_error("TU1.5 completed decal cache contains a non-finite vertex");
            }
            out.quads.push_back(quad);
        }
        result.push_back(std::move(out));
    }
    return result;
}
} // namespace halo2
