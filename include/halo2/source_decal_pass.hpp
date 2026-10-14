#pragma once
#include "halo2/object_lighting.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
struct SourceDecalPass {
  PixelProgram program;
  unsigned blend{},source{},destination{},operation=0x8006;
};
// Whole TU1.5 43A30 program/state producer, layers2..5. Lighting layers0/1
// require the original lightmap/prelight scheduling owner and remain separate.
inline SourceDecalPass source_decal_pass(unsigned layer) {
  if(layer<2 || layer>5)throw FormatError("Decal layer requires original lighting/error owner");
  SourceDecalPass out;auto &w=out.program.words;
  w[0]=0x18140000;w[26]=0xc0;w[53]=0x11001;w[54]=1;
  if(layer<4) {
    w[8]=0xc;w[35]=layer==2?0x081ca03c:0x081c203c;
    w[46]=0xc00;w[53]=0x11002;
    out.source=0x306;out.destination=layer==2?0x300:0;
    out.blend=layer==2?6:9;
  } else if(layer==4) {
    w[8]=0x0c1c0000;w[34]=0x08040000;w[45]=0xc0;
    out.source=out.destination=1;out.operation=0x8008;out.blend=12;
  } else {
    w[0]=0x38140000;w[1]=0x1c0d0000;w[8]=0x0f140000;
    w[9]=0x0c0d1c00;w[10]=0xff;w[27]=0xc0;
    w[34]=0x08040501;w[35]=0x182b200b;w[45]=0xcd;w[46]=0xd00;
    w[53]=0x11002;out.source=1;out.destination=0x303;out.blend=2;
  }
  return out;
}
// Whole134730 mode0 followed by1349E0, before183C10 forces alphaFF.
// RGB packing follows the original nearest-even conversion, not truncation.
inline std::uint32_t source_decal_rgb(Vec3 lower,Vec3 upper,float fraction) {
  const auto channel=[&](float a,float b) {
    volatile float difference=b-a;
    volatile float scaled=difference*fraction;
    volatile float result=a+scaled;
    return result;
  };
  return pack_object_lighting_rgb({channel(lower.x,upper.x),channel(lower.y,upper.y),
                                  channel(lower.z,upper.z)});
}
// Whole17FB60's retained alpha branch. Its32CC0C conversion truncates.
inline std::uint8_t source_decal_alpha(float lifetime,float decay,float elapsed) {
  if(!(lifetime>0 && decay>0))return 255;
  volatile float remaining=lifetime-elapsed;
  if(!(decay>remaining))return 255;
  volatile float fraction=remaining/decay;
  volatile float value=fraction*256.f;
  return std::uint8_t(std::clamp(float(value),0.f,255.f));
}
} // namespace halo2
