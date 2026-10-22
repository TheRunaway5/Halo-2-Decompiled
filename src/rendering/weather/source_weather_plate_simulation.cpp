#include "halo2/source_weather_plate_simulation.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> b,std::size_t p) {
    std::uint32_t v;std::memcpy(&v,b.data()+p,4);return v;
}
float scalar(std::span<const std::byte> b,std::size_t p) {
    return std::bit_cast<float>(word(b,p));
}
void store(std::span<std::byte> b,std::size_t p,std::uint32_t v) {
    std::memcpy(b.data()+p,&v,4);
}
void put(std::span<std::byte> b,std::size_t p,float v) {
    store(b,p,std::bit_cast<std::uint32_t>(v));
}
std::int32_t cvtt(float v) {
    if(!std::isfinite(v) || v>=2147483648.f || v< -2147483648.f)
        return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(v);
}
double norm(float x,float y,float z) {
    // Original3777D: (z*z+x*x)+y*y, each x87 operation at PC53.
    return std::sqrt((double(z)*z+double(x)*x)+double(y)*y);
}
std::array<float,2> inverse_xy(std::span<const std::byte> view,
                             std::array<float,3> p) {
    const float scale=scalar(view,0x8c);
    if(scale!=1.f)for(auto &v:p)v=v*scale;
    return {((scalar(view,0xa8)*p[2]+scalar(view,0x9c)*p[1])+
        scalar(view,0x90)*p[0])+scalar(view,0xb4),
        ((scalar(view,0xac)*p[2]+scalar(view,0xa0)*p[1])+
        scalar(view,0x94)*p[0])+scalar(view,0xb8)};
}
}
bool source_weather_step_plate(std::span<std::byte> plate,
    const SourceWeatherPlateSimulationInput &in,std::uint32_t &auxiliary) {
    if(plate.size()!=0x3a8 || in.view.size()!=0xf4 || in.weather.size()<0xac)
        throw std::invalid_argument("weather plate step requires complete owned source banks");
    std::int16_t slot;std::memcpy(&slot,in.view.data(),2);
    if(slot<0 || slot>3)
        throw std::invalid_argument("weather plate source view slot is outside0..3");
    const float angle=scalar(in.view,0x40);
    if(!std::isfinite(angle) || std::abs(double(angle)*.5)>=9223372036854775808.)
        throw std::invalid_argument("weather plate exceptional FPTAN angle is unproved");
    const auto block=std::size_t(slot)*0xb8;
    const auto view=in.view,weather=in.weather;
    put(plate,block+0x14c,in.intensity);
    const auto flags=word(plate,0x94);const bool phase=(flags&1)!=0;
    std::array<float,3> previous;
    for(unsigned axis=0;axis<3;++axis) {
        previous[axis]=scalar(plate,block+0x110+axis*4);
        store(plate,block+0x110+axis*4,word(view,0x18+axis*4));
    }
    const float dx=(scalar(plate,block+0x110)-previous[0])*scalar(view,0x24);
    const float dy=(scalar(plate,block+0x114)-previous[1])*scalar(view,0x28);
    const float dz=(scalar(plate,block+0x118)-previous[2])*scalar(view,0x2c);
    const float range=scalar(plate,0x7c)-scalar(plate,0x78);
    if(flags&2u) {put(plate,0x18,.25f);put(plate,0x1c,.5f);put(plate,0x20,.75f);}
    if(phase) {
        const float accumulated=((dz+dy)+dx)/range+scalar(plate,block+0x140);
        float value=accumulated-float(cvtt(accumulated));
        if(value<0.f)value=value+1.f;
        put(plate,block+0x140,value);
    }
    auto random=[&] {
        auxiliary=auxiliary*0x19660du+0x3c6ef35fu;
        return double(auxiliary>>16)*std::bit_cast<float>(0x37800080u);
    };
    for(unsigned layer=0;layer<3;++layer) {
        const auto parameter=0x18+layer*4;
        const auto point=block+0xc8+layer*12;
        const auto noise=block+0x150+layer*12;
        const auto velocity=0x24+layer*12;
        const auto relative=block+0x104+layer*4;
        float distance=scalar(plate,parameter);
        if(phase) {
            const float current=scalar(plate,block+0x140);
            if(current>distance)distance=distance+1.f;
            distance=distance-current;put(plate,relative,distance);
            const float centered=distance*2.f-1.f;
            put(plate,relative-0x18,(1.f-centered*centered)*scalar(plate,parameter+0x70));
        } else {
            store(plate,relative,word(plate,parameter));
            store(plate,relative-0x18,word(plate,parameter+0x70));
        }
        distance=distance*range+scalar(plate,0x78);
        std::array<float,3> old;
        for(unsigned axis=0;axis<3;++axis)old[axis]=scalar(plate,point+axis*4);
        // FPTAN's extended result is consumed by FMUL before the float store.
        const float projected=float(std::tan(static_cast<long double>(scalar(view,0x40))*.5L)*distance);
        float zero_y=0.f,zero_x=0.f,local_z=0.f-distance;
        const float world_scale=scalar(view,0xc0);
        if(world_scale!=1.f) {
            zero_y=world_scale*0.f;zero_x=zero_y;local_z=local_z*world_scale;
        }
        const std::array<float,3> next={
            ((zero_y*scalar(view,0xd0)+local_z*scalar(view,0xdc))+zero_x*scalar(view,0xc4))+scalar(view,0xe8),
            ((scalar(view,0xe0)*local_z+scalar(view,0xd4)*zero_y)+scalar(view,0xc8)*zero_x)+scalar(view,0xec),
            ((scalar(view,0xe4)*local_z+scalar(view,0xd8)*zero_y)+scalar(view,0xcc)*zero_x)+scalar(view,0xf0)};
        for(unsigned axis=0;axis<3;++axis)put(plate,point+axis*4,next[axis]);
        // Literal random-walk order is z,x,y. Its intermediate is stored as
        // float before the original comparisons; unordered values are retained.
        for(const unsigned axis:{2u,0u,1u}) {
            const double unit=random();
            float value=float(((unit+unit)-1.)*scalar(weather,0xa8)+scalar(plate,noise+axis*4));
            if(-1.f>value)value=-1.f;else if(value>1.f)value=1.f;
            put(plate,noise+axis*4,value);
        }
        float x=scalar(weather,0x90)*scalar(plate,noise)+scalar(weather,0x30);
        float y=scalar(weather,0x94)*scalar(plate,noise+4)+scalar(weather,0x34);
        float z=(scalar(weather,0x98)*scalar(plate,noise+8)+scalar(weather,0x38))-scalar(weather,0x9c);
        const float response=scalar(plate,parameter+0xa4);
        if(response>.0001f) {const float inverse=1.f/response;x=inverse*x;y=inverse*y;z=inverse*z;}
        put(plate,velocity,x+scalar(plate,velocity));
        put(plate,velocity+4,scalar(plate,velocity+4)+y);
        put(plate,velocity+8,scalar(plate,velocity+8)+z);
        const double length=norm(scalar(plate,velocity),scalar(plate,velocity+4),scalar(plate,velocity+8));
        const float maximum=scalar(plate,relative+0x70);
        if(length>maximum) {
            if(!(.0001f>std::abs(length))) {
                const float inverse=1.f/float(length);
                for(unsigned axis=0;axis<3;++axis)put(plate,velocity+axis*4,inverse*scalar(plate,velocity+axis*4));
            }
            for(unsigned axis=0;axis<3;++axis)put(plate,velocity+axis*4,maximum*scalar(plate,velocity+axis*4));
        }
        if(flags&4u)put(plate,relative+0x30,scalar(plate,parameter+0x30)*projected);
        else store(plate,relative+0x30,word(plate,parameter+0x30));
        std::array<float,3> moved;
        for(unsigned axis=0;axis<3;++axis)moved[axis]=in.elapsed*scalar(plate,velocity+axis*4)+old[axis];
        const auto old_xy=inverse_xy(view,moved),new_xy=inverse_xy(view,next);
        const auto offset=block+0x11c+layer*8;
        put(plate,offset,(new_xy[0]-old_xy[0])+scalar(plate,offset));
        put(plate,offset+4,((0.f-new_xy[1])-(0.f-old_xy[1]))+scalar(plate,offset+4));
    }
    return true;
}
}
