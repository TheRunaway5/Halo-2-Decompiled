#pragma once

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_codec_format_four_decode_frame.hpp"
#include "physics/runtime/native_tu15_codec_model_secondary_data_frame.hpp"

namespace halo2::native_codec_model_format_four_return_detail {
// The second selector has already pushed 283A3C and jumped to 293290.
// The decoder owns that return word before the parent restores its frame.
template<class State,class Before>void run_private_tail_return(State& state,Before before) {
    native_integer_fp_detail::Scope scope{state};
    scope.child([&]{native_codec_format_four_decode_detail::run_private(state,std::ref(before));});
    native_codec_model_secondary_data_detail::Frame parent{state,before,scope};
    parent.finish_parent();
}
} // namespace halo2::native_codec_model_format_four_return_detail
#endif
