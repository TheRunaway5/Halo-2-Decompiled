#include "halo2/pickup_visibility.hpp"
#include "halo2/item_point.hpp"
#include <bit>
#include <algorithm>
#include <cmath>

namespace halo2 {
namespace {
float literal(std::uint32_t bits) {return std::bit_cast<float>(bits);}
Vec3 delta(Vec3 end,Vec3 start) {return {end.x-start.x,end.y-start.y,end.z-start.z};}
Vec3 shifted(Vec3 point,Vec3 side,float distance) {
  const float x=side.x*distance,y=side.y*distance,z=side.z*distance;
  return {x+point.x,y+point.y,z+point.z};
}
bool admitted_types(std::uint8_t unit_type,std::uint8_t item_type) {
  return ((1u<<(unit_type&31))&1) && ((1u<<(item_type&31))&0x1c);
}
}
PickupCapsuleDefinition Cache::load_pickup_capsule_definition(std::uint32_t datum) const {
  const auto tag=query_tag({.handle=TagHandle{datum}});
  if(!tag || tag->group!="bipd")throw FormatError("Pickup capsule requires actual biped definition");
  if(tag->size<0x274)throw FormatError("Truncated pickup biped metadata");
  const auto root=metadata_offset(tag->address,0x274);const Bytes fields(storage_);
  return {fields.u32(root+0x264),fields.f32(root+0x268),fields.f32(root+0x26c),fields.f32(root+0x270)};
}
PickupCapsule pickup_capsule(Vec3 root,const PickupCapsuleDefinition &definition,float crouch) {
  PickupCapsule result{root,0,definition.radius};
  if(!(definition.flags&1))result.bottom.z=definition.radius+root.z;
  if(!(definition.flags&2)) {
    const float difference=definition.crouching-definition.standing;
    const float product=difference*crouch;
    const float height=product+definition.standing;
    const float twice=definition.radius*literal(0x40000000);
    result.extent=height-twice;
  }
  return result;
}
PickupNormalizer pickup_side_normalizer(Vec3 vector) {
  PickupNormalizer result;result.input=result.output=vector;
  const double x=double(vector.x)*double(vector.x),y=double(vector.y)*double(vector.y),
      z=double(vector.z)*double(vector.z);
  const double xy=x+y,sum=xy+z;
  result.length=std::sqrt(sum);result.length_spill=float(result.length);
  if(double(literal(0x38d1b717))>std::abs(result.length))return result;
  result.returned=result.length_spill;
  const float inverse=literal(0x3f800000)/result.length_spill;result.inverse=inverse;
  result.output={vector.x*inverse,vector.y*inverse,vector.z*inverse};
  return result;
}
PickupVisibilityResult pickup_visibility(const PickupCapsule &capsule,Vec3 point,
    std::uint32_t unit,std::uint32_t item,std::uint8_t unit_type,std::uint8_t item_type,
    const PickupVisibilityQuery &query) {
  PickupVisibilityResult result;
  if(!admitted_types(unit_type,item_type))return result;
  auto cast=[&](Vec3 start,Vec3 endpoint,std::uint32_t mask) {
    const PickupVisibilityCast request{start,delta(endpoint,start),mask,unit,item};
    result.casts.push_back(request);return query(request);
  };
  auto top=capsule.bottom;top.z=top.z+capsule.extent;
  if(!cast(top,point,0x14800001).hit || !cast(capsule.bottom,point,0x14800001).hit)return result;
  const auto difference=delta(point,capsule.bottom);
  const Vec3 up{0,0,1};
  // Keep the original multiply/subtract order, including signed zero.
  const float yz=up.y*difference.z,zx=difference.z*up.x,zy=up.z*difference.y,
      yx=difference.y*up.x,zz=up.z*difference.x,xy=up.y*difference.x;
  result.normalizer=pickup_side_normalizer({zy-yz,zx-zz,xy-yx});
  result.side=result.normalizer->returned==0?Vec3{0,-1,0}:result.normalizer->output;
  const auto bottom_positive=shifted(capsule.bottom,result.side,literal(0x3d99999a));
  const auto bottom_negative=shifted(capsule.bottom,result.side,literal(0xbd99999a));
  const auto top_positive=shifted(top,result.side,literal(0x3d99999a));
  const auto top_negative=shifted(top,result.side,literal(0xbd99999a));
  result.positive_target=shifted(point,result.side,literal(0x3d4ccccd));
  const float x=result.side.x*literal(0x3d4ccccd),y=result.side.y*literal(0x3d4ccccd),
      z=result.side.z*literal(0x3d4ccccd);
  result.negative_target={point.x-x,point.y-y,point.z-z};
  // The original clips from the item center toward each shifted endpoint.
  auto positive=cast(point,result.positive_target,0x24800001);
  if(positive.hit)result.positive_target=positive.point;
  auto negative=cast(point,result.negative_target,0x24800001);
  if(negative.hit)result.negative_target=negative.point;
  if(!cast(top_positive,result.positive_target,0x14800001).hit ||
      !cast(top_negative,result.negative_target,0x14800001).hit ||
      !cast(bottom_positive,result.positive_target,0x14800001).hit ||
      !cast(bottom_negative,result.negative_target,0x14800001).hit)return result;
  result.visible=false;return result;
}
PickupVisibilityResult source_item_pickup_visibility(const PickupCapsule &capsule,
    const ItemPointDefinition &definition,const ItemPointState &item,const RawCollisionBsp &collision,
    std::uint32_t unit,std::uint8_t unit_type,std::uint8_t item_type,
    const PickupVisibilityQuery &query) {
  // C57C8/C57E5 precede FBFA0. A rejected type must never request an item point.
  if(!admitted_types(unit_type,item_type))return {};
  const auto point=source_item_point(definition,item,collision);
  // C5845 consumes the output without testing EAX: a root fallback is valid.
  return pickup_visibility(capsule,point.point,unit,item.datum,unit_type,item_type,query);
}
}
