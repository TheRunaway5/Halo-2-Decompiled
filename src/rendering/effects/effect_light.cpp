#include "halo2/effect_light.hpp"
#include "halo2/kernels.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/shader_animation.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
namespace halo2 {
EffectLightDefinition source_effect_light_definition(std::span<const std::byte> raw){
  const Bytes p(raw);p.require(0,228);
  if(p.u16(4)!=0)throw FormatError("Effect light requires original frustum shape owner");
  if(p.u32(0x34)!=0)throw FormatError("Effect light requires original HSV interpolation owner");
  if(p.u32(0xc4)||p.u32(0xcc)||p.u32(0xd4))throw FormatError("Effect light requires original animation owner");
  if(p.u16(0xb6)>=16)throw FormatError("Effect light fade curve escapes its original table bank");
  EffectLightDefinition out;std::copy_n(raw.begin(),228,out.bytes.begin());
  out.flags=p.u32(0);out.lens=p.u32(0x94);out.radius=p.f32(0x18);out.specular_radius=p.f32(0x1c);out.duration=p.f32(0xb0);
  return out;
}
EffectLightDefinition load_effect_light(const Cache &cache,std::uint32_t datum){
  const auto p=cache.load_source_tag_metadata(datum);
  if(p.group()!="ligh")throw FormatError("Effect light reference is not ligh");
  auto definition=source_effect_light_definition(p.bytes_at(p.source_address(),228));
  const auto type=Bytes(definition.bytes).u16(0xb6);
  if(type && type!=5) {
    const auto tables=cache.shader_function_tables();
    definition.timed_curve={tables,&tables->transition[type]};
  }
  return definition;
}
EffectLightState source_effect_light_state(const EffectLightDefinition &d,float control,
    std::int32_t elapsed,std::int16_t tick_rate,std::span<const std::uint8_t,1024> curve){
  if(!std::isfinite(control)||tick_rate<=0)throw FormatError("Invalid effect light clock/control");
  const Bytes p(d.bytes);EffectLightState out;
  if(d.duration>0){
    const float duration=float(tick_rate)*d.duration;
    const float fraction=float(elapsed)/duration;
    const auto type=p.u16(0xb6);
    if(type && type!=5 && !d.timed_curve)throw FormatError("Effect light requires its actual selected transition table");
    const float fade=type?fade_curve_fraction(fraction,d.timed_curve?
        std::span<const std::uint8_t,1024>(*d.timed_curve):curve):std::clamp(fraction,0.f,1.f);
    out.control=(1.f-fade)*control;
    const auto color=[&](unsigned low,unsigned high){
      const float a=p.f32(low)*(1.f-out.control),b=p.f32(high)*out.control;
      return std::clamp(a+b,0.f,1.f);
    };
    out.diffuse={color(0x58,0x64),color(0x5c,0x68),color(0x60,0x6c)};
    out.specular={color(0x40,0x4c),color(0x44,0x50),color(0x48,0x54)};
    const float low=(1.f-out.control)*p.f32(0x70),high=p.f32(0x74)*out.control;
    out.brightness=low+high;
  }else{
    out.control=control;
    out.diffuse={p.f32(0x64),p.f32(0x68),p.f32(0x6c)};
    out.specular={p.f32(0x4c),p.f32(0x50),p.f32(0x54)};
    out.brightness=p.f32(0x74);
  }
  // C0C80 clamps both timed interpolation and untimed authored endpoints.
  for(auto* color:{&out.diffuse,&out.specular}) {
    color->x=std::clamp(color->x,0.f,1.f);color->y=std::clamp(color->y,0.f,1.f);color->z=std::clamp(color->z,0.f,1.f);
  }
  out.brightness=std::clamp(out.brightness*2.f,0.f,4.f);
  const float low=(1.f-out.control)*p.f32(8),high=p.f32(12)*out.control;
  out.size=low+high;
  return out;
}
EffectLightUniforms source_effect_light_uniforms(const EffectLightUniformInput &in){
  if(!std::isfinite(in.radius) || in.radius<=0 || !std::isfinite(in.brightness))
    throw FormatError("Invalid spherical effect-light radius/brightness");
  EffectLightUniforms out{in.previous};auto &c=out.constants;
  const float inverse=-1.f/in.radius;
  c[0]={in.position.x,in.position.y,in.position.z,1};
  std::copy_n(c[0].begin(),3,c[1].begin());
  c[2]={in.left.x*inverse,in.left.y*inverse,in.left.z*inverse,0};
  c[3]={in.up.x*inverse,in.up.y*inverse,in.up.z*inverse,0};
  c[4]={in.forward.x*inverse,in.forward.y*inverse,in.forward.z*inverse,inverse*.5f};
  c[5]={0,0,0,0};out.gel_basis_u=c[2];out.gel_basis_v=c[3];
  for(unsigned i=0;i<3;++i){c[2][i]=c[4][i]*in.gel_u+c[2][i];c[3][i]=c[4][i]*in.gel_v+c[3][i];}
  const std::array<float,3> diffuse{in.diffuse.x*in.brightness,in.diffuse.y*in.brightness,in.diffuse.z*in.brightness};
  const std::array<float,3> specular{in.specular.x*in.brightness,in.specular.y*in.brightness,in.specular.z*in.brightness};
  const float maximum=std::max(*std::max_element(diffuse.begin(),diffuse.end()),*std::max_element(specular.begin(),specular.end()));
  out.color_shift=maximum<=1?0:maximum<=2?1:2;
  const float scale=out.color_shift==0?1.f:out.color_shift==1?.5f:.25f;
  for(unsigned i=0;i<3;++i){c[6][i]=std::clamp(diffuse[i]*scale,0.f,1.f);c[7][i]=std::clamp(specular[i]*scale,0.f,1.f);}
  return out;
}
EffectLightViewFades source_effect_light_view_fades(const EffectLightDefinition &definition,
    Vec3 position,float radius,Vec3 camera_position){
  for(float v:{position.x,position.y,position.z,radius,camera_position.x,camera_position.y,camera_position.z})
    if(!std::isfinite(v))throw FormatError("Invalid effect light view-fade input");
  const Bytes p(definition.bytes);
  // Literal source banks4405F0..440640; each class indexes five distances.
  constexpr std::array<std::array<float,5>,5> distance{{{20,15,10,5,0},
    {15,10,7,3,0},{2.5f,2,1.5f,1,.5f},{2.5f,2,1.5f,1,.5f},{0,0,0,0,0}}};
  const auto fade=[&](unsigned bank,unsigned class_offset,float fade_range){
    const auto kind=bank==4?0:std::int16_t(p.u16(class_offset));
    if(kind<0||kind>=5)throw FormatError("Invalid original light distance class");
    // C0A50 keeps position subtraction, squares, sqrt and two subtractions
    // in x87 PC53 before its single float store and SSE division/clamping.
    const double x=double(position.x)-camera_position.x,y=double(position.y)-camera_position.y,
      z=double(position.z)-camera_position.z;
    const float value=float((std::sqrt((z*z+x*x)+y*y)-radius)-distance[bank][kind]);
    const float fraction=value/std::max(fade_range,.1f);
    return 1.f-std::clamp(fraction,0.f,1.f);
  };
  EffectLightViewFades out;
  out.overall=fade(0,0xb8,10);out.diffuse=fade(1,0xba,5);
  out.specular=fade(2,0xba,radius);out.secondary=fade(3,0xbc,1);
  out.volume=fade(4,0,radius*.5f);
  if(out.overall<out.diffuse)out.diffuse=out.overall;
  if(out.diffuse<out.secondary)out.secondary=out.diffuse;
  return out;
}
EffectLightSphere source_effect_light_sphere(const EffectLightDefinition &definition,
    Vec3 position,Vec3 forward,Vec3 up,float size){
  const Bytes p(definition.bytes);
  if(p.u16(4)!=0)throw FormatError("Effect light sphere requires authored shape0");
  EffectLightSphere out;
  const auto put=[&](unsigned index,float value){out.shape[index]=std::bit_cast<std::uint32_t>(value);};
  put(1,definition.radius);put(2,definition.specular_radius);
  out.diffuse_radius=definition.radius*size;out.specular_radius=definition.specular_radius*size;
  out.radius=out.specular_radius<out.diffuse_radius?out.diffuse_radius:out.specular_radius;
  out.left={forward.y*up.z-forward.z*up.y,forward.z*up.x-up.z*forward.x,
    up.y*forward.x-forward.y*up.x};
  put(7,out.left.x);put(8,out.left.y);put(9,out.left.z);
  put(11,position.x);put(12,position.y);put(13,position.z);
  put(15,out.diffuse_radius);put(16,out.specular_radius);put(17,out.radius);
  out.shape[6]=1;
  // C28B0 deliberately uses the authored maximum size, not the tick's size.
  float bound=definition.radius;
  if(!(definition.flags&2))bound=definition.specular_radius<bound?bound:definition.specular_radius;
  bound=p.f32(0xc)*bound;
  out.spatial_radius=bound<=p.f32(0x98)?p.f32(0x98):bound;
  return out;
}
EffectLightUniforms source_effect_light_view_uniforms(const EffectLightDefinition &definition,
    const EffectLightState &state,Vec3 position,Vec3 forward,Vec3 up,
    const EffectLightViewFades &fades,float selected_fade,
    const std::array<ShaderVector,9> &previous){
  const auto sphere=source_effect_light_sphere(definition,position,forward,up,state.size);
  const float diffuse=fades.overall*selected_fade;
  const float specular=fades.secondary<=fades.overall?fades.secondary:fades.overall;
  return source_effect_light_uniforms({position,forward,up,sphere.left,sphere.radius,
    {state.diffuse.x*diffuse,state.diffuse.y*diffuse,state.diffuse.z*diffuse},
    {state.specular.x*specular,state.specular.y*specular,state.specular.z*specular},
    state.brightness,0,0,previous});
}
}
