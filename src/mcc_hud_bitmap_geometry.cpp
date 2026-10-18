#include "halo2/mcc_hud_bitmap_geometry.hpp"
#include "halo2/frontend.hpp"
#include <bit>
#include <cstdint>
#include <string_view>

namespace halo2 {
namespace {
struct Page {
  unsigned physical_width, physical_height, logical_width, logical_height;
};
struct Sprite {
  unsigned image;
  std::array<std::uint32_t, 6> words;
};
struct Sequence {
  unsigned first, count;
};
struct Bank {
  std::string_view name;
  unsigned first_page, page_count, first_sequence, sequence_count;
};
#include "mcc_hud_bitmap_geometry.inc"
} // namespace

std::optional<std::array<unsigned, 2>>
mcc_hud_bitmap_logical_extent(const UiImage &image, unsigned frame) {
  for (const auto &bank : banks) {
    if (image.name != bank.name)
      continue;
    if (image.images.size() != bank.page_count || frame >= bank.page_count ||
        image.sequences.size() != bank.sequence_count)
      return {};
    for (unsigned n = 0; n < bank.page_count; ++n) {
      const auto &physical = image.images[n];
      const auto &page = pages[bank.first_page + n];
      if (physical.width != page.physical_width ||
          physical.height != page.physical_height || physical.layers != 1 ||
          physical.depth)
        return {};
    }
    for (unsigned n = 0; n < bank.sequence_count; ++n) {
      const auto &actual = image.sequences[n];
      const auto &sequence = sequences[bank.first_sequence + n];
      if (actual.size() != sequence.count)
        return {};
      for (unsigned s = 0; s < sequence.count; ++s) {
        const auto &a = actual[s];
        const auto &expected = sprites[sequence.first + s];
        const std::array words{
            std::bit_cast<std::uint32_t>(a.left),
            std::bit_cast<std::uint32_t>(a.right),
            std::bit_cast<std::uint32_t>(a.top),
            std::bit_cast<std::uint32_t>(a.bottom),
            std::bit_cast<std::uint32_t>(a.registration.x),
            std::bit_cast<std::uint32_t>(a.registration.y)};
        if (a.image != expected.image || words != expected.words)
          return {};
      }
    }
    const auto &page = pages[bank.first_page + frame];
    // MCC's extra silenced-SMG backpack page has no original page owner.
    if (!page.logical_width || !page.logical_height)
      return {};
    return std::array{page.logical_width, page.logical_height};
  }
  return {};
}
} // namespace halo2
