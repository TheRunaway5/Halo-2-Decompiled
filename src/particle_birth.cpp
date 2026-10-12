#include "halo2/particle_birth.hpp"
#include <cmath>

namespace halo2 {
namespace {
float property(const ParticleProperty &p,const std::array<float,17> &values,
    const ShaderFunctionTables &tables) {
  if(p.input<0 || p.input>=17 || p.range<0 || p.range>=17 || p.modifier_input>=17)
    throw FormatError("particle birth property exceeds original value cache");
  float out=shader_function_value(p.curve,values[p.input],values[p.range],tables);
  if(p.modifier_input>=0) {
    if(p.modifier==1)out=values[p.modifier_input]+out;
    else if(p.modifier==2)out=values[p.modifier_input]*out;
  }
  return out;
}
std::uint32_t random_next(std::uint32_t &seed) {
  seed=seed*1664525u+1013904223u;
  return seed;
}
Vec3 random_direction(std::uint32_t &seed,const OriginalData &data) {
  return data.weapon_directions[((random_next(seed)>>16)*1026u)>>16];
}
Vec3 scale(Vec3 v,float factor) {
  return {factor*v.x,v.y*factor,v.z*factor};
}
}
ParticleBirthState initialize_particle_birth(const ParticleBirthDefinition &definition,
    const std::array<float,17> &values,std::uint32_t &seed,
    const OriginalData &data,const ShaderFunctionTables &tables) {
  if(definition.shape>5)
    throw FormatError("particle birth shape requires original initialization evidence");
  ParticleBirthState out;
  const float life=property(definition.properties[0],values,tables);
  out.inverse_lifetime=1.f/(life>.01f?life:.01f);
  out.rotation_rate=property(definition.properties[2],values,tables)*(1.f/360.f);
  if(definition.shape==0 || definition.shape==5) {
    // TU1.1 24B294..24B352: position stays zero. Two local-bank draws
    // produce azimuth and polar, retaining x87 products through sin/cos.
    // The actual SMG's authored angle is zero; signed sine/cosine products
    // and both RNG draws still belong to the original successful birth.
    const double azimuth_fraction=double(random_next(seed)>>16)*double(1.f/65535.f);
    const float azimuth=float(azimuth_fraction*double(6.2831855f));
    const float maximum_polar=property(definition.properties[4],values,tables)*.017453292f;
    const double polar_fraction=double(random_next(seed)>>16)*double(1.f/65535.f);
    const double polar=polar_fraction*double(maximum_polar);
    const double radial=std::sin(polar);
    out.velocity={float(std::cos(polar)),float(std::sin(double(azimuth))*radial),
      float(radial*std::cos(double(azimuth)))};
    if(definition.shape==5) {
      // This branch stores polar before FCOS and radial before either
      // azimuth product; the zero-origin cone keeps the x87 intermediates.
      const float stored_radial=float(radial);
      out.velocity={float(std::cos(double(float(polar)))),
        float(std::sin(double(azimuth))*double(stored_radial)),
        float(std::cos(double(azimuth))*double(stored_radial))};
      // Original249170/24B240's offset-cone branch then samples a third
      // random value for a table direction on the authored radius sphere.
      const float radius=property(definition.properties[3],values,tables);
      out.position=scale(random_direction(seed,data),radius);
    }
  }else if(definition.shape==1) {
    // Original249170's ring branch uses one draw. Sine/cosine are stored
    // before the position products; angle is not an input to this branch.
    const double fraction=double(random_next(seed)>>16)*double(1.f/65535.f);
    const float azimuth=float(fraction*double(6.2831855f));
    const float radius=property(definition.properties[3],values,tables);
    const float sine=float(std::sin(double(azimuth)));
    const float cosine=float(std::cos(double(azimuth)));
    out.velocity={0.f,sine,cosine};
    out.position={0.f,sine*radius,cosine*radius};
  }else if(definition.shape==2) {
    const float radius=property(definition.properties[3],values,tables);
    out.velocity=random_direction(seed,data);
    out.position=scale(out.velocity,radius);
  }else if(definition.shape==3) {
    const Vec3 direction=random_direction(seed,data);
    const float radius=property(definition.properties[3],values,tables);
    out.position=scale(direction,radius);
    // Original uses zero minus each component, preserving zero-sign behavior.
    out.velocity={0.f-direction.x,0.f-direction.y,0.f-direction.z};
  }else {
    // Inlined x87 draws retain the fraction through the 2*pi multiply before
    // storing azimuth as float. A float fraction temporary loses that order.
    const double fraction=double(random_next(seed)>>16)*double(1.f/65535.f);
    const float azimuth=float(fraction*double(6.2831855f));
    const float polar=property(definition.properties[4],values,tables)*.017453292f;
    const float radius=property(definition.properties[3],values,tables);
    const float sine=float(std::sin(double(azimuth))),cosine=float(std::cos(double(azimuth)));
    out.position={0.f,sine*radius,cosine*radius};
    const double radial=std::sin(double(polar));
    out.velocity={float(std::cos(double(polar))),float(double(sine)*radial),float(radial*double(cosine))};
  }
  // Executable 2499D0..2499E3 stores Y*Y in XMM3 and Z*Z in XMM2.
  const float squared=(out.velocity.x*out.velocity.x+out.velocity.y*out.velocity.y)+out.velocity.z*out.velocity.z;
  if(squared!=0.f) {
    // Observed Xemu RSQRTSS rounds sqrt to binary32 before its reciprocal.
    // A double reciprocal sqrt differs for 28 of the 128 live observations.
    const float root=std::sqrt(squared);
    out.velocity=scale(out.velocity,1.f/root);
  }
  out.velocity=scale(out.velocity,property(definition.properties[1],values,tables));
  return out;
}
} // namespace halo2
