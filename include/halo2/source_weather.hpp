#pragma once
#include <cstdint>

namespace halo2 {
// Retail36AB0/398D0 weather intensity state. This is separate from particle
// generation, weather geometry, and the listener's source weather gain.
struct SourceWeatherState {
    std::int32_t stopped{};
    float current=1.f,target=1.f;
    std::int32_t transition{};
    float rate{};
};
struct SourceWeatherRecord {
    SourceWeatherState state;
    std::uint32_t marker{};
};
static_assert(sizeof(SourceWeatherState)==20);
static_assert(sizeof(SourceWeatherRecord)==24);
struct SourceWeatherClock {float previous{},elapsed{};};
// Synchronous borrow of the actual owner, not a weather-state copy. Source
// rendering keeps these globals separate from the checkpointed command record.
struct SourceWeatherRenderState {
    SourceWeatherState &state;
    SourceWeatherClock &clock;
    SourceWeatherRecord &record;
};
// Commands write the retained record when supplied; only398D0 reads it back.
void source_weather_start(SourceWeatherState &,float seconds,SourceWeatherRecord * = nullptr);
void source_weather_stop(SourceWeatherState &,float seconds,SourceWeatherRecord * = nullptr);
void source_weather_change(SourceWeatherState &,float seconds,float target,SourceWeatherRecord * = nullptr);
void source_weather_advance(SourceWeatherState &,float elapsed,SourceWeatherRecord * = nullptr);
// Retail36B50: only source view slot0 updates the shared elapsed clock. The
// caller supplies the original absolute double realtime, not simulation time.
bool source_weather_view_tick(SourceWeatherState &,SourceWeatherClock &,
                              double realtime,std::int16_t view_slot,
                              SourceWeatherRecord * = nullptr);
}
