#pragma once
#include "halo2/native_tu15_word_copy_kernel.hpp"

namespace halo2::native_codec_translation_detail {
// A codec row contains three scalar translation components. Reuse the ordered
// streaming copy: aliases observe each completed preceding component store.
template<class View>void decode(View&view){
    auto row=view.translation();
    native_tu15_copy_words(view,row,false);
    view.finish();
}
} // namespace halo2::native_codec_translation_detail
