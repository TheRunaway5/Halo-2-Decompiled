#pragma once
#include <cstdint>

namespace halo2::native_campaign_ai_tick {

// The source globals can be republished by an AI phase. Read them again at
// the counter update and after the last callback before marking the tick.
template<class World>
void update(World &world) {
    const auto ai = world.state();
    if (!world.enabled(ai) || world.game_mode_excludes_ai()) {
        world.finish_idle();
        return;
    }

    if (!world.update_requested(ai)) {
        if (world.was_active(ai)) {
            world.retire_active_state();
            world.clear_active();
        }
        world.finish_idle();
        return;
    }

    world.prepare_update();
    const auto current_ai = world.counter_owner();
    const auto counter = world.update_counter(current_ai);
    if (counter > 0) world.decrement_counter(current_ai, counter);
    world.update_subsystems();
    world.mark_active();
    world.finish_update();
}

} // namespace halo2::native_campaign_ai_tick
