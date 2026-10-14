#include "halo2/particle_sprite.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
float value(const ParticleProperty &p,const ParticleSpriteInput &in,
    const ShaderFunctionTables &tables,bool fraction=false) {
  if(p.input<0 || p.input>=17 || p.range<0 || p.range>=17 || p.modifier_input>=17)
    throw FormatError("particle property input outside retail value cache");
  float v=fraction?shader_function_fraction(p.curve,
      !p.curve.empty() && std::to_integer<unsigned>(p.curve[0])==1?0.f:in.values[p.input],
      p.curve.size()>1 && (std::to_integer<unsigned>(p.curve[1])&1)?in.values[p.range]:0.f,tables):
    shader_function_value(p.curve,in.values[p.input],in.values[p.range],tables);
  if(p.modifier_input>=0) {
    if(p.modifier==1)v=in.values[p.modifier_input]+v;
    else if(p.modifier==2)v=in.values[p.modifier_input]*v;
  }
  return v;
}
Vec3 unpack(std::uint32_t color) {
  return {float((color>>16)&255)/255.f,float((color>>8)&255)/255.f,float(color&255)/255.f};
}
std::array<float,4> uv(const std::array<float,4> &b,unsigned flip) {
  return {flip&1?b[1]:b[0],flip&2?b[3]:b[2],
    flip&1?b[0]-b[1]:b[1]-b[0],flip&2?b[2]-b[3]:b[3]-b[2]};
}
}
ParticleSpriteDefinition Cache::load_particle_sprite(std::uint32_t datum) const {
  const auto tag=query_tag({.handle=TagHandle{datum}, .group=tag_groups::particle, .require_address=true});
  if(!tag)throw FormatError("missing original particle sprite tag");
  const Bytes b(cache_bytes());
  const auto root=metadata_offset(tag->address,0xbc);
  ParticleSpriteDefinition d;
  d.flags=b.u32(root);d.billboard_style=std::int16_t(b.u16(root+4));
  d.required_values=b.u32(root+0x98);d.first_sequence=std::int16_t(b.u16(root+8));
  d.sequence_count=std::int16_t(b.u16(root+0xa));
  for(unsigned i=0;i<5;++i) {
    const auto at=root+0x1c+i*0x10;auto &p=d.properties[i];
    p.input=std::int16_t(b.u16(at));p.range=std::int16_t(b.u16(at+2));
    p.modifier=b.u16(at+4);p.modifier_input=std::int16_t(b.u16(at+6));
    const auto size=b.u32(at+8);
    if(size>4096)throw FormatError("excessive particle property curve");
    if(size) {
      const auto raw=b.slice(metadata_offset(b.u32(at+0xc),size),size);
      p.curve.assign(raw.begin(),raw.end());
    }
  }
  for(unsigned i=0;i<4;++i) {
    d.geometry[i]=b.f32(root+0x9c+i*4);d.bounds[i]=b.f32(root+0xac+i*4);
  }
  // A real-valued shader parameter can accompany the diffuse bitmap (for
  // example E22900B4). Its tag-reference group is also bitm, but type is 1
  // and the datum is NONE. Select the actual type-0 bitmap parameter.
  const auto parameter_count=b.u32(root+0x14);
  if(!parameter_count || parameter_count>65536)throw FormatError("particle sprite shader parameters missing");
  const auto parameters=metadata_offset(b.u32(root+0x18),std::size_t(parameter_count)*0x28);
  bool found_bitmap=false;
  for(unsigned i=0;i<parameter_count;++i) {
    const auto parameter=parameters+i*0x28;
    if(b.u16(parameter+4)!=0)continue;
    if(found_bitmap || b.u32(parameter+8)!=0x6269746d || b.u32(parameter+0xc)==0xffffffffu)
      throw FormatError("particle sprite requires one explicit bitmap parameter");
    d.bitmap=b.u32(parameter+0xc);found_bitmap=true;
  }
  if(!found_bitmap)throw FormatError("particle diffuse bitmap missing");
  const auto bitmap=query_tag({.handle=TagHandle{d.bitmap}, .group=tag_groups::bitmap, .require_address=true});
  if(!bitmap)throw FormatError("particle sprite bitmap tag missing");
  const auto at=metadata_offset(bitmap->address,0x44);d.bitmap_type=std::int16_t(b.u16(at));
  const auto count=b.u32(at+0x3c);
  if(count>65536)throw FormatError("excessive particle bitmap sequences");
  const auto sequences=count?metadata_offset(b.u32(at+0x40),count*0x3c):0;
  for(unsigned i=0;i<count;++i) {
    const auto s=sequences+i*0x3c;
    const auto n=b.u32(s+0x34);
    if(n>65536)throw FormatError("excessive particle bitmap frames");
    const auto first=n?metadata_offset(b.u32(s+0x38),n*0x20):0;
    auto &sequence=d.sequences.emplace_back();
    for(unsigned j=0;j<n;++j) {
      const auto f=first+j*0x20;
      sequence.push_back({{b.f32(f+8),b.f32(f+0xc),b.f32(f+0x10),b.f32(f+0x14)},
        {b.f32(f+0x18),b.f32(f+0x1c)}});
    }
  }
  return d;
}
ParticleSpriteSubmission particle_sprite_submission(const ParticleSpriteDefinition &d,
    const ParticleSpriteInput &in,const ShaderFunctionTables &tables) {
  auto color=unpack(shader_function_color(d.properties[0].curve,value(d.properties[0],in,tables,true)));
  if(d.flags&0x180) {
    const auto seed=unpack(in.seed_color);
    color={color.x*seed.x*2.f,color.y*seed.y*2.f,color.z*seed.z*2.f};
  }
  const auto channel=[](float v){return std::uint32_t(std::nearbyint(double(std::clamp(v,0.f,1.f))*255.));};
  ParticleSpriteSubmission out{in.position,in.direction,
    value(d.properties[2],in,tables)*in.scale,value(d.properties[3],in,tables)*6.2831854820251465f,
    channel(value(d.properties[1],in,tables)*in.color[3])<<24 |
    channel(in.color[0]*color.x)<<16 | channel(in.color[1]*color.y)<<8 |
    channel(in.color[2]*color.z)};
  const unsigned flip=((d.flags&2) && (in.random1&1)?1:0) |
    ((d.flags&4) && (in.random2&1)?2:0);
  if(d.flags&0x10000) {
    out.geometry=d.geometry;out.uv_a=uv(d.bounds,flip);out.uv_b=out.uv_a;
  } else {
    std::array<float,4> a{0,1,0,1},b=a;Vec2 pivot{.5f,.5f};
    if(d.bitmap_type==3) {
      auto index=in.selected_sequence;
      if(d.flags&0x10) {
        if(d.sequences.empty())throw FormatError("random particle sequence missing");
        index=std::clamp(int(in.random2%std::max<int>(d.sequence_count,1))+d.first_sequence,
          0,int(d.sequences.size())-1);
      }
      if(index>=0) {
        if(unsigned(index)>=d.sequences.size())throw FormatError("particle context sequence outside bitmap");
        const auto &sequence=d.sequences[index];const int count=int(sequence.size());
        if(count>0) {
          const float frame=value(d.properties[4],in,tables)*float(d.flags&8?count-1:count);
          const int f=int(std::floor(frame));
          if(f<0)throw FormatError("negative particle animation frame");
          const auto &first=sequence[f%count],&next=sequence[(f+1)%count];
          if(!(d.flags&0x20))out.blend=frame-float(f);
          a=first.bounds;b=next.bounds;
          pivot={first.pivot.x*(1.f-out.blend)+next.pivot.x*out.blend,
            first.pivot.y*(1.f-out.blend)+next.pivot.y*out.blend};
        }
      }
    }
    const float ax=a[1]-a[0],ay=a[3]-a[2],bx=b[1]-b[0],by=b[3]-b[2];
    if(ax==0.f || ay==0.f || bx==0.f || by==0.f)throw FormatError("empty particle atlas bounds");
    const float iax=1.f/ax,iay=1.f/ay,ibx=1.f/bx,iby=1.f/by;
    float aspect_x=1,aspect_y=1;
    if(ax>ay)aspect_y=ay*iax;else aspect_x=ax*iay;
    if(bx>by)aspect_y=by*ibx*out.blend+(1.f-out.blend)*aspect_y;
    else aspect_x=bx*iby*out.blend+(1.f-out.blend)*aspect_x;
    const float ix=(1.f-out.blend)*iax+ibx*out.blend,iy=(1.f-out.blend)*iay+iby*out.blend;
    const float width=in.width*aspect_x;
    out.uv_a=uv(a,flip);out.uv_b=uv(b,flip);
    const float x=ix*pivot.x*2.f-1.f,y=iy*pivot.y*2.f-1.f;
    const float x0=(-1.f-x)*width,y0=(-1.f-y)*aspect_y;
    const float x1=(1.f-x)*width,y1=(1.f-y)*aspect_y;
    out.geometry={x0,y0,x1-x0,y1-y0};
  }
  if(d.flags&0x2000)out.angle+=1.5707963705062866f;
  return out;
}
ParticleSpriteSubmission particle_sprite_submission(const ParticleSpriteDefinition &d,
    ParticleSpriteInput in,ParticleValueCache &cache,const ParticleValueSources &sources,const ShaderFunctionTables &tables) {
  complete_particle_values(cache,d.required_values&0xf80fu,sources);
  in.values=cache.values;
  return particle_sprite_submission(d,in,tables);
}
std::array<std::uint32_t,46> particle_sprite_pushbuffer(const ParticleSpriteSubmission &s,
    const std::array<std::uint32_t,8> &bindings) {
  std::array<std::uint32_t,46> result{};
  unsigned cursor=0;
  const auto data4=[&](unsigned binding,const std::array<float,4> &v) {
    result[cursor++]=0x100000u+0x1a00u+bindings[binding]*16u;
    for(float component:v)result[cursor++]=std::bit_cast<std::uint32_t>(component);
  };
  const auto data2=[&](unsigned binding,float a,float b) {
    result[cursor++]=0x80000u+0x1880u+bindings[binding]*8u;
    result[cursor++]=std::bit_cast<std::uint32_t>(a);
    result[cursor++]=std::bit_cast<std::uint32_t>(b);
  };
  data4(0,{s.direction.x,s.direction.y,s.direction.z,1.f});
  data2(1,s.blend,0.f);
  data4(3,{s.position.x,s.position.y,s.position.z,1.f});
  data2(4,s.size,s.angle);
  result[cursor++]=0x40000u+0x1940u+bindings[7]*4u;
  result[cursor++]=s.argb;
  data4(2,s.uv_a);
  data4(5,s.geometry);
  data4(6,s.uv_b);
  for(const auto word:std::array<std::uint32_t,13>{
      0x417fcu,8u,0x40201818u,0u,0u,1u,0u,0x10001u,0u,0x10000u,0u,0x417fcu,0u})
    result[cursor++]=word;
  return result;
}
std::array<ShaderVector,16> particle_sprite_vertex_attributes(const ParticleSpriteSubmission &s) {
  std::array<ShaderVector,16> v{};
  v[1]={s.direction.x,s.direction.y,s.direction.z,1};
  v[2]={s.blend,0,0,1};v[3]=s.uv_a;
  v[4]={s.position.x,s.position.y,s.position.z,1};
  v[5]={s.size,s.angle,0,1};v[6]=s.geometry;v[7]=s.uv_b;
  // 23ED30 writes the unmodified ARGB word with SET_VERTEX_DATA4UB.
  // Its low byte enters X: B,G,R,A. Classification 7/8 perform .zyx.
  v[8]={float(s.argb&255)/255.f,float((s.argb>>8)&255)/255.f,
      float((s.argb>>16)&255)/255.f,float(s.argb>>24)/255.f};
  return v;
}
} // namespace halo2
