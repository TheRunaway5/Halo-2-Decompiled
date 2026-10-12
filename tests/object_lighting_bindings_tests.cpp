#include "halo2/object_lighting.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace halo2;
Vec3 clamp_vector(Vec3 v,Vec3 low,Vec3 high) {return {std::clamp(v.x,low.x,high.x),std::clamp(v.y,low.y,high.y),std::clamp(v.z,low.z,high.z)};}
ShaderVector lightvec(Vec3 v) {return {v.x,v.y,v.z,0};}
void original_bindings(PixelProgram &program,ShaderConstants &constants,
    const MaterialPass &pass,const ObjectLightingState &state,
    std::span<const std::int32_t> switches,float specular,float ambient) {
  std::array<ShaderVector,256> colors{};
  std::array<float,256> scalars{};
  colors[14]=lightvec(state.primary_direction);colors[15]=lightvec(state.primary_color);
  colors[16]=lightvec(state.secondary_direction);colors[17]=lightvec(state.secondary_color);
  colors[23]=lightvec(clamp_vector(state.primary_color*specular,{}, {1,1,1}));
  colors[24]=lightvec(clamp_vector(state.ambient*(ambient+1.f),{}, {1,1,1}));
  colors[35]=lightvec(clamp_vector(state.primary_color*(specular*.5f),{}, {1,1,1}));
  const auto total=state.ambient+state.primary_color;
  scalars[23]=std::clamp(total.z*.114f+total.y*.587f+total.x*.299f,0.f,1.f);
  const auto lighting=[](unsigned source){return (source>=14 && source<=17) || source==23 || source==24 || source==35;};
  std::vector<ShaderRemap> vertex,pixel;
  for(const auto& r:pass.vertex_externs)if(lighting(r.source))vertex.push_back(r);
  for(const auto& r:pass.pixel_externs)
    if(((r.component&7)==4 && lighting(r.source)) || ((r.component&7)<4 && r.source==23))
      pixel.push_back(r);
  remap_vertex_constants(constants,colors,vertex);
  remap_pixel_externs(program,scalars,colors,pixel,pass.fragments,switches);
}
int main() {
 unsigned seed=0xa17109u;const auto next=[&]() {seed=1664525u*seed+1013904223u;return seed;};
 const auto scalar=[&]() {return float(int(next()%8193)-4096)/256.f;};
 for(unsigned trial=0;trial<4096;++trial) {
  MaterialPass pass;
  for(unsigned i=0;i<24;++i) {
   pass.vertex_externs.push_back({std::uint16_t(next()%32),std::uint8_t(next()%8),std::uint8_t(next()%43)});
   pass.pixel_externs.push_back({std::uint16_t(((next()%9)<<6)|((next()%2)<<10)|((next()%2)<<11)),std::uint8_t(next()%48),std::uint8_t(next()%43)});
  }
  ObjectLightingState state;state.primary_direction={scalar(),scalar(),scalar()};
  state.secondary_direction={scalar(),scalar(),scalar()};state.primary_color={scalar(),scalar(),scalar()};
  state.secondary_color={scalar(),scalar(),scalar()};state.ambient={scalar(),scalar(),scalar()};
  PixelProgram actual,expected;for(auto &word:actual.words)word=next();expected=actual;
  ShaderConstants ac,ex;for(auto &bank:ac)for(auto &v:bank)for(auto &x:v)x=scalar();ex=ac;
  const auto specular=scalar(),ambient=scalar();
  original_bindings(expected,ex,pass,state,{},specular,ambient);
  apply_object_lighting_bindings(actual,ac,pass,state,{},specular,ambient);
  if(actual.words!=expected.words || std::memcmp(&ac,&ex,sizeof(ac)))throw std::runtime_error("compact lighting bindings differ from original batch implementation");
 }
 std::cout<<"4096 complete lighting banks match the original batch implementation\n";
}
