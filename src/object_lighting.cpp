#include "halo2/object_lighting.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cmath>
#include <bit>

namespace halo2 {
namespace {
Vec3 read_vector(const Bytes &b,std::size_t at) {
  Vec3 v{b.f32(at),b.f32(at+4),b.f32(at+8)};
  if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z))
    throw FormatError("nonfinite object lighting vector");
  return v;
}
Vec3 unpack(std::uint32_t c) {
  constexpr float scale=1.f/255.f;
  return {float((c>>16)&255)*scale,float((c>>8)&255)*scale,float(c&255)*scale};
}
Vec3 clamp_vector(Vec3 v,Vec3 low,Vec3 high) {
  return {std::clamp(v.x,low.x,high.x),std::clamp(v.y,low.y,high.y),std::clamp(v.z,low.z,high.z)};
}
Vec3 unit(Vec3 v) {
  const float n=float(std::sqrt(double(v.x)*v.x+double(v.z)*v.z+double(v.y)*v.y));
  return std::abs(n)>=.0001f ? v*(1.f/n) : v;
}
}
// TU1.5 11A360 (base 117C80): retain the horizontal heading.
Vec3 source_object_light_direction(Vec3 v,float radians) {
  const float cosine=float(std::cos(double(radians)));
  if(cosine<std::abs(v.z))v.z=v.z<0?-cosine:cosine;
  if(std::abs(v.x)>=.01f || std::abs(v.y)>=.01f) {
    const float n=float(std::sqrt(double(v.x)*v.x+double(v.y)*v.y));
    if(std::abs(n)>=.0001f)v.x*=1.f/n,v.y*=1.f/n;
    else v.x=1,v.y=0;
  } else v.x=1,v.y=0;
  const float n=float(std::sqrt(double(v.x)*v.x+double(v.y)*v.y));
  if(std::abs(n)>=.0001f)v.x*=1.f/n,v.y*=1.f/n;
  // 11A48F..11A4A7 retains the x87 square/subtract/root through each
  // multiplication. Rounding the root to float first changes live tank light.
  const double magnitude=std::sqrt(1.0-double(v.z)*v.z);
  v.x=float(double(v.x)*magnitude);v.y=float(double(v.y)*magnitude);return v;
}
namespace {
Vec3 rotate_z(Vec3 v,float degrees) {
  const double angle=double(degrees)*double(.01745329238474369f);
  const float s=float(std::sin(angle)),c=float(std::cos(angle));
  return {v.x*c-v.y*s,v.y*c+v.x*s,v.z*c+v.z*(1.f-c)};
}
ShaderVector vector(Vec3 v) {return {v.x,v.y,v.z,0};}
}
ObjectLightingDefinition Cache::load_object_lighting_definition() const {
  const Bytes b(cache_bytes());
  const auto scenario=query_tag({.group=tag_groups::scenario});
  if(!scenario)throw FormatError("object lighting scenario missing");
  const auto at=metadata_offset(scenario->address,scenario->size);
  auto datum=b.u32(at+0x33c);
  if(datum==0xffffffffu) {
    const auto globals=query_tag({.group=tag_groups::globals});
    if(!globals)throw FormatError("default object lighting globals missing");
    datum=b.u32(metadata_offset(globals->address,globals->size)+0x184);
  }
  const auto tag=query_tag({.handle=TagHandle{datum}, .group=tag_groups::chocolate_mountain});
  if(!tag)throw FormatError("authored global object lighting missing");
  const auto root=metadata_offset(tag->address,tag->size);
  const auto count=b.u32(root);
  if(count>65536)throw FormatError("excessive object lighting variables");
  ObjectLightingDefinition result;result.name=tag->name;result.tables=shader_function_tables();
  if(!count)return result;
  const auto first=metadata_offset(b.u32(root+4),std::size_t(count)*144);
  const auto curve=[&](std::size_t p) {
    const auto size=b.u32(p);if(size>4096)throw FormatError("excessive object lighting curve");
    if(!size)return std::vector<std::byte>{};
    const auto data=b.slice(metadata_offset(b.u32(p+4),size),size);
    return std::vector<std::byte>(data.begin(),data.end());
  };
  for(unsigned i=0;i<count;++i) {
    const auto p=first+i*144;
    result.variables.push_back({b.u32(p),b.f32(p+4),read_vector(b,p+8),read_vector(b,p+20),
      b.f32(p+32),curve(p+36),read_vector(b,p+44),read_vector(b,p+56),
      read_vector(b,p+68),read_vector(b,p+80),b.f32(p+92),curve(p+96),
      read_vector(b,p+104),read_vector(b,p+116),curve(p+128),curve(p+136)});
  }
  return result;
}
std::optional<Texture> Cache::load_object_surface_texture(std::uint32_t shader) const {
  // 13DA30 resolves the first BitmapProperties index; D30D0 always reads
  // bitmap-data record zero, independently of animated GPU bitmap frames.
  const auto tag=query_tag({.handle=TagHandle{shader}, .group=tag_groups::shader});
  if(!tag || !tag->address)return {};
  const Bytes b(cache_bytes());const auto root=metadata_offset(tag->address,tag->size);
  if(!b.u32(root+32))return {};
  const auto definition=metadata_offset(b.u32(root+36),124);
  if(!b.u32(definition+0x5c))return {};
  const auto property=metadata_offset(b.u32(definition+0x60),4);
  const auto index=std::int16_t(b.u16(property));
  if(index==-1)return {};
  if(index<0 || unsigned(index)>=b.u32(definition+4))
    throw FormatError("object surface bitmap property outside shader");
  const auto bitmaps=metadata_offset(b.u32(definition+8),b.u32(definition+4)*12);
  const auto datum=b.u32(bitmaps+unsigned(index)*12);
  if(datum==0xffffffffu || !bitmap_count(datum))return {};
  auto texture=load_bitmap(datum,0);
  const auto bitmap=query_tag({.handle=TagHandle{datum}});
  const auto bitmap_root=metadata_offset(bitmap->address,bitmap->size);
  const auto record=metadata_offset(b.u32(bitmap_root+72),116);
  const auto format=b.u16(record+12);
  // 135CA0's CPU color unpack differs from the GPU's alpha-only and
  // two-channel fetch swizzles. This texture is dedicated to CPU queries.
  if(texture.format==11 && (format==0 || format==22 || format==23))
    for(std::size_t i=0;i<texture.pixels.size();i+=4) {
      if(format==0)texture.pixels[i]=texture.pixels[i+1]=texture.pixels[i+2]=std::byte{0};
      if(format==22)texture.pixels[i+2]=std::byte{0},texture.pixels[i+3]=std::byte{255};
      if(format==23) {
        texture.pixels[i+2]=texture.pixels[i];texture.pixels[i]=std::byte{0};
        texture.pixels[i+3]=std::byte{255};
      }
    }
  return texture;
}
std::optional<std::uint32_t> sample_object_texture(const Texture &texture,Vec2 uv,float detail) {
  if(!texture.width || !texture.height || texture.layers!=1 || texture.pixels.empty())return {};
  if(!std::isfinite(uv.x)||!std::isfinite(uv.y)||!std::isfinite(detail))
    throw FormatError("nonfinite CPU object texture query");
  const unsigned additional=texture.mips.empty()?0:unsigned(texture.mips.size()-1);
  const auto level=detail>=1?0:unsigned(std::nearbyint((1.f-detail)*float(additional)));
  if(level>additional)throw FormatError("CPU object texture mip outside authored image");
  const bool compressed=texture.format>=14 && texture.format<=16;
  unsigned width=std::max(1u,texture.width>>level),height=std::max(1u,texture.height>>level);
  if(compressed)width=(width+3)&~3u,height=(height+3)&~3u;
  const auto coordinate=[](float value,unsigned dimension) {
    const auto rounded=std::int64_t(std::nearbyint(float(dimension)*value-.5f));
    return unsigned((rounded%dimension+dimension)%dimension);
  };
  const auto x=coordinate(uv.x,width),y=coordinate(uv.y,height);
  const auto offset=texture.mips.empty()?0:texture.mips[level].offset;
  const Bytes bytes(texture.pixels);
  if(texture.format==11) {
    const auto at=offset+(std::size_t(y)*width+x)*4;
    const auto p=bytes.u32(at);
    return (p&0xff00ff00u)|((p&255u)<<16)|((p>>16)&255u);
  }
  if(!compressed)throw FormatError("unsupported original CPU object texture format");
  const auto size=texture.format==14?8:16;
  const auto block=offset+(std::size_t(y/4)*(width/4)+x/4)*size;
  const auto color=block+(size==8?0:8);
  const auto rgb565=[](unsigned v) {
    const unsigned r=v>>11,g=(v>>5)&63,b=v&31;
    return std::array<unsigned,4>{(r<<3)|(r>>2),(g<<2)|(g>>4),(b<<3)|(b>>2),255};
  };
  const auto c0=bytes.u16(color),c1=bytes.u16(color+2);
  std::array<std::array<unsigned,4>,4> colors{rgb565(c0),rgb565(c1),{}, {}};
  if(c0>c1) {
    for(unsigned k=0;k<3;++k) {
      colors[2][k]=(2*colors[0][k]+colors[1][k]+1)/3;
      colors[3][k]=(colors[0][k]+2*colors[1][k]+1)/3;
    }
    colors[2][3]=colors[3][3]=255;
  } else {
    for(unsigned k=0;k<3;++k)colors[2][k]=(colors[0][k]+colors[1][k])/2;
    colors[2][3]=255;
  }
  const auto pixel=(y&3)*4+(x&3);
  auto rgba=colors[(bytes.u32(color+4)>>(pixel*2))&3];
  if(texture.format==15)rgba[3]=((bytes.u16(block+(y&3)*2)>>((x&3)*4))&15)*17;
  if(texture.format==16) {
    const auto a0=unsigned(std::to_integer<unsigned char>(bytes.slice(block,1)[0]));
    const auto a1=unsigned(std::to_integer<unsigned char>(bytes.slice(block+1,1)[0]));
    std::array<unsigned,8> alpha{a0,a1};
    if(a0>a1)for(unsigned i=2;i<8;++i)alpha[i]=((8-i)*a0+(i-1)*a1)/7;
    else {
      for(unsigned i=2;i<6;++i)alpha[i]=((6-i)*a0+(i-1)*a1)/5;
      alpha[6]=0;alpha[7]=255;
    }
    std::uint64_t indices=0;
    for(unsigned i=0;i<6;++i)indices|=std::uint64_t(std::to_integer<unsigned char>(bytes.slice(block+2+i,1)[0]))<<(8*i);
    rgba[3]=alpha[(indices>>(pixel*3))&7];
  }
  return rgba[3]<<24|rgba[0]<<16|rgba[1]<<8|rgba[2];
}
std::uint32_t pack_object_lighting_rgb(Vec3 v) {
  const auto byte=[](float value) {
    if(!std::isfinite(value))throw FormatError("nonfinite original RGB packing");
    return std::uint32_t(std::int64_t(std::nearbyint(double(value)*255.)))&255u;
  };
  return byte(v.x)<<16|byte(v.y)<<8|byte(v.z);
}
std::array<float,21> ObjectLightingState::source_values() const {
  return {ambient.x,ambient.y,ambient.z,shadow_direction.x,shadow_direction.y,shadow_direction.z,
    accuracy,shadow_opacity,primary_color.x,primary_color.y,primary_color.z,
    primary_direction.x,primary_direction.y,primary_direction.z,
    secondary_color.x,secondary_color.y,secondary_color.z,
    secondary_direction.x,secondary_direction.y,secondary_direction.z,0};
}
ObjectLightingState object_lighting(const ObjectLightingDefinition &definition,
    const ObjectLightmapSample &sample,unsigned type) {
  const ObjectLightingVariables *v=nullptr;
  for(const auto& candidate:definition.variables)
    if((candidate.affected_objects&1) || (candidate.affected_objects&(1u<<((type+1)&31))))v=&candidate;
  if(!v || !definition.tables)
    throw FormatError("object lighting requires original fallback state for unmatched type");
  ObjectLightingState result;
  // Original D37F0 keeps the direction's length as lighting accuracy.
  result.accuracy=float(std::sqrt(double(sample.incident.x)*sample.incident.x+
    double(sample.incident.y)*sample.incident.y+double(sample.incident.z)*sample.incident.z));
  const auto incident=unit(sample.incident);
  const auto lightmap=unpack(sample.lightmap_rgb)+Vec3{v->brightness_offset,v->brightness_offset,v->brightness_offset};
  result.primary_color=clamp_vector(lightmap,v->primary_minimum,v->primary_maximum);
  result.primary_direction=source_object_light_direction(incident,v->exclusion_degrees*.01745329238474369f)*-1.f;
  const auto secondary=clamp_vector(lightmap,v->secondary_minimum,v->secondary_maximum);
  const auto diffuse=clamp_vector(unpack(sample.diffuse_rgb),v->diffuse_minimum,v->diffuse_maximum);
  const float squared=result.accuracy*result.accuracy,inverse=1.f-squared;
  result.secondary_color={diffuse.x*secondary.x*squared*2.f+result.primary_color.x*inverse,
    result.primary_color.y*inverse+diffuse.y*secondary.y*squared*2.f,
    result.primary_color.z*inverse+diffuse.z*secondary.z*squared*2.f};
  result.secondary_direction=rotate_z(result.primary_direction*-1.f,v->secondary_rotation_degrees);
  result.ambient=clamp_vector(lightmap,v->ambient_minimum,v->ambient_maximum);
  result.shadow_direction=incident*-1.f;
  if(result.shadow_direction.z>-.75f) {
    result.shadow_direction.z=std::clamp(result.shadow_direction.z,-1.f,-.75f);
    result.shadow_direction=unit(result.shadow_direction);
  }
  const auto eval=[&](const auto& curve,float input) {
    return shader_function_value(curve,input,1.f,*definition.tables);
  };
  const float shadow_input=std::clamp((lightmap.z*.11f+lightmap.y*.59f+lightmap.x*.3f)*2.f,0.f,1.f);
  result.shadow_opacity=eval(v->shadow_scale,shadow_input);
  result.primary_color=result.primary_color*eval(v->primary_scale,result.accuracy);
  result.secondary_color=result.secondary_color*eval(v->secondary_scale,result.accuracy);
  const float ambient_input=lightmap.z*.114f+lightmap.y*.587f+lightmap.x*.299f;
  const float ambient_scale=eval(v->ambient_scale,ambient_input)*(1.f-(result.accuracy*.1f+.6f))*1.4f;
  result.ambient=result.ambient*ambient_scale;
  result.primary_color=clamp_vector(result.primary_color,{}, {1,1,1});
  result.secondary_color=clamp_vector(result.secondary_color,{}, {1,1,1});
  result.ambient=clamp_vector(result.ambient,{}, {1,1,1});
  return result;
}
bool object_lighting_query_due(const ObjectLightingCache &cache,std::int32_t frame,
    Vec3 origin,float distance,bool frozen) {
  auto elapsed=frame-cache.last_query_frame;
  if(!elapsed)return false;
  const bool initial=!cache.lighting_valid || !cache.sample_valid;
  if(!initial && frozen)return false;
  if(elapsed<0)elapsed=1;
  const auto interval=distance>400?3:distance>200?6:distance>100?9:
    distance>50?15:distance>10?45:60;
  if(cache.last_query_frame!=-1 && elapsed<=interval)return false;
  const auto movement=origin-cache.query_origin;
  return !cache.sample_valid || movement.x*movement.x+movement.y*movement.y+
    movement.z*movement.z>=.002500000176951289f;
}
namespace {
Vec3 smooth_incident(Vec3 current,Vec3 target,float step) {
  const auto magnitude=[](Vec3 v) {
    return float(std::sqrt(double(v.x)*v.x+double(v.y)*v.y+double(v.z)*v.z));
  };
  const auto current_length=std::clamp(magnitude(current),.0001f,10.f);
  const auto target_length=std::clamp(magnitude(target),.0001f,10.f);
  const auto a=current*(1.f/current_length),b=target*(1.f/target_length);
  const float cosine=a.x*b.x+a.y*b.y+a.z*b.z;
  if(std::abs(cosine)<.9998999834060669f && current_length>.0001f && target_length>.0001f) {
    const float angle=float(std::acos(cosine));
    const auto axis=unit(Vec3{b.z*a.y-b.y*a.z,a.z*b.x-b.z*a.x,b.y*a.x-a.y*b.x});
    const float sine=float(std::sin(double(angle*step))),rotation=float(std::cos(double(angle*step)));
    const float projection=(axis.x*a.x+axis.y*a.y+axis.z*a.z)*(1.f-rotation);
    const float length=current_length+std::clamp(target_length-current_length,-step,step);
    return {(axis.x*projection+a.x*rotation-(axis.z*a.y-axis.y*a.z)*sine)*length,
      (axis.y*projection+a.y*rotation-(a.z*axis.x-axis.z*a.x)*sine)*length,
      (axis.z*projection+a.z*rotation-(a.x*axis.y-axis.x*a.y)*sine)*length};
  }
  const auto difference=target-current;
  return current+clamp_vector(difference,{-step,-step,-step},{step,step,step});
}
std::uint32_t smooth_rgb(std::uint32_t current,std::uint32_t target,float step) {
  const auto a=unpack(current),b=unpack(target);
  return pack_object_lighting_rgb(a+clamp_vector(b-a,{-step,-step,-step},{step,step,step}));
}
}
void update_object_lighting_cache(ObjectLightingCache &cache,
    const ObjectLightingDefinition &definition,unsigned type,std::int32_t frame,
    Vec3 origin,Vec3 velocity,bool attempted,const std::optional<ObjectLightmapSample> &query,
    bool interpolation,bool frozen) {
  if(cache.last_query_frame==frame)return;
  const bool initial=!cache.lighting_valid || !cache.sample_valid;
  if(!initial && frozen){cache.last_rendered_frame=frame;return;}
  const bool discontinuity=cache.last_rendered_frame==-1 || frame-cache.last_rendered_frame>1;
  if(attempted) {
    cache.last_query_frame=frame;
    if(query)cache.target=*query,cache.query_origin=origin,cache.sample_valid=true;
  }
  if(cache.sample_valid) {
    if(!cache.lighting_valid || !interpolation || initial || discontinuity)cache.current=cache.target;
    else {
      const float speed=float(std::sqrt(double(velocity.x)*velocity.x+double(velocity.y)*velocity.y+double(velocity.z)*velocity.z));
      const float rate=std::clamp(float((double(speed)-.15000000596046448)*.03350083902478218),0.f,1.f);
      //3DFE0's displacement branch still tests rate>0 before admitting
      // interpolation. Movement alone preserves the sample; a typed MACH
      // owner reaches the source's separate BADC0 mask0x80 branch.
      if(rate>0 || type==7) {
        cache.current.incident=smooth_incident(cache.current.incident,cache.target.incident,rate*.01f);
        cache.current.lightmap_rgb=smooth_rgb(cache.current.lightmap_rgb,cache.target.lightmap_rgb,rate*.5f);
        cache.current.diffuse_rgb=smooth_rgb(cache.current.diffuse_rgb,cache.target.diffuse_rgb,rate*.5f);
      }
    }
    cache.lighting=object_lighting(definition,cache.current,type);cache.lighting_valid=true;
  }
  cache.last_rendered_frame=frame;
}
void apply_object_lighting_bindings(PixelProgram &program,ShaderConstants &constants,
    const MaterialPass &pass,const ObjectLightingState &state,
    std::span<const std::int32_t> switches,float specular,float ambient) {
  std::array<ShaderVector,36> colors{};
  std::array<float,24> scalars{};
  colors[14]=vector(state.primary_direction);colors[15]=vector(state.primary_color);
  colors[16]=vector(state.secondary_direction);colors[17]=vector(state.secondary_color);
  colors[23]=vector(clamp_vector(state.primary_color*specular,{}, {1,1,1}));
  colors[24]=vector(clamp_vector(state.ambient*(ambient+1.f),{}, {1,1,1}));
  colors[35]=vector(clamp_vector(state.primary_color*(specular*.5f),{}, {1,1,1}));
  const auto total=state.ambient+state.primary_color;
  scalars[23]=std::clamp(total.z*.114f+total.y*.587f+total.x*.299f,0.f,1.f);
  const auto lighting=[](unsigned source){return (source>=14 && source<=17) || source==23 || source==24 || source==35;};
  // This filter admits only colors through 35 and scalar 23. Apply each
  // retained mapping in source order without allocating temporary lists.
  for(const auto& r:pass.vertex_externs)if(lighting(r.source))
    remap_vertex_constants(constants,colors,{&r,1});
  for(const auto& r:pass.pixel_externs)
    if(((r.component&7)==4 && lighting(r.source)) || ((r.component&7)<4 && r.source==23))
      remap_pixel_externs(program,scalars,colors,{&r,1},pass.fragments,switches);
}
void apply_object_lighting(Material &material,const ObjectLightingState &state) {
  const auto child=[&](std::shared_ptr<Material>& pass) {
    if(!pass)return;
    if(!pass.unique())pass=std::make_shared<Material>(*pass);
    apply_object_lighting(*pass,state);
  };
  child(material.environment_pass);
  for(auto& pass:material.world_overlays_before)child(pass);
  for(auto& pass:material.world_overlays_after)child(pass);
  for(auto& pass:material.world_bloom_passes)child(pass);
  if(!material.forward_pass || material.forward_pass_index>=material.passes.size())return;
  auto& pass=material.passes[material.forward_pass_index];
  apply_object_lighting_bindings(material.forward_program,material.program_constants,
    pass,state,material.shader_switches,material.specular_brightness,material.ambient_brightness);
  // Subsequent shader animation begins with these object-context constants.
  for(const auto& r:pass.vertex_externs) {
    if(r.source<14 || (r.source>17 && r.source!=23 && r.source!=24 && r.source!=35))continue;
    const unsigned bank=(r.destination>>4)&1,index=r.destination&15;
    pass.constants[bank][index]=material.program_constants[bank][index];
  }
}
} // namespace halo2
