#include "halo2/water.hpp"
#include "halo2/math.hpp"
#include "halo2/source_scene_copy.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace halo2 {
namespace {
Vec3 source_normalized(Vec3 v) {
  // 30BF0 retains an effectively zero vector instead of inventing a basis.
  // The squared length and square root execute on x87 at double precision;
  // 30BF0 rounds the result to float before its SSE reciprocal/multiplies.
  const float n=float(std::sqrt((double(v.x)*v.x+double(v.y)*v.y)+double(v.z)*v.z));
  if(std::abs(n)>=.0001f) v=v*(1.f/n);
  return v;
}
}
Vec3 water_world_position(const WaterFrame &frame,Vec3 local) {
  const auto &m=frame.local_to_world;
  return {((m[0][1]*local.y+m[0][0]*local.x)+m[0][2]*local.z)+m[0][3],
          ((m[1][1]*local.y+m[1][0]*local.x)+m[1][2]*local.z)+m[1][3],
          ((m[2][1]*local.y+m[2][0]*local.x)+m[2][2]*local.z)+m[2][3]};
}
PixelProgram water_copy_program() {
  return source_scene_copy_program();
}
std::array<ShaderVector,4> water_copy_taps(float width,float height) {
  if(!std::isfinite(width) || !std::isfinite(height) || width<=0 || height<=0)
    throw FormatError("invalid water copy normalized source spans");
  std::array<ShaderVector,4> out;
  for(unsigned i=0;i<4;++i)out[i]={((i&1)? .25f:-.25f)*width/512.f,
      ((i&2)? .25f:-.25f)*height/256.f,0,1};
  return out;
}
void bind_water_pool_materials(const BspWater &water,std::span<Material> materials) {
  if(water.definitions.empty())return;
  const auto bind=[](const auto &self,Material &material)->void {
    if(material.water_pass && material.forward_vertex==7)material.water_definition=0;
    if(material.environment_pass)self(self,*material.environment_pass);
    if(material.planar_fog_pass)self(self,*material.planar_fog_pass);
    for(auto &pass:material.world_overlays_before)if(pass)self(self,*pass);
    for(auto &pass:material.world_overlays_after)if(pass)self(self,*pass);
    for(auto &pass:material.world_bloom_passes)if(pass)self(self,*pass);
    for(auto &pass:material.world_planar_passes)if(pass)self(self,*pass);
  };
  for(auto &material:materials)bind(bind,material);
}
WaterFrame water_frame(const WaterDefinition &definition,Vec3 eye,Vec3 forward,
                       float vertical_fov,float viewport_aspect,bool anamorphic) {
  const auto &s=definition.parameters;WaterFrame out;
  const double angle=std::atan2(double(forward.x),double(forward.y))+double(s[35]);
  const float sine=float(std::sin(angle)),cosine=float(std::cos(angle));
  // TU1.5 3C640 builds the signed basis with ADDSS/SUBSS from +0,
  // then scales it; stage2 subtracts each product from +0. Moving that
  // subtraction onto the scale changes the source's signed-zero words.
  const ShaderVector basis{cosine,0.f+sine,cosine,0.f-sine};
  const std::array<float,3> scales{s[26],s[27],s[28]};
  for(unsigned i=0;i<3;++i)for(unsigned axis=0;axis<4;++axis) {
    const float product=scales[i]*basis[axis];
    out.bump_matrices[i+1][axis]=i==1?0.f-product:product;
  }
  out.constants[0]={1,-1,-1,.5f};
  out.constants[1]={s[36],0.f-s[37],s[38]>.0001f?1.f/s[38]:s[38],
      s[39]>.0001f?1.f/s[39]:s[39]};
  const double pitch=std::sin(double(s[30]));
  const ShaderVector direction{float(std::cos(double(s[29]))*pitch),
      float(std::sin(double(s[29]))*pitch),float(std::cos(double(s[30]))),s[25]};
  for(unsigned i=0;i<3;++i)out.constants[2][i]=float((double(direction[i])+1.)*.5);
  out.constants[2][3]=.4970000088214874f;
  if(s[34]>.0001f && s[40]>.0001f && s[41]>.0001f && s[41]-s[40]>.0001f) {
    const double inverse=1./s[34];
    const float low=float(inverse*s[40]*16777215.),high=float(inverse*s[41]*16777215.);
    out.constants[3][0]=1.f/(high-low);out.constants[3][1]=-out.constants[3][0]*low;
  }
  out.constants[4]={s[13],s[14],s[15],1};
  out.horizon_color={s[22],s[23],s[24]};out.infinite=s[31]!=0;
  for(unsigned i=0;i<3;++i)out.horizon_argb|=(std::uint32_t(std::lrint(double(s[22+i])*255.))&255u)<<(16-8*i);
  if(!out.infinite)return out;
  const float distance=std::max(std::abs(eye.z-s[33]),.0001f);
  const Vec3 up{0,0,1},left=source_normalized(cross(up,forward));
  const Vec3 horizontal=source_normalized(cross(left,up));
  const Vec3 origin{eye.x,eye.y,s[33]-distance*s[42]};
  const float aspect=viewport_aspect*(anamorphic?1.5f:1.f);
  const long double source_tangent=std::tan(static_cast<long double>(s[31])*.5L)*s[32];
  const float tangent=float(source_tangent);
  const float half_angle=std::min(vertical_fov*.5f,.7806857824325562f);
  const float view_tangent=float(std::tan(static_cast<long double>(half_angle))*aspect);
  float width=view_tangent/tangent;
  if(width<=1)width=1;else if(width>1)width*=1.13f;
  width*=distance;
  out.local_to_world={ShaderVector{horizontal.x*distance,left.x*width,up.x,origin.x},
      ShaderVector{horizontal.y*distance,left.y*width,up.y,origin.y},
      ShaderVector{horizontal.z*distance,left.z*width,up.z,origin.z}};
  const float near_x=float(std::tan(static_cast<long double>(s[31])*.04f*-12.f));
  const float near_y=float(std::sqrt(static_cast<long double>(near_x)*near_x+1.L)*source_tangent),far_x=s[34];
  const float far_y=float(std::sqrt(static_cast<long double>(far_x)*far_x+1.L)*source_tangent);
  for(unsigned i=0;i<4;++i)out.corners[i]=water_world_position(out,
      {i<2?near_x:far_x,i==0?-near_y:i==1?near_y:i==2?far_y:-far_y,0});
  return out;
}

BspWater Cache::load_bsp_water(std::size_t index) const {
  if(index>=bsps_.size())throw FormatError("water BSP index outside scenario");
  const auto &bsp=bsps_[index];const Bytes metadata(Bytes(cache_bytes()).slice(bsp.file_offset,bsp.size));
  
  const auto [count,start]=TagAddressSpace(metadata, bsp.memory_address).block(16+0x214, 0xac);BspWater out;
  for(std::size_t i=0;i<count;++i) {
    const auto at=start+i*0xac;WaterDefinition definition;definition.shader=metadata.u32(at+4);
    for(unsigned j=13;j<43;++j) {
      definition.parameters[j]=metadata.f32(at+j*4);
      if(!std::isfinite(definition.parameters[j]))throw FormatError("nonfinite authored water state");
    }
    const auto [resource_count,resources]=TagAddressSpace(metadata, bsp.memory_address).block(at+0x20, 16);
    if(resource_count) {
      const auto raw=raw_data(metadata.u32(at+0x10),metadata.u32(at+0x14));const Bytes data(raw);
      const std::size_t base=std::size_t(metadata.u32(at+0x18))+8;
      std::size_t positions=0,position_size=0,indices=0,index_size=0,stride=0;
      for(std::size_t r=0;r<resource_count;++r) {
        const auto res=resources+r*16;const auto type=metadata.u16(res+4),channel=metadata.u16(res+6);
        const auto size=metadata.u32(res+8),offset=metadata.u32(res+12);
        data.require(base+offset,size);
        if(type==32){indices=base+offset;index_size=size;}
        if(type==56 && channel==0){positions=base+offset;position_size=size;}
        if(type==56 && channel==32) {
          if(size!=32)throw FormatError("unexpected water vertex declaration count");
          stride=std::to_integer<unsigned>(data.slice(base+offset+1,1)[0]);
        }
      }
      if(stride!=12 || !position_size || position_size%stride || !index_size || index_size%8)
        throw FormatError("unsupported water position-only quad layout");
      definition.geometry.minimum={std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max()};
      definition.geometry.maximum=definition.geometry.minimum*-1.f;
      // 3C3C0 calls 1C8A0 with D3DPT_QUADLIST (8); preserve each quad's
      // original winding with the same 0-2 diagonal during triangle expansion.
      for(std::size_t q=0;q<index_size/2;q+=4) {
        std::array<unsigned,4> quad{};
        for(unsigned k=0;k<4;++k) {
          quad[k]=data.u16(indices+(q+k)*2);
          if(quad[k]>=position_size/stride)throw FormatError("water index outside position stream");
        }
        for(const unsigned k:{0u,1u,2u,0u,2u,3u}) {
          const auto p=positions+quad[k]*stride;
          const Vec3 position{data.f32(p),data.f32(p+4),data.f32(p+8)};
          if(!std::isfinite(position.x)||!std::isfinite(position.y)||!std::isfinite(position.z))
            throw FormatError("nonfinite water position");
          definition.geometry.vertices.push_back(Vertex{position,{1,1,1},{},0});
          definition.geometry.minimum=minimum(definition.geometry.minimum,position);
          definition.geometry.maximum=maximum(definition.geometry.maximum,position);
        }
      }
      // Some DLC BSPs retain the raw infinite grid with a NONE shader.
      // 3C3C0 never submits that resource; retain it without inventing a draw.
      if(definition.shader!=0xffffffffu) {
        Material material=load_material(definition.shader);
        if(!prepare_water_material(*this,material,true))throw FormatError("water geometry has unsupported authored pass");
        definition.geometry.materials.push_back(std::move(material));
      }
    }
    out.definitions.push_back(std::move(definition));
  }
  return out;
}
}
