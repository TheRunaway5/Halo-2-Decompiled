#pragma once
#include "halo2/native_tu15_word_copy_kernel.hpp"

namespace halo2::native_codec_row_copy_detail {
enum class Row { quaternion, translation };
// The view acquires each profile's distinct first word and destination at
// their authored boundaries. The remaining words stream through real backing.
template<class View>void decode(View&view){
    auto row=view.acquire();
    native_tu15_copy_words(view,row,false);
    view.finish();
}
} // namespace halo2::native_codec_row_copy_detail
