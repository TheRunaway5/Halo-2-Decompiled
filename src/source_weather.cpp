#include "halo2/source_weather.hpp"

namespace halo2 {
namespace {
void retain(const SourceWeatherState &state,SourceWeatherRecord *record) {
    if(record)*record={state,0xdeadbeefu};
}
float duration(float seconds) {return seconds<.0001f?.0001f:seconds;}
}
void source_weather_start(SourceWeatherState &state,float seconds,SourceWeatherRecord *record) {
    seconds=duration(seconds);
    if(state.stopped==1){state.transition=1;state.rate=state.target/seconds;}
    retain(state,record);
}
void source_weather_stop(SourceWeatherState &state,float seconds,SourceWeatherRecord *record) {
    seconds=duration(seconds);
    if(state.stopped==0){state.transition=2;state.rate=(0.f-state.current)/seconds;}
    retain(state,record);
}
void source_weather_change(SourceWeatherState &state,float seconds,float target,SourceWeatherRecord *record) {
    seconds=duration(seconds);
    if(target>1.f)target=1.f;else if(target<0.f)target=0.f;
    state.target=target;
    if(state.stopped!=1){state.rate=(target-state.current)/seconds;state.transition=3;}
    retain(state,record);
}
void source_weather_advance(SourceWeatherState &state,float elapsed,SourceWeatherRecord *record) {
    if(record && record->marker==0xdeadbeefu)state=record->state;
    switch(state.transition){
    case 1:
        state.current=state.rate*elapsed+state.current;
        if(state.current>=state.target){state.current=state.target;state.transition=0;}
        state.stopped=0;break;
    case 2:
        state.current=state.rate*elapsed+state.current;
        if(state.current<=0.f){state.current=0.f;state.transition=0;state.stopped=1;}
        break;
    case 3:
        state.current=state.rate*elapsed+state.current;
        if(state.rate>0.f && state.current>=state.target){state.current=state.target;state.transition=0;}
        else if(state.rate<0.f && state.current<=state.target){
            state.current=state.target;state.transition=0;if(state.target==0.f)state.stopped=1;
        }
        break;
    }
    retain(state,record);
}
bool source_weather_view_tick(SourceWeatherState &state,SourceWeatherClock &clock,
                              double realtime,std::int16_t view_slot,SourceWeatherRecord *record) {
    if(view_slot!=0)return false;
    clock.elapsed=clock.previous==0.f?0.f:float(realtime-double(clock.previous));
    if(clock.elapsed<0.f)clock.elapsed=0.f;
    clock.previous=float(realtime);
    source_weather_advance(state,clock.elapsed,record);
    return true;
}
}
