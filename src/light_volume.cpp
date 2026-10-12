#include "halo2/light_volume.hpp"
#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
namespace {
float saturate(float v) { return std::clamp(v,0.f,1.f); }
Vec3 transform_axis_point(const Transform &t, Vec3 p, bool endpoint = false) {
  if(t.scale!=1.f)p=p*t.scale;
  // 30830 sums X + Z + Y, except the positive endpoint's Y: Z + Y + X.
  return {(t.basis[0].x*p.x+t.basis[2].x*p.z)+t.basis[1].x*p.y+t.position.x,
          (endpoint?(t.basis[2].y*p.z+t.basis[1].y*p.y)+t.basis[0].y*p.x:
                    (t.basis[0].y*p.x+t.basis[2].y*p.z)+t.basis[1].y*p.y)+t.position.y,
          (t.basis[0].z*p.x+t.basis[2].z*p.z)+t.basis[1].z*p.y+t.position.z};
}
Vec2 projected_axis(const LightVolumeState &s,const LightVolumeView &v) {
  const auto a=transform_axis_point(v.world_to_view,s.marker.position);
  auto b=transform_axis_point(v.world_to_view,s.marker.position+s.marker.basis[0],true);
  if(std::abs(b.z)<.00009999999747378752f)
    b=transform_axis_point(v.world_to_view,s.marker.position-s.marker.basis[0]);
  const float ia=std::abs(a.z)<.00009999999747378752f?0.f:1.f/a.z;
  const float ib=std::abs(b.z)<.00009999999747378752f?0.f:1.f/b.z;
  if(ia==0.f || ib==0.f)return {};
  return {0.f-v.projection_scale.x*(b.x*ib-a.x*ia),
          0.f-v.projection_scale.y*(b.y*ib-a.y*ia)};
}
std::uint32_t packed(float alpha,Vec3 color) {
  // 131E50 keeps the product on x87 until FISTP. A float product changes
  // half-byte cases, e.g. opacity .3f must pack to 77 rather than 76.
  const auto channel=[](float v){return std::uint32_t(std::nearbyint(double(v)*255.));};
  return channel(alpha)<<24 | channel(color.x)<<16 | channel(color.y)<<8 | channel(color.z);
}
Transform marker_transform(Vec3 position,std::array<float,4> q) {
  // Original 1421B0 ->141E10 uses 2/norm, without normalizing with sqrt.
  const float norm=((q[0]*q[0]+q[1]*q[1])+q[2]*q[2])+q[3]*q[3];
  const float s=norm>.00009999999747378752f?2.f/norm:0.f;
  const float xs=q[0]*s,ys=q[1]*s,zs=q[2]*s;
  const float wx=q[3]*xs,wy=q[3]*ys,wz=q[3]*zs;
  const float xx=q[0]*xs,xy=q[0]*ys,xz=q[0]*zs;
  const float yy=q[1]*ys,yz=q[1]*zs,zz=q[2]*zs;
  return {1,{{{1.f-(yy+zz),xy+wz,xz-wy},
    {xy-wz,1.f-(xx+zz),yz+wx},{xz+wy,yz-wx,1.f-(xx+yy)}}},position};
}
} // namespace
PixelProgram light_volume_program() {
  PixelProgram p;
  p.words[8]=0x080c0000; p.words[34]=0x04140000;
  p.words[45]=0xc0; p.words[53]=0x11001; p.words[54]=1;
  return p;
}
LightVolumeProjection light_volume_title_projection(Vec3 eye,Vec3 forward,Vec3 up,
    Vec2 axes,RenderViewport viewport) {
  const auto normalize=[](Vec3 v) {
    const float length=float(std::sqrt((double(v.z)*v.z+double(v.y)*v.y)+double(v.x)*v.x));
    return std::abs(length)<.00009999999747378752f?v:v*(1.f/length);
  };
  // 2FD90 forms these axes before normalizing any of them.
  const Vec3 right{up.z*forward.y-up.y*forward.z,
    forward.z*up.x-up.z*forward.x,up.y*forward.x-forward.y*up.x};
  const Vec3 adjusted_up{forward.z*right.y-forward.y*right.z,
    forward.x*right.z-forward.z*right.x,forward.y*right.x-forward.x*right.y};
  const Transform camera{1,{normalize(right),normalize(adjusted_up),normalize(forward*-1.f)},eye};
  constexpr float near=.015625f,far=1024.f;
  const float scale=float(double(far)/(double(far)-double(near)));
  return {inverse_transform(camera),{{{axes.x,0,0,0},{0,axes.y,0,0},
    {0,0,0.f-scale,-1},{0,0,(0.f-scale)*near,0}}},viewport,far};
}
std::optional<std::array<LightVolumeVertex,4>> light_volume_quad(
    const LightVolumeSprite &sprite,float cosine,float sine,const LightVolumeProjection &v) {
  const float width=std::max(sprite.radius.x,sprite.radius.y),height=sprite.radius.y;
  if(!(width>0.f && height>0.f))return {};
  auto point=sprite.center;
  if(v.world_to_view.scale!=1.f)point=point*v.world_to_view.scale;
  const auto &b=v.world_to_view.basis;
  const auto &t=v.world_to_view.position;
  const Vec3 view{((b[2].x*point.z+b[1].x*point.y)+b[0].x*point.x)+t.x,
                  ((b[2].y*point.z+b[1].y*point.y)+b[0].y*point.x)+t.y,
                  ((b[2].z*point.z+b[1].z*point.y)+b[0].z*point.x)+t.z};
  ShaderVector clip{};
  for(unsigned i=0;i<4;++i)
    clip[i]=((v.projection[2][i]*view.z+v.projection[1][i]*view.y)+
        v.projection[0][i]*view.x)+v.projection[3][i];
  if(!(clip[2]>0.f))return {};
  const float inverse=1.f/clip[3];
  const ShaderVector screen{
    (((clip[0]*inverse+1.f)*v.viewport.width+v.viewport.left*2.f)-1.f)*.5f,
    (((1.f-clip[1]*inverse)*v.viewport.height+v.viewport.top*2.f)-1.f)*.5f,
    std::min(1.f,clip[2]*inverse)*16777215.f,
    (clip[3]/v.far_distance)*16777215.f};
  const Vec2 extent{((v.viewport.width*inverse)*v.projection[0][0])*.5f,
                     ((v.viewport.height*inverse)*v.projection[1][1])*.5f};
  const float wc=width*cosine,hs=height*sine,ws=width*sine,hc=height*cosine;
  const float a=(wc-hs)*extent.x,bx=(ws+hc)*extent.y;
  const float c=(hs+wc)*extent.x,d=(ws-hc)*extent.y;
  // Captured NV2A inline v9 converts packed bytes by division, not by a
  // rounded reciprocal multiply. Preserve those exact vertex input values.
  const ShaderVector color{float((sprite.argb>>16)&255)/255.f,
    float((sprite.argb>>8)&255)/255.f,float(sprite.argb&255)/255.f,
    float(sprite.argb>>24)/255.f};
  return std::array<LightVolumeVertex,4>{{
    {{screen[0]+a,screen[1]-bx,screen[2],screen[3]},color,{1,0}},
    {{screen[0]+c,screen[1]-d,screen[2],screen[3]},color,{1,1}},
    {{screen[0]-a,screen[1]+bx,screen[2],screen[3]},color,{0,1}},
    {{screen[0]-c,screen[1]+d,screen[2],screen[3]},color,{0,0}}}};
}
std::vector<LightVolumeAttachment> Cache::load_light_volume_attachments(
    std::uint32_t object,const RenderModel &model,float seconds,const Transform *world) const {
  const auto tag=query_tag({.handle=TagHandle{object}, .require_address=true});
  if(!tag)throw FormatError("missing light-volume object");
  if(tag->group!="scen")throw FormatError("light-volume attachment requires scenery");
  const Bytes b(storage_);
  
  const auto root=metadata_offset(tag->address,0x9c);
  const auto [count,first]=metadata_block(root+0x94, 0x18);
  const auto mode=query_tag({.group=tag_groups::render_model, .name=model.name, .require_address=true});
  if(!count)return {};
  if(!mode)throw FormatError("missing light-volume render model");
  const auto [groups,group]=metadata_block(metadata_offset(mode->address,0x60)+0x58, 0xc);
  std::optional<AnimationClip> idle;
  if(model.animation!=0xffffffffu)
    if(const auto name=find_animation_name(model.animation,"idle"))
      idle=load_named_animation(model.animation,*name);
  std::vector<LightVolumeAttachment> attachments;
  for(std::size_t i=0;i<count;++i) {
    const auto at=first+i*0x18;
    if(b.u32(at)!=0x4d475332 || b.u32(at+4)==0xffffffffu)continue;
    // Original 41980/BE6D0. These inputs are absent on the title carrier.
    if(b.u16(root+2)&0x200 || b.u32(at+0x10) || b.u16(at+0xc))
      throw FormatError("light-volume object fade/function/color input not implemented");
    LightVolumeAttachment attachment{b.u32(at+4),{}};
    for(std::size_t j=0;j<groups;++j) {
      const auto g=group+j*0xc;
      if(b.u32(g)!=b.u32(at+8))continue;
      const auto [markers,marker]=metadata_block(g+4, 0x24);
      for(std::size_t k=0;k<std::min<std::size_t>(markers,65);++k) {
        const auto m=marker+k*0x24;
        if(std::to_integer<unsigned>(b.slice(m,1)[0])!=255)
          throw FormatError("region-filtered light-volume marker not implemented");
        const auto node=std::to_integer<unsigned>(b.slice(m+2,1)[0]);
        auto pose=pose_model_node(model,node,idle?&*idle:nullptr,seconds);
        // B8D30 supplies world node matrices to 1D8F50. The node must be
        // placed before composing its marker; reversing these operations
        // changes the title carrier's middle engine by one depth unit.
        if(world)pose=compose_transform(*world,pose);
        const auto local=marker_transform({b.f32(m+4),b.f32(m+8),b.f32(m+0xc)},
          {b.f32(m+0x10),b.f32(m+0x14),b.f32(m+0x18),b.f32(m+0x1c)});
        attachment.markers.push_back(compose_transform(pose,local));
      }
      break;
    }
    attachments.push_back(std::move(attachment));
  }
  return attachments;
}
Texture Cache::load_light_volume_default_bitmap() const {
  const auto globals=query_tag({.group=tag_groups::globals, .require_address=true});
  if(!globals)throw FormatError("missing rasterizer globals for glow");
  const Bytes b(storage_);
  const auto root=metadata_offset(globals->address,0x110);
  if(b.u32(root+0x108)!=1)throw FormatError("unsupported rasterizer globals count");
  const auto raster=metadata_offset(b.u32(root+0x10c),0x1c8);
  return load_bitmap(b.u32(raster+0x34),0);
}
LightVolumeDefinition Cache::load_light_volume(std::uint32_t datum) const {
  const auto tag=query_tag({.handle=TagHandle{datum}, .group=tag_groups::light_volume, .require_address=true});
  if(!tag)throw FormatError("missing original light-volume tag");
  const Bytes b(storage_);
  
  const auto curve=[&](std::size_t at) {
    const auto size=b.u32(at);
    if(size>4096)throw FormatError("excessive light-volume curve");
    if(!size || !b.u32(at+4))return std::vector<std::byte>{};
    const auto bytes=b.slice(metadata_offset(b.u32(at+4),size),size);
    return std::vector<std::byte>(bytes.begin(),bytes.end());
  };
  const auto root=metadata_offset(tag->address,0x10);
  LightVolumeDefinition definition{b.f32(root),b.f32(root+4),{}};
  const auto [count,first]=metadata_block(root+8, 0x98);
  for(std::size_t i=0;i<count;++i) {
    const auto at=first+i*0x98;
    LightVolume volume;
    volume.flags=b.u32(at);volume.bitmap=b.u32(at+8);
    volume.sprite_count=b.u32(at+0xc);volume.runtime_flags=b.u32(at+0x5c);
    if(volume.sprite_count>256)throw FormatError("light-volume exceeds original sprite limit");
    if(volume.runtime_flags&1)throw FormatError("linear-radius light-volume path not implemented");
    for(unsigned c=0;c<5;++c)volume.curves[c]=curve(at+0x10+c*8);
    const auto [aspects,aspect]=metadata_block(at+0x38, 0x1c);
    if(aspects)volume.aspect=LightVolumeAspect{{curve(aspect),curve(aspect+8)},
      b.f32(aspect+0x10),b.f32(aspect+0x14),b.f32(aspect+0x18)};
    const auto [phases,phase]=metadata_block(at+0x60, 8);
    for(std::size_t j=0;j<phases;++j)
      volume.phases.push_back({b.f32(phase+j*8),b.f32(phase+j*8+4)});
    definition.volumes.push_back(std::move(volume));
  }
  return definition;
}
std::vector<LightVolumeBatch> light_volume_sprites(const LightVolumeDefinition &d,
    const LightVolumeState &s,const LightVolumeView &v,const ShaderFunctionTables &tables) {
  std::vector<LightVolumeBatch> batches;
  auto to_eye=v.eye-s.marker.position;
  const float distance=std::sqrt(to_eye.x*to_eye.x+to_eye.y*to_eye.y+to_eye.z*to_eye.z);
  if(std::abs(distance)>=.00009999999747378752f)to_eye=to_eye*(1.f/distance);
  const float opacity=(d.cutoff>d.falloff?saturate((d.cutoff-distance)/(d.cutoff-d.falloff)):1.f)*s.opacity;
  if(opacity<=0.f)return batches;
  const auto forward=s.marker.basis[0];
  const float cosine=std::abs(float((double(to_eye.z)*forward.z+double(to_eye.y)*forward.y)+double(to_eye.x)*forward.x));
  for(const auto &volume:d.volumes) {
    if(!volume.sprite_count)continue;
    if(volume.runtime_flags&1)throw FormatError("linear-radius light-volume path not implemented");
    const float facing=shader_function_fraction(volume.curves[4],saturate(1.f-cosine),0,tables);
    const float range=1.f-facing;
    float strength=1;
    LightVolumeBatch batch{volume.bitmap,bool(volume.flags&8),1,0,{}};
    if(volume.aspect) {
      const auto &a=*volume.aspect;
      const float angle=saturate(float(std::acos(double(std::clamp(cosine,-1.f,1.f)))*.6366197466850281));
      const float lower=std::clamp(a.threshold*.6366197466850281f,0.f,.9998999834060669f);
      strength=saturate((angle-lower)/(1.f-lower));
      if(strength>0.f) {
        strength=a.exponent>0.f?float(std::pow(double(strength),double(a.exponent))):1.f;
        const auto axis=projected_axis(s,v);
        if(axis.x!=0.f || axis.y!=0.f) {
          const double orientation=std::atan2(double(axis.y),double(axis.x));
          batch.cosine=float(std::cos(orientation));batch.sine=float(std::sin(orientation));
        }
      }
    }
    const float step=1.f/float(volume.sprite_count-1);
    for(unsigned i=0;i<volume.sprite_count;++i) {
      float phase=0;
      if(volume.runtime_flags&2)phase=float(i)*step;
      else if(volume.phases.size()==volume.sprite_count)
        phase=(1.f-range)*volume.phases[i].x+volume.phases[i].y*range;
      const float offset=shader_function_value(volume.curves[0],phase,range,tables)*s.marker.scale;
      const float radius=shader_function_value(volume.curves[1],phase,range,tables)*s.marker.scale;
      const float brightness=shader_function_value(volume.curves[2],phase,range,tables);
      Vec2 aspect{1,1};
      if(volume.aspect) {
        const auto &a=*volume.aspect;
        if(strength>0.f) {
          aspect.x=a.parallel_scale-(a.parallel_scale-shader_function_value(a.curves[0],phase,0,tables))*strength;
          aspect.y=a.parallel_scale-(a.parallel_scale-shader_function_value(a.curves[1],phase,0,tables))*strength;
        } else aspect={a.parallel_scale,a.parallel_scale};
      }
      const float fraction=shader_function_fraction(volume.curves[3],phase,range,tables);
      const auto argb=shader_function_color(volume.curves[3],fraction);
      const Vec3 color{saturate(s.color.x*(float((argb>>16)&255)*(1.f/255.f))),
        saturate(s.color.y*(float((argb>>8)&255)*(1.f/255.f))),
        saturate(s.color.z*(float(argb&255)*(1.f/255.f)))};
      batch.sprites.push_back({s.marker.position+forward*offset,{radius*aspect.x,radius*aspect.y},
        packed(saturate(brightness*opacity),color)});
    }
    batches.push_back(std::move(batch));
  }
  return batches;
}
} // namespace halo2
