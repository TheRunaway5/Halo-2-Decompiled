#include "halo2/pstencil/attached_lights.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <bit>
#include <cmath>
#include <set>
namespace halo2::pstencil {
namespace {
constexpr auto none=0xffffffffu;
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p){return Bytes(m.view(p,4)).u32(0);}
std::uint16_t half(const SourceRuntimeMemory& m,std::uint32_t p){return Bytes(m.view(p,2)).u16(0);}
std::uint32_t record(const SourceRuntimeMemory& m,std::uint32_t pool,std::uint32_t id,
    unsigned capacity,unsigned stride) {
    if(word(m,pool+0x20)!=capacity || word(m,pool+0x24)!=stride ||
       word(m,pool+0x38)>capacity || word(m,pool+0x3c)>word(m,pool+0x38))
        throw FormatError("pStencil attached light lost its native pool contract");
    if(id==none || (id&65535u)>=word(m,pool+0x38) ||
       !(word(m,word(m,pool+0x48)+(id&65535u)/32u*4u)&(1u<<(id&31u))))
        throw FormatError("pStencil attached light requires an occupied native datum");
    const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return m.view(p,n);}};
    const auto at=source_raw_datum_lookup(pool,id,storage);
    if(!at || half(m,at)!=std::uint16_t(id>>16))
        throw FormatError("pStencil attached light lost its full native generation");
    (void)m.view(at,stride);return at;
}
}
std::vector<AttachedLight> attached_lights_tu15(const SourceRuntimeMemory& m,
    std::uint32_t object,std::uint32_t definition) {
    const auto header=record(m,word(m,0x4ecbd0u),object,2048,12);
    const unsigned bytes=half(m,header+6),body=word(m,header+8);
    if(bytes<0x128 || word(m,body)!=definition)
        throw FormatError("pStencil attached light owner differs from its native body");
    (void)m.view(body,bytes);
    const auto size=std::bit_cast<std::int16_t>(half(m,body+0x11c));
    const auto offset=std::bit_cast<std::int16_t>(half(m,body+0x11e));
    if(size<0 || size%8 || (size && (offset<0 || unsigned(offset)+unsigned(size)>bytes)))
        throw FormatError("pStencil attached light bank escapes the native object");
    const auto table=word(m,0x4f0414u),row=table+(definition&65535u)*16u;
    if(word(m,row+4)!=definition)throw FormatError("pStencil light owner lost its full tag identity");
    const auto tag=word(m,row+8),count=word(m,tag+0x94),attachments=word(m,tag+0x98);
    if(count!=unsigned(size)/8u)throw FormatError("pStencil attachment count differs from the authored bank");
    std::vector<AttachedLight> out;std::set<std::uint32_t> seen;
    for(unsigned n=0;n<count;++n) {
        const Bytes slot(m.view(body+unsigned(offset)+n*8u,8));
        if(std::to_integer<unsigned>(slot.slice(0,1)[0])!=0)continue;
        const auto id=slot.u32(4);if(id==none)continue;
        const auto authored=attachments+n*24u;
        if(word(m,authored)!=0x6c696768u || word(m,authored+4)==none || !seen.insert(id).second)
            throw FormatError("pStencil native light attachment differs from its authored slot");
        const auto at=record(m,word(m,0x4ecbdcu),id,350,272);const Bytes light(m.view(at,272));
        if(light.u32(0x4c)!=object || light.u16(0x54)!=n || light.u32(4)!=word(m,authored+4) ||
           light.u32(0x50)!=word(m,authored+16) || light.u16(0x56)!=half(m,authored+12))
            throw FormatError("pStencil native light lost its attachment/function/color binding");
        AttachedLight item{n,id,light.u32(4),light.u16(2),
            {light.f32(0xc4),light.f32(0xd0),
             {light.f32(0xd4),light.f32(0xd8),light.f32(0xdc)},
             {light.f32(0xe0),light.f32(0xe4),light.f32(0xe8)},light.f32(0xec)}};
        for(float value:{item.state.size,item.state.control,item.state.diffuse.x,item.state.diffuse.y,
            item.state.diffuse.z,item.state.specular.x,item.state.specular.y,item.state.specular.z,item.state.brightness})
            if(!std::isfinite(value))throw FormatError("pStencil native attachment light has nonfinite completed state");
        item.frame=camera_transform({light.f32(0x84),light.f32(0x88),light.f32(0x8c)},
            {light.f32(0xac),light.f32(0xb0),light.f32(0xb4)},
            {light.f32(0xb8),light.f32(0xbc),light.f32(0xc0)});
        for(const auto v:{item.frame.position,item.frame.basis[0],item.frame.basis[1],item.frame.basis[2]})
            for(float value:{v.x,v.y,v.z})if(!std::isfinite(value))
                throw FormatError("pStencil native attachment light has a nonfinite transform");
        out.push_back(item);
    }
    return out;
}
}
