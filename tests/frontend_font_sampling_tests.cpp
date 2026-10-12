#include "halo2/frontend.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
void require(bool ok, const char *why) {
  if (!ok) throw std::runtime_error(why);
}
template<class F> void rejects(F &&call, const char *why) {
  try { call(); } catch (const FormatError &) { return; }
  throw std::runtime_error(why);
}
bool same_glyph(const FontGlyph &a, const FontGlyph &b) {
  return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height &&
         a.advance == b.advance && a.bearing_x == b.bearing_x && a.bearing_y == b.bearing_y;
}
void same_layout(const RetailFont &a, const RetailFont &b) {
  require(a.ascent == b.ascent && a.descent == b.descent && a.leading == b.leading &&
          a.kerning == b.kerning && a.glyph_record_indices == b.glyph_record_indices &&
          a.file_fingerprint == b.file_fingerprint && a.glyphs.size() == b.glyphs.size(),
          "sampling changed source font metrics, lookup records or kerning");
  for (const auto &[c, glyph] : a.glyphs)
    require(same_glyph(glyph, b.glyphs.at(c)), "sampling changed original logical glyph");
}
void copied_pixels(const Texture &combined, const Texture &source, unsigned left) {
  require(combined.width >= left + source.width && combined.height >= source.height,
          "combined atlas omits a source image extent");
  for (unsigned y = 0; y < source.height; ++y) {
    const auto input = source.pixels.begin() + std::size_t(y) * source.width * 4;
    const auto output = combined.pixels.begin() + (std::size_t(y) * combined.width + left) * 4;
    require(std::equal(input, input + std::size_t(source.width) * 4, output),
            "source atlas pixels or transparent surrounds changed");
  }
}
void exact_original_uv(const RetailFont &font, unsigned c) {
  const auto &g = font.glyphs.at(c);
  const int x = g.width > 1 ? 1 : 0, y = g.height > 1 ? 1 : 0;
  const int width = int(g.width) - x, height = int(g.height) - y;
  const std::array expected{float(int(g.x) + x) / font.atlas.width,
      float(int(g.y) + y) / font.atlas.height,
      float(int(g.x) + x + width) / font.atlas.width,
      float(int(g.y) + y + height) / font.atlas.height};
  const auto actual = retail_font_sample_uvs(font, c, x, y, width, height);
  for (unsigned i = 0; i < actual.size(); ++i)
    require(std::bit_cast<std::uint32_t>(actual[i]) == std::bit_cast<std::uint32_t>(expected[i]),
            "original raster UV operation order changed");
}
RetailFont synthetic(unsigned factor) {
  RetailFont font;
  font.ascent = 11 * factor;
  font.descent = 3 * factor;
  font.leading = factor;
  font.atlas = {64 * factor, 64 * factor, 11, {}, {}};
  font.atlas.pixels.resize(std::size_t(font.atlas.width) * font.atlas.height * 4);
  for (std::size_t i = 0; i < font.atlas.pixels.size(); ++i)
    font.atlas.pixels[i] = std::byte((i * 13 + factor * 7) & 255);
  font.glyph_record_indices.assign(65536, 0xffffffffu);
  auto glyph = [&](unsigned c, unsigned record, unsigned width = 4, unsigned height = 3) {
    font.glyph_record_indices[c] = record;
    font.glyphs[c] = {factor * 2, factor * 3, width * factor, height * factor,
                      int(8 * factor), -int(factor), int(10 * factor)};
  };
  glyph('A', 1);
  glyph(0x3a9, 2, 3, 4);
  glyph(' ', 3, 0, 0);
  glyph(0x4e00, 40);
  glyph(0x4e01, 40); // Actual shared record relationship, not authored coverage.
  glyph(0x7f, 41);
  glyph(0xd800, 42);
  glyph(0xffff, 43);
  for (unsigned c = 0xe100; c <= 0xe111; ++c) glyph(c, c);
  font.kerning[{'A', 'A'}] = -int(factor);
  return font;
}
void synthetic_tests() {
  auto original = synthetic(1), sampled = synthetic(2);
  const auto before = original;
  for (const auto &[c, glyph] : original.glyphs) exact_original_uv(original, c);
  // Source glyph density is per-glyph and can be nonuniform. The original
  // integer logical clipping rectangle still controls the screen geometry.
  sampled.glyphs.at('A').width = 9;
  sampled.glyphs.at('A').height = 7;
  const auto sampled_before = sampled;
  require(apply_retail_font_sampling(original, sampled) == 2,
          "normal Unicode identity map lost rows or admitted aliases/private controls");
  same_layout(before, original);
  same_layout(sampled_before, sampled);
  copied_pixels(original.atlas, before.atlas, 0);
  copied_pixels(original.atlas, sampled.atlas, before.atlas.width + 1);
  require(original.sampled_glyphs.contains('A') && original.sampled_glyphs.contains(0x3a9),
          "normal Latin or Unicode source row not selected");
  for (const unsigned c : {0x4e00u, 0x4e01u, 0x7fu, 0xd800u, 0xffffu, 0xe100u, 0xe104u, 0xe105u,
                            0xe10cu, 0xe10du}) {
    require(!original.sampled_glyphs.contains(c), "unproved symbol or shared alias was replaced");
    exact_original_uv(original, c);
  }
  const auto &s = original.sampled_glyphs.at('A');
  const auto uv = retail_font_sample_uvs(original, 'A', 1, 1, 2, 1);
  const std::array expected{(s.x + 9.f / 4.f) / original.atlas.width,
      (s.y + 7.f / 3.f) / original.atlas.height,
      (s.x + 27.f / 4.f) / original.atlas.width,
      (s.y + 14.f / 3.f) / original.atlas.height};
  for (unsigned i = 0; i < uv.size(); ++i)
    require(std::bit_cast<std::uint32_t>(uv[i]) == std::bit_cast<std::uint32_t>(expected[i]),
            "clipped logical rectangle does not map to actual higher-resolution pixels");
  rejects([&] { retail_font_sample_uvs(original, 'A', -1, 0, 1, 1); },
          "negative logical sample crop admitted");
  rejects([&] { retail_font_sample_uvs(original, 'A', 0, 0, 5, 1); },
          "sample crop beyond original logical width admitted");
  rejects([&] { apply_retail_font_sampling(original, sampled); }, "second overlay admitted");
  auto missing = synthetic(1), aliases = synthetic(2);
  missing.glyph_record_indices.clear();
  require(apply_retail_font_sampling(missing, aliases) == 0,
          "source-less synthetic font silently guessed authored coverage");
  missing = synthetic(1);
  missing.glyph_record_indices.pop_back();
  rejects([&] { apply_retail_font_sampling(missing, aliases); }, "partial lookup provenance admitted");
  auto shared = synthetic(1);
  aliases.glyph_record_indices[0x1234] = aliases.glyph_record_indices['A'];
  require(apply_retail_font_sampling(shared, aliases) == 1 && !shared.sampled_glyphs.contains('A'),
          "replacement fallback/shared alias counted as a distinct glyph");
  auto unchanged = synthetic(1), identical = synthetic(1);
  require(apply_retail_font_sampling(unchanged, identical) == 0 &&
          unchanged.atlas.pixels == identical.atlas.pixels &&
          unchanged.atlas.width == identical.atlas.width,
          "equal-sized/byte-identical font unnecessarily rebuilt its atlas");
  auto invalid = synthetic(2), stock = synthetic(1);
  invalid.atlas.layers = 6;
  rejects([&] { apply_retail_font_sampling(stock, invalid); }, "cube font atlas admitted");
  same_layout(synthetic(1), stock);
  invalid = synthetic(2);
  invalid.glyphs.at('A').x = invalid.atlas.width;
  rejects([&] { apply_retail_font_sampling(stock, invalid); }, "glyph outside sampled atlas admitted");
  require(stock.sampled_glyphs.empty(), "failed sampler partially published rectangles");
  rejects([&] { decode_font_pixels({}, 256 * 128 + 1); }, "excessive font decode size admitted");
  std::cout << "Synthetic font sampler: source/layout immutability, pixel copies, logical clips, aliases and guards\n";
}
struct Temporary {
  std::filesystem::path path = std::filesystem::temp_directory_path() /
      ("halo2-font-sampling-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  Temporary() { std::filesystem::create_directory(path); }
  ~Temporary() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
};
void rejects_font_bounds(const std::filesystem::path &source) {
  std::ifstream input(source, std::ios::binary | std::ios::ate);
  require(bool(input), "missing sampled font bounds fixture");
  std::vector<std::byte> bytes(std::size_t(input.tellg()));
  input.seekg(0);
  require(bool(input.read(reinterpret_cast<char *>(bytes.data()), bytes.size())), "font fixture read failed");
  const Bytes view(bytes);
  const auto glyph = (std::size_t(view.u32(0x400 + unsigned('A') * 4)) + 0x4040) * 16;
  Temporary temporary;
  for (const auto [offset, value] : {std::pair{glyph + 4, 257u}, std::pair{glyph + 6, 129u},
                                    std::pair{glyph + 2, 8193u}}) {
    auto mutated = bytes;
    mutated[offset] = std::byte(value & 255);
    mutated[offset + 1] = std::byte(value >> 8);
    const auto output = temporary.path / "invalid-font";
    std::ofstream file(output, std::ios::binary);
    file.write(reinterpret_cast<const char *>(mutated.data()), mutated.size());
    file.close();
    rejects([&] { load_retail_font(output); }, "font header above measured bounded dimensions admitted");
  }
  // Physically truncate inside a declared, in-bounds source glyph stream.
  // Rejection must not rely solely on dimension/encoded-size caps.
  const auto stream_end = std::size_t(view.u32(glyph + 12)) + view.u16(glyph + 2);
  require(stream_end > 0x40400 && stream_end <= bytes.size(), "invalid actual glyph truncation control");
  const auto truncated = temporary.path / "truncated-font";
  std::ofstream file(truncated, std::ios::binary);
  file.write(reinterpret_cast<const char *>(bytes.data()), stream_end - 1);
  file.close();
  rejects([&] { load_retail_font(truncated); }, "truncated actual source glyph stream admitted");
}
void actual_fonts(const std::filesystem::path &original_directory,
                  const std::filesystem::path &sampled_directory) {
  unsigned total{}, fonts{};
  for (const auto name : {"conduit-12", "conduit-13", "handel_gothic-11",
                          "handel_gothic-13", "handel_gothic-24"}) {
    auto original = load_retail_font(original_directory / name);
    const auto before = original, sampled = load_retail_font(sampled_directory / name);
    require(sampled.glyphs.at(0xe114).width == 134 && sampled.glyphs.at(0xe121).width == 200,
            "actual wider-than128px private source glyphs were not decoded");
    require(sampled.glyphs.at(0xe054).width == 64 && sampled.glyphs.at(0xe054).height == 48,
            "actual4627-byte private source glyph was not decoded");
    const auto count = apply_retail_font_sampling(original, sampled);
    require(count > 200 && count == original.sampled_glyphs.size(), "actual HD font rows not admitted");
    same_layout(before, original);
    copied_pixels(original.atlas, before.atlas, 0);
    copied_pixels(original.atlas, sampled.atlas, before.atlas.width + 1);
    for (unsigned c = 0xe100; c <= 0xe111; ++c) {
      require(original.sampled_glyphs.contains(c), "reviewed or user-selected controller source pixels omitted");
      const auto &s = original.sampled_glyphs.at(c);
      const auto &g = sampled.glyphs.at(c);
      require(s.x == before.atlas.width + 1 + g.x && s.y == g.y &&
              s.width == g.width && s.height == g.height, "controller source rectangle differs from actual pixels");
    }
    for (const unsigned c : {0xe104u, 0xe105u})
      require(original.sampled_glyphs.at(c).width == 60 &&
              original.sampled_glyphs.at(c).height == 36,
              "user-selected LB/RB rectangles differ from actual source glyphs");
    for (const unsigned c : {0xe112u, 0xe113u, 0xe114u, 0xe115u}) {
      require(!original.sampled_glyphs.contains(c), "unreviewed private symbol was replaced");
      exact_original_uv(original, c);
    }
    std::map<std::uint32_t, unsigned> counts;
    for (const auto record : before.glyph_record_indices) ++counts[record];
    unsigned aliases{};
    for (const auto &[c, g] : before.glyphs)
      if (counts.at(before.glyph_record_indices[c]) > 1) {
        require(!original.sampled_glyphs.contains(c), "real fallback/alias font row was replaced");
        ++aliases;
      }
    require(aliases > 64000, "actual lookup-alias control coverage missing");
    auto unverified = before;
    auto changed_identity = sampled;
    changed_identity.file_fingerprint ^= 1;
    require(apply_retail_font_sampling(unverified, changed_identity) > 200,
            "unverified private artwork identity incorrectly blocks normal Unicode rows");
    require(!unverified.sampled_glyphs.contains(0xe100) && !unverified.sampled_glyphs.contains(0xe104) &&
            !unverified.sampled_glyphs.contains(0xe10c) && !unverified.sampled_glyphs.contains(0xe10d),
            "unknown asset identity silently admitted private artwork");
    total += unsigned(count);
    ++fonts;
    std::cout << name << ": " << count << " sampled rows, " << aliases << " retained aliases\n";
  }
  for (const auto name : {"fixedsys-9", "handel_gothic-14"}) {
    auto original = load_retail_font(original_directory / name);
    const auto before = original, sampled = load_retail_font(sampled_directory / name);
    require(apply_retail_font_sampling(original, sampled) == 0 && original.atlas.pixels == before.atlas.pixels &&
            original.atlas.width == before.atlas.width && original.atlas.height == before.atlas.height,
            "actual byte-identical control font changed");
    same_layout(before, original);
  }
  rejects_font_bounds(sampled_directory / "conduit-12");
  std::cout << fonts << " actual enlarged font families, " << total << " mapped rows; byte-identical controls retained\n";
}
} // namespace
int main(int argc, char **argv) {
  if (argc != 1 && argc != 3) return 77;
  try {
    synthetic_tests();
    if (argc == 3) actual_fonts(argv[1], argv[2]);
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
