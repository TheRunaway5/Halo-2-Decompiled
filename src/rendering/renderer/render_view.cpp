#include "halo2/render_view.hpp"
#include <cmath>
#include <stdexcept>
namespace halo2 {
RenderViewport full_surface_viewport(RenderViewport layout,
    std::uint32_t width,std::uint32_t height) {
  if(!width || !height)throw std::invalid_argument("full-surface extent must be positive");
  const float sx=float(width)/640.f,sy=float(height)/480.f;
  const float left=layout.left*sx,top=layout.top*sy;
  const float right=left+layout.width*sx,bottom=top+layout.height*sy;
  const auto edge=[](float value) {
    // Check before conversion: no overflowing float-to-int conversion is
    // permitted, and the original camera stores each edge in a signed short.
    if(!std::isfinite(value) || value<-32768.f || value>=32768.f)
      throw std::invalid_argument("full-surface camera edge exceeds its signed-short bank");
    return int(value);
  };
  const int l=edge(left),t=edge(top),r=edge(right),b=edge(bottom);
  if(r<=l || b<=t)throw std::invalid_argument("full-surface camera rectangle must be positive");
  return {float(l),float(t),float(r-l),float(b-t)};
}
std::array<ShaderVector,4> viewport_constants(RenderViewport v) {
  float scale=1,bias=.03125f;
  if(v.scaled && !v.scaling_disabled)
    scale=v.render_scale<.0625f?.0625f:v.render_scale>1?1:v.render_scale;
  if(v.mode==2) {v.width=v.height=128;v.left=v.top=0;scale=1;bias=0;}
  return {{{v.width*.5f*scale,0,0,((v.width+1)*.5f+v.left)*scale+bias},
           {0,v.height*-.5f*scale,0,((v.height+1)*.5f+v.top)*scale+bias},
           {v.width*.5f*scale,v.height*-.5f*scale,16777215,0},
           {(v.width*.5f+v.left)*scale+.5f,(v.height*.5f+v.top)*scale+.5f,0,0}}};
}
} // namespace halo2
extern "C" int halo2_viewport_constants(float left,float top,float width,
  float height,float scale,unsigned flags,unsigned mode,float *output) {
  if(!output)return 0;
  const auto values=halo2::viewport_constants({left,top,width,height,scale,
    bool(flags&1),bool(flags&2),mode});
  for(unsigned i=0;i<4;++i)for(unsigned a=0;a<4;++a)output[i*4+a]=values[i][a];
  return 1;
}
