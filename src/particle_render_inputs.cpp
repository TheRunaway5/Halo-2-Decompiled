#include "halo2/particle_render_inputs.hpp"
#include <algorithm>

namespace halo2 {
namespace {
float property(const ParticleProperty &p,const std::array<float,17> &values,
    const ShaderFunctionTables &tables,bool color=false) {
  if(p.input<0 || p.input>=17 || p.range<0 || p.range>=17 || p.modifier_input>=17)
    throw FormatError("particle emitter property exceeds original value cache");
  float result;
  if(color) {
    if(p.curve.size()<20)throw FormatError("particle emitter color function missing");
    const auto type=std::to_integer<unsigned>(p.curve[0]);
    const auto flags=std::to_integer<unsigned>(p.curve[1]);
    result=shader_function_fraction(p.curve,type==1?0.f:values[p.input],flags&1?values[p.range]:0.f,tables);
  }else result=shader_function_value(p.curve,values[p.input],values[p.range],tables);
  if(p.modifier_input>=0) {
    if(p.modifier==1)result=values[p.modifier_input]+result;
    else if(p.modifier==2)result=values[p.modifier_input]*result;
  }
  return result;
}
Vec3 tint(const ParticleProperty &p,const std::array<float,17> &values,const ShaderFunctionTables &tables) {
  const auto argb=shader_function_color(p.curve,property(p,values,tables,true));
  constexpr float inverse=1.f/255.f;
  return {float((argb>>16)&255)*inverse,float((argb>>8)&255)*inverse,float(argb&255)*inverse};
}
Vec3 rotate(const AuthoredBasis &m,Vec3 v) {
  // Inline 175610 arithmetic has a distinct order from out-of-line 143070.
  return {(m.left.x*v.y+m.forward.x*v.x)+m.up.x*v.z,
    (m.forward.y*v.x+m.up.y*v.z)+m.left.y*v.y,
    (m.forward.z*v.x+m.up.z*v.z)+m.left.z*v.y};
}
Vec3 matrix_product(const AuthoredBasis &b,Vec3 v,bool later_column) {
  // 142EB0 sums Y/Z as (up + left) + forward for the left/up columns.
  // The recovered C writes one generic order, losing these SSE distinctions.
  if(later_column)return {(b.forward.x*v.x+b.left.x*v.y)+b.up.x*v.z,
    (b.up.y*v.z+b.left.y*v.y)+b.forward.y*v.x,
    (b.up.z*v.z+b.left.z*v.y)+b.forward.z*v.x};
  return {(b.forward.x*v.x+b.left.x*v.y)+b.up.x*v.z,
    (b.forward.y*v.x+b.left.y*v.y)+b.up.y*v.z,
    (b.forward.z*v.x+b.left.z*v.y)+b.up.z*v.z};
}
void first_person_frame(AuthoredBasis &rotation,Vec3 &position,const ParticleValueSources &source) {
  const AuthoredBasis offset{source.camera_forward,cross(source.camera_up,source.camera_forward),source.camera_up};
  rotation={matrix_product(offset,rotation.forward,false),matrix_product(offset,rotation.left,true),matrix_product(offset,rotation.up,true)};
  position={(position.y*offset.left.x+position.z*offset.up.x)+position.x*offset.forward.x,
    (position.x*offset.forward.y+position.y*offset.left.y)+position.z*offset.up.y,
    (position.x*offset.forward.z+position.y*offset.left.z)+position.z*offset.up.z};
  position={position.x+source.camera_position.x,position.y+source.camera_position.y,position.z+source.camera_position.z};
}
}
ParticleSystemRenderDefinition Cache::load_particle_system_render_definition(std::uint32_t owner,
    std::uint32_t event,unsigned system) const {
  const Bytes b(storage_);
  const auto tag=query_tag({.handle=TagHandle{owner}, .require_address=true});
  if(!tag)throw FormatError("particle system definition owner missing");
  const auto select=[&](std::size_t at,unsigned stride,unsigned index) {
    const auto count=b.u32(at);
    if(count>65536 || index>=count)throw FormatError("particle system definition index exceeds original block");
    return metadata_offset(b.u32(at+4),std::size_t(count)*stride)+std::size_t(index)*stride;
  };
  std::size_t at;
  if(tag->group=="effe") {
    const auto root=metadata_offset(tag->address,0x1c);
    const auto selected=select(root+0x14,0x38,event);
    at=select(selected+0x30,0x38,system);
  }else if(tag->group=="prt3" && event==0xffffffffu) {
    // 1751D0's parent-definition virtual v7 (224240, elements88).
    at=select(metadata_offset(tag->address,0x8c)+0x84,0x38,system);
  }else if(tag->group=="bsdt" && event==0xffffffffu) {
    at=select(metadata_offset(tag->address,0x1c)+0x14,0x38,system);
  }else throw FormatError("particle system definition owner group requires original dispatch");
  ParticleSystemRenderDefinition result;
  result.particle_datum=b.u32(at+4);result.coordinates=std::int16_t(b.u16(at+0xc));
  result.flags=b.u16(at+0x16);
  const auto count=b.u32(at+0x30);
  if(count>65536)throw FormatError("particle emitter definition count exceeds original block");
  const auto emitters=count?metadata_offset(b.u32(at+0x34),std::size_t(count)*0xb8):0;
  for(unsigned n=0;n<count;++n) {
    const auto e=emitters+std::size_t(n)*0xb8;auto &out=result.emitters.emplace_back();
    out.flags=b.u32(e+0xb0);out.required_values=b.u32(e+0xb4);
    const auto read_property=[&](std::size_t p,ParticleProperty &property) {
      property.input=std::int16_t(b.u16(p));property.range=std::int16_t(b.u16(p+2));
      property.modifier=b.u16(p+4);property.modifier_input=std::int16_t(b.u16(p+6));
      const auto size=b.u32(p+8);
      if(size>4096)throw FormatError("particle emitter property function too large");
      if(size) {
        const auto bytes=b.slice(metadata_offset(b.u32(p+0xc),size),size);
        property.curve.assign(bytes.begin(),bytes.end());
      }
    };
    for(unsigned i=0;i<3;++i)read_property(e+0x48+i*0x10,out.properties[i]);
    constexpr std::array<unsigned,5> birth_offsets{0x18,0x28,0x38,0x7c,0x8c};
    for(unsigned i=0;i<birth_offsets.size();++i)read_property(e+birth_offsets[i],out.birth.properties[i]);
    out.birth.shape=b.u32(e+0x78);
    read_property(e+8,out.emission_rate);
    out.offset={b.f32(e+0x9c),b.f32(e+0xa0),b.f32(e+0xa4)};
    out.orientation={b.f32(e+0xa8),b.f32(e+0xac)};
  }
  return result;
}
void for_each_particle_render_input(const ParticleSystemRenderDefinition &definition,
    const ParticleRenderState &state,ParticleRenderValueCache &cache,
    const ShaderFunctionTables &tables,const ParticleRenderCallback &callback) {
  if(state.system_identity!=cache.system_identity) {
    cache.system_identity=state.system_identity;cache.values.evaluated&=0xfffff98fu;
  }
  if(state.location_identity!=cache.location_identity) {
    cache.location_identity=state.location_identity;cache.values.evaluated&=0xfffecf7fu;
  }
  if(state.emitters.size()>definition.emitters.size())throw FormatError("live particle emitter exceeds authored definitions");
  auto source=state.sources;
  for(unsigned n=0;n<state.emitters.size();++n) {
    const auto &emitter=state.emitters[n];const auto &properties=definition.emitters[n];
    if(emitter.particles.empty())continue;
    float multiplier=1.f;
    if(state.uses_camera_scale) {
      if(!state.camera_scale)throw FormatError("particle camera-scale source missing");
      multiplier=*state.camera_scale;
    }
    auto rotation=emitter.rotation;auto origin=emitter.position;
    if(definition.coordinates && state.first_person)first_person_frame(rotation,origin,source);
    complete_particle_values(cache.values,properties.required_values&0x107f0u,source);
    std::array<float,4> color{};float scale=1.f;
    const auto evaluate=[&](unsigned condition) {
      if(condition&1)color[3]=property(properties.properties[2],cache.values.values,tables);
      if(condition&2) {
        const auto c=tint(properties.properties[1],cache.values.values,tables);
        color[0]=c.x;color[1]=c.y;color[2]=c.z;
      }
      if(condition&4)scale=property(properties.properties[0],cache.values.values,tables)*multiplier;
    };
    evaluate(properties.flags&7);
    for(const auto &particle:emitter.particles) {
      source.particle=particle.values;
      if(particle.identity!=cache.particle_identity) {
        cache.particle_identity=particle.identity;cache.values.evaluated&=0xffff07f0u;
      }
      complete_particle_values(cache.values,properties.required_values&0xf80fu,source);
      evaluate((~properties.flags)&7);
      ParticleSpriteInput input;
      input.position=particle.values.position;input.direction=particle.velocity;
      if(definition.coordinates) {
        input.position=rotate(rotation,input.position);
        input.position={origin.x+input.position.x,origin.y+input.position.y,origin.z+input.position.z};
        input.direction=rotate(rotation,input.direction);
      }
      input.values=cache.values.values;input.scale=scale;input.color=color;
      input.random1=particle.values.random[0];input.random2=particle.values.random[1];input.seed_color=particle.seed_color;
      callback(particle.identity,input,cache.values,source);
    }
  }
}
} // namespace halo2
