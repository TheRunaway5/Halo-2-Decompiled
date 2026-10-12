#include "halo2/particle_emitter.hpp"
#include <cmath>

namespace halo2 {
namespace {
float emission_rate(const ParticleProperty &p,const std::array<float,17> &values,
    const ShaderFunctionTables &tables) {
  if(p.input<0 || p.input>=17 || p.range<0 || p.range>=17 || p.modifier_input>=17)
    throw FormatError("particle rate property exceeds original value cache");
  float out=shader_function_value(p.curve,values[p.input],values[p.range],tables);
  if(p.modifier_input>=0) {
    if(p.modifier==1)out=values[p.modifier_input]+out;
    else if(p.modifier==2)out=values[p.modifier_input]*out;
  }
  return out*30.f; // Original 247100.
}
Vec3 rotate_vector(const AuthoredBasis &m,Vec3 v) {
  // Out-of-line 143070 differs from the rendering producer's inline rotation.
  return {(m.up.x*v.z+m.left.x*v.y)+m.forward.x*v.x,
    (m.up.y*v.z+m.forward.y*v.x)+m.left.y*v.y,
    (m.up.z*v.z+m.forward.z*v.x)+m.left.z*v.y};
}
AuthoredBasis orientation_basis(Vec2 angles) {
  // Original 142DA0 with the caller's explicit zero roll; retain products and
  // zero-minus operations for source rounding and signed zero.
  const float cy=float(std::cos(double(angles.x))),sy=float(std::sin(double(angles.x))),
    cp=float(std::cos(double(angles.y))),sp=float(std::sin(double(angles.y))),cr=1.f,sr=0.f;
  const float cc=cy*cr,cs=cy*sr,sc=sy*cr,ss=sy*sr;
  return {{cy*cp,sy*cp,sp},
    {(0.f-cs*sp)-sc,cc-ss*sp,cp*sr},
    {ss-cc*sp,(0.f-sc*sp)-cs,cp*cr}};
}
Vec3 matrix_column(const AuthoredBasis &b,Vec3 v,bool later) {
  if(later)return {(b.forward.x*v.x+b.left.x*v.y)+b.up.x*v.z,
    (b.up.y*v.z+b.left.y*v.y)+b.forward.y*v.x,
    (b.up.z*v.z+b.left.z*v.y)+b.forward.z*v.x};
  return {(b.forward.x*v.x+b.left.x*v.y)+b.up.x*v.z,
    (b.left.y*v.y+b.forward.y*v.x)+b.up.y*v.z,
    (b.left.z*v.y+b.forward.z*v.x)+b.up.z*v.z};
}
}
void update_particle_emitter(const ParticleSystemRenderDefinition &system,unsigned index,
    ParticleEmitterState &state,std::uint16_t system_flags,float dt,
    const ParticleEmitterFrame *frame,const std::array<float,17> &values,std::uint32_t color,
    const ShaderFunctionTables &tables,const ParticleEmissionCallback &callback) {
  if(index>=system.emitters.size())throw FormatError("particle emitter update exceeds authored definitions");
  const auto &definition=system.emitters[index];
  float scale=1.f;state.previous_position=state.position;
  if(frame) {
    if(system_flags&0x400)scale=frame->scale;
    state.rotation=frame->rotation;
    state.position={frame->position.x*scale,frame->position.y*scale,frame->position.z*scale};
    const auto offset=rotate_vector(state.rotation,definition.offset);
    if(!(std::fabs(double(definition.orientation.y))<double(.0001f)) ||
       !(std::fabs(double(definition.orientation.x))<double(.0001f))) {
      const auto local=orientation_basis(definition.orientation),base=state.rotation;
      state.rotation={matrix_column(base,local.forward,false),matrix_column(base,local.left,true),matrix_column(base,local.up,true)};
    }
    state.position={state.position.x+offset.x,state.position.y+offset.y,state.position.z+offset.z};
  }
  if(!(system.flags&4) || state.first_particle==0xffffffffu)
    state.emission_remainder=emission_rate(definition.emission_rate,values,tables)*dt+state.emission_remainder;
  if(state.emission_remainder+.0001f>=1.f) {
    float fraction=1.f,increment=0.f;
    if((system_flags&0x200) && (system.flags&0x80)) {
      fraction=0.f;increment=1.f/state.emission_remainder;
    }else dt=0.f;
    while(state.emission_remainder+.0001f>=1.f) {
      state.emission_remainder-=1.f;
      callback({fraction,dt,scale,color},state);
      fraction+=increment;
    }
  }
}
} // namespace halo2
