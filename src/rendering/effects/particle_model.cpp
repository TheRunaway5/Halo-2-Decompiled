#include "halo2/particle_model.hpp"
#include "halo2/particle_material.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/math.hpp"
#include <cmath>

namespace halo2 {
ParticleModelDefinition load_particle_model(const Cache &cache,std::uint32_t datum) {
  const auto tag=cache.load_source_tag_metadata(datum);
  if(tag.group()!="PRTM")throw FormatError("Particle model requires an authored PRTM tag");
  const Bytes b(tag.bytes_at(tag.source_address(),tag.size()));b.require(0,0xe0);
  const auto block=[&](unsigned at,unsigned stride) {
    const auto d=read_tag_block_descriptor(b,at);
    return Bytes(d.count?tag.bytes_at(d.address,d.extent(stride,65536)):std::span<const std::byte>{});
  };
  ParticleModelDefinition out;out.flags=b.u32(0);out.orientation=b.u32(4);
  out.shader=b.u32(0x1c);out.required_values=b.u32(0xd8);
  if(out.orientation>4 || b.u32(0x78))throw FormatError("Particle model requires another original orientation/child owner");
  for(unsigned i=0;i<4;++i) {
    const auto at=0x20+i*0x10;auto &p=out.properties[i];
    p.input=std::int16_t(b.u16(at));p.range=std::int16_t(b.u16(at+2));
    p.modifier=b.u16(at+4);p.modifier_input=std::int16_t(b.u16(at+6));
    const auto n=b.u32(at+8);if(n>4096)throw FormatError("Particle model function exceeds bounds");
    const auto bytes=n?tag.bytes_at(b.u32(at+12),n):std::span<const std::byte>{};
    p.curve.assign(bytes.begin(),bytes.end());
  }
  const auto spans=block(0x80,8),vertices=block(0x88,0x38),indices=block(0x90,2);
  if(!b.u32(0x80) || !b.u32(0x88) || !b.u32(0x90))throw FormatError("Particle model has no resident authored geometry");
  for(unsigned i=0;i<b.u32(0x80);++i) {
    const unsigned first=spans.u16(i*8+4),count=spans.u16(i*8+6);
    if(first>b.u32(0x90) || count>b.u32(0x90)-first || count%3)
      throw FormatError("Particle model triangle span escapes index bank");
    out.spans.push_back({std::uint16_t(first),std::uint16_t(count)});
  }
  //1C330 assigns registers by the actual declaration's semantic ordinal.
  const auto material=cache.load_particle_material(datum,15,0,1);
  const auto declaration=cache.load_source_vertex_declaration(material.vertex.vertex_datum,1);
  if(declaration.semantics!=std::vector<std::uint16_t>{0,3,14})
    throw FormatError("Particle model GPU stream requires its authored position/UV/radiance declaration");
  for(unsigned i=0;i<b.u32(0x88);++i) {
    auto &v=out.vertices.emplace_back();
    for(unsigned j=0;j<declaration.semantics.size();++j) {
      unsigned off=0;bool uv=false;
      switch(SourceVertexSemantic(declaration.semantics[j])) {
      case SourceVertexSemantic::Position:off=0;break;
      case SourceVertexSemantic::Normal:off=12;break;
      case SourceVertexSemantic::Tangent:off=24;break;
      case SourceVertexSemantic::Binormal:off=36;break;
      case SourceVertexSemantic::UV:off=48;uv=true;break;
      case SourceVertexSemantic::Radiance:continue; //224FD0 sets this constant attribute per particle.
      default:throw FormatError("Particle model requires another authored stream semantic");
      }
      const auto a=i*0x38+off;v[j]={vertices.f32(a),vertices.f32(a+4),uv?0.f:vertices.f32(a+8),1.f};
    }
  }
  for(unsigned i=0;i<b.u32(0x90);++i) {
    const auto v=indices.u16(i*2);if(v>=out.vertices.size())throw FormatError("Particle model index escapes vertex bank");
    out.indices.push_back(v);
  }
  return out;
}
ParticleModelSubmission particle_model_submission(const ParticleModelDefinition &d,
    const ParticleSpriteInput &in,std::uint16_t model_random,ParticleValueCache &cache,
    const ParticleValueSources &sources,const ShaderFunctionTables &tables) {
  complete_particle_values(cache,d.required_values&0xf80fu,sources);
  const auto value=[&](const ParticleProperty &p) {
    if(p.input<0 || p.input>=17 || p.range<0 || p.range>=17 || p.modifier_input>=17)
      throw FormatError("Particle model property exceeds value bank");
    float v=shader_function_value(p.curve,cache.values[p.input],cache.values[p.range],tables);
    if(p.modifier_input>=0) {
      if(p.modifier==1)v=cache.values[p.modifier_input]+v;
      else if(p.modifier==2)v=cache.values[p.modifier_input]*v;
    }
    return v;
  };
  const float angle=value(d.properties[3])*6.2831854820251465f;
  const float sine=float(std::sin(double(angle))),cosine=float(std::cos(double(angle)));
  const Vec3 axis=(d.orientation==2 || d.orientation==4)?Vec3{0,0,1}:Vec3{1,0,0};
  const float x2=axis.x*axis.x,y2=axis.y*axis.y,z2=axis.z*axis.z;
  const float xy=(1.f-cosine)*axis.x*axis.y,zx=(1.f-cosine)*axis.z*axis.x,yz=(1.f-cosine)*axis.z*axis.y;
  std::array<Vec3,3> rotation{{{(1.f-x2)*cosine+x2,xy+sine*axis.z,zx-sine*axis.y},
    {xy-sine*axis.z,(1.f-y2)*cosine+y2,yz+sine*axis.x},
    {zx+sine*axis.y,yz-sine*axis.x,(1.f-z2)*cosine+z2}}};
  for(unsigned i=0;i<3;++i)rotation[i]=rotation[i]*(value(d.properties[i])*in.scale);
  Vec3 forward,left,up;
  if(d.orientation==0) {forward=sources.camera_forward*-1.f;up=sources.camera_up;left=cross(up,forward);}
  else if(d.orientation==1 || d.orientation==2) {
    //30BF0 accumulates/square-roots in x87 before its binary32 store, then
    // uses SSE for reciprocal/scaling. Rounding each squared component to
    // binary32 first changes the original224FD0 recharge transform.
    up=in.direction;const double nx=up.x,ny=up.y,nz=up.z;
    const double magnitude=std::sqrt((nx*nx+ny*ny)+nz*nz);
    const float m=float(magnitude);
    if(!(std::abs(magnitude)<double(.0001f)))up=up*(1.f/m);
    const float x=std::abs(up.x),y=std::abs(up.y),z=std::abs(up.z);
    if(y>=x && z>=x)forward={0,up.z,0.f-up.y};
    else if(z>=y)forward={0.f-up.z,0,up.x};
    else forward={up.y,0.f-up.x,0}; //11D000, deliberately not normalized again.
    left=cross(up,forward);
  } else {up={0,0,1};left=cross(up,sources.camera_forward);forward=cross(left,up);}
  for(unsigned i=0;i<3;++i) {
    const auto v=rotation[i];
    rotation[i]={(forward.x*v.x+left.x*v.y)+up.x*v.z,
      i?(up.y*v.z+left.y*v.y)+forward.y*v.x:(forward.y*v.x+left.y*v.y)+up.y*v.z,
      i?(up.z*v.z+left.z*v.y)+forward.z*v.x:(forward.z*v.x+left.z*v.y)+up.z*v.z};
  }
  const auto span=d.spans.at(model_random%d.spans.size());
  return {{{{rotation[0].x,rotation[1].x,rotation[2].x,in.position.x},
    {rotation[0].y,rotation[1].y,rotation[2].y,in.position.y},
    {rotation[0].z,rotation[1].z,rotation[2].z,in.position.z}}},span[0],span[1]};
}
}
