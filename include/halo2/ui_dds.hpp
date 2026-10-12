#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
// Ordinary 2D DDS pages for native UI assets. Validates the complete header,
// mip chain and payload before converting source channels for GPU storage.
Texture read_ui_dds_texture(const Bytes&);
}
