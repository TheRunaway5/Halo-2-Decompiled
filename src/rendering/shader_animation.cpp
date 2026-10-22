#include "halo2/shader_animation.hpp"
#include "halo2/appearance.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <set>
#if defined(__SSE__) || defined(_M_IX86) || defined(_M_X64)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {

std::vector<std::byte> curve(const Bytes &b, std::size_t at,
                             std::int64_t magic) {
  const auto n = b.u32(at);
  if (!n)
    return {};
  if (n > 4096)
    throw FormatError("excessive shader function size");
  const auto p = TagAddressSpace(b,magic).offset(b.u32(at+4),n);
  const auto s = b.slice(p, n);
  return {s.begin(), s.end()};
}
template <class T>
std::span<const T> range(const std::vector<T> &v, ShaderRange r) {
  if (r.first() > v.size() || r.count() > v.size() - r.first())
    throw FormatError("shader animation range outside block");
  return std::span<const T>(v).subspan(r.first(), r.count());
}
unsigned byte(const Bytes &b, unsigned at) {
  return std::to_integer<unsigned>(b.slice(at, 1)[0]);
}
float finite(float x) {
  if (!std::isfinite(x))
    throw FormatError("nonfinite shader animation value");
  return x;
}
float clamp(float x) { return std::clamp(finite(x), 0.f, 1.f); }
float spline2_parameter(const Bytes &b,unsigned base,float input) {
  const auto p=[&](unsigned index){return finite(b.f32(base+index*4));};
  const float dx23=p(4)-p(6),dy23=p(5)-p(7);
  const float distance23=dx23*dx23+dy23*dy23;
  constexpr float epsilon=0.0001f;
  if(!(distance23>epsilon))return input;
  const float dx01=p(0)-p(2),dy01=p(1)-p(3);
  const float distance01=dx01*dx01+dy01*dy01;
  if(std::abs(double(distance01))<double(epsilon))return 1.f;
  if(std::abs(double(distance01)-double(distance23))<double(epsilon))return input;
  // Literal13B899..13B8C9: the bit estimate and RSQRTSS are distinct.
  // This executes the host x86 estimate; Xbox estimate precision remains a
  // hardware-validation boundary, not a substituted exact reciprocal sqrt.
#if defined(__SSE__) || defined(_M_IX86) || defined(_M_X64)
  const float reciprocal=_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(distance23)));
#else
  throw FormatError("spline2 requires an admitted source RSQRTSS platform owner");
  const float reciprocal=0.f;
#endif
  const auto estimate_word=(std::bit_cast<std::uint32_t>(distance01)>>1)+0x1fc00000u;
  const float exponent=float(double(reciprocal)*double(std::bit_cast<float>(estimate_word)));
  return clamp(float(std::pow(double(input),double(exponent))));
}
float periodic(unsigned type, float x, const ShaderFunctionTables &tables) {
  if (type >= tables.periodic.size())
    throw FormatError("periodic shader curve outside table");
  if (!type)
    return 1;
  // Retail 17C900 scales by 45E058 (36.57143), not the table length.
  const float scaled = finite(x * 36.57143f);
  const float fraction = float(std::fmod(double(scaled), 1.0));
  const float rounded = std::nearbyint(scaled - fraction);
  const auto integer =
      double(rounded) >= 2147483648. || double(rounded) < -2147483648.
          ? INT32_MIN
          : std::int32_t(rounded);
  const auto index = std::uint32_t(integer) & 1023;
  const auto &table = tables.periodic[type];
  const float a = table[index] * (1.f / 255.f);
  float b = table[(index + 1) & 1023] * (1.f / 255.f);
  if ((1u << type) & 0xc0) {
    if (a > .75f && b < .25f)
      b += 1;
    const float out = (1.f - fraction) * a + b * fraction;
    return out > 1 ? out - 1 : out;
  }
  return (1.f - fraction) * a + b * fraction;
}
float transition(unsigned type, float x, const ShaderFunctionTables &tables) {
  x = clamp(x);
  if (type >= tables.transition.size())
    throw FormatError("transition shader curve outside table");
  if (!type)
    return x;
  const float scaled = x * 1023;
  const float fraction = float(std::fmod(double(scaled), 1.0));
  const auto i = unsigned(std::nearbyint(scaled - .1f));
  const auto &t = tables.transition[type];
  const float a = t[i] * (1.f / 255.f);
  if (i == 1023)
    return a;
  return clamp(a * (1 - fraction) + t[i + 1] * (1.f / 255.f) * fraction);
}
float scalar(std::span<const std::byte> bytes, float value) {
  if (bytes.empty())
    return 0;
  const Bytes b(bytes);
  b.require(0, 20);
  if (!(byte(b, 1) & 0xf0))
    return finite((b.f32(8) - b.f32(4)) * clamp(value) + b.f32(4));
  return value;
}
float operation(float v, unsigned mode) {
  const float angle = v * 6.2831854820251465f;
  switch (mode) {
  case 0:
    return v;
  case 1:
    return 1 - v;
  case 2:
    return clamp(float((std::cos(double(angle)) + 1) * .5));
  case 3:
    return clamp(float(.5 - std::cos(double(angle)) * .5));
  case 4:
    return clamp(float((std::sin(double(angle)) + 1) * .5));
  case 5:
    return clamp(float(.5 - std::sin(double(angle)) * .5));
  default:
    return v; // Retail switch leaves modes 6 and 7 unchanged.
  }
}
std::uint32_t interpolate_color(std::uint32_t a, std::uint32_t b, float t) {
  const auto weight = std::int32_t(std::nearbyint(t * 16384.f));
  std::uint32_t out = 0;
  for (unsigned k = 0; k < 4; ++k) {
    const auto x = int((a >> (k * 8)) & 255), y = int((b >> (k * 8)) & 255);
    // Signed MMX PMULHW and PSRAW, including negative rounding.
    const auto product = (y - x) * 8 * weight;
    const auto hi =
        product >= 0 ? product / 65536 : -int((-product + 65535) / 65536);
    const auto round = hi + 1;
    const auto delta = round >= 0 ? round / 2 : -int((-round + 1) / 2);
    out |= std::uint32_t(std::clamp(x + delta, 0, 255)) << (k * 8);
  }
  return out;
}
ShaderVector color_vector(std::uint32_t c) {
  return {((c >> 16) & 255) * (1.f / 255.f), ((c >> 8) & 255) * (1.f / 255.f),
          (c & 255) * (1.f / 255.f), ((c >> 24) & 255) * (1.f / 255.f)};
}
std::uint32_t pack(ShaderVector v) {
  constexpr unsigned shift[]{16, 8, 0, 24};
  std::uint32_t out = 0;
  for (unsigned i = 0; i < 4; ++i)
    // Original131E50 multiplies on x87 then FISTP without a float spill.
    // The source float times integer255 is exact at double precision.
    out |= std::uint32_t(std::nearbyint(double(clamp(v[i])) * 255.0)) << shift[i];
  return out;
}
void transform(const std::array<float, 16> &v, unsigned mode,
               ShaderVector &out) {
  float sine = 0, cosine = 1;
  if (!(std::abs(v[13]) < .0001f)) {
    const float angle = v[13] * 6.2831854820251465f;
    sine = std::sin(angle);
    cosine = std::cos(angle);
  }
  const float xx = v[7] * cosine, xy = 0.f - v[7] * sine, yx = v[8] * sine,
              yy = v[8] * cosine,
              tx = (1.f - cosine + sine) * v[7] * .5f + v[10],
              ty = (1.f - cosine - sine) * v[8] * .5f + v[11];
  switch (mode) {
  case 5:
  case 6:
  case 7:
  case 8:
    out[mode - 5] = xx;
    break;
  case 9:
    out[0] = xx;
    out[1] = yy;
    break;
  case 10:
    out[2] = xx;
    out[3] = yy;
    break;
  case 11:
    out[0] = tx;
    out[1] = ty;
    break;
  case 12:
    out[2] = tx;
    out[3] = ty;
    break;
  case 13:
    out = {xx, yy, tx, ty};
    break;
  case 14:
    out = {xx, xy, 0, tx};
    break;
  case 15:
    out = {yx, yy, 0, ty};
    break;
  case 16:
    out[0] = xx;
    out[1] = yy;
    out[2] = v[9];
    break;
  case 17:
    out[0] = tx;
    out[1] = ty;
    out[2] = v[12];
    break;
  case 18:
    out = {xx, xy, 0, tx};
    break;
  case 19:
    out = {yx, yy, 0, ty};
    break;
  case 20:
    out = {0, 0, v[9], v[12]};
    break;
  default:
    throw FormatError("invalid animated bitmap transform mode");
  }
}
} // namespace

ShaderAnimation read_shader_animation(const Bytes &b, std::size_t d,
                                      std::int64_t magic) {
  ShaderAnimation a;
  const auto lods = TagAddressSpace(b, magic).block(d + 28, 6);
  for (std::size_t i = 0; i < lods.count; ++i) {
    const auto p = lods.offset + i * 6;
    a.lods.push_back({b.u32(p), {b.u16(p + 4)}});
  }
  auto ranges = [&](unsigned at, std::vector<ShaderRange> &out) {
    const auto v = TagAddressSpace(b, magic).block(d + at, 2);
    for (std::size_t i = 0; i < v.count; ++i)
      out.push_back({b.u16(v.offset + i * 2)});
  };
  ranges(36, a.layers);
  ranges(44, a.passes);
  ranges(76, a.parameters);
  const auto implementations = TagAddressSpace(b, magic).block(d + 52, 10);
  for (std::size_t i = 0; i < implementations.count; ++i) {
    const auto p = implementations.offset + i * 10;
    a.implementations.push_back({{b.u16(p)}, {b.u16(p + 6)}, {b.u16(p + 8)}});
  }
  const auto overlays = TagAddressSpace(b, magic).block(d + 60, 20);
  for (std::size_t i = 0; i < overlays.count; ++i) {
    const auto p = overlays.offset + i * 20;
    a.overlays.push_back({b.u32(p), b.u32(p + 4), finite(b.f32(p + 8)),
                          curve(b, p + 12, magic)});
  }
  const auto refs = TagAddressSpace(b, magic).block(d + 68, 4),
             maps = TagAddressSpace(b, magic).block(d + 84, 4);
  for (std::size_t i = 0; i < refs.count; ++i) {
    const auto p = refs.offset + i * 4;
    a.references.push_back(
        {std::int16_t(b.u16(p)), std::int16_t(b.u16(p + 2))});
  }
  for (std::size_t i = 0; i < maps.count; ++i) {
    const auto p = maps.offset + i * 4;
    a.mappings.push_back(
        {b.u16(p), std::uint8_t(byte(b, p + 2)), std::uint8_t(byte(b, p + 3))});
  }
  for (const auto &p : a.parameters)
    range(a.references, p);
  for (const auto &r : a.references)
    if (r.function < 0 || std::size_t(r.function) >= a.overlays.size())
      throw FormatError("shader overlay index outside block");
  for (const auto &i : a.implementations) {
    range(a.mappings, i.bitmap_transforms);
    range(a.mappings, i.pixel_constants);
    range(a.mappings, i.vertex_constants);
  }
  return a;
}

std::shared_ptr<const ShaderFunctionTables>
Cache::shader_function_tables() const {
  if (function_tables_)
    return function_tables_;
  if(mcc_)
    throw FormatError("Native MCC material library is missing; update the port and repair its cache");
  auto tables = std::make_shared<ShaderFunctionTables>();
  std::array<std::byte, 28 * 1024> bytes{};
  std::ifstream stream((vista_ && !xbox_donor_root_.empty()?xbox_donor_root_:shared_directory_.parent_path()) / "default.xbe",
                       std::ios::binary);
  stream.seekg(
      0x464bc8); // VA 468D48, twelve periodic then sixteen transition LUTs.
  if (!stream.read(reinterpret_cast<char *>(bytes.data()), bytes.size()))
    throw FormatError("shader function tables require retail default.xbe");
  std::uint64_t hash = 14695981039346656037ull;
  for (auto v : bytes)
    hash = (hash ^ std::to_integer<unsigned>(v)) * 1099511628211ull;
  if (hash != 0x22708b4cc478ddb7ull)
    throw FormatError("unsupported retail shader function tables");
  for (unsigned i = 0; i < 28; ++i)
    for (unsigned j = 0; j < 1024; ++j)
      (i < 12 ? tables->periodic[i][j] : tables->transition[i - 12][j]) =
          std::to_integer<std::uint8_t>(bytes[i * 1024 + j]);
  function_tables_ = tables;
  return tables;
}

std::vector<ObjectFunction>
Cache::load_object_functions(std::uint32_t datum) const {
  const auto tag = query_tag({.handle=TagHandle{datum}});
  if (!tag || tag->size < 108)
    throw FormatError("truncated object function definition");
  const Bytes b(cache_bytes());
  const auto d = metadata_offset(tag->address, tag->size);
  const auto definitions = TagAddressSpace(b, metadata_magic_).block(d + 0x64, 32);
  std::vector<ObjectFunction> out;
  for (std::size_t i = 0; i < definitions.count; ++i) {
    const auto p = definitions.offset + i * 32;
    out.push_back({b.u32(p), b.u32(p + 4), b.u32(p + 8), b.u32(p + 12),
                   finite(b.f32(p + 16)), curve(b, p + 20, metadata_magic_),
                   b.u32(p + 28)});
  }
  return out;
}

float shader_function_fraction(std::span<const std::byte> bytes, float input,
                               float r, const ShaderFunctionTables &tables) {
  finite(input);
  finite(r);
  if (bytes.empty())
    return 0;
  const Bytes b(bytes);
  b.require(0, 20);
  const auto type = byte(b, 0), flags = byte(b, 1);
  if (type >= 11)
    throw FormatError("invalid shader function type");
  constexpr unsigned counts[]{0, 1, 2, 4, 6, 20, 32, 12, 4, 3, 12};
  // Identity and constant functions do not dereference parameter arrays.
  if (type == 0)
    return clamp(input);
  if (type == 1)
    return flags & 1 ? clamp(r) : 0;
  if (type == 6 || type == 8)
    return 0; // Retail 13B390 default branch.
  b.require(20, counts[type] * 4 * ((flags & 1) ? 2 : 1));
  auto eval = [&](unsigned n) {
    const unsigned base = 20 + n * counts[type] * 4;
    auto p = [&](unsigned i) { return finite(b.f32(base + i * 4)); };
    switch (type) {
    case 2:
      return (p(1) - p(0)) * transition(byte(b, 2 + n), input, tables) + p(0);
    case 3:
      return (p(3) - p(2)) *
                 periodic(byte(b, 2 + n), p(0) * input + p(1), tables) +
             p(2);
    case 4:
      return p(4) * input + p(5);
    case 5: {
      const float t0 = clamp((input - p(9)) * p(13)),
                  t1 = clamp((input - p(10)) * p(14)),
                  t2 = clamp((input - p(11)) * p(15));
      return p(19) * t2 + p(18) * t1 + p(17) * t0 + p(16);
    }
    case 7: {
      const float t2 = input * input, t3 = t2 * input;
      return p(9) * t2 + p(10) * input + p(8) * t3 + p(11);
    }
    case 10: {
      const float t=spline2_parameter(b,base,input);
      const float t2=t*t,t3=t2*t;
      return p(9)*t2+p(10)*t+p(8)*t3+p(11);
    }
    case 9:
      return std::abs(p(2)) < .0001f || (p(2) < 0 && std::abs(input) < .0001f)
                 ? 1.f
                 : float((p(1) - p(0)) * std::pow(double(input), double(p(2))) +
                         p(0));
    default:
      return 0.f;
    }
  };
  const float lo = eval(0);
  return clamp((flags & 1) ? (1.f - r) * lo + eval(1) * r : lo);
}
float shader_function_value(std::span<const std::byte> b, float input, float r,
                            const ShaderFunctionTables &tables) {
  return scalar(b, shader_function_fraction(b, input, r, tables));
}
std::uint32_t shader_function_color(std::span<const std::byte> bytes,
                                    float input) {
  if (bytes.empty())
    return 0xffffffffu;
  const Bytes b(bytes);
  b.require(0, 20);
  const auto flags = byte(b, 1), kind = flags >> 4;
  if (byte(b, 0) == 1 && (!(flags & 1) || kind <= 1))
    return b.u32(4);
  input = clamp(input);
  if (!kind) {
    const auto v = std::uint32_t(std::nearbyint(input * 255.f));
    return 0xff000000u | v << 16 | v << 8 | v;
  }
  if (kind == 1)
    return b.u32(4);
  if (kind == 2)
    return interpolate_color(b.u32(4), b.u32(16), input);
  if (kind == 3) {
    const float scaled = input * 2;
    const auto i = unsigned(std::floor(scaled));
    return interpolate_color(b.u32(i == 0 ? 4 : 8), b.u32(i == 0 ? 8 : 16),
                             i >= 2 ? 1.f : scaled - i);
  }
  if (kind == 4) {
    const float scaled = input * 3;
    const auto i = unsigned(std::floor(scaled)), slot = std::min(i, 2u);
    return interpolate_color(b.u32(4 + slot * 4), b.u32(8 + slot * 4),
                             i >= 3 ? 1.f : scaled - i);
  }
  return 0xffffffffu;
}

FunctionValue
evaluate_object_function(std::span<const ObjectFunction> functions,
                         const ShaderFunctionTables &tables, std::uint32_t name,
                         float seconds, const ObjectFunctionInput &input,
                         const ObjectFunctionHandler &handler,bool original_builtin_constants) {
  std::set<std::uint32_t> pending;
  std::function<FunctionValue(std::uint32_t)> eval =
      [&](std::uint32_t id) -> FunctionValue {
    if (input)
      if (const auto v = input(id))
        return {finite(*v), true};
    if(handler)
      if(const auto value=handler(id))return {finite(value->value),value->enabled};
    if (!id || (original_builtin_constants && id == 0x030005a6))
      return {1, true};
    if (original_builtin_constants && id == 0x040005a7)
      return {};
    if (!pending.insert(id).second)
      throw FormatError("cyclic object shader function");
    FunctionValue out;
    for (const auto &f : functions)
      if (f.output == id) {
        const bool implicit = !(f.flags & 2);
        // Retail 108D90's built-in one/zero inputs; authored output functions
        // are resolved by BAB40, not recursively used as input handlers.
        float v = (!f.input || (original_builtin_constants && f.input == 0x030005a6)) ? 1.f : 0.f;
        bool enabled = v != 0;
        if (input)
          if (const auto override = input(f.input)) {
            out = {finite(*override), true};
            continue; // BA8C0 interpolated override.
          }
        if(handler)
          if(const auto value=handler(f.input)) {
            v=finite(value->value);enabled=value->enabled;
          }
        const bool periodic_curve =
            !f.curve.empty() && f.curve[0] == std::byte{3};
        if (enabled || implicit || periodic_curve) {
          if (periodic_curve) {
            if (f.flags & 8)
              throw FormatError(
                  "random object shader phase requires object datum");
            v *= shader_function_value(f.curve, seconds, 1, tables);
          } else {
            if (f.flags & 1)
              v = 1 - v;
            v = shader_function_value(f.curve, v, 1, tables);
            if (implicit)
              enabled = v > 0;
          }
        }
        if (f.scale) {
          const auto scale = eval(f.scale);
          v *= scale.value;
          enabled = enabled && scale.enabled;
        }
        if (enabled && f.threshold > 0)
          enabled = v > f.threshold;
        if (f.flags & 4)
          enabled = true;
        if (f.enable && enabled) {
          if(const auto gate=handler?handler(f.enable):std::optional<FunctionValue>{})
            enabled=gate->enabled;
          else {
            const auto input_gate = input ? input(f.enable) : std::optional<float>{};
            if (!input_gate || !*input_gate)enabled = false;
          }
        }
        out = {enabled ? clamp(v) : 0, enabled};
      }
    pending.erase(id);
    return out;
  };
  return eval(name);
}

bool animate_forward_constants(const ShaderAnimation &a,const MaterialPass &pass,
    const std::array<std::int32_t,8> &switches,double seconds,const ShaderInput &input,
    PixelProgram &program,ShaderConstants &constants) {
  if(a.overlays.empty())return false;
  if(!a.tables)throw FormatError("missing animated shader execution context");
  if (pass.template_pass >= a.passes.size())
    throw FormatError("animated shader pass outside definition");
  const auto implementations =
      range(a.implementations, a.passes[pass.template_pass]);
  if (pass.implementation >= implementations.size())
    throw FormatError("animated shader implementation outside pass");
  const auto &impl = implementations[pass.implementation];
  auto overlay = [&](unsigned parameter) -> const ShaderOverlay & {
    if (parameter >= a.parameters.size())
      throw FormatError("animated shader parameter outside definition");
    const auto refs = range(a.references, a.parameters[parameter]);
    if (refs.empty())
      throw FormatError("animated shader parameter has no overlay");
    return a.overlays[refs[0].function];
  };
  auto fraction = [&](const ShaderOverlay &o) {
    float v = input ? input(o.input) : 0, r = input ? input(o.range) : 0;
    if (!o.input) {
      if (o.period == 0 || !std::isfinite(seconds))
        throw FormatError("invalid animated shader clock");
      v = finite(float(seconds / o.period));
      if (o.curve.empty() || o.curve[0] != std::byte{3})
        v = float(std::fmod(double(v), 1.0));
    }
    return shader_function_fraction(o.curve, v, r, *a.tables);
  };
  std::array<std::array<float, 16>, 8> properties{};
  for (const auto &r : range(a.mappings, impl.bitmap_transforms)) {
    const unsigned slot = r.destination & 255;
    if (slot >= properties.size() || r.source >= a.parameters.size())
      throw FormatError("invalid animated bitmap property");
    for (const auto &ref : range(a.references, a.parameters[r.source])) {
      if (ref.transform < 0 || ref.transform >= 10)
        throw FormatError("invalid animated bitmap property component");
      const auto &o = a.overlays[ref.function];
      properties[slot][6 + ref.transform] = scalar(o.curve, fraction(o));
    }
  }
  for (const auto &r : range(a.mappings, impl.vertex_constants)) {
    auto &out = constants[(r.destination >> 4) & 1][r.destination & 15];
    const auto mode = (r.destination >> 5) & 31;
    if (mode < 5) {
      const auto &o = overlay(r.source);
      const float v = fraction(o);
      if (mode < 4)
        out[mode] = scalar(o.curve, v);
      else {
        const auto c = color_vector(shader_function_color(o.curve, v));
        std::copy_n(c.begin(), 3, out.begin());
      }
    } else
      transform(properties[(r.destination >> 10) & 7], mode, out);
  }
  for (auto r : range(a.mappings, impl.pixel_constants)) {
    const auto &o = overlay(r.source);
    const float v = fraction(o);
    ShaderVector out{};
    const auto component = r.component & 7;
    if (component == 4) {
      const auto c = color_vector(shader_function_color(o.curve, v));
      // The recovered C++ repeats green here; executing retail 1B580 shows
      // the third channel is blue. Preserve the executable's actual write.
      out = {c[0], c[1], c[2], 0};
    } else if (component < 4)
      out[component] = operation(scalar(o.curve, v), (r.component >> 3) & 7);
    else
      throw FormatError("invalid animated pixel component");
    const std::array<std::uint32_t, 1> value{pack(out)};
    r.source = 0;
    remap_pixel_constants(program, value, std::span<const ShaderRemap>(&r, 1),
                          pass.fragments, switches);
  }
  return true;
}
bool animate_forward_material(Material &m,double seconds,const ShaderInput &input) {
  if(!m.forward_pass || m.animation.overlays.empty())return false;
  if(m.forward_pass_index>=m.passes.size())throw FormatError("missing animated shader execution context");
  const auto &pass=m.passes[m.forward_pass_index];
  m.program_constants=pass.constants;m.forward_program=pass.program;
  if(m.object_render_colors)reapply_object_colors(m);
  const auto changed=animate_forward_constants(m.animation,pass,m.shader_switches,seconds,input,
      m.forward_program,m.program_constants);
  const auto &constants=m.program_constants;
  m.forward_diffuse=constants[0][10];m.forward_specular=constants[0][11];
  std::copy_n(constants[1].begin(),8,m.forward_transforms.begin());
  return changed;
}
} // namespace halo2
