#pragma once
#include <cstdint>

namespace halo2::native_node_progress_query_detail {
enum class Return { ordinary, lower, upper };

// TU1.5 1CF0E0 keeps its scratch word as the return value when the
// resolved descriptor supplies a nonpositive divisor. Lookup is the genuine
// local 1E3150 owner, with the authored register and stack inputs.
template<class View> std::uint32_t query(View& view) {
    if(!view.begin())return view.finish(Return::ordinary);
    view.lookup_descriptor();
    if(!view.positive_divisor()){
        view.reload_scratch();return view.finish(Return::ordinary);
    }
    return view.finish(view.normalized_ratio());
}
} // namespace halo2::native_node_progress_query_detail
