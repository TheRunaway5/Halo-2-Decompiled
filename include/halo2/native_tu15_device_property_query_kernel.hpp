#pragma once
#include <cstdint>

namespace halo2::native_device_property_query_detail {
enum class Key { unknown, delay, loop, scalar_134, locked, scalar_140, change_power, change_position };
enum class Output { clamped, direct };

// The original query does not clamp every selected property. Preserve each
// early direct publication, and keep an unknown key from publishing either output.
template<class View> std::uint32_t query(View& view) {
    Output output=Output::clamped;
    switch(view.select()) {
    case Key::unknown:return view.unknown();
    case Key::delay:output=view.delay();break;
    case Key::loop:view.loop();break;
    case Key::scalar_134:view.scalar_134();break;
    case Key::locked:output=view.locked();break;
    case Key::scalar_140:view.scalar_140();break;
    case Key::change_power:output=view.change_power();break;
    case Key::change_position:output=view.change_position();break;
    }
    if(output==Output::clamped)view.clamp();
    return view.publish();
}
} // namespace halo2::native_device_property_query_detail
