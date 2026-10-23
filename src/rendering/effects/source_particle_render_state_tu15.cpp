#include "halo2/source_particle_render_state_tu15.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <bit>
#include <set>

namespace halo2 {
SourceParticleRenderStateTu15 source_particle_render_state_tu15(
    const Cache &cache,const SourceRuntimeMemory &memory,std::uint32_t system,
    std::uint32_t location,const SourceParticleRenderCameraTu15 &camera) {
    const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
    const auto scalar=[&](std::uint32_t p){return std::bit_cast<float>(word(p));};
    const auto short_word=[&](std::uint32_t p){return Bytes(memory.view(p,2)).u16(0);};
    const auto vector=[&](std::uint32_t p){return Vec3{scalar(p),scalar(p+4),scalar(p+8)};};
    const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return memory.view(p,n);}};
    const auto record=[&](std::uint32_t global,std::uint32_t datum,unsigned stride) {
        const auto pool=word(global),capacity=word(pool+0x20),high=word(pool+0x38);
        if(!capacity || capacity>65536 || high>capacity || word(pool+0x24)!=stride ||
            datum==0xffffffffu || (datum&65535u)>=high)
            throw FormatError("Particle render readback requires a current original pool record");
        const auto p=source_raw_datum_lookup(pool,datum,storage),index=datum&65535u;
        if(!p || !(word(word(pool+0x48)+index/32u*4u)&(1u<<(index%32u))))
            throw FormatError("Particle render readback lost its full allocation identity");
        memory.view(p,stride);return p;
    };
    const auto s=record(0x51df74u,system,84),l=record(0x52d71cu,location,52);
    // A location must occur in this system's actual full-identity chain.
    std::set<std::uint32_t> locations;bool owned{};
    for(auto id=word(s+0x30);id!=0xffffffffu;) {
        if(!locations.insert(id).second)throw FormatError("Particle location chain cycles");
        const auto p=record(0x52d71cu,id,52);owned|=id==location;id=word(p+0xc);
    }
    if(!owned)throw FormatError("Particle location belongs to another source system");
    SourceParticleRenderStateTu15 out;
    out.definition=cache.load_particle_system_render_definition(word(s+0x10),word(s+0x14),short_word(s+0xe));
    auto &state=out.state;state.system_identity=system;state.location_identity=location;
    auto &sources=state.sources;
    sources.system=ParticleSystemValueState{scalar(s+4),scalar(s+0x28),scalar(s+0x2c),{}};
    if(const auto instance=word(s+0x18);instance!=0xffffffffu) {
        const auto effect=record(0x4f720cu,instance,400);
        sources.system->effect_scales=std::array{scalar(effect+0x6c),scalar(effect+0x70)};
    }
    sources.location=ParticleLocationValueState{
        std::to_integer<std::uint8_t>(memory.view(l+3,1)[0]),scalar(l+0x1c)};
    const auto clock=word(0x51df54u);
    sources.game_tick=std::bit_cast<std::int32_t>(word(clock+8));sources.game_rate=scalar(clock+4);
    sources.camera_position=camera.position;sources.camera_forward=camera.forward;sources.camera_up=camera.up;
    state.first_person=std::to_integer<unsigned>(memory.view(l+2,1)[0])!=0;
    state.uses_camera_scale=(out.definition.flags&0x400u)!=0;state.camera_scale=camera.scale;
    if(state.uses_camera_scale && !state.camera_scale)
        throw FormatError("Original particle renderer requires its actual camera-scale owner");
    std::set<std::uint32_t> emitters,particles;
    for(auto id=word(l+4);id!=0xffffffffu;) {
        if(!emitters.insert(id).second || emitters.size()>out.definition.emitters.size())
            throw FormatError("Particle emitter chain cycles or exceeds its authored definitions");
        const auto e=record(0x52d718u,id,76);auto &emitter=state.emitters.emplace_back();
        emitter.rotation={vector(e+0x10),vector(e+0x1c),vector(e+0x28)};emitter.position=vector(e+0x34);
        for(auto particle=word(e+4);particle!=0xffffffffu;) {
            if(!particles.insert(particle).second)throw FormatError("Particle chain cycles or shares an allocation");
            const auto p=record(0x52d714u,particle,64);
            ParticleRenderParticle value;value.identity=particle;
            value.values={scalar(p+8),scalar(p+0x10),scalar(p+0x38),vector(p+0x1c),
                {short_word(p+0x14),short_word(p+0x16),short_word(p+0x18),short_word(p+0x1a)}};
            // +34 is angular velocity. Original224550 consumes the packed
            // lighting color at +3C, after the independent +38 phase.
            value.velocity=vector(p+0x28);value.seed_color=word(p+0x3c);
            emitter.particles.push_back(value);particle=word(p+4);
        }
        if(emitter.particles.size()!=short_word(e+2))
            throw FormatError("Particle emitter count differs from its actual live chain");
        id=word(e+8);
    }
    return out;
}
}
