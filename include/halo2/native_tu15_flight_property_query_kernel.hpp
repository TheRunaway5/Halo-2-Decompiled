#pragma once
#include <cstdint>

namespace halo2::native_flight_property_query_detail {
enum class Selection { unsupported, flight, extra_flag, speed };

// TU1.5 E0900 chooses the value source before publishing the two outputs.
// The extra flag uses its own constant and bypasses the clamp.
template<class View> std::uint32_t query(View& view) {
    switch(view.select()) {
    case Selection::unsupported:return view.unsupported();
    case Selection::flight:view.flight_value();break;
    case Selection::extra_flag:
        if(!view.flag_active())return view.unsupported();
        view.flag_value();return view.publish();
    case Selection::speed:
        if(!view.speed_enabled())return view.unsupported();
        view.speed_value();break;
    }
    view.clamp();return view.publish();
}
} // namespace halo2::native_flight_property_query_detail
