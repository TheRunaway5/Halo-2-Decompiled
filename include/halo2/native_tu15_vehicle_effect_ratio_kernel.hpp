#pragma once

namespace halo2::native_vehicle_effect_ratio_detail {

enum class Exit { common, elapsed_fraction, saturated };

// The previously qualified value policy has two active intervals: elapsed
// time from zero, and elapsed time beyond a nonzero baseline. The view keeps
// the comparison inputs separate from the later arithmetic acquisitions.
template<class View> void evaluate(View& view) {
    view.acquire_body();
    if (!view.has_elapsed_time()) {
        view.finish(Exit::common);
        return;
    }

    view.acquire_effect_definition();
    if (view.starts_at_zero()) {
        if (view.within_initial_interval()) {
            view.initial_fraction();
            view.finish(Exit::elapsed_fraction);
        } else {
            view.saturate();
            view.finish(Exit::saturated);
        }
        return;
    }

    if (view.past_baseline())
        view.baseline_fraction();
    view.finish(Exit::common);
}

} // namespace halo2::native_vehicle_effect_ratio_detail
