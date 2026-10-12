#pragma once
#include "halo2/cache.hpp"
#include <array>
#include <cstdint>

namespace halo2 {
// Original mutable weat+10 wind bank, consumed by4B3D0. Unread words are
// retained so updating the bank cannot invent or discard authored parameters.
struct SourceWeatherWind {
    std::uint32_t retained_prefix{};
    Vec3 direction;
    float speed_variation{},speed_minimum{},speed_maximum{},speed_state{};
    Vec3 velocity, gust_direction;
    float gust_direction_variation{},gust_magnitude_variation{};
    std::uint32_t reserved{};
    float gust_magnitude{},gust_spread{},gust_x{},gust_y{},gust_state{};
    Vec3 gust_axis_x,gust_axis_y,gust_velocity;
    float bend_rate{};
    std::array<std::uint32_t,6> retained_bend_parameters{};
    float bend_delta{};
};
static_assert(sizeof(SourceWeatherWind)==0x9c);
//36A40 advances the caller's actual auxiliary stream once. The primary game
//stream is never used here.4B3D0 consumes four draws even at zero elapsed.
float source_weather_wind_random(std::uint32_t &auxiliary,float previous,
                                 float variation,float minimum,float maximum);
void source_weather_wind_step(SourceWeatherWind &,float source_elapsed,
                              std::uint32_t &auxiliary);
}
