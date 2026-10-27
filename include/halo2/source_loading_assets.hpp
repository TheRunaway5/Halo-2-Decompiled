#pragma once
#include "halo2/cache.hpp"
#include <array>
namespace halo2 {
struct SourceLoadingBitmapBank {
    std::string name;
    TagHandle handle;
    std::vector<Texture> frames;
};
using SourceLoadingBitmapAssets=std::array<SourceLoadingBitmapBank,5>;
// All actual MCC loading roles/pages. The Xbox source owns resident mip
// selection; admitted MCC Cache/texture resources own every runtime pixel.
// There is no DVD lookup, executable discovery, reconstruction or resampling.
SourceLoadingBitmapAssets load_source_loading_bitmap_assets(const Cache&);
}
