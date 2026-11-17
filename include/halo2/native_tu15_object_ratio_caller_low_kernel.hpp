#pragma once
#include <cstdint>

namespace halo2::native_object_ratio_caller_detail {
enum class LowRoute : std::uint32_t {
    direct_flag=0x1048b8, first_field=0x1048d4, second_field=0x1048e1, third_field=0x1048ee,
    comparison=0x10490f, conditional_flag=0x104955, scaled_byte=0x10498e,
    alternate_extended=0x10476f, recent_record=0x104a98,
    record_ratio=0x104b50, combat_ratio=0x1049ee, unknown=0x1049d0,
};

template<class View> LowRoute select_low_route(View& view) {
    if(view.equal())return LowRoute::recent_record;
    view.compare(0x0b0005a9u);
    if(view.above()){
        view.compare(0x0e0006e6u);
        if(view.above()){
            if(view.is_key(0x0f000574u))return LowRoute::record_ratio;
            return view.is_key(0x0f0006e5u)?LowRoute::combat_ratio:LowRoute::unknown;
        }
        if(view.equal())return LowRoute::combat_ratio;
        if(view.is_key(0x0c000569u))return LowRoute::alternate_extended;
        if(view.is_key(0x0d0006e7u))return LowRoute::combat_ratio;
        return view.is_key(0x0e000572u)?LowRoute::recent_record:LowRoute::unknown;
    }
    if(view.equal())return LowRoute::scaled_byte;
    view.compare(0x05000081u);
    if(view.above()){
        if(view.is_key(0x09000568u))return LowRoute::conditional_flag;
        return view.is_key(0x0a000066u)?LowRoute::comparison:LowRoute::unknown;
    }
    if(view.equal())return LowRoute::third_field;
    if(view.is_key(0x03000577u))return LowRoute::second_field;
    if(view.is_key(0x04000567u))return LowRoute::first_field;
    return view.is_key(0x05000024u)?LowRoute::direct_flag:LowRoute::unknown;
}
} // namespace halo2::native_object_ratio_caller_detail
