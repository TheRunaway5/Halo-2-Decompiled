#pragma once
#include <cstdint>

namespace halo2::native_map_world_property_caller_detail {
// TU1.5 11E490 delegates to the property query only for an attached datum.
// The child result remains in the live ABI; the caller owns the final AL.
template<class View> std::uint32_t query(View& view) {
    if(!view.find_attachment())return view.finish_absent();
    view.query_attachment();
    return view.finish_child();
}
} // namespace halo2::native_map_world_property_caller_detail
