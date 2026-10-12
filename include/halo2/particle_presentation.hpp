#pragma once
#include "halo2/particle_creation.hpp"
#include "halo2/render_interpolation.hpp"
namespace halo2 {
inline ParticleSimulationState particle_presentation(const ParticleSimulationState *previous,
    const ParticleSimulationState &current,float fraction) {
    validate_render_fraction(fraction);
    auto out=current;
    if(!previous || previous->salt!=current.salt)return out;
    out.position=render_lerp(previous->position,current.position,fraction);
    out.velocity=render_lerp(previous->velocity,current.velocity,fraction);
    out.age=std::lerp(previous->age,current.age,fraction);
    out.phase=std::lerp(previous->phase,current.phase,fraction);
    return out;
}
}
