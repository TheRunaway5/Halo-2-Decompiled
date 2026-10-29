#pragma once
#include "halo2/cache.hpp"
#include <zlib.h>

namespace halo2::detail {
#include "ui/title_forever_artwork.inc"

inline const Texture &title_forever_texture() {
  static const auto texture = [] {
    static_assert(forever_width <= 2048 && forever_height <= 256);
    std::vector<unsigned char> alpha(forever_width * forever_height);
    uLongf size = alpha.size();
    if (uncompress(alpha.data(), &size, forever_alpha_zlib,
                   sizeof(forever_alpha_zlib)) != Z_OK || size != alpha.size())
      throw FormatError("Invalid website FOREVER artwork");
    Texture result{forever_width, forever_height, 11,
                   std::vector<std::byte>(alpha.size() * 4, std::byte{255})};
    for (std::size_t i = 0; i < alpha.size(); ++i)
      result.pixels[i * 4 + 3] = std::byte(alpha[i]);
    return result;
  }();
  return texture;
}
} // namespace halo2::detail
