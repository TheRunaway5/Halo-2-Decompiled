#pragma once
#include <cstdint>
#include <functional>

namespace halo2 {
// Original 510C54 allocation. The byte at +1 is the governor's pause gate;
// +8 is maintained by the tick consumer, not by 146660.
struct SourceGameTime {
    std::uint8_t active{},paused{};
    std::int16_t tick_rate{};
    float seconds_per_tick{};
    std::int32_t tick{};
    float scale{},leftover_ticks{};
    float speed_timer{},speed_duration{},speed_initial{},speed_final{};
};
static_assert(sizeof(SourceGameTime)==0x24);
struct SourceGameTimeContext {
    bool options_present{},game_active{};
    std::int8_t simulation_mode{}; // Literal game_options+C; no native LAN mapping.
    std::int16_t configured_tick_rate{}; // Literal game_options+E.
    bool unlimited_ticks{}; // Actual 510C50+5 admission.
};
struct SourceGameTimeBudget {
    std::int32_t ticks{};
    bool synchronous{};
    // 146660 reloads 510C54 after 680C0. Services can replace its owner.
    SourceGameTime *time{};
};
using SourceGameTimeNetworkBudget=std::function<SourceGameTimeBudget()>;
struct SourceGameTimeStep {float elapsed{};std::int32_t ticks{};};

// Complete 1465B0, 146980, 146860 and 146660 arithmetic. The caller owns the
// original floating-point environment and platform elapsed-time admission.
void source_game_time_initialize(SourceGameTime &,std::int16_t configured_tick_rate);
void source_game_time_speed(SourceGameTime &,std::int16_t configured_tick_rate,float speed);
void source_game_time_speed_ramp(SourceGameTime &,std::int16_t configured_tick_rate,
                                 float initial,float final,float duration);
SourceGameTimeStep source_game_time_advance(SourceGameTime *&,const SourceGameTimeContext &,
    float seconds,const SourceGameTimeNetworkBudget & = {});

// Complete 137F70 loop. Before each tick, stop supplies actual 12BE90; tick
// executes the admitted native tick owner. The governor bank may change there.
std::int32_t source_game_time_consume(SourceGameTime *&,SourceGameTimeStep &,
    const std::function<bool()> &stop,const std::function<void()> &tick);
// Only 137ED0's final clock store, after its world-update services have run.
// 2B4A0/2B540 reset this owner to zero; render packets copy the double unchanged.
void source_world_time_accumulate(double &realtime,float admitted_elapsed);
}
