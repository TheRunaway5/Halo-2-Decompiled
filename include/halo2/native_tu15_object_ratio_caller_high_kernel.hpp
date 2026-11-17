#pragma once

namespace halo2::native_object_ratio_caller_detail {
enum class HighResult { finish, clamp, unknown };

template<class View> HighResult run_high_keys(View& view) {
    if (view.exact_high_key())
        return view.sample_clock_digit(true) ? HighResult::clamp : HighResult::finish;
    if (view.subtract_high_key(0x16000571u))
        return view.sample_record(false) ? HighResult::clamp : HighResult::finish;
    if (view.subtract_high_offset(8u)) {
        view.sample_flag(false);
        return HighResult::finish;
    }
    if (!view.subtract_high_offset(0x00fffff2u)) return HighResult::unknown;
    return view.sample_clock_digit(false) ? HighResult::clamp : HighResult::finish;
}

template<class View> HighResult run_upper_middle_keys(View& view) {
    if (view.subtract_high_key(0x14000570u))
        return view.sample_record(false) ? HighResult::clamp : HighResult::finish;
    if (!view.subtract_high_offset(8u)) return HighResult::unknown;
    view.sample_flag(true);
    return HighResult::finish;
}
} // namespace halo2::native_object_ratio_caller_detail
