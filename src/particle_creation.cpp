#include "halo2/particle_creation.hpp"

namespace halo2 {
namespace {
std::uint32_t random_next(std::uint32_t &seed) {
  seed=seed*1664525u+1013904223u;return seed;
}
std::uint16_t random_field(std::uint32_t &seed) {
  // 247950 retains both products on x87 before its float store/CVTTSS2SI.
  const double fraction=double(random_next(seed)>>16)*double(1.f/65535.f);
  const float scaled=float(fraction*65535.0);
  return std::uint16_t(std::int32_t(scaled));
}
Vec3 rotate(const AuthoredBasis &m,Vec3 v) {
  return {(m.up.x*v.z+m.left.x*v.y)+m.forward.x*v.x,
    (m.up.y*v.z+m.forward.y*v.x)+m.left.y*v.y,
    (m.up.z*v.z+m.forward.z*v.x)+m.left.z*v.y}; // 143070.
}
}
void create_emitter_particle(const ParticleSystemRenderDefinition &definition,unsigned index,
    ParticleEmitterState &emitter,ParticleSimulationState &particle,const ParticleCreationInput &input,
    ParticleRenderValueCache &cache,const ParticleValueSources &owners,std::uint32_t &seed,
    const OriginalData &data,const ShaderFunctionTables &tables) {
  if(index>=definition.emitters.size())throw FormatError("particle creation exceeds authored emitter definitions");
  const auto &birth=definition.emitters[index].birth;
  if(birth.shape>5)
    throw FormatError("particle creation shape requires original initialization evidence");
  particle.next_particle=emitter.first_particle;emitter.first_particle=input.allocated_datum;
  particle.flags=std::uint16_t((particle.flags&0xe004u)|4u);
  const auto bucket=((random_next(seed)>>16)*5u)>>16;
  particle.flags=std::uint16_t((particle.flags&0x1fffu)|(bucket<<13));
  particle.age=0.f;
  for(auto &value:particle.random)value=random_field(seed);
  particle.system_fraction=input.system_fraction;
  if(cache.particle_identity!=input.particle_identity) {
    cache.particle_identity=input.particle_identity;cache.values.evaluated&=0xffff07f0u;
  }
  auto sources=owners;
  sources.particle=ParticleValueState{particle.age,particle.system_fraction,particle.phase,particle.position,particle.random};
  complete_particle_values(cache.values,0xf80fu,sources);
  const auto initialized=initialize_particle_birth(birth,cache.values.values,seed,data,tables);
  particle.inverse_lifetime=initialized.inverse_lifetime;particle.rotation_rate=initialized.rotation_rate;
  particle.position={input.request.scale*initialized.position.x,input.request.scale*initialized.position.y,input.request.scale*initialized.position.z};
  particle.velocity=initialized.velocity;
  particle.age+=(input.request.fraction*input.request.dt)*particle.inverse_lifetime;
  if(definition.coordinates==0) {
    const float remaining=1.f-input.request.fraction;
    const Vec3 origin{emitter.position.x*input.request.fraction+emitter.previous_position.x*remaining,
      emitter.previous_position.y*remaining+emitter.position.y*input.request.fraction,
      emitter.previous_position.z*remaining+emitter.position.z*input.request.fraction};
    const auto position=rotate(emitter.rotation,particle.position);
    particle.position={origin.x+position.x,origin.y+position.y,origin.z+position.z};
    particle.velocity=rotate(emitter.rotation,particle.velocity);
    if(definition.flags&0x10)particle.velocity={particle.velocity.x+input.effect_velocity.x,
      input.effect_velocity.y+particle.velocity.y,input.effect_velocity.z+particle.velocity.z};
  }else {
    const float remaining=(1.f-input.request.fraction)*input.request.dt;
    particle.position={particle.velocity.x*remaining+particle.position.x,
      particle.velocity.y*remaining+particle.position.y,particle.velocity.z*remaining+particle.position.z};
  }
  particle.seed_color=input.request.color;++emitter.particle_count;
}
} // namespace halo2
