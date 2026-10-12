#include "halo2/physics_body_matrix.hpp"
#include <array>
#include <stdexcept>

namespace halo2 {
namespace {
using Memory=SourcePhysicsContactMemory;
using Observer=SourcePhysicsContactRecordObserver;
std::uint32_t read(const Memory&m,std::uint32_t address) {
  std::array<std::byte,4>bytes{};m.read(address,bytes);std::uint32_t value{};
  for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(8*n);
  return value;
}
void store(Memory&m,const Observer&observe,std::uint32_t site,
           std::uint32_t address,std::span<const std::byte>bytes) {
  SourcePhysicsContactRecordStore row{site,address,{},{bytes.begin(),bytes.end()}};
  row.before.resize(bytes.size());m.read(address,row.before);m.write(address,bytes);
  if(observe)observe(row);
}
void word(Memory&m,const Observer&observe,std::uint32_t site,
          std::uint32_t address,std::uint32_t value) {
  std::array<std::byte,4>bytes{};
  for(unsigned n=0;n<4;++n)bytes[n]=std::byte((value>>(8*n))&255u);
  store(m,observe,site,address,bytes);
}
}
void source_physics_body_matrix_1_1(Memory&m,std::uint32_t component,
    std::uint32_t index,std::uint32_t output,std::uint32_t scratch,
    const SourcePhysicsBodyMatrixCorrection&correct,const Observer&observe) {
  if(scratch&15u)throw std::invalid_argument("Original1D42D0 requires aligned stack transform storage");
  const auto slot=read(m,component+0x70)+index*0x60u;
  const auto motion=read(m,read(m,slot+0x40)+0x3c);
  const auto transform=motion+0x80;
  if(transform&15u)throw std::invalid_argument("Original1D42D0 requires aligned motion transform storage");
  std::array<std::byte,16>row{};m.read(transform,row);
  const auto flags=read(m,component+4); //1D42F0 precedes first stack store.
  store(m,observe,0x1d42f8,scratch,row);
  constexpr std::array sites{0x1d4300u,0x1d430fu,0x1d4318u};
  for(unsigned n=1;n<4;++n) {
    m.read(transform+n*16,row);store(m,observe,sites[n-1],scratch+n*16,row);
  }
  if(flags&0x40u) {
    if(!correct)throw std::runtime_error("Required original2E0B60 body-matrix correction owner unavailable");
    correct(m,scratch,component+0x30,observe);
  }
  word(m,observe,0x1d4334,output,read(m,0x465030));
  constexpr std::array offsets{0u,4u,8u,0x10u,0x14u,0x18u,0x20u,0x24u,0x28u,0x30u,0x34u,0x38u};
  constexpr std::array out_sites{0x1d433du,0x1d4348u,0x1d4353u,0x1d435eu,
      0x1d4369u,0x1d4374u,0x1d437fu,0x1d438au,0x1d4395u,0x1d43a0u,0x1d43abu,0x1d43b6u};
  for(unsigned n=0;n<offsets.size();++n)
    word(m,observe,out_sites[n],output+4+n*4,read(m,scratch+offsets[n]));
}
} // namespace halo2
