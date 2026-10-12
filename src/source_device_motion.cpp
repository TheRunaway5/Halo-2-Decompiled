#include "halo2/source_device_motion.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
bool source_device_channel_step(float &position,float &velocity,float target,
    float dt,float acceleration,float speed,bool loop,float low,float high) {
    if(!std::isfinite(position) || !std::isfinite(velocity) || !std::isfinite(target) ||
       !std::isfinite(dt) || dt<=0 || !std::isfinite(acceleration) || acceleration<0 ||
       !std::isfinite(speed) || speed<0 || !std::isfinite(low) || !std::isfinite(high) || low>high)
        throw FormatError("invalid source device channel input");
    const float previous_velocity=velocity,previous_position=position;
    const float speed_squared=speed*speed;
    float delta=target-position;
    if(loop){
        const float half=(high-low)*.5f;
        if(delta>half)delta=delta-half*2.f;
        else if(delta<0.f-half)delta=half*2.f+delta;
    }
    const float acceleration_distance=(dt*dt)*acceleration;
    const float speed_distance=dt*speed;
    const float snap_distance=acceleration_distance<=speed_distance?acceleration_distance:speed_distance;
    // x87 uses the retail 53-bit precision mode, and does not spill this
    // multiply/subtract or the acceleration comparison to single precision.
    if(double(snap_distance)>=std::abs(double(delta)-double(previous_velocity)*double(dt))){
        velocity=0.f;position=target<low?low:target>high?high:target;return true;
    }
    const double distance_acceleration=std::abs(double(delta))*double(acceleration);
    const double braking_speed_squared=distance_acceleration+distance_acceleration;
    float desired=braking_speed_squared<double(speed_squared)?float(std::sqrt(braking_speed_squared)):speed;
    if(delta<0)desired=0.f-desired;
    const double requested_acceleration=(double(desired)-double(previous_velocity))/double(dt);
    float applied=float(requested_acceleration);
    if(std::abs(requested_acceleration)>double(acceleration))
        applied=float(applied==0.f?0:applied<0.f?-1:1)*acceleration;
    const float dv=applied*dt;
    velocity=dv+previous_velocity;
    position=((dv*.5f+velocity)*dt)+previous_position;
    if(loop){
        if(position<low)position=(high-low)+position;
        else if(position>high)position=position-(high-low);
    }
    if(position<low)position=low;
    else if(position>high)position=high;
    return false;
}
SourceDeviceMotionStep source_device_motion_step(const SourceDeviceMotionDefinition &d,
    SourceDeviceMotionState &s,std::optional<float> position_target,
    std::optional<float> power_target,float dt,bool retain_initialization) {
    if(s.flags&0x18)throw FormatError("source scripted device channel producer unavailable");
    for(const float v:{s.position,s.position_velocity,s.power,s.power_velocity,d.power_rate,
        d.power_acceleration,d.position_rate,d.position_acceleration,d.unpowered_rate,
        d.unpowered_acceleration,d.delay,dt})
        if(!std::isfinite(v))throw FormatError("nonfinite source device motion");
    if(dt<=0 || d.power_rate<0 || d.power_acceleration<0 || d.position_rate<0 ||
       d.position_acceleration<0 || d.unpowered_rate<0 || d.unpowered_acceleration<0 ||
       (position_target && !std::isfinite(*position_target)) ||
       (power_target && !std::isfinite(*power_target)))
        throw FormatError("invalid source device motion input");
    SourceDeviceMotionStep out;
    s.flags&=~0x100u;
    if(position_target){
        if(*position_target==s.position && s.position_velocity==0 && !(s.flags&0x80))s.delay_ticks=0;
        else {
            const float inverse_power=1.f-s.power;
            const float acceleration=d.position_acceleration*s.power+d.unpowered_acceleration*inverse_power;
            const float speed=d.position_rate*s.power+d.unpowered_rate*inverse_power;
            const bool was_positive=s.position_velocity>0;
            if(acceleration>0 && speed>0)s.flags|=0x100;
            if(float(s.delay_ticks)*dt>=d.delay || s.position!=0 || s.position>*position_target){
                const float old_velocity=s.position_velocity,old_position=s.position;
                if(speed<std::abs(old_velocity))s.position_velocity=was_positive?speed:0.f-speed;
                const bool reached=source_device_channel_step(s.position,s.position_velocity,*position_target,
                    dt,acceleration,speed,bool(d.flags&1));
                if(reached)out.effects.push_back(d.effects[was_positive?2:3]);
                else if(s.position_velocity!=0 && s.position_velocity*old_velocity<=0)
                    out.effects.push_back(d.effects[s.position_velocity>old_velocity?0:1]);
                if(old_position!=s.position || (s.flags&0x80)){out.position_dirty=true;s.flags|=4;}
            }else {
                s.delay_ticks=std::bit_cast<std::int16_t>(std::uint16_t(std::uint16_t(s.delay_ticks)+1));
                if(s.delay_ticks==1)out.effects.push_back(d.delay_effect);
            }
            out.updated=true;
        }
    }
    if(power_target && (*power_target!=s.power || s.power_velocity!=0)){
        const float old=s.power;
        source_device_channel_step(s.power,s.power_velocity,*power_target,dt,
            d.power_acceleration,d.power_rate,false);
        if(old!=s.power){out.power_dirty=true;s.flags|=4;}
        out.updated=true;
    }
    if(position_target && (s.flags&0x80)){
        if(!retain_initialization)s.flags&=~0x80u;
        out.updated=true;
    }
    return out;
}
SourceDeviceMotionDefinition Cache::load_source_device_motion_definition(std::uint32_t datum)const {
    const auto metadata=load_source_tag_metadata(datum);
    if((metadata.group()!="mach" && metadata.group()!="ctrl" && metadata.group()!="lifi") || metadata.size()<0x118)
        throw FormatError("source device motion definition missing");
    const Bytes data(metadata.bytes_at(metadata.source_address(),metadata.size()));
    SourceDeviceMotionDefinition d;d.flags=data.u32(0xbc);
    d.power_rate=data.f32(0xc0);d.power_acceleration=data.f32(0xc4);
    d.position_rate=data.f32(0xc8);d.position_acceleration=data.f32(0xcc);
    d.unpowered_rate=data.f32(0xd0);d.unpowered_acceleration=data.f32(0xd4);
    d.delay=data.f32(0x10c);d.delay_effect=data.u32(0x114);
    for(unsigned i=0;i<4;++i)d.effects[i]=data.u32(0xe0+i*8);
    return d;
}
}
