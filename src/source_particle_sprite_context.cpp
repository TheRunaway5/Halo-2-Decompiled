#include "halo2/source_particle_sprite_context.hpp"
#include <bit>

namespace halo2 {
std::optional<SourceParticleSpriteContext> source_particle_sprite_context(
    const Cache &cache,std::uint32_t particle,const ParticleSpriteDefinition &sprite) {
    const auto read=[&](std::uint32_t address,std::size_t size) {
        // An alias may forward to another retained packet. Require the real
        // containing storage rather than treating a zero-sized alias as backing.
        for(const auto &owner:cache.select_tags({.contains=TagAddressRange{address,size},
            .require_address=true,.minimum_size=1})) {
            (void)cache.tag_payload(owner.handle());
            return cache.read_tag_bytes(address,size);
        }
        throw FormatError("Particle bitmap context escapes its retained metadata");
    };
    const auto owner=cache.query_tag({.handle=TagHandle{particle},
        .group=tag_groups::particle,.require_address=true});
    if(!owner)throw FormatError("Particle context has no actual particle definition");
    const Bytes definition(read(owner->address,0x94));
    if(!definition.u32(0x8c))return {};
    const Bytes processed(read(definition.u32(0x90),0x64));
    if(!processed.u32(0x5c))
        throw FormatError("Particle context has no original bitmap selector");
    const auto selected=std::bit_cast<std::int16_t>(Bytes(read(processed.u32(0x60),2)).u16(0));
    if(selected==-1)return SourceParticleSpriteContext{-1,1,false};
    if(selected<0 || unsigned(selected)>=processed.u32(4))
        throw FormatError("Particle context bitmap selector escapes its original table");
    const auto bitmap_id=Bytes(read(processed.u32(8)+unsigned(selected)*12u,4)).u32(0);
    if(bitmap_id==0xffffffffu)return SourceParticleSpriteContext{-1,1,false};
    if(bitmap_id!=sprite.bitmap)
        throw FormatError("Particle context bitmap differs from the imported sprite owner");
    const auto bitmap=cache.query_tag({.handle=TagHandle{bitmap_id},
        .group=tag_groups::bitmap,.require_address=true});
    if(!bitmap)throw FormatError("Particle context has no actual bitmap definition");
    const Bytes root(read(bitmap->address,0x4c));
    const auto sequences=root.u32(0x3c);
    if(sequences>65536 || sequences!=sprite.sequences.size())
        throw FormatError("Particle context sequence bank differs from its actual bitmap");
    if(!root.u32(0x44))return {};
    const Bytes data(read(root.u32(0x48),8));
    const auto width=std::bit_cast<std::int16_t>(data.u16(4));
    const auto height=std::bit_cast<std::int16_t>(data.u16(6));
    if(width<=0 || height<=0)
        throw FormatError("Particle context requires admitted positive bitmap dimensions");
    SourceParticleSpriteContext out;
    if(sequences) {
        out.selected_sequence=std::int32_t(sprite.first_sequence)%std::int32_t(sequences);
        if(sprite.bitmap_type==3 && out.selected_sequence<0)
            throw FormatError("Particle atlas context precedes its authored sequence bank");
    }
    if(width!=height)out.width=float(width)/float(height);
    return out;
}
}
