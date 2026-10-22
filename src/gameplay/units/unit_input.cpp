#include "halo2/unit_input.hpp"
#include <cstring>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint16_t word(std::span<const std::byte> bytes,std::size_t offset) {
  return std::uint16_t(std::to_integer<unsigned>(bytes[offset]) |
      (std::to_integer<unsigned>(bytes[offset+1])<<8));
}
void extent(std::span<const std::byte> unit,std::size_t minimum) {
  if(unit.size()<minimum)throw std::invalid_argument("Truncated source unit storage");
}
}
void apply_source_unit_input(std::span<std::byte> unit,std::span<const std::byte,0x7c> input) {
  extent(unit,0x242);
  auto copy=[&](std::size_t destination,std::size_t source,std::size_t count) {
    std::memcpy(unit.data()+destination,input.data()+source,count);
  };
  // Preserve original field order and bits; opaque fields acquire no invented
  // semantic labels or floating-point normalization.
  for(unsigned n=0;n<3;++n)copy(0x1b0+n*4,0x14+n*4,4);
  copy(0x1c0,0x20,4);copy(0x1c4,0x24,4);copy(0x1bc,4,1);
  if(word(input,6)==word(unit,0x214))copy(0x214,6,4);
  if(word(input,0xa)!=0xffff)copy(0x23d,0xa,1);
  copy(0x241,0xc,1);copy(0x148,0x10,4);
  for(unsigned n=0;n<3;++n)copy(0x180+n*4,0x40+n*4,4);
  for(unsigned n=0;n<3;++n)copy(0x15c+n*4,0x34+n*4,4);
  for(unsigned n=0;n<3;++n)copy(0x150+n*4,0x28+n*4,4);
  copy(0x14c,0,4);
  for(unsigned n=0;n<3;++n)copy(0x1a4+n*4,0x4c+n*4,4);
  for(unsigned n=0;n<9;++n)copy(0x1c8+n*4,0x58+n*4,4);
}
std::int16_t source_inventory_empty_slot(std::span<const std::byte> unit) {
  extent(unit,0x228);
  for(std::int16_t slot=0;slot<4;++slot) {
    const auto offset=0x218+std::size_t(slot)*4;
    if(word(unit,offset)==0xffff && word(unit,offset+2)==0xffff)return slot;
  }
  return -1;
}
} // namespace halo2
