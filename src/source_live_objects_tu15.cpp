#include "halo2/source_live_objects_tu15.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <bit>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p,unsigned n=4) {
    const auto bytes=m.view(p,n);std::uint32_t v=0;
    for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return v;
}
std::int16_t short_at(const SourceRuntimeMemory &m,std::uint32_t p) {
    return std::bit_cast<std::int16_t>(std::uint16_t(word(m,p,2)));
}
SourceRawDatumPoolServices storage(const SourceRuntimeMemory &m) {
    return {.read=[&m](auto p,auto n){return m.view(p,n);}};
}
}
std::vector<SourceLiveObjectTu15> source_live_objects_tu15(const SourceRuntimeMemory &m,
    NativeRuntimeAddress native_pool) {
    const auto pool=m.source_address(native_pool,0x4c);
    const auto capacity=word(m,pool+0x20),stride=word(m,pool+0x24),high=word(m,pool+0x38);
    if(capacity!=2048 || stride!=12 || high>capacity)
        throw FormatError("TU1.5 live object readback requires the original object header pool");
    const auto headers=word(m,pool+0x44),bits=word(m,pool+0x48);
    const auto backing=storage(m);std::vector<SourceLiveObjectTu15> result;
    for(unsigned index=0;index<high;++index) {
        if(!(word(m,bits+(index/32u)*4u)&(1u<<(index%32u))))continue;
        const auto header=headers+index*stride,body=word(m,header+8);
        if(!body)continue;
        const auto object=(word(m,header,2)<<16)|index;
        if(source_raw_datum_lookup(pool,object,backing)!=header || word(m,header+6,2)<0x128)
            throw FormatError("TU1.5 live object readback has an incomplete full generation");
        const auto type=word(m,header+3,1);
        if(type>12 || word(m,body+0xaa,1)!=type || word(m,body)==0xffffffffu)
            throw FormatError("TU1.5 live object header/body type or definition differs");
        m.view(body,word(m,header+6,2));
        result.push_back({object,word(m,body),word(m,body+0x14),std::uint8_t(type)});
    }
    return result;
}
SourceModelRegionBank source_live_model_regions_tu15(const SourceRuntimeMemory &m,
    NativeRuntimeAddress native_pool,std::uint32_t object) {
    const auto pool=m.source_address(native_pool,0x4c),header=source_raw_datum_lookup(pool,object,storage(m));
    if(!header || word(m,header+6,2)<0x128)throw FormatError("TU1.5 model region readback requires a full live object");
    const auto body=word(m,header+8);
    if(!body)throw FormatError("TU1.5 model regions have no constructed body");
    const auto extent=short_at(m,body+0x118),offset=short_at(m,body+0x11a);
    if(extent<0 || extent%10 || extent/10>16 || (extent && (offset<0 ||
        unsigned(offset)+unsigned(extent)>word(m,header+6,2))))
        throw FormatError("TU1.5 model region descriptor escapes its constructed body");
    const unsigned count=unsigned(extent)/10u;const auto bank=body+unsigned(offset),states=bank+count*2u;
    SourceModelRegionBank result;
    result.selection.variant_index=std::bit_cast<std::int8_t>(std::uint8_t(word(m,body+0xb1,1)));
    result.selection.permutations.reserve(count);result.selection.states.reserve(count);result.looping_effects.reserve(count);
    for(unsigned i=0;i<count;++i) {
        result.selection.permutations.push_back(std::uint8_t(word(m,bank+i,1)));
        const auto state=states+i*8u;
        result.selection.states.push_back({std::bit_cast<std::int8_t>(std::uint8_t(word(m,state,1))),
            std::uint8_t(word(m,state+1,1)),std::uint8_t(word(m,state+2,1))});
        result.looping_effects.push_back(word(m,state+4));
    }
    return result;
}
std::vector<Vec3> source_live_object_colors_tu15(const SourceRuntimeMemory &m,
    NativeRuntimeAddress native_pool,std::uint32_t object) {
    const auto pool=m.source_address(native_pool,0x4c),header=source_raw_datum_lookup(pool,object,storage(m));
    if(!header || word(m,header+6,2)<0x128)throw FormatError("TU1.5 color readback requires a full live object");
    const auto body=word(m,header+8);
    if(!body)throw FormatError("TU1.5 colors have no constructed body");
    const auto extent=short_at(m,body+0x124),offset=short_at(m,body+0x126);
    if(extent<0 || extent%24 || extent/24>4 || (extent && (offset<0 ||
        unsigned(offset)+unsigned(extent)>word(m,header+6,2))))
        throw FormatError("TU1.5 color descriptor escapes its constructed body");
    const unsigned count=unsigned(extent)/24u;std::vector<Vec3> colors;colors.reserve(count);
    auto at=body+unsigned(offset)+count*12u;
    for(unsigned i=0;i<count;++i,at+=12u)colors.push_back({std::bit_cast<float>(word(m,at)),
        std::bit_cast<float>(word(m,at+4)),std::bit_cast<float>(word(m,at+8))});
    return colors;
}
}
