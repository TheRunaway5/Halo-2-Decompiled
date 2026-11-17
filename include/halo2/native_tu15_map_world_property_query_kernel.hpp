#pragma once
#include <cstdint>

namespace halo2::native_map_world_property_query_detail {
enum class Selection { unsupported, flag, first_byte, second_byte };

// TU1.5 1DB800 either publishes a scalar and active byte or leaves both
// outputs untouched. The transport owns the original register publication.
template<class View> std::uint32_t query(View& view) {
    switch (view.select()) {
    case Selection::unsupported: return view.unsupported();
    case Selection::flag: view.flag_value(); break;
    case Selection::first_byte: view.byte_value(false); break;
    case Selection::second_byte: view.byte_value(true); break;
    }
    return view.publish();
}
} // namespace halo2::native_map_world_property_query_detail
