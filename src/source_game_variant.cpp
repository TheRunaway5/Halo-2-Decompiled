#include "halo2/source_game_variant.hpp"
#include "halo2/frontend.hpp"
#include <bit>
#include <algorithm>
#include <limits>

namespace halo2 {
namespace {
constexpr std::array<SourceGameVariantDescriptor,112> descriptors{{
#include "source_game_variant_descriptors.inc"
}};
std::uint32_t read(const SourceGameVariant &v, unsigned offset, unsigned width) {
  std::uint32_t value = 0;
  for (unsigned n=0; n<width; ++n)
    value |= std::uint32_t(v.bytes.at(offset+n)) << (n*8);
  return value;
}
void write(SourceGameVariant &v, unsigned offset, std::uint32_t value, unsigned width=4) {
  for (unsigned n=0; n<width; ++n)
    v.bytes.at(offset+n)=std::uint8_t(value >> (n*8));
}
const SourceGameVariantDescriptor *descriptor(std::int32_t parameter) {
  return parameter>=0 && parameter<int(descriptors.size())
      ? &descriptors[unsigned(parameter)] : nullptr;
}
} // namespace
void GameVariantSettings::set(unsigned parameter, int value) {
  std::erase_if(writes_, [&](const auto &write) { return write.first == parameter; });
  append(parameter, value);
}
std::span<const SourceGameVariantDescriptor,112> source_game_variant_descriptors() {
  return descriptors;
}
void source_game_variant_set_title(SourceGameVariant &v, std::u16string_view title) {
  bool ended = false;
  for (unsigned i = 0; i < 32; ++i) {
    const auto value = !ended && i < 31 && i < title.size() ? title[i] : char16_t{};
    ended |= value == 0;
    write(v, 4 + i * 2, std::uint16_t(value), 2);
  }
}
std::u16string source_game_variant_title(const SourceGameVariant &v) {
  std::u16string title;
  for (unsigned i = 0; i < 32; ++i) {
    const auto value = char16_t(read(v, 4 + i * 2, 2));
    if (!value) break;
    title += value;
  }
  return title;
}
std::int32_t source_game_variant_type(const SourceGameVariant &v) {
  return std::bit_cast<std::int32_t>(read(v, 0x44, 4));
}
SourceGameVariant source_game_variant_default(unsigned engine, std::u16string_view title) {
  // Literal stores in original19D220 / TU1A0A90, with zeroed stack buffer.
  constexpr std::array<unsigned,7> types{2,3,7,4,1,9,8};
  constexpr std::array<unsigned,7> scores{25,120,15,120,3,3,300};
  if (engine>=types.size()) throw FormatError("Unknown multiplayer game engine");
  SourceGameVariant v;
  write(v,0,0xff000000u);
  write(v,0x44,types[engine]);
  write(v,0x48,0xf9au | unsigned(engine>=4));
  write(v,0x50,scores[engine]);
  write(v,0x54,480);
  write(v,0x74,16); write(v,0x78,16);
  write(v,0x80,engine>=4 ? 10 : 5); write(v,0x84,engine>=4 ? 10 : 5);
  write(v,0xa8,2); write(v,0xac,10);
  source_game_variant_set_title(v, title);
  switch (engine) {
  case 0: write(v,0xf0,2); break;
  case 1: write(v,0xf4,1); break;
  case 2: write(v,0xf0,0x1b); write(v,0xf4,2,2); break;
  case 3: write(v,0xf4,60,2); break;
  case 4: write(v,0xf0,2); write(v,0xf4,30); break;
  case 5:
    write(v,0xf0,0x22); write(v,0xf4,30); write(v,0x100,1);
    write(v,0x108,5,2); write(v,0x10a,3,2); break;
  case 6: write(v,0xf0,0x50003); write(v,0xf4,5,2); break;
  }
  return v;
}
bool source_game_variant_normalize(SourceGameVariant &v, std::u16string_view slayer_title) {
  const auto before = v;
  auto mask = [&](unsigned offset, std::uint32_t bits, unsigned width = 4) {
    write(v, offset, read(v, offset, width) & bits, width);
  };
  auto clamp = [&](unsigned offset, int lower, int upper, unsigned width = 4) {
    const auto raw = read(v, offset, width);
    const int value = width == 1 ? int(std::bit_cast<std::int8_t>(std::uint8_t(raw)))
        : width == 2 ? int(std::bit_cast<std::int16_t>(std::uint16_t(raw)))
                     : std::bit_cast<std::int32_t>(raw);
    write(v, offset, std::uint32_t(std::clamp(value, lower, upper)), width);
  };
  mask(0, 1, 2); write(v, 0x42, 0, 2); mask(0x48, 0x7fff);
  // Literal shared scalar stores in19D650/1A0EC0. Signed byte and short
  // interpretation belongs to this checker, independently of getter width.
  for (const auto offset : {0x50u,0x54u,0x7cu,0x80u,0x84u,0xacu}) clamp(offset,0,0x7fff);
  for (const auto offset : {0x58u,0x88u,0xa4u,0xa8u}) clamp(offset,0,2);
  clamp(0x4c,0,6); clamp(0x74,0,16); clamp(0x78,0,16);
  write(v,0xb0,0); clamp(0xb4,0,8);
  constexpr std::array<int,12> maxima{3,7,7,4,4,4,6,6,0x13,3,0x14,0x14};
  for (unsigned i=0;i<maxima.size();++i) clamp(0xcc+i,0,maxima[i],1);
  const auto source_type = source_game_variant_type(before);
  clamp(0x44,1,9);
  if (source_game_variant_type(v) == source_type) {
    switch (source_type) {
    case 9:
      clamp(0x108,0,15,2); clamp(0x10a,0,15,2);
      [[fallthrough]];
    case 1:
      write(v,0x48,read(v,0x48,4) | 1); mask(0xf0,0xff);
      clamp(0xf4,0,0x7fff); clamp(0xf8,0,2); clamp(0xfc,0,1);
      clamp(0x100,0,3); clamp(0x104,0,2);
      break;
    case 2: mask(0xf0,7); break;
    case 3:
      mask(0xf0,7); clamp(0xf4,0,3,2); clamp(0xf6,0,1,2);
      clamp(0xf8,0,2,2); clamp(0xfa,0,3,2);
      break;
    case 4: mask(0xf0,0x1f); clamp(0xf4,0,0x7fff,2); break;
    case 7:
      mask(0x48,0xfffffffe); mask(0xf0,0x7f); clamp(0xf4,0,2,2);
      break;
    case 8:
      clamp(0xf0,1,8,2); clamp(0xf2,1,0x7fff,2); clamp(0xf4,1,0x7fff,2);
      break;
    default: write(v,0x44,10); break;
    }
  }
  if (v.bytes == before.bytes) return true;
  v = source_game_variant_default(0, slayer_title);
  return false;
}
bool source_game_variant_valid(const SourceGameVariant &v) {
  auto copy = v;
  return source_game_variant_normalize(copy);
}
void source_game_variant_set(SourceGameVariant &v, std::int32_t parameter,
                             std::int32_t value) {
  const auto *d=descriptor(parameter);
  if (!d || d->kind<0) return;
  if (d->kind==1) {
    const auto mask=std::uint32_t(1) << (d->operand & 31);
    const auto old=read(v,d->offset,4);
    write(v,d->offset,value ? old|mask : old&~mask);
  } else {
    // Every original numeric descriptor in both pinned images has scale1.
    // Thus the original round(1/scale) multiply is exact, including narrowing.
    if (d->operand!=0x3f800000u) throw FormatError("Unrecovered game variant scale");
    const auto width=d->kind==3 ? 1u : d->kind==4 ? 2u : d->kind==5 ? 4u : 0u;
    if (!width) throw FormatError("Unrecovered game variant descriptor");
    write(v,d->offset,std::bit_cast<std::uint32_t>(value),width);
  }
}
std::int32_t source_game_variant_get(const SourceGameVariant &v, std::int32_t parameter) {
  const auto *d=descriptor(parameter);
  if (!d || d->kind<0) return 0;
  if (d->kind==1) return (read(v,d->offset,4) >> (d->operand & 31)) & 1;
  if (d->kind==3) return std::int32_t(read(v,d->offset,1));
  if (d->kind==4)
    return std::bit_cast<std::int16_t>(std::uint16_t(read(v,d->offset,2)));
  if (d->kind==5) {
    // Original CVTSI2SS -> MULSS(scale1) -> x87 FISTP. The intermediate
    // float still rounds large integers and can produce the indefinite value.
    const float value=float(std::bit_cast<std::int32_t>(read(v,d->offset,4)));
    return double(value)>=2147483648. ? std::numeric_limits<std::int32_t>::min()
        : std::int32_t(value);
  }
  throw FormatError("Unrecovered game variant descriptor");
}
SourceGameVariant source_game_variant(const UiVariant &variant) {
  auto result=source_game_variant_default(variant.engine);
  for (const auto &[parameter,value] : variant.values)
    source_game_variant_set(result,std::bit_cast<std::int32_t>(std::uint32_t(parameter)),value);
  return result;
}
} // namespace halo2
