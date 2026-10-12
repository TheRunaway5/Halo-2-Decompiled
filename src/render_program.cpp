#include "halo2/render_program.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
namespace {
template <class T>
std::span<const T> range(std::span<const T> data, ShaderRange r) {
  if (r.first() > data.size() || r.count() > data.size() - r.first())
    throw FormatError("shader range outside block");
  return data.subspan(r.first(), r.count());
}
std::size_t permutation_index(const ShaderFragment &fragment,
                              std::span<const std::int32_t> switches) {
  std::int32_t selected = 0;
  if (fragment.switch_index != -1) {
    if (fragment.switch_index < 0 ||
        std::size_t(fragment.switch_index) >= switches.size())
      throw FormatError("unresolved shader switch");
    selected = switches[fragment.switch_index];
  }
  if (selected < 0 || std::size_t(selected) >= fragment.permutations.count())
    throw FormatError("shader switch outside permutation range");
  return fragment.permutations.first() + selected;
}
} // namespace
PackedVertexProgramBank pack_vertex_program_bank(const VertexProgramBank &bank) {
  if(bank.first_constant>192 || bank.constants.size()>192-bank.first_constant ||
     bank.constants.size()>158 ||
     (bank.camera_inputs ? bank.first_constant!=0 || bank.constants.size()<12 :
                           bank.first_constant<34))
    throw FormatError("draw vertex globals outside source register bank");
  PackedVertexProgramBank result;
  result.range={bank.first_constant,unsigned(bank.constants.size()),
                bank.prt_inputs?1u:0u,bank.camera_inputs?1u:0u};
  std::copy(bank.constants.begin(),bank.constants.end(),result.constants.begin());
  return result;
}
PixelProgram
assemble_pixel_program(const PixelProgram &definition,
                       std::span<const ShaderFragment> fragments,
                       std::span<const ShaderPermutation> permutations,
                       std::span<const ShaderCombiner> combiners,
                       std::span<const std::int32_t> switches) {
  if (fragments.empty())
    return definition;
  if (fragments.size() > 8)
    throw FormatError("too many pixel shader fragments");
  PixelProgram result{};
  // Texture modes, dot mapping and input texture survive fragment assembly.
  std::copy_n(definition.words.begin() + 54, 3, result.words.begin() + 54);
  std::size_t stage = 0;
  bool final = false;
  for (std::size_t f = 0; f < fragments.size(); ++f) {
    range(permutations, fragments[f].permutations);
    result.fragment_offsets[f] = static_cast<std::uint8_t>(stage);
    const auto &permutation =
        permutations[permutation_index(fragments[f], switches)];
    const auto records = range(combiners, permutation.combiners);
    for (std::size_t i = 0; i < records.size(); ++i) {
      const auto &c = records[i].words;
      if ((permutation.flags & 1) && i + 1 == records.size()) {
        result.words[8] = c[0];
        result.words[9] = c[2];
        result.words[43] = c[4];
        result.words[44] = c[5];
        final = true;
        break;
      }
      if (stage == 8)
        throw FormatError("pixel program exceeds eight combiners");
      result.words[34 + stage] = c[0];
      result.words[45 + stage] = c[1];
      result.words[stage] = c[2];
      result.words[26 + stage] = c[3];
      result.words[10 + stage] = c[4];
      result.words[18 + stage] = c[5];
      ++stage;
    }
  }
  if (!final) {
    result.words[8] = 12;
    result.words[9] = 0x2000;
  }
  result.words[53] =
      static_cast<std::uint32_t>(std::max<std::size_t>(stage, 1)) | 0x11100;
  return result;
}
void remap_vertex_constants(ShaderConstants &destination,
                            std::span<const ShaderVector> source,
                            std::span<const ShaderRemap> remaps) {
  // Component masks are the retail jump-table at 00017000, including XY/ZW
  // bitmap transforms. Banks are uploaded separately by 00018D70/00018E80.
  constexpr std::array<unsigned, 21> masks{
      1, 2, 4, 8, 7, 1, 2, 4, 8, 3, 12, 3, 12, 15, 11, 11, 7, 7, 15, 15, 15};
  for (const auto &r : remaps) {
    const unsigned component = (r.destination >> 5) & 31;
    if (component >= masks.size() || r.source >= source.size())
      throw FormatError("invalid vertex constant remap");
    auto &out = destination[(r.destination >> 4) & 1][r.destination & 15];
    for (unsigned axis = 0; axis < 4; ++axis)
      if (masks[component] & (1u << axis))
        out[axis] = source[r.source][axis];
  }
}
void remap_pixel_constants(PixelProgram &destination,
                           std::span<const std::uint32_t> source,
                           std::span<const ShaderRemap> remaps,
                           std::span<const ShaderFragment> fragments,
                           std::span<const std::int32_t> switches) {
  constexpr std::array<std::uint32_t, 8> masks{
      0xff0000, 0xff00, 0xff, 0xff000000, 0xffffff, 0xff0000, 0xff00, 0xff};
  for (const auto &r : remaps) {
    if (r.source >= source.size())
      throw FormatError("invalid pixel constant source");
    const auto packed = r.destination;
    std::size_t slot = (packed >> 6) & 15;
    if (!fragments.empty()) {
      const unsigned fragment = packed & 7;
      if (fragment >= fragments.size())
        throw FormatError("invalid pixel constant fragment");
      const auto &f = fragments[fragment];
      if (f.switch_index != -1) {
        permutation_index(f, switches);
        if (((packed >> 3) & 7) != unsigned(switches[f.switch_index]))
          continue;
      }
      if (slot != 8)
        slot += destination.fragment_offsets[fragment];
    }
    if (slot > 8)
      throw FormatError("invalid pixel constant register");
    const auto bank = (packed >> 10) & 1;
    const auto index = slot == 8 ? 43 + bank : 10 + bank * 8 + slot;
    const auto mask = masks[r.component & 7];
    destination.words[index] =
        (destination.words[index] & ~mask) | (source[r.source] & mask);
  }
}
void remap_pixel_externs(PixelProgram &destination,
                         std::span<const float> scalars,
                         std::span<const ShaderVector> colors,
                         std::span<const ShaderRemap> remaps,
                         std::span<const ShaderFragment> fragments,
                         std::span<const std::int32_t> switches) {
  ShaderVector values{};
  for (const auto &r : remaps) {
    if (!fragments.empty()) {
      const auto fragment = r.destination & 7;
      if (fragment >= fragments.size())
        throw FormatError("invalid pixel extern fragment");
      const auto &f = fragments[fragment];
      if (f.switch_index != -1) {
        permutation_index(f,switches);
        if (((r.destination >> 3) & 7) != unsigned(switches[f.switch_index]))
          continue;
      }
    }
    const auto component = r.component & 7, mode = (r.component >> 3) & 7;
    if (component < 4) {
      if (r.source >= scalars.size() || !std::isfinite(scalars[r.source]))
        throw FormatError("invalid pixel scalar extern");
      float v=scalars[r.source];
      // The retail helper evaluates these periodic functions on x87. Keep
      // the original float period; this host implementation has no claim of
      // bit-identical transcendental precision.
      const long double angle=static_cast<long double>(v)*6.2831854820251465f;
      switch (mode) {
      case 1: v=1-v; break;
      case 2: v=float((std::cos(angle)+1)*.5L); break;
      case 3: v=float(.5L-std::cos(angle)*.5L); break;
      case 4: v=float((std::sin(angle)+1)*.5L); break;
      case 5: v=float(.5L-std::sin(angle)*.5L); break;
      default: break;
      }
      values[component]=v;
    } else if (component==4) {
      if (r.source >= colors.size()) throw FormatError("invalid pixel color extern");
      std::copy_n(colors[r.source].begin(),3,values.begin());
    } else {
      // Pixel externs call 1B2C0 with texture=0: dimension components are 1.
      values[component-5]=1;
    }
    for (auto &v : values) {
      if (!std::isfinite(v)) throw FormatError("nonfinite pixel extern");
      if (r.destination & 0x800) v=(v+1)*.5f;
      v=std::clamp(v,0.f,1.f);
    }
    const auto byte=[](float v) {return std::uint32_t(std::nearbyint(double(v)*255));};
    const std::array<std::uint32_t,1> packed{{byte(values[3])<<24 |
      byte(values[0])<<16 | byte(values[1])<<8 | byte(values[2])}};
    auto mapping=r; mapping.source=0;
    remap_pixel_constants(destination,packed,{&mapping,1},fragments,switches);
  }
}
} // namespace halo2

// Narrow validation ABI for the instruction oracle; never used by the runtime.
extern "C" int halo2_assemble_pixel_program(
    const std::uint32_t *definition, const std::uint32_t *fragments,
    unsigned fragment_count, const std::uint16_t *permutations,
    unsigned permutation_count, const std::uint32_t *combiners,
    unsigned combiner_count, const std::int32_t *switches,
    std::uint32_t *output) {
  try {
    if (fragment_count > 8 || permutation_count > 512 || combiner_count > 512)
      return 0;
    halo2::PixelProgram d;
    std::copy_n(definition, 60, d.words.begin());
    std::vector<halo2::ShaderFragment> f;
    for (unsigned i = 0; i < fragment_count; ++i)
      f.push_back({static_cast<std::int16_t>(fragments[i]),
                   {std::uint16_t(fragments[i] >> 16)}});
    std::vector<halo2::ShaderPermutation> p;
    for (unsigned i = 0; i < permutation_count; ++i)
      p.push_back({static_cast<std::int16_t>(permutations[i * 3]),
                   permutations[i * 3 + 1],
                   {permutations[i * 3 + 2]}});
    std::vector<halo2::ShaderCombiner> c(combiner_count);
    for (unsigned i = 0; i < combiner_count; ++i)
      std::copy_n(combiners + i * 8, 8, c[i].words.begin());
    const auto result =
        halo2::assemble_pixel_program(d, f, p, c, {switches, 8});
    std::copy(result.words.begin(), result.words.end(), output);
    return 1;
  } catch (const halo2::FormatError &) {
    return 0;
  }
}
extern "C" int halo2_remap_vertex_constants(const float *initial,
                                            const float *source,
                                            const std::uint32_t *mappings,
                                            unsigned count, float *output) {
  try {
    if (count > 512)
      return 0;
    halo2::ShaderConstants constants;
    for (unsigned bank = 0; bank < 2; ++bank)
      for (unsigned reg = 0; reg < 16; ++reg)
        std::copy_n(initial + bank * 64 + reg * 4, 4,
                    constants[bank][reg].begin());
    std::array<halo2::ShaderVector, 256> values;
    for (unsigned i = 0; i < 256; ++i)
      std::copy_n(source + i * 4, 4, values[i].begin());
    std::vector<halo2::ShaderRemap> remaps;
    for (unsigned i = 0; i < count; ++i)
      remaps.push_back({std::uint16_t(mappings[i]),
                        std::uint8_t(mappings[i] >> 16),
                        std::uint8_t(mappings[i] >> 24)});
    halo2::remap_vertex_constants(constants, values, remaps);
    for (unsigned bank = 0; bank < 2; ++bank)
      for (unsigned reg = 0; reg < 16; ++reg)
        std::copy(constants[bank][reg].begin(), constants[bank][reg].end(),
                  output + bank * 64 + reg * 4);
    return 1;
  } catch (const halo2::FormatError &) {
    return 0;
  }
}
extern "C" int halo2_remap_pixel_constants(
    const std::uint32_t *initial, const std::uint32_t *source,
    const std::uint32_t *mappings, unsigned count,
    const std::uint32_t *fragments, unsigned fragment_count,
    const std::int32_t *switches, const std::uint8_t *offsets,
    std::uint32_t *output) {
  try {
    if (count > 127 || fragment_count > 8)
      return 0;
    halo2::PixelProgram program;
    std::copy_n(initial, 60, program.words.begin());
    std::copy_n(offsets, 8, program.fragment_offsets.begin());
    std::vector<halo2::ShaderFragment> f;
    for (unsigned i = 0; i < fragment_count; ++i)
      f.push_back({static_cast<std::int16_t>(fragments[i]),
                   {std::uint16_t(fragments[i] >> 16)}});
    std::vector<halo2::ShaderRemap> remaps;
    for (unsigned i = 0; i < count; ++i)
      remaps.push_back({std::uint16_t(mappings[i]),
                        std::uint8_t(mappings[i] >> 16),
                        std::uint8_t(mappings[i] >> 24)});
    halo2::remap_pixel_constants(program, {source, 256}, remaps, f,
                                 {switches, 8});
    std::copy(program.words.begin(), program.words.end(), output);
    return 1;
  } catch (const halo2::FormatError &) {
    return 0;
  }
}

extern "C" int halo2_remap_pixel_externs(
    const std::uint32_t *initial, const float *scalars, const float *colors,
    const std::uint32_t *mappings, unsigned count,
    const std::uint32_t *fragments, unsigned fragment_count,
    const std::int32_t *switches, const std::uint8_t *offsets,
    std::uint32_t *output) {
  try {
    if (count > 127 || fragment_count > 8) return 0;
    halo2::PixelProgram program;
    std::copy_n(initial,60,program.words.begin());
    std::copy_n(offsets,8,program.fragment_offsets.begin());
    std::array<halo2::ShaderVector,256> vectors{};
    for (unsigned i=0;i<256;++i)
      std::copy_n(colors+i*4,4,vectors[i].begin());
    std::vector<halo2::ShaderRemap> remaps;
    for (unsigned i=0;i<count;++i)
      remaps.push_back({std::uint16_t(mappings[i]),std::uint8_t(mappings[i]>>16),
                        std::uint8_t(mappings[i]>>24)});
    std::vector<halo2::ShaderFragment> f;
    for (unsigned i=0;i<fragment_count;++i)
      f.push_back({static_cast<std::int16_t>(fragments[i]),
                    {std::uint16_t(fragments[i]>>16)}});
    halo2::remap_pixel_externs(program,{scalars,256},vectors,remaps,f,{switches,8});
    std::copy(program.words.begin(),program.words.end(),output);
    return 1;
  } catch (const halo2::FormatError &) { return 0; }
}
