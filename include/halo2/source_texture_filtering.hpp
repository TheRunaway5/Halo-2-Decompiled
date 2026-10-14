#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
struct SourceTextureFiltering {
  unsigned min_filter{},mag_filter{},anisotropy=1;
};
inline SourceTextureFiltering source_texture_filtering(const ShaderTextureState& state) {
  if(state.min_filter<1 || state.min_filter>3 || state.mag_filter<1 || state.mag_filter>3)
    throw FormatError("unsupported retail texture filter");
  SourceTextureFiltering result{state.min_filter,state.mag_filter,1};
  if(state.min_filter==3 || state.mag_filter==3) {
    // Unchanged TU1.5 D3D 409F62..409FCB: the authored value is an
    // ordinal. 0 selects point filtering; 1..4 encode 1/2/4/8 samples.
    // Both min and mag are replaced, including mixed filter requests.
    if(state.anisotropy>4)throw FormatError("unsupported original anisotropy ordinal");
    result.min_filter=result.mag_filter=state.anisotropy?2u:1u;
    result.anisotropy=state.anisotropy?1u<<(state.anisotropy-1):1u;
  }
  return result;
}
}
