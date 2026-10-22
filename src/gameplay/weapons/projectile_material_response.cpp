#include "halo2/projectile_material_response.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cstring>
#include <cmath>
#include <limits>
#include <vector>

namespace halo2 {
float source_projectile_vector_angle(Vec3 a,Vec3 b){
  const float l=(b.x*b.x+b.y*b.y+b.z*b.z)*(a.x*a.x+a.y*a.y+a.z*a.z);
  float result=0;
  if(l!=0){
    const float d=b.z*a.z+b.y*a.y+b.x*a.x;
    float c=d/l*d*2.f-1.f;
    c=c < -1.f ? -1.f : c;
    c=c > 1.f ? 1.f : c;
    // The original CRT returns x87 ST0; its half multiplication precedes
    // the binary32 store. Do not round acos to float before multiplying.
    result=float(std::acos(double(c))*.5);
    if(d<0)result=std::bit_cast<float>(0x40490fdbu)-result;
  }
  return result;
}
std::optional<unsigned> source_projectile_material_effect_slot(std::int16_t response){
  if(response>=0&&response<=4)return 15u+unsigned(response);
  if(response==5||response==6)return 20u;
  return {};
}
namespace {
template<class T>T get(std::span<const std::byte> data,std::size_t offset){
  if(offset>data.size()||sizeof(T)>data.size()-offset)
    throw FormatError("Projectile material response backing is truncated");
  T value;std::memcpy(&value,data.data()+offset,sizeof value);return value;
}
}
SourceProjectileImpactAngles source_projectile_impact_angles(
    std::span<const std::byte> row,Vec3 normal,Vec3 direction,Vec3 velocity,
    const SourceProjectileResponseServices&s){
  if(row.size()!=0x58)throw FormatError("Projectile response row must retain88 source bytes");
  const auto random_fraction=[&]{
    if(!s.rng)throw FormatError("Original projectile impact noise RNG owner unavailable");
    auto&state=s.rng();state=state*0x19660du+0x3c6ef35fu;
    return double(state>>16)*double(s.rng_scale);
  };
  float approach=0.f-(normal.z*velocity.z+normal.y*velocity.y+velocity.x*normal.x);
  const auto velocity_noise=get<float>(row,0x3c);
  // TU1.5 FFF02..FFF37 keeps the random fraction, multiplication and
  // subtraction in x87 PC53 until the final binary32 store.
  if(velocity_noise>0.f)
    approach=float(double(approach)-random_fraction()*double(velocity_noise));
  float angle=source_projectile_vector_angle(normal,direction)-std::bit_cast<float>(0x3fc90fdbu);
  const auto angular_noise=get<float>(row,0x38);
  if(angular_noise>0.f){
    const double low=-double(angular_noise),high=double(angular_noise);
    const double fraction=random_fraction();
    angle=float((high-low)*fraction+low+double(angle));
  }
  return {angle,approach};
}
float source_projectile_contact_damage_scale(std::uint32_t flags,Vec3 position,
    Vec3 contact,float traveled,float damage_range,float stored_scale){
  if(!(flags&16u))return stored_scale;
  // TU1.5 FFBC5..FFC01: x87 PC53 differences and length (x*x+z*z+y*y),
  // sqrt, flight distance, division and subtraction precede a binary32
  // store. The stored factor then multiplies the scale with MULSS.
  const double x=double(contact.x)-double(position.x),
      y=double(contact.y)-double(position.y),z=double(contact.z)-double(position.z);
  const float remaining=float(1.-(std::sqrt(x*x+z*z+y*y)+double(traveled))/double(damage_range));
  const float scale=stored_scale*remaining;
  return scale<0.f?0.f:scale>1.f?1.f:scale;
}
float source_projectile_effect_scale(std::span<const std::byte> row,float angle,float damage_scale){
  const auto mode=get<std::int16_t>(row,0x34);
  float value=mode==0?damage_scale:mode==1?angle*std::bit_cast<float>(0x3f22f983u):1.f;
  return value<0?0:value>1?1:value;
}
SourceProjectileImpactPresentation source_projectile_default_impact_presentation(
    Vec3 normal,Vec3 velocity,float damage_scale,float object_scale){
  const auto clamp=[](float value){return value<0.f?0.f:value>1.f?1.f:value;};
  const auto approach=0.f-(normal.z*velocity.z+normal.y*velocity.y+velocity.x*normal.x);
  return {0,clamp(damage_scale),clamp(object_scale),approach>.25f};
}
SourceProjectileDamagePresentation source_projectile_damage_presentation(
    const SourceProjectileDamageInputs&in){
  SourceProjectileDamagePresentation out{in.body_material,0.f};
  // TU1.5 DA3CE..DA40D, before the vitality consumers.
  if(in.amount==0.f && in.before_shield>0.f && !in.region_absorbs)
    out={in.shield_material,in.before_shield};
  // DA6DD..DA769: these are the normalized requests, including overflow,
  // rather than the amount of vitality actually lost. Ties select body.
  const auto threshold=std::bit_cast<float>(0x38d1b717u);
  if(in.shield_request>threshold || in.region_request>threshold || in.body_request>threshold){
    const auto other=in.body_request>in.region_request?in.body_request:in.region_request;
    if(in.shield_request>other)out={in.shield_material,in.shield};
    else out.scale=in.body<0.f?0.f:in.body>1.f?1.f:in.body;
  }
  return out;
}
std::optional<std::size_t> source_projectile_material_response(
    std::span<const std::byte> rows,std::span<const std::int16_t> parents,
    std::int16_t material){
  if(rows.size()%0x58||rows.size()/0x58>std::size_t(std::numeric_limits<std::int32_t>::max())||
      parents.size()>std::size_t(std::numeric_limits<std::int32_t>::max()))
    throw FormatError("Invalid projectile material response bank");
  // The executable does not guard malformed cycles. This owned-data adapter
  // rejects them without inventing a fallback/default response.
  std::vector<bool> visited(parents.size());
  while(material>=0&&std::size_t(material)<parents.size()){
    const auto index=std::size_t(material);
    if(visited[index])throw FormatError("Cyclic projectile material parent bank");
    visited[index]=true;
    for(std::size_t i=0;i<rows.size()/0x58;++i)
      if(get<std::int16_t>(rows,i*0x58+0x10)==material)return i;
    material=parents[index];
  }
  return std::nullopt;
}
std::int16_t choose_source_projectile_response(std::span<const std::byte> row,
    const SourceProjectileResponseInputs&in,const SourceProjectileResponseServices&s){
  if(row.size()!=0x58)throw FormatError("Projectile response row must retain88 source bytes");
  const auto chance=get<float>(row,0x18);
  bool eligible=chance>0.f;
  const auto angle_high=get<float>(row,0x20);
  // UCOMISS/LAHF/TEST/JNP skips signed zero only. COMISS/JB rejects
  // unordered lower/upper comparisons exactly like an out-of-range value.
  if(angle_high!=0.f){
    if(eligible){if(!in.angle)throw FormatError("Original projectile impact angle owner unavailable");
      eligible=*in.angle>=get<float>(row,0x1c)&&angle_high>=*in.angle;}
  }
  const auto speed_high=get<float>(row,0x28);
  if(speed_high!=0.f){
    if(eligible){if(!in.speed)throw FormatError("Original projectile impact speed owner unavailable");
      eligible=*in.speed>=get<float>(row,0x24)&&speed_high>=*in.speed;}
  }
  const auto flags=get<std::uint16_t>(row,0x16);
  auto unit_hit=[&]{if(in.collision_type!=4)return false;
    if(!s.object_type)throw FormatError("Original projectile target type owner unavailable");
    return ((1u<<(s.object_type()&31u))&3u)!=0;
  };
  if((flags&2)&&eligible)eligible=!unit_hit();
  if((flags&1)&&eligible)eligible=unit_hit();
  // COMISS one,chance/JBE skips the RNG branch for >=one and unordered.
  if(s.one>chance&&eligible){
    if(!s.rng)throw FormatError("Original projectile response RNG owner unavailable");
    auto&state=s.rng();state=state*0x19660du+0x3c6ef35fu;
    // FILD high16 / FMUL under the measured x87 PC53 context. Both
    // operands fit exactly in binary64; there is no binary32 store before
    // the original comparison. The high16 value never needs correction.
    volatile double random=double(state>>16)*double(s.rng_scale);
    eligible=double(chance)>random;
  }
  return get<std::int16_t>(row,eligible?0x14:2);
}
} // namespace halo2
