#pragma once
#include "halo2/particle_model.hpp"
#include "halo2/particle_render_inputs.hpp"
#include "halo2/source_particle_sprite_context.hpp"
#include "halo2/particle_resource_error.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
// Immutable authored draw inputs only. These checks do not evaluate native
// particle values, run source code, or consume a random value.
namespace particle_draw_resource_detail {
inline void require(bool value,const char* why) {if(!value)throw ParticleResourceError(why);}
inline void property(const ParticleProperty& value,bool color=false) {
    require(value.input>=0 && value.input<17 && value.range>=0 && value.range<17 && value.modifier_input<17,
        "Particle draw property escapes the authored value-cache interface");
    if(value.curve.empty())return;
    const Bytes bytes(value.curve);bytes.require(0,20);
    const auto byte=[&](unsigned offset){return std::to_integer<unsigned>(value.curve[offset]);};
    const auto type=byte(0),flags=byte(1);
    require(type<11,"Particle draw function has an unsupported authored type");
    if(!color && !(flags&0xf0))
        require(std::isfinite(bytes.f32(4)) && std::isfinite(bytes.f32(8)),
            "Particle scalar draw function has nonfinite authored endpoints");
    // These are exactly the parameter arrays read by shader_function_fraction.
    // Identity/constant/default branches never dereference their arrays.
    if(type==0 || type==1 || type==6 || type==8)return;
    constexpr unsigned counts[]{0,1,2,4,6,20,32,12,4,3,12};
    const auto count=counts[type];bytes.require(20,count*4*((flags&1)?2:1));
    for(unsigned bank=0;bank<((flags&1)?2u:1u);++bank) {
        const auto finite=[&](unsigned index) {
            require(std::isfinite(bytes.f32(20+(bank*count+index)*4)),
                "Particle draw function has a nonfinite authored parameter");
        };
        if(type==2) {require(byte(2+bank)<16,"Particle transition draw function escapes its table");finite(0);finite(1);}
        else if(type==3) {require(byte(2+bank)<12,"Particle periodic draw function escapes its table");for(unsigned n=0;n<4;++n)finite(n);}
        else if(type==4) {finite(4);finite(5);}
        else if(type==5) {for(unsigned n:{9u,10u,11u,13u,14u,15u,16u,17u,18u,19u})finite(n);}
        else if(type==7) {for(unsigned n=8;n<12;++n)finite(n);}
        else if(type==9) {for(unsigned n=0;n<3;++n)finite(n);}
        else if(type==10) {for(unsigned n=0;n<12;++n)finite(n);}
    }
}
}
inline void validate_particle_draw_resource(const ParticleSpriteDefinition& sprite,
    const SourceParticleSpriteContext& context)try {
    using namespace particle_draw_resource_detail;
    if(!context.has_bitmap)return; // Original bitmap selector emits no sprite.
    require(std::isfinite(context.width),"Particle sprite context has a nonfinite bitmap aspect");
    for(unsigned index=0;index<4;++index)property(sprite.properties[index],index==0);
    if(sprite.flags&0x10000) {
        for(auto value:sprite.geometry)require(std::isfinite(value),"Particle direct geometry is nonfinite");
        for(auto value:sprite.bounds)require(std::isfinite(value),"Particle direct UV bounds are nonfinite");
        return;
    }
    if(sprite.bitmap_type!=3)return;
    int first=context.selected_sequence,last=first;
    if(sprite.flags&0x10) {
        require(!sprite.sequences.empty(),"Particle random atlas sequence bank is missing");
        first=std::clamp<int>(sprite.first_sequence,0,int(sprite.sequences.size())-1);
        last=std::clamp<int>(sprite.first_sequence+std::max<int>(sprite.sequence_count,1)-1,0,int(sprite.sequences.size())-1);
    }
    if(first<0)return;
    require(unsigned(last)<sprite.sequences.size(),"Particle selected atlas sequence escapes its authored bank");
    bool animated{};
    for(int sequence=first;sequence<=last;++sequence)for(const auto& frame:sprite.sequences[sequence]) {
        animated=true;
        for(auto value:frame.bounds)require(std::isfinite(value),"Particle atlas bounds are nonfinite");
        require(frame.bounds[0]!=frame.bounds[1] && frame.bounds[2]!=frame.bounds[3],
            "Particle atlas frame has empty authored bounds");
        require(std::isfinite(frame.pivot.x) && std::isfinite(frame.pivot.y),"Particle atlas pivot is nonfinite");
    }
    if(animated)property(sprite.properties[4]);
}catch(const ParticleResourceError&){throw;}
catch(const FormatError& error){throw ParticleResourceError(error.what());}
inline void validate_particle_draw_resource(const ParticleModelDefinition& model)try {
    using namespace particle_draw_resource_detail;
    require(model.orientation<=4,"Particle model orientation is unsupported");
    for(const auto& value:model.properties)property(value);
    require(!model.spans.empty() && !model.vertices.empty() && !model.indices.empty(),
        "Particle model draw has no authored geometry bank");
    for(auto index:model.indices)require(index<model.vertices.size(),"Particle model index escapes its authored vertices");
    for(const auto& span:model.spans)require(span[0]<=model.indices.size() && span[1]<=model.indices.size()-span[0] && !(span[1]%3),
        "Particle model span escapes its authored triangles");
}catch(const ParticleResourceError&){throw;}
catch(const FormatError& error){throw ParticleResourceError(error.what());}
inline void validate_particle_draw_resource(const ParticleSystemRenderDefinition& system)try {
    using namespace particle_draw_resource_detail;
    for(const auto& emitter:system.emitters) {
        for(unsigned index=0;index<3;++index)property(emitter.properties[index],index==1);
        require(!emitter.properties[1].curve.empty(),"Particle emitter draw color function is missing");
    }
}catch(const ParticleResourceError&){throw;}
catch(const FormatError& error){throw ParticleResourceError(error.what());}
}
