#include "halo2/source_weather_wind.hpp"
#include "halo2/math.hpp"
#include <bit>
#include <cmath>
#include <cstddef>

namespace halo2 {
static_assert(offsetof(SourceWeatherWind,velocity)==0x20);
static_assert(offsetof(SourceWeatherWind,gust_velocity)==0x70);
static_assert(offsetof(SourceWeatherWind,bend_delta)==0x98);
float source_weather_wind_random(std::uint32_t &auxiliary,float previous,
                                 float variation,float minimum,float maximum) {
    auxiliary=auxiliary*0x19660du+0x3c6ef35fu;
    //36A40 retains x87 double precision until its FST; the upper clamp is
    //then SSE against that stored float. Rounding the random unit early drifts.
    const double unit=double(auxiliary>>16)*std::bit_cast<float>(0x37800080u);
    const double delta=(unit+unit)-1.;
    const double varied=delta*variation+previous;
    if(minimum>varied)return minimum;
    const float stored=float(varied);
    return stored>maximum?maximum:stored;
}
void source_weather_wind_step(SourceWeatherWind &wind,float elapsed,std::uint32_t &auxiliary) {
    wind.speed_state=source_weather_wind_random(auxiliary,wind.speed_state,
        wind.speed_variation*elapsed,0.f,1.f);
    const float speed=(wind.speed_maximum-wind.speed_minimum)*wind.speed_state+wind.speed_minimum;
    wind.velocity=wind.direction*speed;
    wind.gust_x=source_weather_wind_random(auxiliary,wind.gust_x,
        wind.gust_direction_variation*elapsed,-1.f,1.f);
    wind.gust_y=source_weather_wind_random(auxiliary,wind.gust_y,
        wind.gust_direction_variation*elapsed,-1.f,1.f);
    wind.gust_state=source_weather_wind_random(auxiliary,wind.gust_state,
        wind.gust_magnitude_variation*elapsed,0.f,1.f);
    const float x=wind.gust_x*wind.gust_spread;
    wind.gust_velocity=wind.gust_axis_x*x+wind.gust_direction;
    const float y=wind.gust_y*wind.gust_spread;
    wind.gust_velocity=wind.gust_axis_y*y+wind.gust_velocity;
    const auto &v=wind.gust_velocity;
    //30BF0 computes the norm in x87, stores it once, then divides with SSE.
    const double source_norm=std::sqrt((double(v.x)*v.x+double(v.y)*v.y)+double(v.z)*v.z);
    const float magnitude=float(source_norm);
    if(std::abs(source_norm)>=.0001f)wind.gust_velocity=wind.gust_velocity*(1.f/magnitude);
    wind.gust_velocity=wind.gust_velocity*(wind.gust_magnitude*wind.gust_state);
    wind.velocity=wind.velocity+wind.gust_velocity;
    wind.bend_delta=wind.bend_rate*elapsed;
}
}
