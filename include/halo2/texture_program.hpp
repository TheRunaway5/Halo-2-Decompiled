#pragma once
#include "halo2/render_program.hpp"

namespace halo2 {
// Addressing operations currently executable with decoded RGBA 2D/cube views
// and capability-gated volumes bound to authored PROJECT2D/PROJECT3D stages.
// Luminance bump, HILO and depth-replace operations need additional
// image/state bindings; do not admit them as a successfully translated shader.
inline bool texture_program_supported(const PixelProgram &program) {
  for(unsigned stage=0;stage<4;++stage) {
    const unsigned mode=program.words[54]>>(stage*5)&31;
    switch(mode) {
    case 0:case 1:case 2:case 3:case 4:case 5:case 15:case 16:break;
    case 6:if(!stage)return false;break;
    case 9:case 11:case 12:case 14:case 17:
      if(!stage || ((program.words[55]>>((stage-1)*4))&7)>2)return false;
      break;
    default:return false;
    }
  }
  return true;
}
} // namespace halo2
