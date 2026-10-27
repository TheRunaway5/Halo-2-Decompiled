#include "halo2/motion_sensor.hpp"
#include "halo2/native_parallel.hpp"
#include <algorithm>
#include <optional>
#include <cmath>
#include <bit>

namespace halo2 {
PixelProgram source_motion_sensor_final_program_tu15() {
  PixelProgram result;auto &p=result.words;
  p[0]=0x18110000;p[8]=8;p[9]=0x1800;p[10]=0xffffffff;
  p[26]=0x80;p[34]=0x08010000;p[43]=0xff00ff00;p[45]=0x80;p[53]=1;p[54]=1;
  return result;
}
namespace {
void finite(float v) {
  if(!std::isfinite(v))throw FormatError("Non-finite original motion sensor input");
}
}
bool source_motion_sensor_moving(const MotionSensorObject &o,float minimum,float sneak) {
  finite(o.velocity.x);finite(o.velocity.y);finite(o.velocity.z);finite(minimum);finite(sneak);
  float speed=0;
  if(!(o.control_c1&1)) {
    const float z=o.velocity.z*.33f;
    // Original24590 uses x87 for the norm, then1A1DA9 spills binary32.
    speed=float(std::sqrt((double(o.velocity.x)*o.velocity.x+
                           double(o.velocity.y)*o.velocity.y)+double(z)*z));
  }
  const bool unit=((1u<<(o.type&31))&3)!=0;
  return speed>=(unit && o.player_13c!=0xffffffffu ? sneak*1.05f : minimum);
}
bool source_motion_sensor_visible(const MotionSensorObject &o,float minimum,float sneak,
                                  bool multiplayer,bool force) {
  const bool unit=((1u<<(o.type&31))&3)!=0;
  if(unit && ((o.control_14a&0x21) || (o.estimate_mode!=0 && o.estimate_mode!=3)))return true;
  const bool unhidden=!unit || multiplayer || !(o.flags_134&8);
  const bool flag30=unit && (o.flags_134&(1u<<30));
  return (unhidden && (source_motion_sensor_moving(o,minimum,sneak) || flag30)) || force;
}
MotionSensorSample source_motion_sensor_sample(bool present,Vec3 origin,float yaw,float range,
    std::span<const MotionSensorCandidate> objects) {
  MotionSensorSample result;
  if(!present)return result;
  finite(origin.x);finite(origin.y);finite(origin.z);finite(yaw);finite(range);
  if(!(range>0))throw FormatError("Original motion sensor range must be positive");
  const float angle=0.f-(yaw+1.5707963705062866f);
  const float sine=float(std::sin(double(angle))),cosine=float(std::cos(double(angle)));
  const float scale=1.f/range;
  for(const auto &o:objects) {
    if(result.count==result.blips.size())break;
    if(!o.present || !o.visible)continue;
    finite(o.position.x);finite(o.position.y);
    const float dx=o.position.x-origin.x,dy=o.position.y-origin.y;
    // The original accumulates Y squared before X squared.
    if(!(range*range>dy*dy+dx*dx))continue;
    const float x=((dy*sine-dx*cosine)*scale+1.f)*32.f;
    const float y=((dy*cosine+dx*sine)*scale+1.f)*32.f;
    result.blips[result.count++]={std::uint8_t(std::clamp(x,0.f,64.f)),
        std::uint8_t(std::clamp(y,0.f,64.f)),o.type,o.size};
  }
  return result;
}
void source_motion_sensor_store_samples(MotionSensorHistory &h,
    const std::array<std::optional<MotionSensorSample>,4> &samples) {
  if(h.sample_index>=10)throw FormatError("Original motion-sensor history index outside table");
  h.sample_index=h.sample_index==0?9:h.sample_index-1;
  for(unsigned view=0;view<samples.size();++view)if(samples[view]) {
    if(samples[view]->count>16)throw FormatError("Original motion-sensor sample exceeds capacity");
    h.players[view][h.sample_index]=*samples[view];
  }
}
void source_motion_sensor_reset(MotionSensorHistory &h,unsigned view) {
  if(view>=h.players.size())throw FormatError("Original motion-sensor local player outside table");
  for(auto &sample:h.players[view])sample.count=0;
}
float source_motion_sensor_pulse(std::int32_t tick,float rate) {
  finite(rate);
  const float time=float(std::fmod(double(tick)*rate,double(2.1f)));
  return time<2.0375f ? 1.f/((time+.0625f)*1.1f) : .4f;
}
MotionSensorRenderPlan source_motion_sensor_render_plan(const MotionSensorHistory &h,
    unsigned view,std::int16_t mode,std::array<std::int16_t,2> origin,
    std::array<std::int16_t,2> top_left,std::int32_t tick,float rate) {
  if(view>=h.players.size() || h.sample_index>=10)
    throw FormatError("Original motion-sensor history outside table");
  MotionSensorRenderPlan result;
  result.center={float(int(origin[0])+top_left[1]),float(int(origin[1])+top_left[0])};
  result.pulse=source_motion_sensor_pulse(tick,rate);
  result.visible=mode!=2 && mode!=3;
  if(!result.visible)return result;
  // Literal original445320 and47FF9C bytes. The fourth offset is the adjacent
  // original word00000003, read by the source's clamped size selector.
  constexpr std::array<Vec3,9> colors{{{0,0,0},{1,.5f,0},{1,1,0},{1,0,0},
      {1,1,0},{1,0,0},{.5f,.5f,1},{1,1,.3f},{1,.3f,.3f}}};
  constexpr std::array<float,4> offsets{0,-.75f,1,std::bit_cast<float>(3u)};
  for(unsigned i=0;i<10;++i) {
    const auto &sample=h.players[view][(h.sample_index+i)%10];
    if(sample.count>16)throw FormatError("Original motion-sensor sample exceeds capacity");
    const float fraction=float(10-i)*.1f,scale=fraction*fraction;
    const float intensity=float(std::pow(1.0-double(fraction),3.5)*7.0+1.0);
    for(unsigned j=0;j<sample.count;++j) {
      const auto &blip=sample.blips[j];
      if(!blip.type)continue;
      if(blip.type>=colors.size() || blip.size>=offsets.size())
        throw FormatError("Original motion-sensor blip outside color/offset table");
      float alpha=1;
      if(blip.type==6)alpha=float((std::sin(double(tick)*double(rate)*double(3.14159265f))+1.0)*
          double(.033333335f)+1.0);
      result.blips.push_back({{float(blip.x),float(blip.y)},scale,
          alpha*intensity+offsets[blip.size],colors[blip.type],blip.type==6});
    }
  }
  return result;
}
Texture source_motion_sensor_composite(const MotionSensorRenderPlan &plan,
    const MotionSensorArtwork &art,Vec3 color) {
  return source_motion_sensor_composite(plan,art,color,64);
}
Texture source_motion_sensor_composite(const MotionSensorRenderPlan &plan,
    const MotionSensorArtwork &art,Vec3 color,unsigned raster_size) {
  if(raster_size<64 || raster_size>512 || raster_size%64)
    throw FormatError("Motion-sensor raster size must be a multiple of64 in64..512");
  const float density=float(raster_size)/64;
  for(const auto *t:{&art.sweep,&art.mask,&art.blip,&art.custom})
    if(!t->width || !t->height || t->format!=11 ||
       t->pixels.size()<std::size_t(t->width)*t->height*4)
      throw FormatError("Original motion-sensor artwork must be decoded RGBA");
  struct PreparedSample {const Texture *texture;unsigned width,height;std::size_t offset;};
  auto prepare=[](const Texture &t,float lod) {
    const auto level=t.mips.empty()?0u:unsigned(std::clamp(int(std::floor(lod+.5f)),0,int(t.mips.size()-1)));
    const auto width=t.mips.empty()?t.width:t.mips[level].width,
               height=t.mips.empty()?t.height:t.mips[level].height;
    const auto offset=t.mips.empty()?0:t.mips[level].offset;
    return PreparedSample{&t,width,height,offset};
  };
  auto sample=[](const PreparedSample &s,float u,float v,float border_alpha=0.f) {
    const auto &t=*s.texture;const auto width=s.width,height=s.height;const auto offset=s.offset;
    const float x=u*width-.5f,y=v*height-.5f;
    const int ix=int(std::floor(x)),iy=int(std::floor(y));
    const float fx=x-ix,fy=y-iy;
    std::array<float,4> out{};
    for(int j=0;j<2;++j)for(int i=0;i<2;++i) {
      const int px=ix+i,py=iy+j;
      const float weight=(i?fx:1-fx)*(j?fy:1-fy);
      //254200 selects border addressing.2548F0 changes its ARGB border
      //to46000000 before the sweep and retains it through the mask. RGB
      //is black, but alpha70/255 preserves contacts beyond the pulse ring.
      if(px<0 || py<0 || px>=int(width) || py>=int(height)) {
        out[3]+=border_alpha*weight;
        continue;
      }
      for(unsigned c=0;c<4;++c)
        out[c]+=std::to_integer<unsigned>(t.pixels[offset+(py*width+px)*4+c])/255.f*weight;
    }
    return out;
  };
  Texture out;out.width=out.height=raster_size;out.format=11;
  out.pixels.resize(std::size_t(raster_size)*raster_size*4);
  auto quantize=[](float v){return std::clamp(std::round(v*255.f),0.f,255.f)/255.f;};
  std::vector<std::array<float,4>> target(std::size_t(raster_size)*raster_size);
  // Blip quads and sample levels are prepared in source order. Row bands then
  // composite in parallel: each pixel still takes its blips in order, then the
  // sweep and mask, so every output byte is unchanged.
  struct Blip {
    PreparedSample texture;float x0,x1,y0,y1,scale;std::array<float,3> rgb;
    int left,right,top,bottom;
  };
  std::vector<Blip> blips;
  std::optional<PreparedSample> prepared_sweep,prepared_mask;
  if(plan.visible) {
    for(const auto &b:plan.blips) {
      //254490: color is multiplied by the history scale, while the quad's
      // half extent is intensity*2. These two arguments are not interchangeable.
      const float extent=b.intensity*2;
      if(!(extent>0))continue;
      const auto &texture=b.custom?art.custom:art.blip;
      // NV2A screen coordinates have four fractional bits, truncated toward
      // zero. The original-game capture's passthrough shader preserves this
      // conversion (xemu0.8.136 pgraph/glsl/vsh.c:roundScreenCoords). Round each
      // edge independently before coverage and interpolation, not the center.
      auto screen=[](float value){return std::trunc(value*16.f)/16.f;};
      const float x0=screen(b.point.x-extent),x1=screen(b.point.x+extent),
                  y0=screen(b.point.y-extent),y1=screen(b.point.y+extent);
      if(x1<=x0 || y1<=y0)continue;
      const int left=std::max(0,int(std::ceil(x0*density-.5f))),
          right=std::min(int(raster_size),int(std::ceil(x1*density-.5f))),
          top=std::max(0,int(std::ceil(y0*density-.5f))),
          bottom=std::min(int(raster_size),int(std::ceil(y1*density-.5f)));
      if(left>=right || top>=bottom)continue;
      const auto prepared_blip=prepare(texture,
          std::log2(std::max(float(texture.width)/((x1-x0)*density),
                            float(texture.height)/((y1-y0)*density))));
      blips.push_back({prepared_blip,x0,x1,y0,y1,b.scale,{b.color.x,b.color.y,b.color.z},left,right,top,bottom});
    }
    prepared_sweep=prepare(art.sweep,
        std::log2(plan.pulse*(raster_size==64?1.f:float(art.sweep.width)/raster_size)));
    prepared_mask=prepare(art.mask,0);
  }
  const std::array<float,3> rgb{color.x,color.y,color.z};
  auto *output_bytes=out.pixels.data();
  constexpr unsigned bands=16; // raster_size is a multiple of64.
  const unsigned rows=raster_size/bands;
  native_parallel_for(bands,[&](std::size_t band) {
    const unsigned first=unsigned(band)*rows,last=first+rows;
    if(plan.visible) {
      for(const auto &b:blips)
        for(int y=std::max(b.top,int(first));y<std::min(b.bottom,int(last));++y)for(int x=b.left;x<b.right;++x) {
          const auto texel=sample(b.texture,((x+.5f)/density-b.x0)/(b.x1-b.x0),
                                  (b.y1-(y+.5f)/density)/(b.y1-b.y0));
          auto &pixel=target[y*raster_size+x];
          for(unsigned c=0;c<3;++c)pixel[c]=quantize(pixel[c]+texel[c]*b.scale*b.rgb[c]);
        }
      for(unsigned y=first;y<last;++y)for(unsigned x=0;x<raster_size;++x) {
        const float u=(x+.5f)/raster_size,v=(y+.5f)/raster_size;
        const auto sweep=sample(*prepared_sweep,.5f+plan.pulse*(.5f-u),
                                          .5f+plan.pulse*(v-.5f),70.f/255);
        auto &pixel=target[y*raster_size+x];
        for(unsigned c=0;c<3;++c)pixel[c]=quantize(sweep[c]*rgb[c]+pixel[c]*sweep[3]);
        //254200 clears the A8R8G8B8 target to00000000, then selects
        // COLORWRITEENABLE=00010101.2548F0 retains that RGB-only mask through
        // sweep and mask. Their source alpha controls RGB blending, but must
        // not become destination alpha: the final ONE/INVSRCALPHA pass would
        // otherwise darken the authored background and expose a square pulse.
        const float mask=sample(*prepared_mask,1-u,v,70.f/255)[3];
        for(auto &channel:pixel)channel=quantize(channel*mask);
      }
    }
    for(std::size_t p=std::size_t(first)*raster_size;p<std::size_t(last)*raster_size;++p)
      for(unsigned c=0;c<4;++c)output_bytes[p*4+c]=std::byte(unsigned(std::round(target[p][c]*255)));
  });
  return out;}
} // namespace halo2
