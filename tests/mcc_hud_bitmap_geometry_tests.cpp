#include "halo2/frontend.hpp"
#include "halo2/mcc_hud_bitmap_geometry.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using namespace halo2;
void require(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
struct Block { unsigned count, first; };
Block block(const Cache &cache, unsigned address, unsigned stride) {
  const Bytes row(cache.read_tag_bytes(address, 8));
  Block out{row.u32(0), row.u32(4)};
  require(out.count <= 1024, "Unbounded actual bitmap metadata");
  if (out.count) (void)cache.read_tag_bytes(out.first, std::size_t(out.count) * stride);
  return out;
}
UiImage image(const Cache &cache, const Tag &tag) {
  UiImage out; out.name = tag.name;
  const auto pages = block(cache, tag.address + 68, cache.is_mcc() ? 168 : 116);
  for (unsigned n = 0; n < pages.count; ++n) {
    const Bytes row(cache.read_tag_bytes(pages.first + n * (cache.is_mcc() ? 168 : 116), 16));
    Texture texture; texture.width = row.u16(4); texture.height = row.u16(6);
    out.images.push_back(std::move(texture));
  }
  const auto sequences = block(cache, tag.address + 60, 60);
  for (unsigned n = 0; n < sequences.count; ++n) {
    const auto at = sequences.first + n * 60;
    const Bytes row(cache.read_tag_bytes(at, 60));
    const auto sprites = block(cache, at + 52, 32);
    std::vector<UiSprite> values;
    for (unsigned s = 0; s < sprites.count; ++s) {
      const Bytes source(cache.read_tag_bytes(sprites.first + s * 32, 32));
      values.push_back({source.u16(0), source.f32(8), source.f32(12),
                        source.f32(16), source.f32(20), {source.f32(24), source.f32(28)}});
    }
    if (values.empty()) for (unsigned s = 0; s < row.u16(34); ++s)
      values.push_back({unsigned(row.u16(32)) + s, 0, 1, 0, 1, {}});
    out.sequences.push_back(std::move(values));
  }
  return out;
}
} // namespace

#ifdef HALO2_TEST_WRAP_MAIN
extern "C" int __wrap_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
try {
  if (argc != 3) return 2;
  if (!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2])) return 77;
  Cache original(argv[1]), mcc(argv[2]);
  require(!original.is_mcc() && mcc.is_mcc(), "Expected original and MCC map pair");
  unsigned banks = 0, frames = 0, negatives = 0;
  for (const auto &tag : original.tags()) {
    if (tag.group != "bitm" || !tag.address || !tag.name.starts_with("ui\\hud\\")) continue;
    const auto found = std::find_if(mcc.tags().begin(), mcc.tags().end(), [&](const auto &t) {
      return t.group == "bitm" && t.address && t.name == tag.name;
    });
    require(found != mcc.tags().end(), "MCC lost an original HUD bitmap owner");
    const auto logical = image(original, tag), physical = image(mcc, *found);
    for (unsigned frame = 0; frame < logical.images.size(); ++frame) {
      const auto original_extent = mcc_hud_bitmap_logical_extent(logical, frame);
      require(!original_extent || ((*original_extent)[0] == logical.images[frame].width &&
              (*original_extent)[1] == logical.images[frame].height), "Existing original-cache geometry changed");
      const auto extent = mcc_hud_bitmap_logical_extent(physical, frame);
      require(extent && (*extent)[0] == logical.images[frame].width &&
              (*extent)[1] == logical.images[frame].height, "Actual bank did not retain original logical dimensions");
      ++frames;
    }
    for (unsigned frame = unsigned(logical.images.size()); frame < physical.images.size(); ++frame)
      require(!mcc_hud_bitmap_logical_extent(physical, frame), "MCC-only page acquired a fabricated original owner");
    auto reject = [&](auto mutate) {
      auto changed = physical; mutate(changed);
      require(!mcc_hud_bitmap_logical_extent(changed, 0), "Changed or mod-authored bitmap tuple was resized"); ++negatives;
    };
    reject([](auto &v) { v.name += "_custom"; });
    reject([](auto &v) { v.images.pop_back(); });
    reject([](auto &v) { ++v.images.back().width; });
    reject([](auto &v) { ++v.images.back().height; });
    reject([](auto &v) { v.images.back().layers = 6; });
    reject([](auto &v) { v.images.back().depth = 1; });
    reject([](auto &v) { v.sequences.emplace_back(); });
    require(!mcc_hud_bitmap_logical_extent(physical, unsigned(physical.images.size())), "Out-of-bank page projected"); ++negatives;
    for (unsigned n = 0; n < physical.sequences.size(); ++n) if (!physical.sequences[n].empty()) {
      reject([&](auto &v) { v.sequences[n].pop_back(); });
      reject([&](auto &v) { ++v.sequences[n][0].image; });
      reject([&](auto &v) { v.sequences[n][0].left += .125f; });
      reject([&](auto &v) { v.sequences[n][0].registration.x += .125f; });
      break;
    }
    ++banks;
  }
  require(banks == 70 && frames == 328, "Incomplete independently owned source HUD census");
  std::cout << "PASS banks=" << banks << " frames=" << frames
            << " negatives=" << negatives << "; actual authored shapes/UV metadata, not framebuffer parity\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n'; return 1;
}
