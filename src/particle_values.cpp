#include "halo2/particle_values.hpp"
#include <bit>
#include <cmath>
#include <limits>

namespace halo2 {
namespace {
float fraction(float value) {
  // SSE CVTTSS2SI returns the integer-indefinite value on overflow/NaN.
  const auto whole=!std::isfinite(value) || value>=2147483648.f || value< -2147483648.f?
    std::numeric_limits<std::int32_t>::min():std::int32_t(value);
  const float out=value-float(whole);
  return out<0.f?out+1.f:out;
}
Vec3 normalize_source(Vec3 v) {
  const float squared=(v.x*v.x+v.y*v.y)+v.z*v.z;
  if(squared!=0.f) {
    // 4EFB0 retains the x87 reciprocal through each component multiply;
    // it never stores the reciprocal as a float before those products.
    const double inverse=1.0/std::sqrt(double(squared));
    v={float(double(v.x)*inverse),float(double(v.y)*inverse),float(double(v.z)*inverse)};
  }
  return v;
}
}
float particle_source_value(unsigned index,const ParticleValueSources &s) {
  const auto *p=s.particle?&*s.particle:nullptr;
  const auto *system=s.system?&*s.system:nullptr;
  const auto *location=s.location?&*s.location:nullptr;
  switch(index) {
    case 0:return p?p->field08:0.f;
    case 1:return p?p->field10:0.f;
    case 2:case 3:return p?float(p->random[index-2])*(1.f/65535.f):0.f;
    case 14:case 15:return p?float(p->random[index-12])*(1.f/65535.f):0.f;
    case 4:return system?system->field04:0.f;
    case 5:return system?system->random_a:0.f;
    case 6:return system?system->random_b:0.f;
    case 7:return location?location->field1c:0.f;
    case 8:return float(s.game_tick)*s.game_rate;
    case 9:case 10:
      if(!system)throw FormatError("original particle effect-scale query requires a system");
      return system->effect_scales?(*system->effect_scales)[index-9]:0.f;
    case 11:return p?fraction(p->field38):0.f;
    case 12:case 13:
      if(p && location) {
        Vec3 direction{p->position.x-s.camera_position.x,p->position.y-s.camera_position.y,p->position.z-s.camera_position.z};
        Vec3 axis=s.camera_forward;
        if(index==13)axis={
          s.camera_up.y*s.camera_forward.z-s.camera_up.z*s.camera_forward.y,
          s.camera_up.z*s.camera_forward.x-s.camera_up.x*s.camera_forward.z,
          s.camera_up.x*s.camera_forward.y-s.camera_up.y*s.camera_forward.x};
        direction=normalize_source(direction);
        return fraction(((direction.z*axis.z+direction.y*axis.y)+axis.x*direction.x)*p->field38);
      }
      return 0.f;
    case 16:return location?float(location->field03)*(1.f/255.f):0.f;
    default:return 0.f;
  }
}
void complete_particle_values(ParticleValueCache &cache,std::uint32_t requested,const ParticleValueSources &s) {
  if(requested&~0x1ffffu)throw FormatError("particle value request exceeds original 17-slot cache");
  auto missing=requested&~cache.evaluated;
  while(missing) {
    const unsigned index=std::countr_zero(missing);
    missing&=missing-1;
    cache.values[index]=particle_source_value(index,s);
  }
  cache.evaluated|=requested;
}
} // namespace halo2
