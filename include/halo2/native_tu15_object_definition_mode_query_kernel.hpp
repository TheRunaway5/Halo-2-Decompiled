#pragma once

namespace halo2::native_object_definition_mode_query_detail {
// 165590 succeeds only in the original mode, with a non-sentinel datum
// and the required word in the actual object definition.
template<class View> bool query(View& view) {
    if(view.mode_is_nine()&&view.has_datum()&&view.definition_word_is_one())
        view.accept();
    return view.finish();
}
} // namespace halo2::native_object_definition_mode_query_detail
