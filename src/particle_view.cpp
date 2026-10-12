#include "halo2/particle_view.hpp"
#include <bit>

namespace halo2 {
ParticleViewConstants particle_view_constants(std::span<const float,9> orientation) {
  // These fixed-angle FSIN/FCOS results are verified against both the unchanged
  // 23E460 instructions and the original Vulkan register upload. Preserve
  // their exact float encodings rather than depend on a host libm's rounding.
  constexpr std::array<std::array<std::uint32_t,2>,16> rotation{{
    {0x00000000u,0x3f800000u},{0x3ed03fc9u,0x3f69de1du},
    {0x3f3e3ebeu,0x3f2b4c24u},{0x3f737871u,0x3e9e3779u},
    {0x3f7e98fdu,0xbdd6130bu},{0x3f5db3d7u,0xbf000001u},
    {0x3f167917u,0xbf4f1bbeu},{0x3e54e6c8u,0xbf7a67e2u},
    {0xbe54e6d4u,0xbf7a67e1u},{0xbf16791au,0xbf4f1bbcu},
    {0xbf5db3d8u,0xbefffffdu},{0xbf7e98fdu,0xbdd612f4u},
    {0xbf737870u,0x3e9e377eu},{0xbf3e3ebcu,0x3f2b4c27u},
    {0xbed03fc4u,0x3f69de1fu},{0x343bbd2eu,0x3f800000u}}};
  ParticleViewConstants result{};result[0]={53.f,15.f,0.f,0.15915494f};
  for(unsigned n=0;n<rotation.size();++n)
    for(unsigned c=0;c<2;++c)result[n+1][c]=std::bit_cast<float>(rotation[n][c]);
  for(unsigned n=1;n<=16;++n) {
    const auto next=n==16?1:n+1;
    result[n][2]=result[next][0]-result[n][0];result[n][3]=result[next][1]-result[n][1];
  }
  for(unsigned axis=0;axis<3;++axis)
    result[17+axis]={orientation[axis],orientation[3+axis],orientation[6+axis],0.f};
  result[20]={1.f,0.f,0.99f,42.f};result[21]={2.f,3.051851e-05f,1.f,42.f};
  return result;
}
}
