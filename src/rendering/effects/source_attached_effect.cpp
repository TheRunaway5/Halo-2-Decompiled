#include "halo2/source_attached_effect.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cmath>
#include <set>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p,unsigned n=4) {
    const auto bytes=m.view(p,n);std::uint32_t out{};
    for(unsigned i=0;i<n;++i)out|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return out;
}
}
void validate_source_attached_effects(std::span<const SourceAttachedEffect> effects) {
    if(effects.size()>4095)throw FormatError("Source effect attachment bank exceeds signed body extent");
    std::set<std::uint16_t> slots;
    for(const auto &e:effects) {
        if(e.slot>=4095 || !slots.insert(e.slot).second || !std::isfinite(e.scale_a) || !std::isfinite(e.scale_b))
            throw FormatError("Invalid source attachment slot or controls");
        if(e.instance==0xffffffffu) {
            if(e.definition!=0xffffffffu || e.flags || std::bit_cast<std::uint32_t>(e.scale_a) ||
                std::bit_cast<std::uint32_t>(e.scale_b) || e.colors!=std::array<std::uint32_t,2>{} || !e.locations.empty())
                throw FormatError("Absent source attachment carries invented effect state");
        }else if((e.instance&65535u)>=256 || e.definition==0xffffffffu)
            throw FormatError("Source attachment effect identity exceeds its original pool");
        if(e.render) {
            validate_source_effect_replica_render_tu15(*e.render);
            if(e.render->identity!=e.instance||e.render->flags!=e.flags||e.render_clock.flags!=e.flags||
               !std::isfinite(e.render_clock.elapsed)||!std::isfinite(e.render_clock.target)||!std::isfinite(e.render_clock.field_68))
                throw FormatError("Source attachment render packet differs from its current allocation/clock");
        }
        if(e.locations.size()>512)throw FormatError("Source effect locations exceed their original pool");
        for(const auto &location:e.locations) {
            const auto finite=[](Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);};
            if(location.name==0xffffffffu || !std::isfinite(location.local.scale) ||
                !finite(location.local.position) || !finite(location.local.basis[0]) ||
                !finite(location.local.basis[1]) || !finite(location.local.basis[2]))
                throw FormatError("Invalid completed source effect location");
        }
    }
}
std::vector<SourceAttachedEffect> source_attached_effects_tu15(
    const SourceRuntimeMemory &m,NativeRuntimeAddress object_pool,
    NativeRuntimeAddress effect_pool,std::uint32_t object) {
    const auto pool=m.source_address(object_pool,0x4c);
    const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return m.view(p,n);}};
    if(word(m,pool+0x20)!=2048 || word(m,pool+0x24)!=12 || word(m,pool+0x38)>2048)
        throw FormatError("Source effect readback requires the actual object header pool");
    const auto header=source_raw_datum_lookup(pool,object,storage);
    const auto occupied=[&](std::uint32_t owner,std::uint32_t datum) {
        const auto index=datum&65535u;
        return (word(m,word(m,owner+0x48)+(index/32u)*4u)&(1u<<(index%32u)))!=0;
    };
    if(!header || !occupied(pool,object) || word(m,header+6,2)<0x128)
        throw FormatError("Source effect readback requires a full current object");
    const auto body=word(m,header+8),body_bytes=word(m,header+6,2);
    if(!body)throw FormatError("Source effect readback has no constructed object body");
    m.view(body,body_bytes);
    const auto size=std::bit_cast<std::int16_t>(std::uint16_t(word(m,body+0x11c,2)));
    const auto offset=std::bit_cast<std::int16_t>(std::uint16_t(word(m,body+0x11e,2)));
    if(size<0 || size%8 || (size && (offset<0 || unsigned(offset)+unsigned(size)>body_bytes)))
        throw FormatError("Source effect attachment descriptor escapes its object");
    std::vector<SourceAttachedEffect> out;
    for(unsigned n=0;n<unsigned(size)/8u;++n) {
        const auto at=body+unsigned(offset)+n*8u;
        if(word(m,at,1)!=2)continue;
        auto &e=out.emplace_back();e.slot=std::uint16_t(n);e.instance=word(m,at+4);
        if(e.instance==0xffffffffu)continue;
        const auto effects=m.source_address(effect_pool,0x4c);
        if(word(m,effects+0x20)!=256 || word(m,effects+0x24)!=400 || word(m,effects+0x38)>256)
            throw FormatError("Source attachment requires the actual effect pool");
        const auto effect=source_raw_datum_lookup(effects,e.instance,storage);
        if(!effect || !occupied(effects,e.instance) || word(m,effect+0x48)!=object)
            throw FormatError("Source attachment lost its full effect generation or owner");
        m.view(effect,400);
        e.definition=word(m,effect+4);e.flags=std::uint16_t(word(m,effect+2,2));
        e.scale_a=std::bit_cast<float>(word(m,effect+0x6c));e.scale_b=std::bit_cast<float>(word(m,effect+0x70));
        e.colors={word(m,effect+0x104),word(m,effect+0x108)};
        e.render=source_effect_replica_render_tu15(m,e.instance,e.definition);
        e.render_clock={e.flags,std::bit_cast<std::int16_t>(std::uint16_t(word(m,effect+0x5c,2))),
            std::bit_cast<std::int16_t>(std::uint16_t(word(m,effect+0x5e,2))),
            std::bit_cast<float>(word(m,effect+0x60)),std::bit_cast<float>(word(m,effect+0x64)),
            std::bit_cast<float>(word(m,effect+0x68))};
        const auto tags=word(m,0x4f0414u),row=tags+(e.definition&65535u)*16u;
        if(word(m,row+4)!=e.definition)throw FormatError("Source effect definition lost its full tag identity");
        const auto definition=word(m,row+8),count=word(m,definition+0xcu),names=word(m,definition+0x10u);
        if(count>32)throw FormatError("Source effect exceeds its original location heads");
        std::set<std::uint32_t> seen;
        for(unsigned location_index=0;location_index<count;++location_index) {
            for(auto datum=word(m,effect+0x7cu+location_index*4u);datum!=0xffffffffu;) {
                if(!seen.insert(datum).second || seen.size()>512)
                    throw FormatError("Source attachment location chain cycles or shares an allocation");
                const auto locations=word(m,0x4f7208u);
                if(word(m,locations+0x20)!=512 || word(m,locations+0x24)!=60 || word(m,locations+0x38)>512)
                    throw FormatError("Source attachment requires its actual location pool");
                const auto location=source_raw_datum_lookup(locations,datum,storage);
                if(!location || !occupied(locations,datum))throw FormatError("Source attachment lost a full location generation");
                SourceAttachedEffectLocation out_location;
                out_location.name=word(m,names+location_index*4u);
                out_location.node=std::bit_cast<std::int16_t>(std::uint16_t(word(m,location+2,2)));
                m.read(location+8u,std::as_writable_bytes(std::span(&out_location.local,1)));
                e.locations.push_back(out_location);datum=word(m,location+4u);
            }
        }
    }
    validate_source_attached_effects(out);return out;
}
}
