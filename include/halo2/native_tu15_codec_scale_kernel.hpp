#pragma once
#include "halo2/native_tu15_word_copy_kernel.hpp"

namespace halo2::native_codec_scale_detail {
// Source-backed scalar codec publication. The view acquires the output
// pointer after the source value, and owns the retained ABI/read boundaries.
template<class View>void decode(View&view){
    auto row=view.scale();native_tu15_copy_words(view,row,false);view.finish();
}
} // namespace halo2::native_codec_scale_detail
