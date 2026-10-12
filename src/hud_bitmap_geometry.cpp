#include "halo2/hud_bitmap_geometry.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
namespace {
float round_float(float value) { volatile float result=value; return result; }
float product(float a,float b) { return round_float(a*b); }
float sum(float a,float b) { return round_float(a+b); }
float difference(float a,float b) { return round_float(a-b); }
}
HudBitmapQuads source_hud_bitmap_geometry(const HudBitmapGeometryInput &in,
                                         const HudBitmapGeometryConstants &constants) {
  const auto &uv=in.sprite_bounds;
  const float reg_x=sum(product(difference(uv[1],uv[0]),in.registration[0]),uv[0]);
  const float reg_y=sum(product(difference(uv[3],uv[2]),in.registration[1]),uv[2]);
  const float left=-std::floor(product(difference(reg_x,uv[0]),in.bitmap_width));
  const float right=std::ceil(product(difference(uv[1],reg_x),in.bitmap_width));
  const float top=-std::floor(product(difference(reg_y,uv[2]),in.bitmap_height));
  const float bottom=std::ceil(product(difference(uv[3],reg_y),in.bitmap_height));
  HudBitmapQuad base{};
  base.sprite_bounds=uv;
  for(unsigned n=0;n<4;++n) {
    const float x=(n==1 || n==2)?right:left;
    const float y=n<2?top:bottom;
    base.positions[n][0]=sum(sum(product(in.horizontal_axis[0],x),product(in.vertical_axis[0],y)),in.anchor[0]);
    base.positions[n][1]=sum(sum(product(in.horizontal_axis[1],x),product(in.vertical_axis[1],y)),in.anchor[1]);
  }
  if(in.flags&1)std::swap(base.sprite_bounds[0],base.sprite_bounds[1]);
  if(in.flags&2)std::swap(base.sprite_bounds[2],base.sprite_bounds[3]);
  if(in.flags&0x10) {
    const float width=difference(base.positions[1][0],base.positions[0][0]);
    const float height=difference(base.positions[2][1],base.positions[1][1]);
    for(unsigned n=0;n<4;++n) {
      if(n!=1 && n!=2)base.positions[n][0]=difference(base.positions[n][0],product(width,constants.stretch));
      if(n<2)base.positions[n][1]=difference(base.positions[n][1],product(height,constants.stretch));
    }
    base.sprite_bounds[0]=difference(base.sprite_bounds[0],product(difference(base.sprite_bounds[1],base.sprite_bounds[0]),constants.stretch));
    base.sprite_bounds[2]=difference(base.sprite_bounds[2],product(difference(base.sprite_bounds[3],base.sprite_bounds[2]),constants.stretch));
  }
  HudBitmapQuads out{};
  out.quads[out.count++]=base;
  const float width=difference(base.positions[1][0],base.positions[0][0]);
  const float height=difference(base.positions[2][1],base.positions[1][1]);
  auto mirrored=[&](bool horizontal,bool vertical) {
    auto quad=base;
    for(unsigned n=0;n<4;++n) {
      if(horizontal && n!=1 && n!=2)quad.positions[n][0]=sum(product(width,constants.mirror),quad.positions[n][0]);
      if(vertical && n<2)quad.positions[n][1]=sum(product(height,constants.mirror),quad.positions[n][1]);
    }
    out.quads[out.count++]=quad;
  };
  if(in.flags&4)mirrored(true,false);
  if(in.flags&8)mirrored(false,true);
  if((in.flags&12)==12)mirrored(true,true);
  return out;
}
} // namespace halo2
